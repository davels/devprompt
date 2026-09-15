/**
 * @file VenvModule.hpp
 */

#pragma once

#include <string_view>
#include <stop_token>
#include "Module.hpp"

namespace devprompt
{

struct Options; 
class CommandContext;

struct VenvModule
{
    static constexpr std::string_view mod_name = "VenvModule";
    static ModuleUsage mod_usage(const Options& options);
    static bool mod_load(const Options& options, std::stop_token stoken, CommandContext& cmdctx);
    
    static void name(std::string& out);
};

} // namespace devprompt
