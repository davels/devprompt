/**
 * @file SvnBackend.hpp
 */

#pragma once

#include "RepoStatus.hpp"

namespace devprompt
{ class Options; }

namespace devprompt::vcs
{

class SvnBackend
{
public:
    RepoQueryResult query(const Options& opts, const RepoQueryVars& vars);
};

} // namespace devprompt::vcs

