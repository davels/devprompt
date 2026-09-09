/**
 * @file runner.cpp
 */

#include "runner.hpp"
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <chrono>
#include <variant>  // for monostate
#include "Options.hpp"
#include "FormatString.hpp"
#include "utils.hpp"
#include "styleparser.hpp"
#include "ModuleRegistry.hpp"
#include "Module.hpp"
#include "ModuleInc.hpp"

namespace devprompt
{
using utils::write_stdout;

namespace
{

bool s_timing_on = false;

inline std::string format_timing(std::chrono::steady_clock::duration elapsed)
{
    std::string elapsedstr;
    using namespace std::chrono;
    if (elapsed < 1ms)
    {
        auto strval = std::to_string(duration_cast<microseconds>(elapsed).count());
        elapsedstr += std::string(strval.length()>3? 0: 3-strval.length(), ' ') + strval;
        elapsedstr += "us \n";    
    }
    else
    {
        auto strval = std::to_string(duration_cast<milliseconds>(elapsed).count());
        elapsedstr += std::string(strval.length()>3? 0: 3-strval.length(), ' ') + strval;
        elapsedstr += "ms \n";    
    }
    return elapsedstr;
}

/// Collect timing data for function call
template<typename F>
auto timed(F&& f)
{
    if (!s_timing_on)
    {
        if constexpr (std::is_void_v<std::invoke_result_t<F>>)
        {
            std::forward<F>(f)();
            return std::pair{std::monostate{},
                             std::chrono::steady_clock::duration{}};
        }
        else
        {
            return std::pair{std::forward<F>(f)(),
                             std::chrono::steady_clock::duration{}};
        }
    }

    const auto start = std::chrono::steady_clock::now();
    if constexpr (std::is_void_v<std::invoke_result_t<F>>)
    {
        std::forward<F>(f)();
        const auto elapsed = std::chrono::steady_clock::now() - start;
        return std::pair{std::monostate{}, elapsed};
    }
    else
    {
        auto result = std::forward<F>(f)();
        const auto elapsed = std::chrono::steady_clock::now() - start;
        return std::pair{std::move(result), elapsed};
    }
}

} // namespace


Expected<void,std::string> run(Options& options)
{
    std::chrono::steady_clock::time_point run_start_time;
    std::chrono::steady_clock::duration parse_elapsed, expand_elapsed;
    s_timing_on = options.show_timing;

    if (s_timing_on)
        run_start_time = std::chrono::steady_clock::now();

    // steal the format string to avoid a copy
    FormatString format(std::move(options.format));

    // parse the format string which also tags used variables
    {
        auto [result,elapsed] = timed([&] { return format.compile(); });
        if (!result)
            return result;
        parse_elapsed = elapsed;
    }
    
    // Expand any specified palette
    Palette palette;
    if (!options.palette.empty())
    {
        auto p = palette_from_string(options.palette);
        if (!p)
            return Unexpected(std::move(p).error());
        palette.swap(*p);
    }

    // check each module for usage
    for (auto& mod: s_modules)
    {
        mod.usage = mod.mod_usage(options);
    }

    // load the module data
    for (auto& mod: s_modules)
    {
        if (mod.usage == ModuleUsage::Unused) continue;
        auto [xvoidx, elapsed] = timed([&] { mod.mod_load(options); });
        mod.elapsed = elapsed;
    }

    // Expand the format string and write it out
    std::string out;
    out.reserve(128);
    {
        auto [xvoidx,elapsed] = timed([&] { format.expand(palette, options.output, out); });
        expand_elapsed = elapsed;
    }
    write_stdout(out);
    
    if (s_timing_on)
    {
        using namespace std::chrono;
        write_stdout("\nPrompt Timings: (");
        const auto elapsed = steady_clock::now() - run_start_time;
        write_stdout(std::to_string(duration_cast<milliseconds>(elapsed).count()));
        write_stdout("ms)\n");
        
        write_stdout("parse:  ");
        write_stdout(format_timing(parse_elapsed));
        
        for (const auto& mod: s_modules)
        {
            if (mod.usage == ModuleUsage::Unused) continue;
            auto len = write_stdout(mod.name);
            //write_stdout(std::string{std::size_t{20}-len, ' ');
            write_stdout(std::string(len<MAX_MODULE_NAME_LENGTH?
                                     MAX_MODULE_NAME_LENGTH-len: 0, ' '));
            write_stdout(": ");
            write_stdout(format_timing(mod.elapsed));
        }

        write_stdout("expand: ");
        write_stdout(format_timing(expand_elapsed));
    }
    return {};
}

} // namespace devprompt
