/**
 * @file ShellModule.hpp
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

struct ShellModule
{
    static constexpr std::string_view mod_name = "ShellModule";
    static ModuleUsage mod_usage(const Options& options, FileScanner& scanner);
    static bool mod_load(const Options& options, const ModuleContext& mctx,
                         std::stop_token stoken, CommandContext& cmdctx);
    
    static void error(std::string& out);
};

} // devprompt::ShellModule
