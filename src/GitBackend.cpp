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
                                       return url.find(item.hostname) != std::string::npos;
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

Expected<RepoStatus,int> git_status_cmd(const Options& options)
{
    // defaults for: git status --porcelain=2
    //   untracked file mode on =  -unormal
    //   branch header disabled
    //   stash header disabled
    //   ignored files disabled
    //   ignore submodules is off = --ignore-submodules=none

    // default for --ignore-submodules is none (full scan all submodules)
    // if !has_untracked 
    //         args.push("--untracked-files=no");
    // if config.ignore_submodules 
    //        args.push("--ignore-submodules=dirty");
    // else if !has_untracked 
    //        args.push("--ignore-submodules=untracked");
    // --show-stash
    //
    
    RepoStatus status;
    std::array<char, 4096> buffer{};
    std::array<char, 50> linebuf{};  // only care about the start of the lines

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
    if (!options.Vcs.show_status)
    {
        // this must be the last argument. it will limit git status to this single file
        // and effectively skip file status scanning
        args.push_back("--");
        args.push_back(".zdummyz");
    }
    
    ShellCommand cmd;
    if (!cmd.start(args.span()))
        return Unexpected(1);

    std::string head_hash;
    std::string remote_alias;
    while (auto line = cmd.readline(buffer, linebuf))
    {
        if (line->empty()) continue;
        char format = (*line)[0];
        
        if (format == '#')
        {
            std::string_view str{line->data(), line->size()};
            //# Branch name
            if (str.starts_with("# branch.head "))
            {
                str.remove_prefix(14);
                if (str == "(detatched)")
                    status.detached = true;
                else
                    status.branch = str;
            }
            else if (str.starts_with("# branch.oid"))
            {
                str.remove_prefix(13);
                if (str.length() >= 7)
                    head_hash = std::string{str.substr(0,7)};
            }
            //# Upsteam url
            else if (options.Vcs.show_remote_icon &&
                     str.starts_with("# branch.upstream "))
            {
                str.remove_prefix(18);
                //auto upstream_name = std::string{str};
    
                // Extract the remote alias name (everything before the first '/')
                auto slash_pos = str.find('/');
                if (slash_pos != std::string_view::npos)
                {
                    remote_alias = str.substr(0, slash_pos); // e.g., "origin"
                }
            }            
            //# Ahead/Behind
            else if (str.starts_with("# branch.ab"))
            {
                str.remove_prefix(12);
                // str is now "+A -B"
                if (str.length() > 4 && str[0] == '+')
                {
                    // parse ahead integer
                    auto [endptr, ec] = std::from_chars(str.data()+1, str.data() + str.size(), status.ahead);
                    if (ec != std::errc()) // stop if not success
                        continue;
                    std::size_t consumed = endptr - str.data();
                    str.remove_prefix(consumed);
                    // parse behind integer
                    if (str.length() > 4 && str[0] == ' ' && str[1] == '-')
                    {
                        std::from_chars(str.data()+2, str.data() + str.size(), status.behind);
                    }
                }
            }
            //# Stash count
            if (str.starts_with("# stash "))
            {
                str.remove_prefix(8);
                std::from_chars(str.data(), str.data() + str.size(), status.stashed);
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
    if (status.detached)
    {
        if (head_hash.empty())
            status.branch = "(detached)";
        else
            status.branch = head_hash;
    }
    if (auto ret = cmd.wait() != 0)
        return Unexpected(ret);

    // host icon based on url of remote 
    if (options.Vcs.show_remote_icon && !remote_alias.empty())
    {
        auto remotearg = "remote." + std::string{remote_alias} + ".url";
        auto url = cmd.run(buffer,
                           "git", "config", "--get", remotearg.c_str());
        if (url)
        {
            status.host_icon = git_host_icon(*url, options);
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


RepoQueryResult GitBackend::query(const Options& options)
{
    std::array<char, 4096> buffer{};
    std::string gitdir;
    ShellCommand cmd;

    // Default to cwd but allow an explicit path
    const std::string work_path = options.path.empty() ? "." : options.path;

    // need the git repo directory for some operations
    if (options.Vcs.show_state || options.Vcs.show_worktree_icon)
    {   
        auto dir = cmd.run(buffer, "git", "-C", work_path.c_str(), "rev-parse", "--path-format=absolute", "--git-dir");
        if (!dir || dir->empty())
            return Unexpected(RepoError::NotRepository);
        gitdir = *dir;
    }

    // repo status: branch, status counts, remote icon
    auto status = git_status_cmd(options);
    if (!status)
        return Unexpected(RepoError::NotRepository);

    // current operation
    if (options.Vcs.show_state)
    {
        auto state = determine_repo_state(gitdir);
        if (!state.empty())
            status->op_in_progress = state;
    }

    // worktree test
    if (options.Vcs.show_worktree_icon)
    {
        auto dir = cmd.run(buffer, "git", "-C", work_path.c_str(), "rev-parse", "--path-format=absolute", "--git-common-dir");
        if (dir && !dir->empty() && *dir != gitdir)
        {
            status->isworktree = true;
        }
    }
    
    return *status;
}

} // namespace vcsprompt::VcsModule

