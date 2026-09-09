/**
 * @file ModuleRegistry.hpp
 */

#pragma once

#include <array>
#include <string_view>
#include <concepts>
#include <tuple>
#include <utility>
#include <chrono>
#include "Module.hpp"
#include "ModuleInc.hpp"

namespace devprompt
{

struct Options;

/// list of all Modules
using Modules = std::tuple<
    VcsModule,
    VenvModule,
    SysModule,
    PathModule,
    OsModule,
    ShellModule
>;

/// Module "interface" as a concept
template<typename T>
concept ModuleLike = requires(const Options& opts) {
    { T::mod_name } -> std::convertible_to<const std::string_view>;
    { T::mod_usage(opts) } -> std::same_as<ModuleUsage>;
    { T::mod_load(opts) } -> std::same_as<bool>;
};

/// Data tracked per module
struct ModuleEntry
{
    // compile-time data
    const std::string_view name;
    ModuleUsage (* const mod_usage)(const Options&);
    bool (* const mod_load)(const Options&);
    // run-time data
    std::chrono::steady_clock::duration elapsed {};
    ModuleUsage usage = ModuleUsage::Unused;
};


//
// Construct the module registry at compile-time from the Modules type list
//

namespace detail
{

template<ModuleLike T>
consteval ModuleEntry make_module_entry()
{
    return ModuleEntry{T::mod_name, &T::mod_usage, &T::mod_load};
}

template<typename Tuple, std::size_t... I>
consteval auto make_modules(std::index_sequence<I...>)
{
    return std::array{
        make_module_entry<std::tuple_element_t<I, Tuple>>()...
    };
}

template<typename Tuple>
consteval auto make_modules()
{
    return make_modules<Tuple>(
        std::make_index_sequence<std::tuple_size_v<Tuple>>{});
}

} // namespace detail

inline auto s_modules = detail::make_modules<Modules>();


//
// Compute the longest module name at compile-time.
//

namespace detail
{

template<typename Tuple, std::size_t... I>
consteval std::size_t max_module_name_length(std::index_sequence<I...>)
{
    return std::max({
        std::tuple_element_t<I, Tuple>::mod_name.size()...
    });
}

template<typename Tuple>
consteval std::size_t max_module_name_length()
{
    return max_module_name_length<Tuple>(
        std::make_index_sequence<std::tuple_size_v<Tuple>>{});
}

} // namespace detail

inline constexpr auto MAX_MODULE_NAME_LENGTH =
    detail::max_module_name_length<Modules>();

} // namespace devprompt
