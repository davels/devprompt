/**
 * @file VcsModule.hpp
 */

#pragma once

#include <string>

namespace devprompt
{
struct Options;
}

namespace devprompt::VcsModule
{
std::string format(const Options& options);
}

