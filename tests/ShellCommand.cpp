/**
 * @file tests/ShellCommand.cpp
 *
 * Catch2 unit tests for ShellCommand.hpp / ShellCommand.cpp.
 *
 * These tests actually spawn real child processes via the real
 * subprocess.h (https://github.com/sheredom/subprocess.h) rather than
 * mocking it, since ShellCommand's whole job is to sit on top of it. They
 * assume a POSIX-like environment (Linux/macOS) with `echo`, `true`,
 * `false`, `printf`, and `sh` available via PATH -- which is exactly what
 * `subprocess_option_search_user_path` relies on internally. On Windows
 * you'll want to swap these for `cmd /c echo ...` equivalents.
 *
 * All of the following have been verified fixed as of this revision
 * (previously tracked as bugs across earlier rounds of testing):
 *  - Move construction/assignment compile and work correctly, and
 *    destroying a moved-from ShellCommand no longer crashes.
 *  - run() correctly distinguishes "output exactly fit the buffer" from
 *    "output was too large for the buffer" via a one-byte peek read, and
 *    reports Unexpected(ERR_BUFFER_OVERFLOW) only for genuine overflow.
 *  - run()'s overflow path calls wait() before returning, so it no longer
 *    leaks the child as a zombie process (verified with waitpid/WNOHANG).
 *  - readline() correctly returns a trailing partial line with no
 *    terminating '\n' instead of silently discarding it.
 *
 * One remaining item, not a bug so much as a documented caveat: calling
 * any method other than the destructor or move-assignment on a moved-from
 * ShellCommand is unsafe (m_impl is null, so e.g. start()/read()/wait()
 * would dereference it). This mirrors how a moved-from std::unique_ptr
 * behaves and isn't unusual, but it's worth being aware of. See the
 * disabled test near the bottom of this file if you want to double-check
 * that behavior after any future changes.
 *
 * Build (Catch2 v3 example):
 *   g++ -std=c++20 -I<path-to-catch2>/include -I<path-to-subprocess.h> \
 *       test_ShellCommand.cpp ShellCommand.cpp -lCatch2Main -lCatch2 \
 *       -o test_ShellCommand
 *   ./test_ShellCommand
 */

#include <catch2/catch_test_macros.hpp>

#include "ShellCommand.hpp"

#include <string_view>
#include <vector>

#if !defined(_WIN32)
#include <sys/wait.h>
#include <unistd.h>
#endif

using namespace devprompt;

// ---------------------------------------------------------------------
// start() / wait()
// ---------------------------------------------------------------------

TEST_CASE("start() launches a process and wait() returns its exit code (success)",
          "[ShellCommand][start][wait]")
{
    ShellCommand sc;
    REQUIRE(sc.start("true"));
    CHECK(sc.wait() == 0);
}

TEST_CASE("wait() reports a non-zero exit code", "[ShellCommand][start][wait]")
{
    ShellCommand sc;
    REQUIRE(sc.start("false"));
    CHECK(sc.wait() == 1);
}

TEST_CASE("start() returns false for a command that cannot be found", "[ShellCommand][start]")
{
    ShellCommand sc;
    CHECK_FALSE(sc.start("this-command-does-not-exist-xyz123"));
}

TEST_CASE("wait() reports 0 after a failed start() -- not a usable error signal",
          "[ShellCommand][start][wait]")
{
    // Callers must check start()'s bool result to detect a launch failure;
    // wait()'s exit code is indistinguishable from a genuine 0-exit process
    // in that case.
    ShellCommand sc;
    REQUIRE_FALSE(sc.start("this-command-does-not-exist-xyz123"));
    CHECK(sc.wait() == 0);
}

TEST_CASE("A ShellCommand can be reused for a second command after the first completes",
          "[ShellCommand][start][reuse]")
{
    ShellCommand sc;
    REQUIRE(sc.start("echo", "first"));
    char buf[64];
    auto n1 = sc.read(buf);
    CHECK(std::string_view(buf, n1) == "first\n");
    CHECK(sc.wait() == 0);

    REQUIRE(sc.start("echo", "second"));
    auto n2 = sc.read(buf);
    CHECK(std::string_view(buf, n2) == "second\n");
    CHECK(sc.wait() == 0);
}

TEST_CASE("A ShellCommand that is never started can be safely destroyed",
          "[ShellCommand][lifecycle]")
{
    ShellCommand sc;
    (void)sc;
    SUCCEED("destructor did not crash on a never-started instance");
}

// ---------------------------------------------------------------------
// start() / run(): the span overload's MaxArguments cap
// ---------------------------------------------------------------------

TEST_CASE("start(span) rejects an argument list longer than MaxArguments",
          "[ShellCommand][start][MaxArguments]")
{
    std::vector<const char*> args;
    args.push_back("echo");
    for (std::size_t i = 0; i < ShellCommand::MaxArguments; ++i) // one more than the cap
        args.push_back("x");
    REQUIRE(args.size() == ShellCommand::MaxArguments + 1);

    ShellCommand sc;
    CHECK_FALSE(sc.start(std::span<const char* const>(args.data(), args.size())));
}

TEST_CASE("start(span) accepts an argument list exactly at MaxArguments",
          "[ShellCommand][start][MaxArguments]")
{
    CommandArgs<ShellCommand::MaxArguments> args;
    REQUIRE(args.push_back("echo"));
    for (std::size_t i = 0; i < ShellCommand::MaxArguments - 1; ++i)
        REQUIRE(args.push_back("x"));
    CHECK(args.size() == ShellCommand::MaxArguments);

    ShellCommand sc;
    REQUIRE(sc.start(args.span()));
    CHECK(sc.wait() == 0);
}

TEST_CASE("The variadic start() template has no MaxArguments cap",
          "[ShellCommand][start][MaxArguments]")
{
    // Unlike the span overload, the variadic template builds a
    // compile-time-sized stack array, so nothing enforces MaxArguments here.
    ShellCommand sc;
    REQUIRE(sc.start("echo", "1", "2", "3", "4", "5", "6", "7", "8", "9",
                      "10", "11", "12", "13", "14", "15", "16", "17", "18"));
    CHECK(sc.wait() == 0);
}

// ---------------------------------------------------------------------
// run(): capturing stdout
// ---------------------------------------------------------------------

TEST_CASE("run() captures stdout and returns it on success", "[ShellCommand][run]")
{
    ShellCommand sc;
    char buf[256];
    auto result = sc.run(buf, "echo", "hello");
    REQUIRE(static_cast<bool>(result));
    CHECK(*result == "hello\n");
}

TEST_CASE("run() returns Unexpected(exitcode) when the command exits non-zero",
          "[ShellCommand][run]")
{
    ShellCommand sc;
    char buf[256];
    auto result = sc.run(buf, "sh", "-c", "exit 3");
    REQUIRE_FALSE(static_cast<bool>(result));
    CHECK(result.error() == 3);
}

TEST_CASE("run() returns Unexpected(1) when the command cannot be launched",
          "[ShellCommand][run]")
{
    ShellCommand sc;
    char buf[256];
    auto result = sc.run(buf, "this-command-does-not-exist-xyz123");
    REQUIRE_FALSE(static_cast<bool>(result));
    CHECK(result.error() == 1);
}

TEST_CASE("run(span) returns Unexpected(1) when the argument list exceeds MaxArguments",
          "[ShellCommand][run][MaxArguments]")
{
    std::vector<const char*> args;
    args.push_back("echo");
    for (std::size_t i = 0; i < ShellCommand::MaxArguments; ++i)
        args.push_back("x");
    REQUIRE(args.size() == ShellCommand::MaxArguments + 1);

    ShellCommand sc;
    char buf[256];
    auto result = sc.run(buf, std::span<const char* const>(args.data(), args.size()));
    REQUIRE_FALSE(static_cast<bool>(result));
    CHECK(result.error() == 1);
}

// ---------------------------------------------------------------------
// run(): buffer-overflow handling
// ---------------------------------------------------------------------

TEST_CASE("run() reports overflow when the output does not fit the buffer",
          "[ShellCommand][run][overflow]")
{
    ShellCommand sc;
    char buf[4]; // "hello\n" is 6 bytes -- genuinely too small
    auto result = sc.run(buf, "echo", "hello");
    REQUIRE_FALSE(static_cast<bool>(result));
    CHECK(result.error() == ShellCommand::ERR_BUFFER_OVERFLOW);
}

TEST_CASE("run() reports overflow for output much larger than the buffer",
          "[ShellCommand][run][overflow]")
{
    ShellCommand sc;
    char buf[3];
    auto result = sc.run(buf, "echo", "hello world, this is a much longer line of output");
    REQUIRE_FALSE(static_cast<bool>(result));
    CHECK(result.error() == ShellCommand::ERR_BUFFER_OVERFLOW);
}

TEST_CASE("run() succeeds when output exactly fills the buffer", "[ShellCommand][run][overflow]")
{
    // "hello\n" is exactly 6 bytes -- this used to be a false-positive
    // overflow; the peek-based check now correctly succeeds here.
    ShellCommand sc;
    char buf[6];
    auto result = sc.run(buf, "echo", "hello");
    REQUIRE(static_cast<bool>(result));
    CHECK(*result == "hello\n");
}

TEST_CASE("run() succeeds once the buffer is even one byte larger than the output",
          "[ShellCommand][run][overflow]")
{
    ShellCommand sc;
    char buf[7]; // "hello\n" (6 bytes) + 1 byte of headroom
    auto result = sc.run(buf, "echo", "hello");
    REQUIRE(static_cast<bool>(result));
    CHECK(*result == "hello\n");
}

TEST_CASE("run() succeeds when the buffer is much larger than the output",
          "[ShellCommand][run][overflow]")
{
    ShellCommand sc;
    char buf[256];
    auto result = sc.run(buf, "echo", "hello");
    REQUIRE(static_cast<bool>(result));
    CHECK(*result == "hello\n");
}

#if !defined(_WIN32)
TEST_CASE("run() does not leak a zombie process on the overflow path",
          "[ShellCommand][run][overflow][posix-only]")
{
    // Previously, hitting the overflow path returned without ever calling
    // wait(), leaking the child as an unreaped zombie. wait() is now
    // called before returning, so nothing should be left to reap here.
    //
    // Assumes this test binary has no other outstanding child processes of
    // its own at this point in the run.
    ShellCommand sc;
    char buf[4];
    auto result = sc.run(buf, "echo", "hello");
    REQUIRE_FALSE(static_cast<bool>(result));

    usleep(100000); // give the child a moment to fully exit
    int status = 0;
    const pid_t reaped = waitpid(-1, &status, WNOHANG);
    CHECK(reaped <= 0); // nothing left over to reap
}
#endif

// ---------------------------------------------------------------------
// read() / stderr()
// ---------------------------------------------------------------------

TEST_CASE("read() returns stdout bytes from the running process", "[ShellCommand][read]")
{
    ShellCommand sc;
    REQUIRE(sc.start("printf", "%s", "abc123"));
    char buf[64];
    auto n = sc.read(buf);
    CHECK(std::string_view(buf, n) == "abc123");
    sc.wait();
}

TEST_CASE("read() returns 0 once stdout is exhausted", "[ShellCommand][read]")
{
    ShellCommand sc;
    REQUIRE(sc.start("true"));
    char buf[64];
    auto n = sc.read(buf);
    CHECK(n == 0);
    sc.wait();
}

TEST_CASE("stderr() returns bytes written to standard error", "[ShellCommand][stderr]")
{
    ShellCommand sc;
    REQUIRE(sc.start("sh", "-c", "echo err-out >&2"));
    char buf[64];
    auto n = sc.stderr(buf);
    CHECK(std::string_view(buf, n) == "err-out\n");
    sc.wait();
}

// ---------------------------------------------------------------------
// readline()
// ---------------------------------------------------------------------

TEST_CASE("readline() splits multi-line output on '\\n'", "[ShellCommand][readline]")
{
    ShellCommand sc;
    REQUIRE(sc.start("printf", "%s", "line1\nline2\n"));
    char rbuf[64];
    char lbuf[64];

    auto l1 = sc.readline(rbuf, lbuf);
    REQUIRE(l1.has_value());
    CHECK(std::string_view(l1->data(), l1->size()) == "line1");

    auto l2 = sc.readline(rbuf, lbuf);
    REQUIRE(l2.has_value());
    CHECK(std::string_view(l2->data(), l2->size()) == "line2");

    auto l3 = sc.readline(rbuf, lbuf);
    CHECK_FALSE(l3.has_value());

    sc.wait();
}

TEST_CASE("readline() strips a trailing '\\r' (CRLF line endings)", "[ShellCommand][readline]")
{
    ShellCommand sc;
    REQUIRE(sc.start("printf", "%s", "abc\r\ndef\r\n"));
    char rbuf[64];
    char lbuf[64];

    auto l1 = sc.readline(rbuf, lbuf);
    REQUIRE(l1.has_value());
    CHECK(std::string_view(l1->data(), l1->size()) == "abc");

    auto l2 = sc.readline(rbuf, lbuf);
    REQUIRE(l2.has_value());
    CHECK(std::string_view(l2->data(), l2->size()) == "def");

    sc.wait();
}

TEST_CASE("readline() reassembles a line spanning multiple internal read() chunks",
          "[ShellCommand][readline]")
{
    ShellCommand sc;
    REQUIRE(sc.start("printf", "%s", "hello world\nsecond\n"));
    char rbuf[3]; // deliberately tiny, forcing many underlying read() calls
    char lbuf[64];

    auto l1 = sc.readline(rbuf, lbuf);
    REQUIRE(l1.has_value());
    CHECK(std::string_view(l1->data(), l1->size()) == "hello world");

    auto l2 = sc.readline(rbuf, lbuf);
    REQUIRE(l2.has_value());
    CHECK(std::string_view(l2->data(), l2->size()) == "second");

    sc.wait();
}

TEST_CASE("readline() truncates a line longer than linebuf without losing line sync",
          "[ShellCommand][readline]")
{
    ShellCommand sc;
    REQUIRE(sc.start("printf", "%s", "abcdefghij\nnext\n"));
    char rbuf[64];
    char lbuf[4]; // smaller than "abcdefghij"

    auto l1 = sc.readline(rbuf, lbuf);
    REQUIRE(l1.has_value());
    CHECK(l1->size() == 4);
    CHECK(std::string_view(l1->data(), l1->size()) == "abcd");

    // Even though "efghij" was discarded, the reader correctly resumed at
    // the next line rather than getting out of sync.
    auto l2 = sc.readline(rbuf, lbuf);
    REQUIRE(l2.has_value());
    CHECK(std::string_view(l2->data(), l2->size()) == "next");

    sc.wait();
}

TEST_CASE("readline() returns nullopt immediately when there is no output at all",
          "[ShellCommand][readline]")
{
    ShellCommand sc;
    REQUIRE(sc.start("true"));
    char rbuf[64];
    char lbuf[64];
    CHECK_FALSE(sc.readline(rbuf, lbuf).has_value());
    sc.wait();
}

TEST_CASE("readline() returns a trailing partial line with no terminating '\\n'",
          "[ShellCommand][readline]")
{
    ShellCommand sc;
    REQUIRE(sc.start("printf", "%s", "no-newline-at-end"));
    char rbuf[64];
    char lbuf[64];

    auto l1 = sc.readline(rbuf, lbuf);
    REQUIRE(l1.has_value());
    CHECK(std::string_view(l1->data(), l1->size()) == "no-newline-at-end");

    // The following call hits true EOF (no data at all) and correctly
    // returns nullopt.
    auto l2 = sc.readline(rbuf, lbuf);
    CHECK_FALSE(l2.has_value());

    sc.wait();
}

TEST_CASE("readline() strips a trailing '\\r' even on an unterminated final line",
          "[ShellCommand][readline]")
{
    ShellCommand sc;
    REQUIRE(sc.start("printf", "%s", "abc\r"));
    char rbuf[64];
    char lbuf[64];

    auto l1 = sc.readline(rbuf, lbuf);
    REQUIRE(l1.has_value());
    CHECK(std::string_view(l1->data(), l1->size()) == "abc");

    sc.wait();
}

// ---------------------------------------------------------------------
// CommandArgs<N>
// ---------------------------------------------------------------------

TEST_CASE("CommandArgs push_back succeeds up to capacity and fails beyond it",
          "[CommandArgs]")
{
    CommandArgs<2> args;
    CHECK(args.size() == 0);
    CHECK(args.push_back("one"));
    CHECK(args.push_back("two"));
    CHECK(args.size() == 2);
    CHECK_FALSE(args.push_back("three")); // capacity exceeded
    CHECK(args.size() == 2);              // unchanged by the failed push
}

TEST_CASE("CommandArgs::span() reflects exactly the elements pushed so far",
          "[CommandArgs]")
{
    CommandArgs<4> args;
    args.push_back("a");
    args.push_back("b");
    auto s = args.span();
    REQUIRE(s.size() == 2);
    CHECK(std::string_view(s[0]) == "a");
    CHECK(std::string_view(s[1]) == "b");
}

// ---------------------------------------------------------------------
// Move semantics
// ---------------------------------------------------------------------

TEST_CASE("A moved-to ShellCommand can continue to interact with the process",
          "[ShellCommand][move]")
{
    ShellCommand sc1;
    REQUIRE(sc1.start("echo", "moved"));
    ShellCommand sc2 = std::move(sc1);
    char buf[64];
    auto n = sc2.read(buf);
    CHECK(std::string_view(buf, n) == "moved\n");
    CHECK(sc2.wait() == 0);
    // sc1 (moved-from) is destroyed at the end of this scope; this no
    // longer crashes now that ~ShellCommand() checks m_impl for null.
}

TEST_CASE("A moved-assigned-to ShellCommand can continue to interact with the process",
          "[ShellCommand][move]")
{
    ShellCommand sc3;
    REQUIRE(sc3.start("echo", "assigned"));
    ShellCommand sc4;
    sc4 = std::move(sc3);
    char buf[64];
    auto n = sc4.read(buf);
    CHECK(std::string_view(buf, n) == "assigned\n");
    CHECK(sc4.wait() == 0);
}

// A moved-from ShellCommand is left with a null Impl, so calling anything
// other than the destructor or move-assignment on it (e.g. start(),
// read(), wait()) is unsafe -- consistent with how a moved-from
// std::unique_ptr behaves. Left disabled since it's expected to crash by
// design; enable only if you want to reconfirm that expectation.
#if 0
TEST_CASE("Using a moved-from ShellCommand beyond destruction/move-assignment is unsafe",
          "[ShellCommand][move][unsafe]")
{
    ShellCommand sc1;
    ShellCommand sc2 = std::move(sc1);
    sc1.start("echo", "should not get here safely"); // dereferences null m_impl
}
#endif
