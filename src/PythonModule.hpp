/**
 * @file PythonModule.hpp
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

struct PythonModule
{
    static constexpr std::string_view mod_name = "PythonModule";
    static ModuleUsage mod_usage(const Options& options, FileScanner& fscanner);
    static bool mod_load(const Options& options, const ModuleContext&,
                         std::stop_token stoken, CommandContext& cmdctx);
    
    static void venv(std::string& out);
    static void version(std::string& out);
};

} // namespace devprompt
