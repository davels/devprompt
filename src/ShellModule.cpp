/**
 * @file ShellModule.cpp
 */

#include <string_view>
#include "ShellModule.hpp"
#include "Options.hpp"
#include "Variable.hpp"

namespace devprompt
{

namespace
{
std::string_view s_error;
} // namespace


ModuleUsage ShellModule::mod_usage(const Options& /*options*/)
{
    if (!IsVariableInUse(VariableId::shell_error))
        return ModuleUsage::Unused;

    return ModuleUsage::Used;
}

/// Load module data
bool ShellModule::mod_load(const Options& options)
{
    if (options.Shell.error != "0")
        s_error = options.Shell.error;
    return true;
}

//
// Variables
//

void ShellModule::error(std::string& out)
{
    out += s_error;
}

} // namespace devprompt::ShellModule

