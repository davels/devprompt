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


ModuleUsage VenvModule::mod_usage(const Options&, FileScanner&)
{
    if (!IsVariableInUse(VariableId::venv_name))
        return ModuleUsage::Unused;

    return ModuleUsage::Used;
}

/// Load module data
bool VenvModule::mod_load(const Options&, const ModuleContext&, std::stop_token, CommandContext&)
{
    const char* env = std::getenv("VIRTUAL_ENV");
    if (!env || *env == '\0')
        return true;

    std::string_view env_path{env};

    // remove any trailing path separator
    while (!env_path.empty() && (env_path.back() == '/' || env_path.back() == '\\'))
        env_path.remove_suffix(1);

    auto separator = env_path.find_last_of("/\\");
    auto label = env_path.substr(separator == std::string_view::npos
                                 ? 0
                                 : separator + 1);

    // for a generic venv folder name, use the parent folder name
    if (label == ".venv" || label == "venv")
    {
        if (separator != std::string_view::npos)
        {
            env_path = env_path.substr(0, separator);
            separator = env_path.find_last_of("/\\");
            label = env_path.substr(separator == std::string_view::npos
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
