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
    std::array<char, 4096> buffer{};

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
        if (!cmd.start("svn", "status", "-q", "--non-interactive", start_path.c_str()))
            return Unexpected(RepoError::Error);
        
        bool beginning_of_line = true;
        while (const auto len = cmd.read(buffer) != 0)
        {
            const std::span<const char> data{buffer.data(), len};
            for (char ch: data)
            {
                if (beginning_of_line)
                {
                    switch (ch)
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
                    beginning_of_line = false;
                }

                if (ch == '\n')
                    beginning_of_line = true;
            }   
        }
    }
    return status;
}

} // namespace devprompt::VcsModule
