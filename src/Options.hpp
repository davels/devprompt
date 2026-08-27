/**
 * @file Options.hpp
 */

#pragma once

#include <string>
#include <string_view>
#include <vector>
#include "Expected.hpp"
#include "AnsiColor.hpp"

namespace devprompt
{

struct Options
{
    // target directory
    std::string path;

    // general options
    bool show_help = false;
    bool show_version = false;
    ansi::Output output = ansi::Output::Bash;

    //# VCS Options
    struct
    {    
        // repo types to scan
        bool git = false;
        bool svn = false;

        // what to include the the VCS display
        bool show_branch        = true;
        bool show_status        = true;
        bool show_untracked     = false;
        bool show_stashed       = false;
        bool show_counts        = false;          // show counts for each status
        bool show_diverged      = true;           // ahead / behind
        bool show_state         = true;           // in progress rebase, merge, etc
        bool show_remote_icon   = true;
        bool show_worktree_icon = true;

        bool ignore_submodules  = false;

        struct RemoteIcon
        {
            std::string hostname;
            std::string icon;
        };
        std::vector<RemoteIcon> remoteicon_mapping;
        
        // other
        std::string_view icon;
        std::string style;
    } Vcs;

    //# VENV Options
    struct
    {
        std::string_view icon;
        std::string style;
    } Venv;
};

Expected<Options, std::string> parse_options(int argc,  char** argv);

} // namespace devprompt
