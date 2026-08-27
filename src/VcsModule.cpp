/**
 * @file VcsModule.cpp
 */

#include "VcsModule.hpp"
#include <string>
#include "Options.hpp"
#include "RepoStatus.hpp"
#include "GitBackend.hpp"
#include "SvnBackend.hpp"
#include "VcsFormatter.hpp"

namespace devprompt::VcsModule
{

std::string format(const Options& options)
{
    if (options.Vcs.git)
    {    
        GitBackend git;
        auto status = git.query(options);
        if (status)
        {
            return format_status(*status, options);
        }
        else if (status.error() != RepoError::NotRepository)
        {
            // maybe print the error
            return {};
        }
    }

    if (options.Vcs.svn)
    {
        SvnBackend svn;
        auto status = svn.query(options);
        if (status)
        {
            return format_status(*status,options);
        }
        else if (status.error() != RepoError::NotRepository)
        {
            // maybe print the error
            return {};
        }        
    }

    // TODO support other repo types

    return {};
}

} // namespace devprompt::VcsModule

