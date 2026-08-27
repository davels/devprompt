/**
 * @file main.cpp
 */

#include <string>
#include <string_view>
#include "Options.hpp"
#include "Version.hpp"
#include "VcsModule.hpp"
#include "VenvModule.hpp"

// For write_stdoud
#if defined(_WIN32)
  #include <windows.h>
#else
   #include <unistd.h>
#endif

using namespace devprompt;

namespace
{

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
    constexpr std::string_view help_text = R"(A lightweight tool to generate a version control string for shell prompts.

Usage: devprompt [OPTIONS] [PATH]

If no repository options are provided, it will check for all supported types.
If no PATH is specified it will use the current directory.

Options:
  --bash
          Write output compatible with the bash shell
  --zsh
          Write output compatible with the zsh shell
  --git
          Scan for git repositories
  --svn
          Scan for subversion (svn) repositories
  --vcs-branch
          Show branch name [default]
  --vcs-no-branch
          Don't show branch name
  --vcs-status
          Show version control status (files changed, etc) [default]
  --vcs-no-status
          Don't show version control status
  --vcs-counts
          Show file count with status
  --vcs-no-counts
          Don't show file count with status [default]
  --vcs-untracked
          Show untracked files. Note that this can be slow on large git repos.
  --vcs-no-untracked
          Don't show untracked files [default]
  --vcs-state
          Show operations in progress (rebase, merge, etc) [default]
  --vcs-no-state
          Don't show operations in progress
  --vcs-diverged
          Show number of commits ahead/behind upstream [default]
  --vcs-no-diverged
          Don't show number of commits ahead/behind
  --vcs-stashed
          Report stashed files
  --vcs-no-stashed
          Don't report stashed files [default]
  --vcs-show-remote-icon
          Show an icon for the remote origin [default]
  --vcs-no-remote-icon
          Don't show an icon for the remote origin
  --vcs-remote-icon <name;icon>
          Add a new remote icon mapping. Requires a string so compare to the remote url
          and an icon. "string;icon"
  --vcs-show-worktree-icon
          Show an icon if the current repo is a worktree, as in (git worktree add) [default]
  --vcs-no-worktree-icon
          Don't show an icon if the current repo is a worktree
  --vcs-style
          Style string to apply to the version control output
  --venv-icon
          Icon to prefix to the venv output
  --venv-style
          Style string to apply to the venv output
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
    
    std::string out;
    out.reserve(64);
    
    out += VcsModule::format(*options);

    std::string mod = VenvModule::format(*options);
    if (!mod.empty())
    {
        if (!out.empty()) out += ' ';
        out += mod;
    }
    
    write_stdout(out);    
    return 0;
}
