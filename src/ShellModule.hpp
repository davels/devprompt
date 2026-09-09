/**
 * @file ShellModule.hpp
 */

#pragma once

#include <string_view>
#include "Module.hpp"

namespace devprompt
{

struct Options;

struct ShellModule
{
    static constexpr std::string_view mod_name = "ShellModule";
    static ModuleUsage mod_usage(const Options& options);
    static bool mod_load(const Options& options);
    
    static void error(std::string& out);
};

} // devprompt::ShellModule
