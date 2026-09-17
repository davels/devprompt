/**
 * @file SvnBackend.cpp
 */

#include "SvnBackend.hpp"
#include <array>
#include <cstdio>
#include <string>
#include "Options.hpp"
#include "Expected.hpp"
#include "ShellCommand.hpp"
#include "utils.hpp"
#include "CommandContext.hpp"

namespace devprompt::vcs
{

RepoQueryResult SvnBackend::query(const std::string& path, const Options& options,
                                  const RepoQueryVars& vars, CommandContext& cmdctx)
{
    // Default to cwd but allow an explicit path
    const std::string work_path = path.empty() ? "." : path;
    RepoStatus status;
    status.repotype = vcs::RepoType::svn;
    
    ShellCommand cmd;
    auto setter = cmdctx.scoped_set(cmd);
    if (!setter)
        return Unexpected(RepoError::Error);
    
    // Get the working-copy revision.
    if (vars.branch && options.Vcs.commit_as_branch && !cmdctx.killed())
    {
        auto revision = cmd.run("svn", "info", "--show-item", "revision", "--non-interactive",
                                work_path.c_str());
        if (!revision)
            return Unexpected(RepoError::NotRepository);
        status.branch = utils::trimmed(*revision);
        status.detached = true;
    }

    // Get working-copy status.
    if (vars.status && !cmdctx.killed())
    {    
        std::array<char, 8> linebuf;
        if (!cmd.start("svn", "status", "-q", "--non-interactive", work_path.c_str()))
            return Unexpected(RepoError::Error);
        while (auto line = cmd.readline(linebuf))
        {
            switch (line->front())  // char in column 0
            {
            case 'M':
            case 'A':
            case 'D':
            case '!':
                ++status.unstaged;
                break;
            case 'C':
                ++status.conflicts;
                break;
            case '?':
                ++status.untracked;
                break;
            }
        }
    }
    
    return status;
}

} // namespace devprompt::vcs
