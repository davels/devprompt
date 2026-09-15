/**
 * @file Options.hpp
 */

#pragma once

#include <string>
#include <vector>
#include <chrono>
#include "Expected.hpp"
#include "Style.hpp"

namespace devprompt
{

struct Options
{
    // target directory
    std::string path;
    
    // format string
    std::string format = R"({fg:green}%sys.user%@%sys.host%[{/} on {fg:magenta} %vcs.branch%] [{fg:red}\[%vcs.status%\]]{/} at {fg:blue}%path.path%\n{fg:green bold}❯{/} )";

    // palette string
    std::string palette;
    
    // general options
    ansi::Output output = ansi::Output::Bash;
    bool show_timings = false;
    bool show_help = false;
    bool show_version = false;
    bool no_async = false;
    unsigned int timeout{0};
    unsigned int module_timeout{0};
    
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
