/**
 * @file GitBackend.hpp
 */

#pragma once

#include "RepoStatus.hpp"

namespace devprompt
{
struct Options;
}

namespace devprompt::VcsModule
{

class GitBackend
{
  public:
    RepoQueryResult query(const Options& options, const RepoQueryVars& vars);
};

} // namespace devprompt::VcsModule

