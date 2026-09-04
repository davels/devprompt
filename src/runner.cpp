/**
 * @file runner.cpp
 */

#include "runner.hpp"
#include <string>
#include <string_view>
#include <type_traits>
#include <chrono>
#include "Options.hpp"
#include "FormatString.hpp"
#include "Modules.hpp"
#include "utils.hpp"
#include "styleparser.hpp"

namespace devprompt
{
using utils::write_stdout;

namespace
{

bool s_timing_on = false;
std::string s_timelog;

inline void record_timing(std::string_view name, std::chrono::steady_clock::duration elapsed)
{
    s_timelog += "  ";
    s_timelog += name;
    s_timelog += ": ";
    using namespace std::chrono;
    if (elapsed < 1ms)
    {
        auto strval = std::to_string(duration_cast<microseconds>(elapsed).count());
        s_timelog += std::string(3-strval.length(), ' ') + strval;
        s_timelog += "us \n";    
    }
    else
    {
        auto strval = std::to_string(duration_cast<milliseconds>(elapsed).count());
        s_timelog += std::string(3-strval.length(), ' ') + strval;
        s_timelog += "ms \n";    
    }
}

/// Collect timing data for function call
template<typename F>
decltype(auto) timed(std::string_view name, F&& f)
{
    if (!s_timing_on)
        return std::forward<F>(f)();

    const auto start = std::chrono::steady_clock::now();

    if constexpr (std::is_void_v<std::invoke_result_t<F>>)
    {
        std::forward<F>(f)();

        const auto elapsed = std::chrono::steady_clock::now() - start;
        record_timing(name, elapsed);
    }
    else
    {
        decltype(auto) result = std::forward<F>(f)();

        const auto elapsed = std::chrono::steady_clock::now() - start;
        record_timing(name, elapsed);
        return result;
    }
}

/// Collect timing data for function if it returns true
template<typename F>
void maybe_timed(std::string_view name, F&& f)
{
    if (!s_timing_on)
    {
        std::ignore = std::forward<F>(f)();
        return;
    }

    const auto start = std::chrono::steady_clock::now();

    if (std::forward<F>(f)())
    {
        const auto elapsed = std::chrono::steady_clock::now() - start;
        record_timing(name, elapsed);
    }
}

} // namespace


Expected<void,std::string> run(const Options& options)
{
    s_timing_on = options.show_timing;

    std::chrono::steady_clock::time_point start_time;
    if (s_timing_on)
        start_time = std::chrono::steady_clock::now();

    FormatString format(options.format);

    // parse the format string which also tags used variables
    auto result = timed("parse", [&] { return format.compile(); });
    if (!result)
    {
        return result;
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

    // Load module data
    maybe_timed("VcsModule  ", [&] { return VcsModule::load(options); });
    maybe_timed("VenvModule ", [&] { return VenvModule::load(options); });
    maybe_timed("SysModule  ", [&] { return SysModule::load(options); });
    maybe_timed("PathModule ", [&] { return PathModule::load(options); });
    maybe_timed("OsModule   ", [&] { return OsModule::load(options); });
    maybe_timed("ShellModule", [&] { return ShellModule::load(options); });
    
    // Expand the format string and write it out
    std::string out;
    out.reserve(128);
    timed("expand", [&] { format.expand(palette, options.output, out); });
    write_stdout(out);
    
    if (s_timing_on)
    {
        using namespace std::chrono;
        write_stdout("\nPrompt Timings: (");
        const auto elapsed = steady_clock::now() - start_time;
        write_stdout(std::to_string(duration_cast<milliseconds>(elapsed).count()));
        write_stdout("ms)\n");
        write_stdout(s_timelog);
    }
    return {};
}

} // namespace devprompt
