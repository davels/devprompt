/**
 * @file VcsFormatter.hpp
 */

#pragma once

#include <string>

namespace devprompt
{
struct Options;
} // namespace devprompt

namespace devprompt::VcsModule
{
struct RepoStatus;

std::string format_status(const RepoStatus& s, const Options& options);
} // namespace devprompt::VcsModule

