/**
 * @file SysModule.hpp
 */

#pragma once

#include <string>

namespace devprompt
{ struct Options; }

namespace devprompt::SysModule
{

bool load(const Options& options);

void user(std::string& out);
void host(std::string& out);
void sudo_icon(std::string& out);

} // devprompt::SysModule
