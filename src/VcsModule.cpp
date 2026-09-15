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
#include "CommandContext.hpp"

namespace devprompt
{

namespace 
{
vcs::RepoStatus s_status;
bool s_showcounts = false;
} // namespace


ModuleUsage VcsModule::mod_usage(const Options& /*options*/)
{
    if (IsVariableInUse(VariableId::vcs_remote_icon) ||
        IsVariableInUse(VariableId::vcs_branch) ||
        IsVariableInUse(VariableId::vcs_state) ||
        IsVariableInUse(VariableId::vcs_wt_icon) ||
        IsVariableInUse(VariableId::vcs_status) ||
        IsVariableInUse(VariableId::vcs_divergence))
        return ModuleUsage::UsedExpensive;

    return ModuleUsage::Unused;
}

/// Load module data
bool VcsModule::mod_load(const Options& options, std::stop_token /*stoken*/, CommandContext& cmdctx)
{
    s_showcounts = options.Vcs.show_counts;
    
    vcs::RepoQueryVars vars {
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
        vcs::GitBackend git;
        auto status = git.query(options, vars, cmdctx);
        if (status)
        {
            s_status = *status;
            return true;
        }
        else if (status.error() != vcs::RepoError::NotRepository)
        {
            // could be an error or a timeout
            return true;
        }
    }

    if (options.Vcs.svn)
    {
        vcs::SvnBackend svn;
        auto status = svn.query(options, vars, cmdctx);
        if (status)
        {
            s_status = *status;
        }
        else if (status.error() != vcs::RepoError::NotRepository)
        {
            // error or timeout
        }        
    }

    return true;
}


//
// Variable functions
//

void VcsModule::remote_icon(std::string& out)
{
    out += s_status.remote_icon;
}

void VcsModule::branch(std::string& out)
{
    if (!s_status.branch.empty())
    {
        if (s_status.detached) out += '@';
        out += s_status.branch;
    }
}

void VcsModule::state(std::string& out)
{
    out += s_status.state;
}

void VcsModule::wt_icon(std::string& out)
{
    if (s_status.isworktree)
        out += " ";  // 󱘎   󰔱 󰐅 󰹩
}

void VcsModule::status(std::string& out)
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

void VcsModule::divergence(std::string& out)
{
    if (s_status.ahead  > 0)
        out += "↑" + std::to_string(s_status.ahead);   // ↑ ⇡
    if (s_status.behind > 0)
        out += "↓" + std::to_string(s_status.behind);  // ↓ ⇣
}

} // namespace devprompt

