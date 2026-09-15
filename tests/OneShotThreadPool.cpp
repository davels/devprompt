/**
 * @file tests/OneShotThreadPool.cpp
 */

#include <catch2/catch_test_macros.hpp>

#include "OneShotThreadPool.hpp"
#include "CommandContext.hpp"

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <stdexcept>
#include <stop_token>
#include <thread>
#include <vector>

using devprompt::OneShotThreadPool;
using devprompt::CommandContext;
using namespace std::chrono_literals;

namespace
{
    // Busy-polls a predicate until it's true or `timeout` elapses.
    // Returns whether the predicate was ever observed true.
    template <class Pred>
    bool wait_until(Pred pred, std::chrono::milliseconds timeout)
    {
        const auto deadline = std::chrono::steady_clock::now() + timeout;
        while (std::chrono::steady_clock::now() < deadline)
        {
            if (pred()) return true;
            std::this_thread::sleep_for(1ms);
        }
        return pred();
    }

    bool wait_for_stop(std::stop_token stoken, std::chrono::milliseconds timeout)
    {
        return wait_until([&stoken] { return stoken.stop_requested(); }, timeout);
    }

    bool wait_for_killed(const CommandContext& cctx, std::chrono::milliseconds timeout)
    {
        return wait_until([&cctx] { return cctx.killed(); }, timeout);
    }
}

// ---------------------------------------------------------------------------
// CommandContext, standalone (no pool involved)
// ---------------------------------------------------------------------------

TEST_CASE("CommandContext: kill() is safe with nothing set, and killed() latches", "[CommandContext]")
{
    CommandContext cctx;
    CHECK_FALSE(cctx.killed());

    cctx.kill();
    CHECK(cctx.killed());

    // clear() only drops the tracked command pointer; it does not reset the
    // killed flag. There's no "un-kill" -- once killed, always killed.
    cctx.clear();
    CHECK(cctx.killed());

    cctx.kill(); // calling kill() again is idempotent / safe
    CHECK(cctx.killed());
}

// ---------------------------------------------------------------------------
// Basic construction / accounting
// ---------------------------------------------------------------------------

TEST_CASE("Getters reflect configuration before start()", "[OneShotThreadPool][basic]")
{
    OneShotThreadPool<4> pool(3);
    CHECK(pool.thread_count() == 3);
    CHECK(pool.task_count() == 0);

    pool.enqueue([](std::stop_token, CommandContext&) {});
    pool.enqueue([](std::stop_token, CommandContext&) {});
    CHECK(pool.task_count() == 2);

    CHECK_FALSE(pool.shared_stop_token().stop_requested());
    CHECK_FALSE(pool.shared_command_context().killed());
}

TEST_CASE("Zero enqueued tasks completes wait_all() immediately", "[OneShotThreadPool][basic]")
{
    OneShotThreadPool<4> pool(2);
    pool.start();
    pool.wait_all(); // must not hang
    CHECK(pool.task_count() == 0);
    CHECK_FALSE(pool.global_timeout_reached());
}

TEST_CASE("Pool with zero worker threads doesn't crash on start/shutdown", "[OneShotThreadPool][basic]")
{
    OneShotThreadPool<4> pool(0);
    CHECK(pool.thread_count() == 0);
    pool.start();
    pool.wait_all();
    SUCCEED("destructor / shutdown() with zero workers did not hang or crash");
}

// ---------------------------------------------------------------------------
// Task execution correctness
// ---------------------------------------------------------------------------

TEST_CASE("Every enqueued task runs exactly once", "[OneShotThreadPool][execution]")
{
    constexpr std::size_t kTasks = 50;
    std::array<std::atomic<int>, kTasks> ran{};
    for (auto& a : ran) a.store(0);

    OneShotThreadPool<kTasks> pool(4);
    for (std::size_t i = 0; i < kTasks; ++i)
    {
        pool.enqueue([&ran, i](std::stop_token, CommandContext&) {
            ran[i].fetch_add(1, std::memory_order_relaxed);
        });
    }

    pool.start();
    pool.wait_all();

    for (auto& a : ran)
    {
        CHECK(a.load() == 1);
    }
}

TEST_CASE("wait_all() blocks until all tasks have actually finished", "[OneShotThreadPool][wait_all]")
{
    constexpr std::size_t kTasks = 8;
    std::atomic<int> finishedCount{0};

    OneShotThreadPool<kTasks> pool(4);
    for (std::size_t i = 0; i < kTasks; ++i)
    {
        pool.enqueue([&finishedCount](std::stop_token, CommandContext&) {
            std::this_thread::sleep_for(20ms);
            finishedCount.fetch_add(1, std::memory_order_relaxed);
        });
    }

    pool.start();
    pool.wait_all();

    CHECK(finishedCount.load() == static_cast<int>(kTasks));
}

TEST_CASE("Tasks are distributed across multiple worker threads", "[OneShotThreadPool][execution]")
{
    constexpr std::size_t kTasks = 40;
    std::vector<std::atomic<std::thread::id>> seenThreads(kTasks);

    OneShotThreadPool<kTasks> pool(4);
    for (std::size_t i = 0; i < kTasks; ++i)
    {
        pool.enqueue([&seenThreads, i](std::stop_token, CommandContext&) {
            std::this_thread::sleep_for(2ms);
            seenThreads[i] = std::this_thread::get_id();
        });
    }

    pool.start();
    pool.wait_all();

    std::vector<std::thread::id> distinct;
    for (auto& t : seenThreads)
    {
        auto id = t.load();
        if (std::find(distinct.begin(), distinct.end(), id) == distinct.end())
            distinct.push_back(id);
    }
    CHECK(distinct.size() > 1);
}

// ---------------------------------------------------------------------------
// Exception safety
// ---------------------------------------------------------------------------

TEST_CASE("Exceptions thrown by a task are swallowed and don't stop the pool", "[OneShotThreadPool][exceptions]")
{
    constexpr std::size_t kTasks = 5;
    std::atomic<int> completed{0};

    OneShotThreadPool<kTasks> pool(2);
    for (std::size_t i = 0; i < kTasks; ++i)
    {
        pool.enqueue([&completed, i](std::stop_token, CommandContext&) {
            completed.fetch_add(1, std::memory_order_relaxed);
            if (i % 2 == 0) throw std::runtime_error("boom");
        });
    }

    pool.start();
    CHECK_NOTHROW(pool.wait_all());
    CHECK(completed.load() == static_cast<int>(kTasks));
}

// ---------------------------------------------------------------------------
// Watchdog behavior
// ---------------------------------------------------------------------------

TEST_CASE("Per-task watchdog cancels a task and kills its CommandContext", "[OneShotThreadPool][watchdog]")
{
    constexpr auto kMaxTask = 50ms;
    std::atomic<bool> sawStop{false};
    std::atomic<bool> sawKilled{false};

    OneShotThreadPool<1> pool(1, kMaxTask, 0ms);
    pool.enqueue([&sawStop, &sawKilled](std::stop_token stoken, CommandContext& cctx) {
        sawStop = wait_for_stop(stoken, 2000ms);
        sawKilled = wait_for_killed(cctx, 2000ms);
    });

    pool.start();
    pool.wait_all();
    pool.stop_watchdog();

    CHECK(sawStop.load());
    CHECK(sawKilled.load());
    CHECK_FALSE(pool.global_timeout_reached()); // only the per-task limit fired
    CHECK_FALSE(pool.shared_command_context().killed()); // shared ctx untouched by per-task path
}

TEST_CASE("Overall watchdog cancels everything and kills every context", "[OneShotThreadPool][watchdog]")
{
    constexpr auto kMaxTotal = 50ms;
    constexpr std::size_t kTasks = 3;
    std::atomic<int> stoppedCount{0};
    std::atomic<int> killedCount{0};

    OneShotThreadPool<kTasks> pool(kTasks, 0ms, kMaxTotal);
    for (std::size_t i = 0; i < kTasks; ++i)
    {
        pool.enqueue([&stoppedCount, &killedCount](std::stop_token stoken, CommandContext& cctx) {
            if (wait_for_stop(stoken, 2000ms)) stoppedCount.fetch_add(1, std::memory_order_relaxed);
            if (wait_for_killed(cctx, 2000ms)) killedCount.fetch_add(1, std::memory_order_relaxed);
        });
    }

    pool.start();
    pool.wait_all();
    pool.stop_watchdog();

    CHECK(pool.global_timeout_reached());
    CHECK(stoppedCount.load() == static_cast<int>(kTasks));
    CHECK(killedCount.load() == static_cast<int>(kTasks));
    // The overall-timeout path also cancels/kills the shared context/token.
    CHECK(pool.shared_stop_token().stop_requested());
    CHECK(pool.shared_command_context().killed());
}

TEST_CASE("After overall timeout, a not-yet-started task inherits a cancelled token and killed context", "[OneShotThreadPool][watchdog]")
{
    // Single worker so tasks run strictly in order. The first task runs
    // long enough to trigger the overall timeout while in flight. By the
    // time the worker picks up the second task:
    //  - it gets a pre-cancelled stop_token (worker_loop checks
    //    m_global_timeout_reached before assigning a fresh token), and
    //  - it sees an already-killed CommandContext, because each worker
    //    reuses the *same* CommandContext object across all the tasks it
    //    runs, and killed() has no reset once set.
    constexpr auto kMaxTotal = 50ms;

    OneShotThreadPool<2> pool(1, 0ms, kMaxTotal);

    pool.enqueue([](std::stop_token stoken, CommandContext&) {
        wait_for_stop(stoken, 2000ms);
    });

    std::atomic<bool> secondTaskStartedCancelled{false};
    std::atomic<bool> secondTaskStartedKilled{false};
    pool.enqueue([&secondTaskStartedCancelled, &secondTaskStartedKilled](std::stop_token stoken, CommandContext& cctx) {
        secondTaskStartedCancelled = stoken.stop_requested();
        secondTaskStartedKilled = cctx.killed();
    });

    pool.start();
    pool.wait_all();
    pool.stop_watchdog();

    CHECK(pool.global_timeout_reached());
    CHECK(secondTaskStartedCancelled.load());
    CHECK(secondTaskStartedKilled.load());
}

TEST_CASE("stop_watchdog() is safe to call even when no watchdog was configured", "[OneShotThreadPool][watchdog]")
{
    OneShotThreadPool<1> pool(1); // both durations default to 0 -> no watchdog thread
    pool.enqueue([](std::stop_token, CommandContext&) {});
    pool.start();
    pool.wait_all();
    CHECK_NOTHROW(pool.stop_watchdog());
    CHECK_FALSE(pool.global_timeout_reached());
}
