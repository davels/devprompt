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

namespace
{

Expected<std::string, int> run_command(const std::string& command)
{
    std::array<char, 4096> buffer{};
    std::string result;

    // direct stderr to stdout
    const std::string cmd = command + " 2>&1";

#ifdef _WIN32
    FILE* pipe = _popen(cmd.c_str(), "r");
#else
    FILE* pipe = popen(cmd.c_str(), "r");
#endif

    if (!pipe)
        return Unexpected(1);
    while (fgets(buffer.data(), buffer.size(), pipe))
        result += buffer.data();

#ifdef _WIN32
    const int status = _pclose(pipe);
#else
    const int status = pclose(pipe);
#endif

    if (status != 0)
        return Unexpected(status);

    return result;
}

} // namespace


RepoQueryResult SvnBackend::query(const Options& opts)
{
    // Default to cwd but allow an explicit path
    const std::string start_path = opts.path.empty() ? "." : opts.path;

    RepoStatus status;;
    std::array<char, 4096> buffer{};

    // Get the working-copy revision.
    ShellCommand cmd;
    auto revision = cmd.run(buffer,
                            "svn", "info", "--show-item", "revision", "--non-interactive",
                            start_path.c_str());
    if (!revision)
        return Unexpected(RepoError::NotRepository);
    status.branch = utils::trimmed(*revision);
    status.detached = true;

    // Get working-copy status.
    if (opts.Vcs.show_status)
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
