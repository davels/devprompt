/**
 * @file utils.hpp
 */

#pragma once

#include <cctype>
#include <string_view>
#include <tuple>

// For write_stdout
#if defined(_WIN32)
  #include <windows.h>
#else
   #include <unistd.h>
#endif

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
