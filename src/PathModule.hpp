/**
 * @file PathModule.h
 */

#pragma once

#include <string_view>
#include "Module.hpp"

namespace devprompt
{

struct Options;

struct PathModule
{
    static constexpr std::string_view mod_name = "PathModule";
    static ModuleUsage mod_usage(const Options& options);
    static bool mod_load(const Options& options);

    static void path(std::string& out);
};

} // devprompt
