/**
 * @file OsModule.hpp
 */

#pragma once

#include <string_view>
#include "Module.hpp"

namespace devprompt
{

struct Options;

struct OsModule
{
    static constexpr std::string_view mod_name = "OsModule";
    static ModuleUsage mod_usage(const Options& options);
    static bool mod_load(const Options& options);

    static void distro_icon(std::string& out);
};

} // namespace devprompt
