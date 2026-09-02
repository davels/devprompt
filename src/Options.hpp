/**
 * @file Options.hpp
 */

#pragma once

#include <string>
#include <vector>
#include "Expected.hpp"
#include "AnsiColor.hpp"

namespace devprompt
{

struct Options
{
    // target directory
    std::string path;

    // format string
    std::string format = R"({fg:green}%sys_user%@%sys_host%[{/} on {fg:magenta} %vcs_branch%] [{fg:red}\[%vcs_status%\]]{/} at {fg:blue}%path_path%\n{fg:green bold}❯{/} )";
    
    // general options
    ansi::Output output = ansi::Output::Bash;
    bool show_timing = false;
    bool show_help = false;
    bool show_version = false;

    struct
    {    
        // repo types to scan
        bool git = false;
        bool svn = false;

        // what to include the the VCS display
        bool show_untracked     = false;
        bool show_stashed       = false;
        bool show_counts        = false;   // show counts for each status

        bool commit_as_branch   = true;    // use @commit when branch name is missing
        bool ignore_submodules  = false;

        struct RemoteIcon
        {
            std::string pattern;
            std::string icon;
        };
        std::vector<RemoteIcon> remoteicon_mapping;
    } Vcs;

    struct
    {
        bool wsl_is_distro = false;  // treat wsl as it's own distro
        bool show_ssh = true;        // show ssh tag on distro when ssh'd
    } Os;

    struct
    {
        std::string error;
    } Shell;
};

Expected<Options, std::string> parse_options(int argc,  char** argv);

} // namespace devprompt
