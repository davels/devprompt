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

namespace devprompt::VcsModule
{

RepoQueryResult SvnBackend::query(const Options& options, const RepoQueryVars& vars)
{
    // Default to cwd but allow an explicit path
    const std::string start_path = options.path.empty() ? "." : options.path;

    RepoStatus status;;
    std::array<char, 4096> buffer;
    ShellCommand cmd;
    
    // Get the working-copy revision.
    if (vars.branch && options.Vcs.commit_as_branch)
    {
        auto revision = cmd.run(buffer,
                                "svn", "info", "--show-item", "revision", "--non-interactive",
                                start_path.c_str());
        if (!revision)
            return Unexpected(RepoError::NotRepository);
        status.branch = utils::trimmed(*revision);
        status.detached = true;
    }

    // Get working-copy status.
    if (vars.status)
    {    
        std::array<char, 8> linebuf;
        if (!cmd.start("svn", "status", "-q", "--non-interactive", start_path.c_str()))
            return Unexpected(RepoError::Error);
        while (auto line = cmd.readline(buffer, linebuf))
        {
            switch ((*line)[0])  // char in column 0
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

} // namespace devprompt::VcsModule
