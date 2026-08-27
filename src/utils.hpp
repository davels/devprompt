/**
 * @file utils.hpp
 */

#pragma once

#include <cctype>
#include <string_view>

namespace devprompt::utils
{

// return a string_view of str with leading and trailing whitespace removed
inline std::string_view trimmed(std::string_view str)
{
    while (!str.empty() && std::isspace(static_cast<unsigned char>(str.front())))
        str.remove_prefix(1);

    while (!str.empty() && std::isspace(static_cast<unsigned char>(str.back())))
        str.remove_suffix(1);

    return str;
}

// return a string_view of str with leading and trailing whitespace removed
inline void trim(std::string_view& str)
{
    while (!str.empty() && std::isspace(static_cast<unsigned char>(str.front())))
        str.remove_prefix(1);

    while (!str.empty() && std::isspace(static_cast<unsigned char>(str.back())))
        str.remove_suffix(1);
}

} // namespace devprompt::utils
