/**
 * @file VenvModule.hpp
 */

#pragma once

#include <string>

namespace devprompt
{
struct Options;
}

namespace devprompt::VenvModule
{
std::string format(const Options& options);
}
