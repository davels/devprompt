/**
 * @file filesystem.hpp
 */

#pragma once

#include <string>

namespace devprompt::filesystem
{

std::string get_current_directory();

bool path_exists(const std::string& path);

bool is_dir(const std::string& path);

std::string expand_path(const std::string& path);

} // namespace devprompt::filesystem
