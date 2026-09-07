/**
 * @file GitBackend.cpp
 *
 * Untracked-file detection walks the working tree and is the slowest part of a status
 * call in large repos. Stash checking is a small extra cost.
 */

#include "GitBackend.hpp"
#include <string>
#include <charconv>
#include <cstddef>
#include "Options.hpp"
#include "ShellCommand.hpp"
#include "filesystem.hpp"

namespace devprompt::VcsModule
{

namespace
{

// std::string current_op_name(git_repository* repo)
// {
//     switch (git_repository_state(repo)) {
//     case GIT_REPOSITORY_STATE_NONE:               return "";
//     case GIT_REPOSITORY_STATE_MERGE:              return "MERGE";
//     case GIT_REPOSITORY_STATE_REVERT:             return "REVERT";
//     case GIT_REPOSITORY_STATE_CHERRYPICK:         return "CHERRY-PICK";
//     case GIT_REPOSITORY_STATE_BISECT:             return "BISECT";
//     case GIT_REPOSITORY_STATE_REBASE:             return "REBASE";
//     case GIT_REPOSITORY_STATE_REBASE_INTERACTIVE: return "REBASE-i";
//     case GIT_REPOSITORY_STATE_REBASE_MERGE:       return "REBASE-m";
//     default:                                      return "IN-PROGRESS";
//     }
// }

std::string git_host_icon(std::string_view url, const Options& options)
{
    auto it = std::ranges::find_if(options.Vcs.remoteicon_mapping,
                                   [url](const auto& item) {
                                       return url.find(item.pattern) != std::string::npos;
                                   });
    if (it != options.Vcs.remoteicon_mapping.end())
    {
        return it->icon;
    }
    if (url.find("github.com") != std::string_view::npos)
        return " ";
    if (url.find("gitlab.com") != std::string_view::npos)
        return " ";
    if (url.find("git.sv.gnu.org") != std::string_view::npos)
        return " ";
    return std::string{};
}

Expected<RepoStatus,int> git_status_cmd(const Options& options, const RepoQueryVars& vars)
{
    RepoStatus status;
    
    CommandArgs<12> args;
    args.push_back("git");
    if (!options.path.empty())
    {
        args.push_back("-C");
        args.push_back(options.path.c_str());
    }
    args.push_back("status");
    args.push_back("--porcelain=2");
    args.push_back("--branch");
    args.push_back("--no-renames");
    if (!options.Vcs.show_untracked)
        args.push_back("--untracked-files=no");
    if (options.Vcs.show_stashed)
        args.push_back(("--show-stash"));
    // TODO logic needs review
    if (options.Vcs.ignore_submodules)
        args.push_back("--ignore-submodules=dirty");
    // else
    //     args.push_back(("--ignore-submodules="))
    if (!vars.status)
    {
        // this must be the last argument. it will limit git status to this single file
        // and effectively skip file status scanning
        args.push_back("--");
        args.push_back(".zdummyz");
    }

    std::array<char, 50> linebuf;  // only care about the start of the lines
    ShellCommand cmd;
    if (!cmd.start(args.span()))
        return Unexpected(1);

    std::string branch_head_hash;
    std::string remote_alias;
    while (auto line = cmd.readline(linebuf))
    {
        if (line->empty()) continue;
        char format = (*line)[0];
        
        if (format == '#')
        {
            std::string_view& header = *line;
            //# Branch name
            if (vars.branch && header.starts_with("# branch.head "))
            {
                header.remove_prefix(14);
                if (header == "(detatched)")
                    status.detached = true;
                else
                    status.branch = header;
            }
            else if (vars.branch && header.starts_with("# branch.oid"))
            {
                header.remove_prefix(13);
                if (header.length() >= 7)
                    branch_head_hash = std::string{header.substr(0,7)};
            }
            //# Upsteam url
            else if (vars.remote_icon && header.starts_with("# branch.upstream "))
            {
                header.remove_prefix(18);
                //auto upstream_name = std::string{header};
    
                // Extract the remote alias name (everything before the first '/')
                auto slash_pos = header.find('/');
                if (slash_pos != std::string_view::npos)
                {
                    remote_alias = header.substr(0, slash_pos); // eg. "origin"
                }
            }            
            //# Ahead/Behind
            else if (vars.divergence && header.starts_with("# branch.ab"))
            {
                header.remove_prefix(12);
                // header is now "+A -B"
                if (header.length() > 4 && header[0] == '+')
                {
                    // parse ahead integer
                    auto [endptr, ec] = std::from_chars(header.data()+1, header.data() + header.size(), status.ahead);
                    if (ec != std::errc()) // stop if not success
                        continue;
                    std::size_t consumed = endptr - header.data();
                    header.remove_prefix(consumed);
                    // parse behind integer
                    if (header.length() > 4 && header[0] == ' ' && header[1] == '-')
                    {
                        std::from_chars(header.data()+2, header.data() + header.size(), status.behind);
                    }
                }
            }
            //# Stash count
            if (header.starts_with("# stash "))
            {
                header.remove_prefix(8);
                std::from_chars(header.data(), header.data() + header.size(), status.stashed);
            }
        }
        else if (format == '?')
        {
            status.untracked++;
        }
        else if (format == 'u')
        {
            status.conflicts++;
        }
        else if (format == '1' || format == '2')
        {
            char x = (*line)[2]; 
            char y = (*line)[3]; 

            // Staged modifications
            if (x == 'M' || x == 'A' || x == 'D' || x == 'R' || x == 'C')
            {
                status.staged++;
            }
            // Unstaged modifications
            if (y == 'M' || y == 'D' || y == 'R' || y == 'C')
            {
                status.unstaged++;
            }
        }
    } 
    // if detached then set branch to the commit hash
    if (status.detached && options.Vcs.commit_as_branch)
    {
        if (branch_head_hash.empty())
            status.branch = "(detached)";
        else
            status.branch = branch_head_hash;
    }
    if (auto ret = cmd.wait() != 0)
        return Unexpected(ret);

    // host icon based on url of remote 
    if (vars.remote_icon && !remote_alias.empty())
    {
        auto remotearg = "remote." + std::string{remote_alias} + ".url";
        auto url = cmd.run("git", "config", "--get", remotearg.c_str());
        if (url)
        {
            status.remote_icon = git_host_icon(*url, options);
        }
    }

    return status;
}

std::string determine_repo_state(const std::string& gitdir)
{
    // 1. REBASE States
    if (path_exists(gitdir + "/rebase-merge"))
    {
        if (path_exists(gitdir + "/rebase-merge/interactive"))
        {
            return "REBASE-i";
        }
        return "REBASE-m";
    }
    if (path_exists(gitdir + "/rebase-apply"))
    {
        return "REBASE";
    }

    // 2. MERGE State
    if (path_exists(gitdir + "/MERGE_HEAD"))
    {
        return "MERGE";
    }

    // 3. REVERT States
    if (path_exists(gitdir + "/REVERT_HEAD"))
    {
        return "REVERT";
    }

    // 4. CHERRY-PICK States
    if (path_exists(gitdir + "/CHERRY_PICK_HEAD"))
    {
        return "CHERRY-PICK";
    }

    // 5. BISECT State
    if (path_exists(gitdir + "/BISECT_LOG"))
    {
        return "BISECT";
    }

    return std::string{};
}

} // namespace


RepoQueryResult GitBackend::query(const Options& options, const RepoQueryVars& vars)
{
    std::string gitdir;
    ShellCommand<512> cmd;

    // Default to cwd but allow an explicit path
    const std::string work_path = options.path.empty() ? "." : options.path;

    // need the git repo directory for some operations
    if (vars.state || vars.wt_icon)
    {   
        auto dir = cmd.run("git", "-C", work_path.c_str(), "rev-parse", "--path-format=absolute", "--git-dir");
        if (!dir || dir->empty())
            return Unexpected(RepoError::NotRepository);
        gitdir = *dir;
    }

    // repo status: branch, status counts, remote icon
    auto status = git_status_cmd(options, vars);
    if (!status)
        return Unexpected(RepoError::NotRepository);

    // current operation
    if (vars.state)
    {
        auto state = determine_repo_state(gitdir);
        if (!state.empty())
            status->state = state;
    }

    // worktree test
    if (vars.wt_icon)
    {
        auto dir = cmd.run("git", "-C", work_path.c_str(), "rev-parse", "--path-format=absolute", "--git-common-dir");
        if (dir && !dir->empty() && *dir != gitdir)
        {
            status->isworktree = true;
        }
    }
    
    return *status;
}

} // namespace vcsprompt::VcsModule

