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
#include "OneShotThreadPool.hpp"
#include "CoreVariables.hpp"
#include "FileScanner.hpp"
#include "filesystem.hpp"

namespace devprompt
{
using utils::write_stdout;

namespace
{

bool s_timing_on = false;
// run statistics
std::chrono::steady_clock::duration s_filescanner_elapsed{};
unsigned int s_poolsize = 0;
bool s_global_timeout_triggered = false;

std::string work_path(const Options& options)
{
    if (options.path.empty())
        return filesystem::get_current_directory();

    return filesystem::expand_path(options.path);
}

Expected<void, std::string> load_modules(const Options& options)
{
    FileScanner fscanner;
    ModuleContext mctx{work_path(options), fscanner};

    if (mctx.path().empty())
        return Unexpected("Invalid path");  // path is not valid
    
    //
    // check each module for usage
    //
    int used_count = 0;
    int expensive_count = 0;
    for (auto& mod: s_modules)
    {
        mod.usage = mod.mod_usage(options, fscanner);
        if (mod.usage == ModuleUsage::Used)
            ++used_count;
        else if (mod.usage == ModuleUsage::UsedExpensive)
            ++expensive_count;
    }

    // only bother with async if we have at least 2 "expensive" modules or 1 "expensive"
    // module and a lot of normal modules
    const bool use_async = !options.no_async && 
        (expensive_count > 1 || (expensive_count == 1 && used_count > 5));

    // (need at least 1 thread if we're using the file scanner)
    int fscanner_bump = fscanner.count() > 0? 1: 0;

    //
    // create thread pool
    //
    // task queue size is set at compile time and we'll never need > s_modules.size()
    // thread pool is 1 per expensive tasks (capped at the smaller of 16 and #cores*2)
    // (and at least 1 thread if we need the file scanner)
    auto poolsize = !use_async? fscanner_bump:
        std::min({static_cast<uint>(expensive_count + fscanner_bump), 
                  std::max(std::thread::hardware_concurrency(),2U) * 2,
                  16U});
    OneShotThreadPool<s_modules.size()> tpool(poolsize,
                                              std::chrono::milliseconds{options.module_timeout},
                                              std::chrono::milliseconds{options.timeout});

    //
    // create a thread pool task for the file scanner
    //
    if (fscanner.count() > 0)
    {
        tpool.enqueue([&fscanner, &mctx, timing=options.show_timings]
                      (std::stop_token stoken, CommandContext&) {
            fscanner.scan_directory(mctx.path(), stoken, timing);
            mctx.set_filescanner_ready(); // notify any threads waiting for the results
        });
    }

    //
    // load the module data
    // 
    // - queue up any async loads
    if (use_async)
    {
        for (auto& mod: s_modules)
        {
            if (mod.usage == ModuleUsage::UsedExpensive)
            {
                mod.async = true;
                tpool.enqueue([&mod, &options, &mctx, timing=options.show_timings](std::stop_token stoken,
                                                                                   CommandContext& cctx) {
                    auto start = timing?
                        std::chrono::steady_clock::now():
                        std::chrono::steady_clock::time_point{};
                    /*mod.result =*/ mod.mod_load(options, mctx, stoken, cctx);
                    if (timing)
                        mod.elapsed = std::chrono::steady_clock::now() - start;
                });
            }
        }
    }

    // even if we're not using worker threads, we still run the thread pool (with zero
    // worker threads) for the timeout watchdog, and potential file scanner task
    tpool.start();
    
    // - run the remaining module loads synchronously
    for (auto& mod: s_modules)
    {
        if (mod.usage == ModuleUsage::Unused || mod.async) continue;

        auto [xvoidx, elapsed] = utils::timed(s_timing_on,
                                              [&] { mod.mod_load(options, mctx,
                                                                 tpool.shared_stop_token(),
                                                                 tpool.shared_command_context()); });
        mod.elapsed = elapsed;
    }

    // wait for the workter threads to finish
    tpool.wait_all();
    tpool.stop_watchdog();
    
    // update Core variable
    if (tpool.global_timeout_reached())
        Core::set_timeout_reached();

    // record load statistics
    s_filescanner_elapsed = fscanner.elapsed();
    s_poolsize = poolsize;
    s_global_timeout_triggered = tpool.global_timeout_reached();

    return {};
}

} // namespace


Expected<void,std::string> run(Options& options)
{
    std::chrono::steady_clock::time_point start_time_run;
    std::chrono::steady_clock::duration parse_elapsed{};
    std::chrono::steady_clock::duration expand_elapsed{};
    s_timing_on = options.show_timings;

    if (s_timing_on)
        start_time_run = std::chrono::steady_clock::now();

    // steal the format string to avoid a copy - Option doesn't need it anymore
    FormatString format(std::move(options.format));
    Palette palette;

    {
        // parse the format string which also tags used variables
        if (auto result = format.compile(); !result)
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
    if (auto result = load_modules(options); !result)
        return result;
    
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
        if (s_poolsize > 0)
        {
            write_stdout(" using ");
            write_stdout(std::to_string(s_poolsize));
            write_stdout(" worker threads");
        }
        if (s_global_timeout_triggered)
            write_stdout(" [timeout triggered]");
        write_stdout("\n");
        
        write_stdout(" parse:     ");
        write_stdout(utils::format_timing(parse_elapsed));
        write_stdout("\n");

        if (s_filescanner_elapsed != s_filescanner_elapsed.zero())
        {
            write_stdout(" file scan: ");
            write_stdout(utils::format_timing(s_filescanner_elapsed));
            write_stdout("\n");
        }
        
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

        write_stdout(" expand:    ");
        write_stdout(utils::format_timing(expand_elapsed));
        write_stdout("\n");
    }
    return {};
}

} // namespace devprompt
