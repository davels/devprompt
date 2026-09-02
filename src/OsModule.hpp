/**
 * @file OsModule.hpp
 */

#pragma once

#include <string>

namespace devprompt
{ struct Options; }

namespace devprompt::OsModule
{

bool load(const Options& options);

void distro_icon(std::string& out);

} // devprompt::OsModule
