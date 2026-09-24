/**
 * @file PathModule.cpp
 */

#include "PathModule.hpp"
#include <cstdlib>
#include <string>
#include <string_view>
#include "Options.hpp"
#include "Variable.hpp"

namespace devprompt
{

namespace
{
std::string s_path;
} // namespace


ModuleUsage PathModule::mod_usage(const Options&, FileScanner&)
{
    if (!IsVariableInUse(VariableId::path_path))
        return ModuleUsage::Unused;

    return ModuleUsage::Used;
}

/// Load module data
bool PathModule::mod_load(const Options&, const ModuleContext& mctx, std::stop_token, CommandContext&)
{
    static constexpr auto HOMEICON = "~";

    s_path = mctx.path();
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

