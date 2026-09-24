/**
 * @file runner.cpp
 */

#include "runner.hpp"
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <chrono>
#include <algorithm>
#include <stop_token>
#include "Options.hpp"
#include "FormatString.hpp"
#include "utils.hpp"
#include "styleparser.hpp"
#include "ModuleRegistry.hpp"
#include "Module.hpp"
#include "ModuleInc.hpp"
#include "OneShotThreadPool.hpp"
#include "CoreVariables.hpp"

namespace devprompt
{
using utils::write_stdout;

namespace
{

bool s_timing_on = false;

std::pair<int,bool> load_modules(const Options& options)
{
    // check each module for usage
    int used_count = 0;
    int expensive_count = 0;
    for (auto& mod: s_modules)
    {
        mod.usage = mod.mod_usage(options);
        if (mod.usage == ModuleUsage::Used)
            ++used_count;
        else if (mod.usage == ModuleUsage::UsedExpensive)
            ++expensive_count;
    }

    // only bother with async if we have at least 2 "expensive" modules or 1 "expensive"
    // module and a lot of normal modules
    const bool use_async = !options.no_async && 
        (expensive_count >= 2 || (expensive_count == 1 && used_count > 5));

    // create thread pool
    // task queue size is set at compile time and we'll never need > s_modules.size()
    // thread pool is 1 per expensive tasks (capped at the smaller of 16 and #cores*2)
    auto poolsize = !use_async? 0U:
        std::min({static_cast<uint>(expensive_count), 
                  std::max(std::thread::hardware_concurrency(),2U) * 2,
                  16U});
    OneShotThreadPool<s_modules.size()> tpool(poolsize,
                                              std::chrono::milliseconds{options.module_timeout},
                                              std::chrono::milliseconds{options.timeout});
    // load the module data

    // - queue up any async loads
    if (use_async)
    {
        for (auto& mod: s_modules)
        {
            if (mod.usage == ModuleUsage::UsedExpensive)
            {
                mod.async = true;
                tpool.enqueue([&mod, &options, timing=options.show_timings](std::stop_token stoken,
                                                                            CommandContext& cctx) {
                    auto start = timing?
                        std::chrono::steady_clock::now():
                        std::chrono::steady_clock::time_point{};
                    /*mod.result =*/ mod.mod_load(options, stoken, cctx);
                    if (timing)
                        mod.elapsed = std::chrono::steady_clock::now() - start;
                });
            }
        }
    }

    // even if we're not using worker threads, we still run the thread pool (with zero
    // worker threads) for the timeout watchdog
    tpool.start();
    
    // - run the remaining loads synchronously
    for (auto& mod: s_modules)
    {
        if (mod.usage == ModuleUsage::Unused || mod.async) continue;

        auto [xvoidx, elapsed] = utils::timed(s_timing_on,
                                              [&] { mod.mod_load(options,
                                                                 tpool.shared_stop_token(),
                                                                 tpool.shared_command_context()); });
        mod.elapsed = elapsed;
    }

    // wait for the workter threads to finish
    if (use_async)
        tpool.wait_all();
    tpool.stop_watchdog();
    
    // update Core variable
    if (tpool.global_timeout_reached())
        Core::set_timeout_reached();
    return {use_async? poolsize: 0, tpool.global_timeout_reached()};
}

} // namespace


Expected<void,std::string> run(Options& options)
{
    std::chrono::steady_clock::time_point start_time_run;
    std::chrono::steady_clock::duration parse_elapsed, expand_elapsed;
    s_timing_on = options.show_timings;

    if (s_timing_on)
        start_time_run = std::chrono::steady_clock::now();

    // steal the format string to avoid a copy - Option doesn't need it anymore
    FormatString format(std::move(options.format));
    Palette palette;

    {
        // parse the format string which also tags used variables
        auto result = format.compile();
        if (!result)
            return result;

        // expand any specified palette
        if (!options.palette.empty())
        {
            auto p = palette_from_string(options.palette);
            if (!p)
                return Unexpected(std::move(p).error());
            palette.swap(*p);
        }
        
        if (s_timing_on)
            parse_elapsed = std::chrono::steady_clock::now() - start_time_run;
    }
    
    // load module data
    auto [poolsize,timedout] = load_modules(options);
    
    // expand the format string and write it out
    std::string out;
    out.reserve(128);
    {
        auto [xvoidx,elapsed] = utils::timed(s_timing_on,
                                             [&] { format.expand(palette, options.output, out); });
        expand_elapsed = elapsed;
    }
    write_stdout(out);
    
    if (s_timing_on)
    {
        using namespace std::chrono;
        write_stdout("\nPrompt Timings: (");
        const auto elapsed = steady_clock::now() - start_time_run;
        write_stdout(std::to_string(duration_cast<milliseconds>(elapsed).count()));
        write_stdout("ms)");
        if (poolsize > 0)
        {
            write_stdout(" using ");
            write_stdout(std::to_string(poolsize));
            write_stdout(" worker threads");
        }
        if (timedout)
            write_stdout(" [timeout triggered]");
        write_stdout("\n");
        
        write_stdout(" parse:  ");
        write_stdout(utils::format_timing(parse_elapsed));
        write_stdout("\n");
        
        for (const auto& mod: s_modules)
        {
            if (mod.usage == ModuleUsage::Unused) continue;
            write_stdout(mod.async? "*": " ");
            auto len = write_stdout(mod.name);
            write_stdout(std::string(len<MAX_MODULE_NAME_LENGTH?
                                     MAX_MODULE_NAME_LENGTH-len: 0, ' '));
            write_stdout(": ");
            write_stdout(utils::format_timing(mod.elapsed));
            write_stdout("\n");
        }

        write_stdout(" expand: ");
        write_stdout(utils::format_timing(expand_elapsed));
        write_stdout("\n");
    }
    return {};
}

} // namespace devprompt
