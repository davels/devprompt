/**
 * @file main.cpp
 */

#include <string>
#include <string_view>
#include "Options.hpp"
#include "Version.hpp"
#include "runner.hpp"
#include "utils.hpp"

using devprompt::utils::write_stdout;
using devprompt::utils::write_stderr;

namespace
{

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

void show_help(char* /*appname*/)
{
    constexpr std::string_view help_text = R"(A lightweight tool for generating shell prompt strings.

Usage: devprompt [OPTIONS] [PATH]

If no version control repository types are specified, it will check for all supported types.
If no PATH is specified it will use the current directory.

Options:
  --format
          The format string for the prompt (see FORMAT STRING below)
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
    const auto options = devprompt::parse_options(argc, argv);
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

    auto result = devprompt::run(*options);
    if (!result)
    {
        show_error(argv[0], result.error());
        return 1;
    }

    return 0;
}
