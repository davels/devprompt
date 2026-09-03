/**
 * @file PathModule.cpp
 */

#include "PathModule.hpp"
#include <cstdlib>
#include <string>
#include <string_view>
#include "Options.hpp"
#include "Variable.hpp"

namespace devprompt::PathModule
{

namespace
{
std::string s_path;
} // namespace


/// Load module data
bool load([[maybe_unused]] const Options& options)
{
    if (!IsVariableInUse(VariableId::path_path))
        return false;

    constexpr auto HOMEICON = "~";
    
    const char* pwd = std::getenv("PWD");
    if (!pwd || *pwd == '\0')
        return true;
    s_path = pwd;
    
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

void path(std::string& out)
{
    out += s_path;
}

} // namespace devprompt::PathModule

