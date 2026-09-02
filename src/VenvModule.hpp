/**
 * @file VenvModule.hpp
 */

#pragma once

#include <string>

namespace devprompt
{ struct Options; }

namespace devprompt::VenvModule
{

bool load(const Options& options);

void name(std::string& out);

}
