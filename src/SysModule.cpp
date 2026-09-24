/**
 * @file SysModule.cpp
 */

#include "SysModule.hpp"
#include <string>

// get_username
#ifdef _WIN32
#include <windows.h>
#else
#include <pwd.h>
#include <sys/types.h>
#include <unistd.h>
#endif

// get_hostname - win32 needs ws2_32.lib
#ifdef _WIN32
#include <winsock2.h>
#else
#include <unistd.h>
#endif

#ifdef _MSC_VER    
#pragma comment(lib, "Ws2_32.lib")
#endif

#include "Variable.hpp"
#include "ShellCommand.hpp"
#include "CommandContext.hpp"
    
namespace devprompt
{

namespace
{
std::string s_user, s_host;
bool s_sudo_active = false;

std::string get_username()
{
#ifdef _WIN32
    char buffer[256];
    DWORD size = sizeof(buffer);

    if (GetUserNameA(buffer, &size))
        return buffer;
    return {};
#else
    if (auto* pw = getpwuid(getuid()))
        return pw->pw_name;
    return {};
#endif
}

std::string get_hostname()
{
    char buffer[256];

    if (gethostname(buffer, sizeof(buffer)) == 0)
        return {buffer};

    return {};
}
} // namespace



ModuleUsage SysModule::mod_usage(const Options&, FileScanner&)
{    
    if (!IsVariableInUse(VariableId::sys_user)
        && !IsVariableInUse(VariableId::sys_host)
        && !IsVariableInUse(VariableId::sys_sudo_icon))
        return ModuleUsage::Unused;

    if (IsVariableInUse(VariableId::sys_sudo_icon))
        return ModuleUsage::UsedExpensive;
    
    return ModuleUsage::Used;
}

/// Load module data
bool SysModule::mod_load(const Options&, const ModuleContext&, std::stop_token, CommandContext& cmdctx)
{
    if (!IsVariableInUse(VariableId::sys_user)
        && !IsVariableInUse(VariableId::sys_host)
        && !IsVariableInUse(VariableId::sys_sudo_icon))
        return false;
    
    if (IsVariableInUse(VariableId::sys_user))
        s_user = get_username();
    if (IsVariableInUse(VariableId::sys_host))
        s_host = get_hostname();
    if (IsVariableInUse(VariableId::sys_sudo_icon) && !cmdctx.killed())
    {
        // not available on win32
#ifndef _WIN32
        // if sudo credentials are still active, the will return 0 with no output
        ShellCommand<16> cmd;
        auto setter = cmdctx.scoped_set(cmd);
        if (!setter)
            return false;
        auto result = cmd.run("sudo", "-n", "true", "2>/dev/null");
        if (result)
            s_sudo_active = true;
#endif
    }
    return true;
}


//
// Variables
//

void SysModule::user(std::string& out)
{
    out += s_user;
}

void SysModule::host(std::string& out)
{
    out += s_host;
}

void SysModule::sudo_icon(std::string& out)
{
    if (s_sudo_active)
        out += " ";
}

} // namespace devprompt

