/**
 * @file jsonparser.hpp
 *
 * Just enough json parsing to pull out specific top level string attributes.
 */

#pragma once

#include <string_view>
#include <optional>
#include "utils.hpp"

namespace devprompt::json
{

namespace detail
{

// return the string value at start of s and advance s
// s must already be at the open quote of the string
constexpr inline std::optional<std::string_view> read_string(std::string_view& s)
{
    if (s.empty() || s.front() != '"')
        return std::nullopt;
    s.remove_prefix(1); // skip open '"'

    std::size_t i = 0;
    while ((i = s.find_first_of("\"\\", i)) != std::string_view::npos)
    {
        if (s[i] == '"') // closing '"' - return inner string
        {
            std::string_view out = s.substr(0, i);
            s.remove_prefix(i + 1); // advance s past the closing '"'
            return out;
        }
        i += 2; // skip backslash + escaped char
    }
    // missing closing '"'
    s = {};
    return std::nullopt;
}

constexpr inline void skip_value(std::string_view& s)
{
    utils::ltrim(s);
    if (s.empty())
        return;

    if (s.front() == '"')
    {
        read_string(s);
        return;
    }

    if (s.front() == '{' || s.front() == '[')
    {
        int depth = 0;
        while (!s.empty())
        {
            const std::size_t i = s.find_first_of("\"{}[]");
            if (i == std::string_view::npos)
            {
                s = {};
                return; // malformed
            }

            if (s[i] == '"')
            {
                s.remove_prefix(i);
                if (!read_string(s))
                    return; // malformed
            }
            else
            {
                depth += (s[i] == '{' || s[i] == '[') ? 1 : -1;
                s.remove_prefix(i + 1);
                if (depth == 0)
                    return; // final close
            }
        }
        return;
    }

    // number / true / false / null
    const std::size_t i = s.find_first_of(",}]");
    s.remove_prefix(i == std::string_view::npos ? s.size() : i);
}

} // namespace detail

/// return value of the top level attribute specified.  NOTE: only works for string values
constexpr inline std::string_view get_string_attribute(std::string_view json, const std::string_view name)
{
    utils::ltrim(json);
    if (json.empty() || json.front() != '{')
        return {};
    json.remove_prefix(1); // skip '{'

    while (true)
    {
        utils::ltrim(json);
        const auto key = detail::read_string(json);
        if (!key)
            return {}; // no key here

        utils::ltrim(json);
        if (json.empty() || json.front() != ':')
            return {};
        json.remove_prefix(1); // skip ':'
        utils::ltrim(json);

        if (*key == name)
        {
            return detail::read_string(json).value_or(std::string_view{});
        }
        
        detail::skip_value(json);
        utils::ltrim(json);

        if (json.empty() || json.front() != ',')
            return {}; // '}' or malformed or not more members
        json.remove_prefix(1);
    }
}

/// return the value of the top level version property from a json string
constexpr inline std::string_view get_version(std::string_view json)
{
    using namespace std::string_view_literals;
    return get_string_attribute(json, "version"sv);
}

/// compile-time tests
static_assert(get_string_attribute(R"({"a":"x","name":"y"})", "name") == "y");
static_assert(get_string_attribute(R"({"a":{"name":"n"}})", "name").empty());
static_assert(get_version(R"({"name":"x","deps":{"version":"9"},"version":"1.2.3"})") == "1.2.3");
static_assert(get_version(R"({"a":{"version":"9"},"version":"1.2.3"})") == "1.2.3");
static_assert(get_version(R"({"a": {"version": "9" }, "version" :"1.2.3" })") == "1.2.3");
static_assert(get_version(R"({"s":"x\"}","version":"2"})") == "2");
static_assert(get_version(R"({"a":[1,{"b":"]"}],"version":"3"})") == "3");
static_assert(get_version(R"({})").empty());
static_assert(get_version(R"({"version":1})").empty());
static_assert(get_version(R"({"":1,"version":"2"})") == "2");

} // namespace devprompt::json
