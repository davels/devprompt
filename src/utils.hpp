/**
 * @file utils.hpp
 */

#pragma once

#include <cctype>
#include <cstddef>
#include <string_view>
#include <array>
#include <charconv>
#include <optional>
#include <chrono>
#include <utility>
#include <variant>  // for monostate

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
static constexpr std::array<bool, 256> WHITESPACE = [] {
    std::array<bool, 256> table{};
    for (unsigned char c : {' ', '\t', '\n', '\r', '\f', '\v'})
        table[c] = true;
    return table;
}();
} // namespace details


inline bool isspace(char ch)
{ return details::WHITESPACE[static_cast<unsigned char>(ch)]; }

[[nodiscard]]
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

[[nodiscard]]
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
[[nodiscard]]
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

[[nodiscard]]
inline std::optional<int> string_to_int(std::string_view str)
{
    int val = 0;
    // from_chars expects a pointer range [first, last)
    auto [ptr, ec] = std::from_chars(str.data(), str.data() + str.size(), val);

    if (ec == std::errc())
        return val;
    else
        return std::nullopt;
}

[[nodiscard]]
inline std::optional<unsigned int> string_to_uint(std::string_view str)
{
    unsigned int val = 0;
    // from_chars expects a pointer range [first, last)
    auto [ptr, ec] = std::from_chars(str.data(), str.data() + str.size(), val);

    if (ec == std::errc())
        return val;
    else
        return std::nullopt;
}


#ifdef _WIN32
// Windows needs utf8 <=> utf16 conversion functions for api calls

inline std::wstring Utf8ToWide(std::string_view s)
{
    if (s.empty()) return std::wstring();
    int len = MultiByteToWideChar(CP_UTF8, 0, s.data(), static_cast<int>(s.size()), nullptr, 0);
    if (len <= 0) return {};  // conversion failed
    std::wstring ws(static_cast<size_t>(len), L'\0');
    ::MultiByteToWideChar(CP_UTF8, 0, s.data(), static_cast<int>(s.size()), ws.data(), len);
    return ws;
}

inline std::string WideToUtf8(std::wstring_view ws)
{
    if (ws.empty()) return std::string();
    int len = ::WideCharToMultiByte(CP_UTF8, 0, ws.data(), static_cast<int>(ws.size()),
                                    nullptr, 0, nullptr, nullptr);
    if (len <= 0) return {};  // conversion failed
    std::string s(static_cast<size_t>(len), '\0');
    ::WideCharToMultiByte(CP_UTF8, 0, ws.data(), static_cast<int>(ws.size()),
                          s.data(), len, nullptr, nullptr);
    return s;
}

#endif // _WIN32

/// Collect timing data for function call
template<typename F>
auto timed(bool timeflag, F&& f)
{
    if (!timeflag)
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

inline std::string format_timing(std::chrono::steady_clock::duration elapsed)
{
    std::string elapsedstr;
    using namespace std::chrono;
    if (elapsed < 1ms)
    {
        auto strval = std::to_string(duration_cast<microseconds>(elapsed).count());
        elapsedstr += std::string(strval.length()>3? 0: 3-strval.length(), ' ') + strval;
        elapsedstr += "us";
    }
    else
    {
        auto strval = std::to_string(duration_cast<milliseconds>(elapsed).count());
        elapsedstr += std::string(strval.length()>3? 0: 3-strval.length(), ' ') + strval;
        elapsedstr += "ms";
    }
    return elapsedstr;
}

// Write string directly to stdoud
inline std::size_t write_stdout(std::string_view str)
{
    if (str.empty()) return 0;
    const char* pstr = str.data();
    std::size_t len = str.length();

#if defined(_WIN32)
    DWORD written = 0;
    HANDLE h = GetStdHandle(STD_ERROR_HANDLE);
    WriteFile(h, pstr, static_cast<DWORD>(len), &written, nullptr);
    return written;
#else
    return ::write(1, pstr, len); // fd 1 = stdout
#endif
}

inline std::size_t write_stderr(std::string_view str)
{
    if (str.empty()) return 0;
    const char* pstr = str.data();
    std::size_t len = str.length();

#if defined(_WIN32)
    DWORD written = 0;
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    WriteFile(h, pstr, static_cast<DWORD>(len), &written, nullptr);
    return written;
#else
    return ::write(2, pstr, len); // fd 2 = stderr
#endif
}

} // namespace devprompt::utils
