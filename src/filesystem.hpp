/**
 * @file filesystem.hpp
 */

#pragma once

#include <string>

namespace devprompt
{

bool path_exists(const std::string& path);

bool is_dir(const std::string& path);

} // namespace
