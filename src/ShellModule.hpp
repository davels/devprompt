/**
 * @file ShellModule.hpp
 */

#pragma once

#include <string>

namespace devprompt
{ struct Options; }

namespace devprompt::ShellModule
{

bool load(const Options& options);

void error(std::string& out);

} // devprompt::ShellModule
