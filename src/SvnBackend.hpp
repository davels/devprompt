/**
 * @file SvnBackend.hpp
 */

#pragma once

#include "RepoStatus.hpp"

namespace devprompt
{
class Options;
} // namespace devprompt

namespace devprompt::VcsModule
{

class SvnBackend
{
public:
    RepoQueryResult query(const Options& opts, const RepoQueryVars& vars);
};

} // namespace devprompt::VcsModule

