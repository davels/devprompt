/**
 * @file main.cpp
 */

#include <string>
#include <string_view>
#include <type_traits>
#include <chrono>
#include "Options.hpp"
#include "Version.hpp"
#include "FormatString.hpp"
#include "VcsModule.hpp"
#include "VenvModule.hpp"
#include "SysModule.hpp"
#include "PathModule.hpp"
#include "OsModule.hpp"
#include "ShellModule.hpp"

// For write_stdoud
#if defined(_WIN32)
  #include <windows.h>
#else
   #include <unistd.h>
#endif

using namespace devprompt;

namespace
{

bool s_timing_on = false;
std::string s_timelog;

// Write string directly to stdoud
inline void write_stdout(std::string_view str)
{
    if (str.empty()) return;
    const char* pstr = str.data();
    std::size_t len = str.length();
    
#if defined(_WIN32)
    DWORD written = 0;
    HANDLE h = GetStdHandle(STD_ERROR_HANDLE);
    WriteFile(h, pstr, static_cast<DWORD>(len), &written, nullptr);
#else
    std::ignore = ::write(1, pstr, len); // fd 1 = stdout
#endif
}

inline void write_stderr(std::string_view str)
{
    if (str.empty()) return;
    const char* pstr = str.data();
    std::size_t len = str.length();

#if defined(_WIN32)
    DWORD written = 0;
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    WriteFile(h, pstr, static_cast<DWORD>(len), &written, nullptr);
#else
    std::ignore = ::write(2, pstr, len); // fd 2 = stderr
#endif
}

inline void record_timing(std::string_view name, std::chrono::steady_clock::duration elapsed)
{
    s_timelog += "  ";
    s_timelog += name;
    s_timelog += ": ";
    using namespace std::chrono;
    if (elapsed < 1ms)
    {
        auto strval = std::to_string(duration_cast<microseconds>(elapsed).count());
        s_timelog += std::string(3-strval.length(), ' ') + strval;
        s_timelog += "us \n";    
    }
    else
    {
        auto strval = std::to_string(duration_cast<milliseconds>(elapsed).count());
        s_timelog += std::string(3-strval.length(), ' ') + strval;
        s_timelog += "ms \n";    
    }
}

/// Collect timing data for function call
template<typename F>
decltype(auto) timed(std::string_view name, F&& f)
{
    if (!s_timing_on)
        return std::forward<F>(f)();

    const auto start = std::chrono::steady_clock::now();

    if constexpr (std::is_void_v<std::invoke_result_t<F>>)
    {
        std::forward<F>(f)();

        const auto elapsed = std::chrono::steady_clock::now() - start;
        record_timing(name, elapsed);
    }
    else
    {
        decltype(auto) result = std::forward<F>(f)();

        const auto elapsed = std::chrono::steady_clock::now() - start;
        record_timing(name, elapsed);
        return result;
    }
}

/// Collect timing data for function if it returns true
template<typename F>
void maybe_timed(std::string_view name, F&& f)
{
    if (!s_timing_on)
    {
        std::ignore = std::forward<F>(f)();
        return;
    }
    
    const auto start = std::chrono::steady_clock::now();

    if (std::forward<F>(f)())
    {
        const auto elapsed = std::chrono::steady_clock::now() - start;
        record_timing(name, elapsed);
    }
}

// the last component from the full path
std::string_view executable_name(std::string_view path)
{
    const auto pos = path.find_last_of("/\\");
    return pos == std::string_view::npos ? path : path.substr(pos + 1);
}

void show_version(char* appname)
{
    std::string msg {executable_name(appname)};
    msg += ' ';
    msg += DEVPROMPT_VERSION;
    msg += '\n';
    write_stdout(msg);
}

void show_help(char* appname)
{
    constexpr std::string_view help_text = R"(A lightweight tool to generate for generating shell prompts.

Usage: devprompt [OPTIONS] [PATH]

If no version control repository types are specified, it will check for all supported types.
If no PATH is specified it will use the current directory.

Options:
  --format
          The format of the prompt string (see FORMAT STRING below)
  --bash
          Write output compatible with the bash shell
  --zsh
          Write output compatible with the zsh shell
  --git
          Scan for git repositories
  --svn
          Scan for subversion (svn) repositories
  --vcs-counts
          Show file counts in vcs status
  --vcs-no-counts
          Don't show file counts in vcs status [default]
  --vcs-untracked
          Show untracked files is vcs status. Note that this can be slow on large git repos.
  --vcs-no-untracked
          Don't show untracked files [default]
  --vcs-stashed
          Show stashed files in vcs status
  --vcs-no-stashed
          Don't show stashed files [default]
  --vcs-commit-as-branch
          If there is no branch name, use commit id as the branch name [default]
  --vcs-no-commit-as-branch
          Don't substitute commit id for missing branch name
  --vcs-remote-pattern <pattern;icon>
          Add a new remote icon mapping. Requires a string to compare to the remote url,
          and an icon.
  --os-wsl-is-distro
          Treat WSL as it's own distro instead of reporting the specific distro used
  --os-no-wsl-is-distro
          For WSL, report the linux distro [default]
  --timing
          Show execution time
  -h, --help
          Print help
  --version
          Print version
)";
    
    write_stdout(help_text);
}

void show_error(char* appname, const std::string& error)
{
    auto shortname = executable_name(appname);
    std::string msg;
    msg.reserve(64);
    msg += shortname;
    msg += ": error: ";
    msg += error;
    msg += '\n';
    msg += '\n';
    msg += "usage: ";
    msg += shortname;
    msg += " [OPTIONS] [PATH]\n\n";
    msg += "for more information try '";
    msg += shortname;
    msg += " --help'\n";
    write_stderr(msg);
}

} // namespace


int main(int argc, char** argv)
{
    const auto options = parse_options(argc, argv);
    if (!options)
    {
        show_error(argv[0], options.error());
        return 1;
    }

    if (options->show_help)
    {
        show_help(argv[0]);
        return 0;
    }
    if (options->show_version)
    {
        show_version(argv[0]);
        return 0;        
    }

    s_timing_on = options->show_timing;

    std::chrono::steady_clock::time_point start_time;
    if (s_timing_on)
        start_time = std::chrono::steady_clock::now();
    
    FormatString format(options->format);

    // parse the format string which also tags used variables
    auto result = timed("parse", [&] { return format.compile(); });
    if (!result)
    {
        show_error(argv[0], result.error());
        return 1;
    }

    // Load module data
    maybe_timed("VcsModule  ", [&] { return VcsModule::load(*options); });
    maybe_timed("VenvModule ", [&] { return VenvModule::load(*options); });
    maybe_timed("SysModule  ", [&] { return SysModule::load(*options); });
    maybe_timed("PathModule ", [&] { return PathModule::load(*options); });
    maybe_timed("OsModule   ", [&] { return OsModule::load(*options); });
    maybe_timed("ShellModule", [&] { return ShellModule::load(*options); });
    
    // Expand the format string and write it out
    std::string out;
    out.reserve(128);
    timed("expand", [&] { format.expand(options->output, out); });
    write_stdout(out);

    if (s_timing_on)
    {
        using namespace std::chrono;
        write_stdout("\nPrompt Timings: (");
        const auto elapsed = steady_clock::now() - start_time;
        write_stdout(std::to_string(duration_cast<milliseconds>(elapsed).count()));
        write_stdout("ms)\n");
        write_stdout(s_timelog);
    }
    return 0;
}
