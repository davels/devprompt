/**
 * @file SysModule.hpp
 */

#pragma once

#include <string_view>
#include "Module.hpp"

namespace devprompt
{

struct Options;

struct SysModule
{
    static constexpr std::string_view mod_name = "SysModule";
    static bool mod_load(const Options& options);
    static ModuleUsage mod_usage(const Options& options);

    static void user(std::string& out);
    static void host(std::string& out);
    static void sudo_icon(std::string& out);
};

} // devprompt
