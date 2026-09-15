/**
 * @file VenvModule.cpp
 */

#include "VenvModule.hpp"
#include <cstdlib>
#include <string>
#include <string_view>
#include "Options.hpp"
#include "Variable.hpp"

namespace devprompt
{

namespace
{
std::string s_name;
} // namespace


ModuleUsage VenvModule::mod_usage(const Options& /*options*/)
{
    if (!IsVariableInUse(VariableId::venv_name))
        return ModuleUsage::Unused;

    return ModuleUsage::Used;
}

/// Load module data
bool VenvModule::mod_load([[maybe_unused]] const Options& options, std::stop_token, CommandContext&)
{
    const char* env = std::getenv("VIRTUAL_ENV");
    if (!env || *env == '\0')
        return true;

    std::string_view path{env};

    // remove any trailing path separator
    while (!path.empty() && (path.back() == '/' || path.back() == '\\'))
        path.remove_suffix(1);

    auto separator = path.find_last_of("/\\");
    auto label = path.substr(separator == std::string_view::npos
                             ? 0
                             : separator + 1);

    if (label == ".venv" || label == "venv")
    {
        if (separator != std::string_view::npos)
        {
            path = path.substr(0, separator);
            separator = path.find_last_of("/\\");
            label = path.substr(separator == std::string_view::npos
                                ? 0
                                : separator + 1);
        }
    }

    s_name = label;
    return true;
}


//
// Variable functions
//

void VenvModule::name(std::string& out)
{
    out += s_name;
}

} // namespace devprompt
