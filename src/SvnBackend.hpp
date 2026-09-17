/**
 * @file SvnBackend.hpp
 */

#pragma once

#include "RepoStatus.hpp"

namespace devprompt
{
class Options;
class CommandContext;
}

namespace devprompt::vcs
{

class SvnBackend
{
public:
    RepoQueryResult query(const std::string& path, const Options& opts, const RepoQueryVars& vars, CommandContext& cmdctx);
};

} // namespace devprompt::vcs

