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
    RepoQueryResult query(const Options& opts);
};

} // namespace devprompt::VcsModule

