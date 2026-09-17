/**
 * @file PathModule.cpp
 */

#include "PathModule.hpp"
#include <cstdlib>
#include <string>
#include <string_view>
#include "Options.hpp"
#include "Variable.hpp"
#include "filesystem.hpp"

namespace devprompt
{

namespace
{
std::string s_path;
} // namespace


ModuleUsage PathModule::mod_usage(const Options& /*options*/)
{
    if (!IsVariableInUse(VariableId::path_path))
        return ModuleUsage::Unused;

    return ModuleUsage::Used;
}

/// Load module data
bool PathModule::mod_load(const Options& /*options*/, std::stop_token, CommandContext&)
{
    constexpr auto HOMEICON = "~";

    s_path = devprompt::filesystem::get_current_directory();
    if (s_path.empty())
        return true;  // failed, nothing more we can do
    
    const char* home_env = std::getenv("HOME");
    if (home_env)
    {
        std::string_view home{home_env};
        if (s_path.starts_with(home))
        {
            if (s_path.length() == home.length())
                s_path = HOMEICON;
            else if (s_path[home.size()] == '/')
                s_path.replace(0, home.size(), HOMEICON);
        }
    }
    return true;
}

//
// Variables
//

void PathModule::path(std::string& out)
{
    out += s_path;
}

} // namespace devprompt

