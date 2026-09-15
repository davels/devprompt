/**
 * @file GitBackend.hpp
 */

#pragma once

#include "RepoStatus.hpp"

namespace devprompt
{
struct Options;
class CommandContext;
}

namespace devprompt::vcs
{

class GitBackend
{
  public:
    RepoQueryResult query(const Options& options, const RepoQueryVars& vars, CommandContext& cmdctx);
};

} // namespace devprompt::vcs

