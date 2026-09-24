/**
 * @file OneShotThreadPool.hpp
 *
 * A very light weight thread pool class tailored for one-shot use with known sizes.
 *
 * - Tasks are all queued up front into a fixed size array before the schedular starts, no
 *   locking required.
 * - Workers claim tasks with a single atomic fetch_add over the array. No mutex, no
 *   condition_variable
 * - wait_all() uses std::atomic<size_t>::wait()/notify_all() rather than a mutex +
 *   condition_variable.
 *
 * watchdog thread (when enabled) uses the only mutex to monitor tasks that take too long
 * and sends a stop request to them via std::stop_token
 *
 * move_only_task acts as a standin for std::move_only_function until we move to C++23
 *
 * enqueue() tasks, start() the pool, wait_all() for tasks to finish
 */

#pragma once

#include <algorithm>
#include <array>
#include <atomic>
#include <cassert>
#include <chrono>
#include <concepts>
#include <cstddef>
#include <memory>
#include <functional>
#include <mutex>
#include <stop_token>
#include <thread>
#include <type_traits>
#include <utility>
#include "CommandContext.hpp"

namespace devprompt
{

namespace detail
{

// ---------------------------------------------------------------------------
// move_only_task - just enough of c++23 std::move_only_function for our use
//
// task is: void F(std::stop_token).
// ---------------------------------------------------------------------------
class move_only_task {
public:
    move_only_task() = default;

    template <class F>
      requires std::invocable<F, std::stop_token, CommandContext&> &&
          (!std::same_as<std::decay_t<F>, move_only_task>)
    move_only_task(F&& f):
        m_impl(std::make_unique<model<std::decay_t<F>>>(std::forward<F>(f))) {}

    move_only_task(move_only_task&&) noexcept = default;
    move_only_task& operator=(move_only_task&&) noexcept = default;
    move_only_task(const move_only_task&) = delete;
    move_only_task& operator=(const move_only_task&) = delete;

    void operator()(std::stop_token stoken, CommandContext& cctx) { m_impl->call(std::move(stoken), cctx); }
    explicit operator bool() const noexcept { return static_cast<bool>(m_impl); }

private:
    struct concept_t
    {
        virtual ~concept_t() = default;
        virtual void call(std::stop_token, CommandContext&) = 0;
    };
    template <class F>
    struct model final: concept_t
    {
        F m_func;
        explicit model(F&& fn) : m_func(std::forward<F>(fn)) {}
        void call(std::stop_token stoken, CommandContext& cctx) override { std::invoke(m_func, std::move(stoken), cctx); }
    };

    std::unique_ptr<concept_t> m_impl;
};

} // namespace detail


// ---------------------------------------------------------------------------
// OneShotThreadPool
// ---------------------------------------------------------------------------
template <std::size_t MaxTasks>
  requires (MaxTasks > 0)
class OneShotThreadPool
{
  public:
    // duration == 0 disables the watchdog entirely.
    explicit OneShotThreadPool(std::size_t numThreads,
                               std::chrono::milliseconds max_task = std::chrono::milliseconds{0},
                               std::chrono::milliseconds max_total = std::chrono::milliseconds{0}):
        m_max_task_duration(max_task),
        m_max_total_duration(max_total),
        m_worker_count(numThreads)
    {}

    ~OneShotThreadPool() { shutdown(); }

    OneShotThreadPool(const OneShotThreadPool&) = delete;
    OneShotThreadPool& operator=(const OneShotThreadPool&) = delete;

    /// Add a task. Must be called before start(), at most MaxTasks times.
    /// F must be callable as F(std::stop_token, CommandContext&)
    template <class F>
    requires std::invocable<F, std::stop_token, CommandContext&>
    void enqueue(F&& f)
    {
        assert(m_worker_count > 0 && "OneShotThreadPool: enqueue() called with zero worker threads");
        assert(!m_started && "OneShotThreadPool: cannot enqueue() after start() has been called");
        assert(m_task_count < MaxTasks && "OneShotThreadPool: exceeded max task capacity");
        m_tasks[m_task_count] = detail::move_only_task(std::forward<F>(f));
        ++m_task_count;
    }

    /// Start worker (and, if configured, watchdog) threads
    void start()
    {
        assert(!m_started && "OneShotThreadPool: start() called more than once");
        m_started = true;
        m_global_start_time = std::chrono::steady_clock::now();

        if (m_worker_count > 0)
        {
            m_workers = std::make_unique<std::thread[]>(m_worker_count);
            m_worker_states = std::make_unique<WorkerState[]>(m_worker_count);
        }
        
        for (std::size_t i = 0; i < m_worker_count; ++i)
        {
            m_workers[i] = std::thread([this, i] { worker_loop(i); });
        }
        
        if (m_max_task_duration.count() > 0 || m_max_total_duration.count() > 0)
        {
             m_watchdog = std::jthread([this](std::stop_token stoken) { watchdog_loop(stoken); });
        }
    }

    /// Blocks until every enqueued task has finished running.
    // NOTE: if a task never returns and never checks its stop_token, this
    // blocks forever
    void wait_all()
    {
        for (;;)
        {
            const std::size_t completed = m_completed_count.load(std::memory_order_acquire);
            if (completed >= m_task_count) return;
            m_completed_count.wait(completed, std::memory_order_acquire);
        }
    }

    void stop_watchdog()
    {
        m_watchdog.request_stop();
    }
    
    std::size_t thread_count() const noexcept { return m_worker_count; }
    std::size_t task_count() const noexcept { return m_task_count; }

    // so single-threaded tasks can get notified by the watchdog thread
    CommandContext& shared_command_context() { return m_shared_cmdctx; }
    std::stop_token shared_stop_token() { return m_shared_stop_source.get_token(); }
    
    // True once the overall duration has been exceeded. Useful after wait_all() to tell
    // "finished normally" apart from "cut short by the overall timeout".
    bool global_timeout_reached() const noexcept
    {
        return m_global_timeout_reached.load(std::memory_order_acquire);
    }
    
    // Joins everything. Called automatically by the destructor.
    void shutdown()
    {
        if (m_shutdown_called) return;
        m_shutdown_called = true;
        if (m_workers)  // workers not created unless start() is called
        {   
            for (std::size_t i = 0; i < m_worker_count; ++i)
            {
                if (m_workers[i].joinable()) m_workers[i].join();
            }
        }
        m_watchdog = {};  // jthread destructor requests_stop() + joins
    }

  private:
    struct WorkerState
    {
        std::atomic<bool> busy{false};
        std::atomic<std::chrono::steady_clock::time_point> start_time{};
        std::mutex stop_mutex;  // guards stop_source
        // initialize with nostopstate to avoid heap allocation for default constructor
        std::stop_source stop_source{std::nostopstate};
        CommandContext cmdctx;
    };

    void worker_loop(std::size_t index)
    {
        WorkerState& state = m_worker_states[index];
        while (true)
        {
            const std::size_t i = m_next_task_index.fetch_add(1, std::memory_order_relaxed);
            if (i >= m_task_count) return;  // no more work, this thread is done

            std::stop_source stopsrc;
            if (m_global_timeout_reached.load(std::memory_order_acquire))
            {
                // The overall timeout is reached: don't hand this task a fresh,
                // un-cancelled token. A task that checks stop_requested() up front can
                // bail immediately instead of starting work.
                stopsrc.request_stop();
            }            
            {
                std::lock_guard lock(state.stop_mutex);
                state.stop_source = stopsrc;
            }
            state.start_time.store(std::chrono::steady_clock::now(), std::memory_order_relaxed);
            state.busy.store(true, std::memory_order_release);

            // run the task
            try
            {
                m_tasks[i](stopsrc.get_token(), state.cmdctx);
            }
            catch (...)
            {
                // don't allow exceptions from a task to propagate out, or it will
                // terminate the program.
            }

            state.busy.store(false, std::memory_order_release);

            m_completed_count.fetch_add(1, std::memory_order_acq_rel);
            m_completed_count.notify_all();
        }
    }

    void watchdog_loop(std::stop_token stoken)
    {
        using namespace std::chrono_literals;

        // Poll at a quarter of whichever configured duration is smaller
        // (only nonzero ones count), clamped to a sane range.
        std::chrono::milliseconds smallest;
        if (m_max_task_duration.count() > 0 && m_max_total_duration.count() > 0)
        {
            smallest = std::min(m_max_task_duration, m_max_total_duration);
        }
        else if (m_max_task_duration.count() > 0)
        {
            smallest = m_max_task_duration;
        }
        else
        {
            smallest = m_max_total_duration;
        }
        const auto poll_interval = std::clamp(smallest / 4, 1ms, 250ms);

        while (!stoken.stop_requested())
        {
            const auto now = std::chrono::steady_clock::now();

            // overall duration
            if (m_max_total_duration.count() > 0 &&
                !m_global_timeout_reached.load(std::memory_order_acquire) &&
                now - m_global_start_time > m_max_total_duration)
            {
                m_global_timeout_reached.store(true, std::memory_order_release);
                for (std::size_t i = 0; i < m_worker_count; ++i)
                {
                    WorkerState& state = m_worker_states[i];
                    if (!state.busy.load(std::memory_order_acquire)) continue;
                    std::lock_guard lock(state.stop_mutex);
                    if (!state.stop_source.stop_requested())
                    {
                        state.stop_source.request_stop();
                    }
                    state.cmdctx.kill();
                }
                m_shared_stop_source.request_stop();
                m_shared_cmdctx.kill();
            }

            // per-task duration
            if (m_max_task_duration.count() > 0)
            {
                for (std::size_t i = 0; i < m_worker_count; ++i)
                {
                    WorkerState& state = m_worker_states[i];
                    if (!state.busy.load(std::memory_order_acquire)) continue;
                    const auto start = state.start_time.load(std::memory_order_relaxed);
                    if (now - start > m_max_task_duration)
                    {
                        std::lock_guard lock(state.stop_mutex);
                        if (state.busy.load(std::memory_order_acquire) &&
                            !state.stop_source.stop_requested())
                        {
                            state.stop_source.request_stop();
                        }
                        state.cmdctx.kill();
                    }
                }
            }
            
            std::this_thread::sleep_for(poll_interval);
        }
    }

    // task details. filled single-threaded by enqueue() before start() is called, then
    // only ever read by worker threads.
    std::array<detail::move_only_task, MaxTasks> m_tasks;
    std::size_t m_task_count = 0;

    std::atomic<std::size_t> m_next_task_index{0};
    std::atomic<std::size_t> m_completed_count{0};

    bool m_started = false;
    std::chrono::steady_clock::time_point m_global_start_time{};
    std::atomic<bool> m_global_timeout_reached{false};

    const std::chrono::milliseconds m_max_task_duration;
    const std::chrono::milliseconds m_max_total_duration;
    const std::size_t m_worker_count;
    std::unique_ptr<std::thread[]> m_workers;
    std::unique_ptr<WorkerState[]> m_worker_states;
    std::jthread m_watchdog;

    std::stop_source m_shared_stop_source;
    CommandContext m_shared_cmdctx;
    bool m_shutdown_called = false;
};

} // namespace devprompt
