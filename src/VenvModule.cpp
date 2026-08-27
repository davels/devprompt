/**
 * @file VenvModule.cpp
 */

#include "VenvModule.hpp"
#include <cstdlib>
#include <string>
#include <string_view>
#include "Options.hpp"
#include "AnsiColor.hpp"

namespace devprompt::VenvModule
{

std::string format(const Options& options)
{
    const char* env = std::getenv("VIRTUAL_ENV");
    if (!env || *env == '\0')
        return {};

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

    std::string out;
    ansi::append_style(options.Venv.style, options.output, out);
    out += options.Venv.icon;
    //out += "\ued1b-";
    out += label;
    return out;
}

} // namespace devprompt::VenvModule
