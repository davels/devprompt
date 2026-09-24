/**
 * @file OsModule.hpp
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

struct OsModule
{
    static constexpr std::string_view mod_name = "OsModule";
    static ModuleUsage mod_usage(const Options& options, FileScanner& fscanner);
    static bool mod_load(const Options& options, const ModuleContext& mctx,
                         std::stop_token stoken, CommandContext& cmdctx);

    static void distro_icon(std::string& out);
};

} // namespace devprompt
