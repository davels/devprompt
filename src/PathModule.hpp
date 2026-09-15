/**
 * @file PathModule.h
 */

#pragma once

#include <string_view>
#include <stop_token>
#include "Module.hpp"

namespace devprompt
{

struct Options;
class CommandContext;

struct PathModule
{
    static constexpr std::string_view mod_name = "PathModule";
    static ModuleUsage mod_usage(const Options& options);
    static bool mod_load(const Options& options, std::stop_token stoken, CommandContext& cmdctx);

    static void path(std::string& out);
};

} // devprompt
