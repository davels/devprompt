/**
 * @file ShellModule.cpp
 */

#include <string_view>
#include "ShellModule.hpp"
#include "Options.hpp"
#include "Variable.hpp"

namespace devprompt::ShellModule
{

namespace
{
std::string_view s_error;

} // namespace

/// Load module data
bool load(const Options& options)
{
    if (!IsVariableInUse(VariableId::shell_error))
        return false;

    if (options.Shell.error != "0")
        s_error = options.Shell.error;
    return true;
}

//
// Variables
//

void error(std::string& out)
{
    out += s_error;
}

} // namespace devprompt::ShellModule

