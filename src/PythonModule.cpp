/**
 * @file PythonModule.cpp
 */

#include "PythonModule.hpp"
#include <cstdlib>
#include <string>
#include <string_view>
#include "Variable.hpp"
#include "Options.hpp"
#include "FileScanner.hpp"
#include "CommandContext.hpp"
#include "ShellCommand.hpp"
#include "utils.hpp"

namespace devprompt
{

namespace
{
unsigned int s_py_fileidx;
std::string s_venv;
std::string s_version;

// sets the static venv name and returns true if a venv is active
bool lookup_venv(bool check_only)
{
    const char* env = std::getenv("VIRTUAL_ENV");
    if (!env || *env == '\0')
        return false;

    if (check_only)
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

    s_venv = label;
    return true;    
}

// sets the static version
void lookup_version(const Options& options, CommandContext& cctx)
{
    ShellCommand<32> cmd;
    auto setter = cctx.scoped_set(cmd);
    if (!setter)
        return;

    auto check = [&cmd](const std::string& pycmd) -> bool {
        if (auto result = cmd.run(pycmd, "--version"))
        {
            auto start = result->find_first_of("0123456789");
            if (start == std::string_view::npos)
                return false;  // not a valid version string

            auto end = result->find_first_of(" \t\r\n", start);
            s_version = result->substr(start, end - start);  // math is fine, even if end==npos
            return true;
        }
        return false;
    };

    if (!options.Python.binary.empty())
    {
        for (auto pycmd:
                 utils::split_view(options.Python.binary, ';')
                 | std::views::transform(utils::trimmed))
        {
            if (pycmd.empty()) continue;
            if (check(std::string(pycmd))) return; // got it
        }
    }
    else
    {
        constexpr std::array pythons = {"python3", "python"};
        for (const auto& pycmd: pythons)
        {
            if (check(pycmd)) return; // got it
        }
    }
}

} // namespace


ModuleUsage PythonModule::mod_usage(const Options& options, FileScanner& fscanner)
{
    bool var_venv = IsVariableInUse(VariableId::python_venv);
    bool var_version = IsVariableInUse(VariableId::python_version);

    if (!var_venv && !var_version)
        return ModuleUsage::Unused;

    if (var_version &&
        (options.Python.vmode == Options::PythonVersionMode::Files ||
         options.Python.vmode == Options::PythonVersionMode::Context))
    {
        s_py_fileidx = fscanner.add_extension("py");
    }

    return var_version? ModuleUsage::UsedExpensive: ModuleUsage::Used;
}

/// Load module data
bool PythonModule::mod_load(const Options& options, const ModuleContext& mctx, std::stop_token, CommandContext& cctx)
{
    bool var_venv = IsVariableInUse(VariableId::python_venv);
    bool var_version = IsVariableInUse(VariableId::python_version);

    bool has_venv = false;
    if (var_venv ||
        (var_version && (options.Python.vmode == Options::PythonVersionMode::Venv ||
                         options.Python.vmode == Options::PythonVersionMode::Context)))
    {
        has_venv = lookup_venv(!var_venv);
    }

    // NOTE: FileScanner access can block so do this as late as possible
    if (var_version &&
        ((options.Python.vmode == Options::PythonVersionMode::Venv && has_venv)
         ||
         (options.Python.vmode == Options::PythonVersionMode::Files &&
          mctx.filescanner().check_pattern(s_py_fileidx))
         ||
         (options.Python.vmode == Options::PythonVersionMode::Context &&
          (has_venv || mctx.filescanner().check_pattern(s_py_fileidx)))))
    {
        lookup_version(options, cctx);
    }

    return true;
}


//
// Variable functions
//

void PythonModule::venv(std::string& out)
{
    out += s_venv;
}

void PythonModule::version(std::string& out)
{
    out += s_version;
}

} // namespace devprompt
