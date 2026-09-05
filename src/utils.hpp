/**
 * @file utils.hpp
 */

#pragma once

#include <cctype>
#include <cstddef>
#include <string_view>
#include <tuple>
#include <array>

// For write_stdout
#if defined(_WIN32)
  #include <windows.h>
#else
   #include <unistd.h>
#endif

namespace devprompt::utils
{

namespace details
{
static constexpr std::array<bool, 256> WHITESPACE = []
{
    std::array<bool, 256> table{};
    for (unsigned char c : {' ', '\t', '\n', '\r', '\f', '\v'})
        table[c] = true;
    return table;
}();
} // namespace details

inline bool isspace(char ch)
{ return details::WHITESPACE[static_cast<unsigned char>(ch)]; }

inline std::size_t skip_whitespace(std::string_view str, std::size_t pos)
{
    while (pos < str.size())
    {
        if (!details::WHITESPACE[static_cast<unsigned char>(str[pos])])
            return pos;
        ++pos;
    }
    return std::string_view::npos;
}

inline std::size_t find_first_whitespace(std::string_view str, std::size_t pos)
{
    while (pos < str.size())
    {
        if (details::WHITESPACE[static_cast<unsigned char>(str[pos])])
            return pos;
        ++pos;
    }
    return std::string_view::npos;
}

// return a string_view of str with leading and trailing whitespace removed
inline std::string_view trimmed(std::string_view str)
{
    while (!str.empty() && isspace(str.front()))
        str.remove_prefix(1);

    while (!str.empty() && isspace(str.back()))
        str.remove_suffix(1);

    return str;
}

// return a string_view of str with leading and trailing whitespace removed
inline void trim(std::string_view& str)
{
    while (!str.empty() && isspace(str.front()))
        str.remove_prefix(1);

    while (!str.empty() && isspace(str.back()))
        str.remove_suffix(1);
}

// Write string directly to stdoud
inline void write_stdout(std::string_view str)
{
    if (str.empty()) return;
    const char* pstr = str.data();
    std::size_t len = str.length();

#if defined(_WIN32)
    DWORD written = 0;
    HANDLE h = GetStdHandle(STD_ERROR_HANDLE);
    WriteFile(h, pstr, static_cast<DWORD>(len), &written, nullptr);
#else
    std::ignore = ::write(1, pstr, len); // fd 1 = stdout
#endif
}

inline void write_stderr(std::string_view str)
{
    if (str.empty()) return;
    const char* pstr = str.data();
    std::size_t len = str.length();

#if defined(_WIN32)
    DWORD written = 0;
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    WriteFile(h, pstr, static_cast<DWORD>(len), &written, nullptr);
#else
    std::ignore = ::write(2, pstr, len); // fd 2 = stderr
#endif
}



} // namespace devprompt::utils
