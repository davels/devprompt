/**
 * @file VenvModule.hpp
 */

#pragma once

#include <string_view>
#include "Module.hpp"

namespace devprompt
{

struct Options; 

struct VenvModule
{
    static constexpr std::string_view mod_name = "VenvModule";
    static ModuleUsage mod_usage(const Options& options);
    static bool mod_load(const Options& options);
    
    static void name(std::string& out);
};

} // namespace devprompt
