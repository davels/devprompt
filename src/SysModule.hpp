/**
 * @file SysModule.hpp
 */

#pragma once

#include <string_view>
#include <stop_token>
#include "Module.hpp"

namespace devprompt
{

struct Options;
class FileScanner;
class ModuleContext;
class CommandContext;

struct SysModule
{
    static constexpr std::string_view mod_name = "SysModule";
    static ModuleUsage mod_usage(const Options& options, FileScanner& fscanner);
    static bool mod_load(const Options& options, const ModuleContext&,
                         std::stop_token stoken, CommandContext& cmdctx);

    static void user(std::string& out);
    static void host(std::string& out);
    static void sudo_icon(std::string& out);
};

} // devprompt
