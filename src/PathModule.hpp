/**
 * @file PathModule.h
 */

#pragma once

#include <string>

namespace devprompt
{ struct Options; }

namespace devprompt::PathModule
{

bool load(const Options& options);

void path(std::string& out);

} // devprompt::PathModule
