/**
 * @file OsModule.cpp
 */

#include "OsModule.hpp"
#include <cstdlib>
#include "Options.hpp"
#include "Variable.hpp"
#include "FileReader.hpp"

namespace devprompt
{

namespace
{
std::string s_distro_icon;

std::string icon_for_distro(std::string_view distro)
{
    if (distro == "debian")
        return " ";
    if (distro == "ubuntu")
        return " ";  // 󰕈
    if (distro == "arch")
        return " ";
    if (distro == "artix")
        return " ";
    if (distro == "fedora")
        return " ";  // 
    if (distro == "rhel")
        return " ";
    if (distro == "centos")
        return " ";
    if (distro == "mint")
        return " ";  // 󰣭 '󰣭"
    if (distro == "manjaro")
        return " ";
    if (distro == "popos")
        return " ";
    // if (distro == "Darwin")
    //     return " ";
    return " ";
}

// determine the distro by checking /etc/os-release
std::string get_os_release_icon()
{
    FileReader file{"/etc/os-release"};
    if (!file)
        return {};
    std::string line;
    line.reserve(256);
    while (file.readline(line))
    {
        if (line.starts_with("ID="))
        {
            return icon_for_distro(std::string_view{line}.substr(3));
        }
    }
    return {};
}

bool check_ssh()
{
    return (std::getenv("SSH_CLIENT")
            || std::getenv("SSH_TTY")
            || std::getenv("SSH_CONNECTION"));
}

} // namespace


ModuleUsage OsModule::mod_usage(const Options& /*options*/)
{
    if (!IsVariableInUse(VariableId::os_distro_icon))
        return ModuleUsage::Unused;

    return ModuleUsage::Used;
}

/// Load module data
bool OsModule::mod_load(const Options& options)
{
#ifdef _WIN32
    s_distro_icon = " ";
#else    
    if (options.Os.wsl_is_distro && std::getenv("WSL_DISTRO_NAME") != nullptr)
    {
        s_distro_icon = " ";  // WSL
    }
    else
    {
        s_distro_icon = get_os_release_icon();
    }
#endif

    if (options.Os.show_ssh && check_ssh())
        s_distro_icon.insert(0, "");
    return true;
}

//
// Variables
//

void OsModule::distro_icon(std::string& out)
{
    out += s_distro_icon;
}

} // namespace devprompt

