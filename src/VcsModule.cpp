/**
 * @file VcsModule.cpp
 */

#include "VcsModule.hpp"
#include <cassert>
#include <numeric>
#include <string>
#include <array>
#include <algorithm>
#include "Options.hpp"
#include "RepoStatus.hpp"
#include "GitBackend.hpp"
#include "SvnBackend.hpp"
#include "Variable.hpp"
#include "CommandContext.hpp"
#include "filesystem.hpp"

namespace devprompt
{

namespace
{
vcs::RepoStatus s_status;
bool s_showcounts = false;

using namespace std::string_view_literals;
struct RepoInfo
{
    vcs::RepoType rtype;
    std::string_view artifact;
};

// artifact object for each repo type
constexpr std::array s_repospec = {
    RepoInfo {vcs::RepoType::git, ".git"sv},
    RepoInfo {vcs::RepoType::svn, ".svn"sv},
};

// repos types that we are checking for
std::array<bool, s_repospec.size()> s_repo_active{};  // initialized to false;

constexpr size_t MAX_ARTIFACT_LENGTH = std::accumulate(
    s_repospec.begin(), s_repospec.end(), std::size_t{0},
    [](size_t current_max, const RepoInfo& item) {
        return std::max(current_max, item.artifact.length());
    });

consteval std::size_t repo_index(vcs::RepoType rtype)
{
    for (std::size_t i = 0; i < s_repospec.size(); ++i)
        if (s_repospec[i].rtype == rtype)
            return i;
    return s_repospec.size();
}

// NOTE: we use the passed repo string as workspace for checking files, but restore it
// before returning.  It should have at least max(artifact.length) + 2 char of empty space
// at the end.
vcs::RepoType check_for_repo_at(std::string& repo)
{
    assert(repo.capacity() >= repo.size() + MAX_ARTIFACT_LENGTH + 2);
    
    for (std::size_t idx=0; auto& rinfo: s_repospec)
    {
        // only check for artifacts of repos that we care about
        if (!s_repo_active[idx++])
            continue;

        auto len = repo.length();  // length before appending the artifact
        repo += rinfo.artifact;
        auto found = devprompt::filesystem::path_exists(repo);
        repo.resize(len);
        if (found)
            return rinfo.rtype;
    }
    return vcs::RepoType::none;
}

std::pair<vcs::RepoType, std::string> check_for_repo(const std::string& path)
{
    std::string repo;
    repo.reserve(path.length() + MAX_ARTIFACT_LENGTH + 2);
    repo = path;
    
#ifdef _WIN32
    // make a windows path look like a unix path
    char drive_letter = '\0';
    if (repo.length() > 2 && repo[1] == ':')
    {
        drive_letter = repo[0];
        repo.erase(0, 2);  // remove "C:" at start
        // normalize path separators
        std::ranges::replace(repo, '\\', '/');
    }
#endif

    // ensure trailing /
    if (repo.back() != '/')  repo += '/';

    // check if this path is under version control by searching up the directory tree
    vcs::RepoType rtype = vcs::RepoType::none;
    for (;;)
    {
        rtype = check_for_repo_at(repo);
        if (rtype != vcs::RepoType::none)
            break;
        
        // we're already at the root directory
        if (repo.length() < 2)
            break;
        
        // strip off the last directory and try again
        auto pos = repo.rfind('/', repo.length()-2);
        if (pos == std::string::npos)
            break;
        else
            repo.resize(pos+1); // keep everything upto and including the slash
    }

    if (rtype == vcs::RepoType::none)
        return {vcs::RepoType::none, {}};

#ifdef _WIN32
    if (drive_letter != '\0')
    {
        repo.insert(0, {drive_letter, ':'});
    }
#endif

    return {rtype, repo};
}

} // namespace


ModuleUsage VcsModule::mod_usage(const Options& options, FileScanner&)
{
    if (IsVariableInUse(VariableId::vcs_remote_icon) ||
        IsVariableInUse(VariableId::vcs_branch) ||
        IsVariableInUse(VariableId::vcs_state) ||
        IsVariableInUse(VariableId::vcs_wt_icon) ||
        IsVariableInUse(VariableId::vcs_status) ||
        IsVariableInUse(VariableId::vcs_divergence))
    {
        if (options.Vcs.git)
            s_repo_active[repo_index(vcs::RepoType::git)] = true;
        if (options.Vcs.svn)
            s_repo_active[repo_index(vcs::RepoType::svn)] = true;

        return ModuleUsage::UsedExpensive;
    }

    return ModuleUsage::Unused;
}

/// Load module data
bool VcsModule::mod_load(const Options& options, const ModuleContext& mctx,
                         std::stop_token, CommandContext& cmdctx)
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

    // quickly check if path is under version control
    auto [rtype, repo] = check_for_repo(mctx.path());
    
    if (options.Vcs.git && rtype == vcs::RepoType::git)
    {
        vcs::GitBackend git;
        auto status = git.query(repo, options, vars, cmdctx);
        if (status)
        {
            s_status = *status;
        }
    }
    else if (options.Vcs.svn && rtype == vcs::RepoType::svn)
    {
        vcs::SvnBackend svn;
        auto status = svn.query(repo, options, vars, cmdctx);
        if (status)
        {
            s_status = *status;
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
        out += " ";  // 󱘎   󰔱 󰐅 󰹩   
}

void VcsModule::status(std::string& out)
{
    auto output = [&out, showcount=s_showcounts] (int count, char ch)
    {
        if (count > 0)
        {
            out += ch;
            if (showcount)
                out += std::to_string(count);
        }
    };
    output(s_status.conflicts, '!');
    output(s_status.unstaged,  '*');
    output(s_status.staged,    '+');
    output(s_status.untracked, '?');
    output(s_status.stashed,   '$');  // 
}

void VcsModule::divergence(std::string& out)
{
    if (s_status.ahead  > 0)
        out += "↑" + std::to_string(s_status.ahead);   // ↑ ⇡
    if (s_status.behind > 0)
        out += "↓" + std::to_string(s_status.behind);  // ↓ ⇣
}

} // namespace devprompt

