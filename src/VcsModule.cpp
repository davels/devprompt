/**
 * @file VcsModule.cpp
 */

#include "VcsModule.hpp"
#include <string>
#include "Options.hpp"
#include "RepoStatus.hpp"
#include "GitBackend.hpp"
#include "SvnBackend.hpp"
#include "Variable.hpp"

namespace devprompt::VcsModule
{

namespace 
{
RepoStatus s_status;
bool s_showcounts = false;
} // namespace


/// Load module data
bool load(const Options& options)
{
    s_showcounts = options.Vcs.show_counts;
    
    RepoQueryVars vars {
        .remote_icon = IsVariableInUse(VariableId::vcs_remote_icon),
        .branch      = IsVariableInUse(VariableId::vcs_branch),
        .state       = IsVariableInUse(VariableId::vcs_state),
        .wt_icon     = IsVariableInUse(VariableId::vcs_wt_icon),
        .status      = IsVariableInUse(VariableId::vcs_status),
        .divergence  = IsVariableInUse(VariableId::vcs_divergence),
    };
    if (!vars.remote_icon && !vars.branch && !vars.state && !vars.wt_icon && !vars.status && !vars.divergence)
        return false;
    
    if (options.Vcs.git)
    {    
        GitBackend git;
        auto status = git.query(options, vars);
        if (status)
        {
            s_status = *status;
            return true;
        }
        else if (status.error() != RepoError::NotRepository)
        {
            // maybe print the error
            return true;
        }
    }

    if (options.Vcs.svn)
    {
        SvnBackend svn;
        auto status = svn.query(options, vars);
        if (status)
        {
            s_status = *status;
        }
        else if (status.error() != RepoError::NotRepository)
        {
            // maybe print the error
        }        
    }

    return true;
}


//
// Variable functions
//

void remote_icon(std::string& out)
{
    out += s_status.remote_icon;
}

void branch(std::string& out)
{
    if (!s_status.branch.empty())
    {
        if (s_status.detached) out += '@';
        out += s_status.branch;
    }
}

void state(std::string& out)
{
    out += s_status.state;
}

void wt_icon(std::string& out)
{
    if (s_status.isworktree)
        out += " ";  // 󱘎   󰔱 󰐅 󰹩
}

void status(std::string& out)
{
    std::string flags;
    auto collector = [&flags, showcount=s_showcounts] (int count, char ch)
    {
        if (count > 0)
        {
            // if (showcount && !flags.empty())
            //     flags += ' ';
            flags += ch;
            if (showcount)
                flags += std::to_string(count);
        }
    };
    collector(s_status.conflicts, '!');
    collector(s_status.unstaged,  '*');
    collector(s_status.staged,    '+');
    collector(s_status.untracked, '?');  
    collector(s_status.stashed,   '$');  // 

    out += flags;
}

void divergence(std::string& out)
{
    if (s_status.ahead  > 0)
        out += "↑" + std::to_string(s_status.ahead);   // ↑ ⇡
    if (s_status.behind > 0)
        out += "↓" + std::to_string(s_status.behind);  // ↓ ⇣
}

} // namespace devprompt::VcsModule

