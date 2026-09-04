/**
 * @file Palette.hpp
 */

#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <optional>
#include <functional>
#include <utility>
#include "Style.hpp"

namespace devprompt
{

/// A table of named styles
class Palette
{
  public:
    bool empty() const
    { return m_table.empty(); }
    
    void reserve(std::size_t size)
    { m_table.reserve(size); }

    void swap(Palette& other)
    { m_table.swap(other.m_table); }
    
    /// Add a palette entry
    template <typename T>
    inline void add(std::string name, T&& style)
    { m_table.emplace_back(std::move(name), std::forward<T>(style)); }
    
    /// Lookup a style by name
    std::optional<std::reference_wrapper<const Style>> lookup(std::string_view name) const;
    
  private:
    struct Entry
    {
        std::string name;
        Style style;
    };
    std::vector<Entry> m_table;
}; // Palette


inline std::optional<std::reference_wrapper<const Style>> Palette::lookup(std::string_view name) const
{
    auto it = std::ranges::find_if(m_table,
                                   [&](const auto& e) { return e.name == name; });
    if (it != m_table.end())
        return std::cref(it->style);
    else
        return std::nullopt;
}

} // namespace devprompt
