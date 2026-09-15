/**
 * @file VcsModule.hpp
 */

#pragma once

#include <string_view>
#include <stop_token>
#include "Module.hpp"

namespace devprompt
{
struct Options;
class CommandContext;

struct VcsModule
{
    static constexpr std::string_view mod_name = "VcsModule";
    static ModuleUsage mod_usage(const Options& options);
    static bool mod_load(const Options& options, std::stop_token stoken, CommandContext& cmdctx);

    static void remote_icon(std::string& out);
    static void branch(std::string& out);
    static void state(std::string& out);
    static void wt_icon(std::string& out);
    static void status(std::string& out);
    static void divergence(std::string& out);
};

} // namespace devprompt

