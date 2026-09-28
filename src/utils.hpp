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
#include <concepts>
#include <ranges>

// For write_stdout
#if defined(_WIN32)
  #include <windows.h>
#else
   #include <unistd.h>
#endif

namespace devprompt::utils
{

namespace detail
{
static constexpr std::array<bool, 256> WHITESPACE = [] {
    std::array<bool, 256> table{};
    for (unsigned char c : {' ', '\t', '\n', '\r', '\f', '\v'})
        table[c] = true;
    return table;
}();
} // namespace detail


[[nodiscard]]
inline constexpr bool isspace(char ch)
{ return detail::WHITESPACE[static_cast<unsigned char>(ch)]; }

[[nodiscard]]
inline constexpr std::size_t skip_whitespace(std::string_view str, std::size_t pos)
{
    while (pos < str.size())
    {
        if (!detail::WHITESPACE[static_cast<unsigned char>(str[pos])])
            return pos;
        ++pos;
    }
    return std::string_view::npos;
}

[[nodiscard]]
inline constexpr std::size_t find_first_whitespace(std::string_view str, std::size_t pos)
{
    while (pos < str.size())
    {
        if (detail::WHITESPACE[static_cast<unsigned char>(str[pos])])
            return pos;
        ++pos;
    }
    return std::string_view::npos;
}

/// modify the provided string_view, removing leading whitespace
inline constexpr void ltrim(std::string_view& sv)
{
    while (!sv.empty() && isspace(sv.front()))
        sv.remove_prefix(1);
}

/// modify the provided string_view, removing trailing whitespace
inline constexpr void rtrim(std::string_view& sv)
{
    while (!sv.empty() && isspace(sv.back()))
        sv.remove_suffix(1);
}

/// modify the provided string_view, removing leading and trailing whitespace
inline constexpr void trim(std::string_view& sv)
{
    ltrim(sv);
    rtrim(sv);
}

/// return a string_view of str with leading and trailing whitespace removed
[[nodiscard]]
inline constexpr std::string_view trimmed(std::string_view sv)
{
    trim(sv);
    return sv;
}


[[nodiscard]]
inline std::optional<int> string_to_int(std::string_view sv)
{
    int val = 0;
    // from_chars expects a pointer range [first, last)
    auto [ptr, ec] = std::from_chars(sv.data(), sv.data() + sv.size(), val);

    if (ec == std::errc())
        return val;
    else
        return std::nullopt;
}

[[nodiscard]]
inline std::optional<unsigned int> string_to_uint(std::string_view sv)
{
    unsigned int val = 0;
    // from_chars expects a pointer range [first, last)
    auto [ptr, ec] = std::from_chars(sv.data(), sv.data() + sv.size(), val);

    if (ec == std::errc())
        return val;
    else
        return std::nullopt;
}

namespace detail
{
inline constexpr std::size_t find_separator(std::string_view sv, char sep)
{ return sv.find(sep); }

inline constexpr std::size_t find_separator(std::string_view sv, std::string_view sep)
{ return sv.find(sep); }

inline constexpr std::size_t separator_length(char)
{ return 1; }

inline constexpr std::size_t separator_length(std::string_view sep)
{ return sep.size(); }
} // namespace detail

/**
 * forward-iterable range that yields tokens from a string split by a separator.
 *
 * Constructor take a string_view source and a separator (char or string_view).
 * Returned tokens are string_view objects from the soruce.
 * 
 * This operates on string_view objects so the source must live for the entire lifetime of
 * this object and any iterators.
 * * do not construct a split_view from a temporary std::string *
 *
 * This is fine as long as some_string outlives the range:
 * auto range = split_view{some_string, ','};
 *
 * This is bad since it's initialized with a temporary object:
 * auto range = split_view{std::string{"a,b,c"}, ','};
 *
 * This is also bad for the same reason:
 * auto range = split_view(func_returning_string(), ',');
 *
 * Also invalid before c++23
 * for (auto token: split_view{func_returning_string(), ','})
 * {   // ...  }
 * // NOTE: c++23 will extended the lifetime of the return value until the end of the
 * // loop, but c++20 does not.
 * 
 */
template <typename Sep>
    requires (std::same_as<Sep, char> || std::same_as<Sep, std::string_view>)
class split_view: public std::ranges::view_interface<split_view<Sep>>
{
  public:
    constexpr split_view(std::string_view input, Sep sep): m_input(input), m_sep(sep) {}

    class iterator
    {
      public:
        using value_type      = std::string_view;
        using difference_type = std::ptrdiff_t;
        
        constexpr iterator() = default; // default-constructed == "end" / finished

        // Return by value not const&. We're returning a member local to the iterator.
        // Returning by reference would lead to a dangling reference when used on a
        // temporary iterator.
        constexpr std::string_view operator*() const { return m_current; }

        // Pointer to iterator member is fine since, by iterator contract, this is only
        // valid as long as the iterator is valid.
        constexpr const std::string_view* operator->() const { return &m_current; }

        constexpr iterator& operator++()
        {
            forward_token();
            return *this;
        }
        
        constexpr iterator operator++(int)
        {
            iterator tmp = *this;
            forward_token();
            return tmp;
        }

        friend constexpr bool operator==(const iterator& a, const iterator& b)
        {
            if (a.m_finished != b.m_finished)
                return false;
            if (a.m_finished)
                return true; // both finished
            return a.m_remaining.data() == b.m_remaining.data() &&
                a.m_remaining.size() == b.m_remaining.size();
        }

      private:
        friend class split_view;

        constexpr iterator(std::string_view sv, Sep sep):
            m_remaining(sv), m_sep(sep), m_finished(false)
        { forward_token(); }

        // advance iterator to the next token
        constexpr void forward_token()
        {
            if (m_finished)
                return;

            if (m_remaining.empty() && m_consumed_all)
            {
                m_finished = true;
                m_current = {};
                return;
            }

            // check for empty string separator - the entire string is one token
            if constexpr (!std::same_as<Sep, char>)
            {
                if (m_sep.empty())
                {
                    m_current = m_remaining;
                    m_remaining = {};
                    m_consumed_all = true;
                    return;
                }
            }
            
            const std::size_t pos = detail::find_separator(m_remaining, m_sep);
            if (pos == std::string_view::npos)
            {
                m_current = m_remaining;
                m_remaining = {};
                m_consumed_all = true;
            }
            else
            {
                m_current = m_remaining.substr(0, pos);
                m_remaining.remove_prefix(pos + detail::separator_length(m_sep));
            }
        }

        std::string_view m_remaining{};
        std::string_view m_current{};
        Sep m_sep{};
        bool m_consumed_all = false;
        bool m_finished = true; // default gives end() iterator sentinel
    };

    constexpr iterator begin() const { return iterator(m_input, m_sep); }
    constexpr iterator end() const { return iterator(); }
    
  private:
    std::string_view m_input;
    Sep m_sep;
};

// CTAD deduction guides to avoid compiler ambiguities
split_view(std::string_view, char) -> split_view<char>;
split_view(std::string_view, std::string_view) -> split_view<std::string_view>;


#ifdef _WIN32
// Windows needs utf8 <=> utf16 conversion functions for api calls

inline std::wstring Utf8ToWide(std::string_view sv)
{
    if (sv.empty()) return std::wstring();
    int len = MultiByteToWideChar(CP_UTF8, 0, sv.data(), static_cast<int>(sv.size()), nullptr, 0);
    if (len <= 0) return {};  // conversion failed
    std::wstring ws(static_cast<size_t>(len), L'\0');
    ::MultiByteToWideChar(CP_UTF8, 0, sv.data(), static_cast<int>(sv.size()), wsv.data(), len);
    return ws;
}

inline std::string WideToUtf8(std::wstring_view wsv)
{
    if (wsv.empty()) return std::string();
    int len = ::WideCharToMultiByte(CP_UTF8, 0, wsv.data(), static_cast<int>(wsv.size()),
                                    nullptr, 0, nullptr, nullptr);
    if (len <= 0) return {};  // conversion failed
    std::string s(static_cast<size_t>(len), '\0');
    ::WideCharToMultiByte(CP_UTF8, 0, wsv.data(), static_cast<int>(wsv.size()),
                          s.data(), len, nullptr, nullptr);
    return s;
}

#endif // _WIN32

/// Collect timing data for function call
template<typename F>
auto timed(bool timeflag, F&& func)
{
    if (!timeflag)
    {
        if constexpr (std::is_void_v<std::invoke_result_t<F>>)
        {
            std::forward<F>(func)();
            return std::pair{std::monostate{},
                             std::chrono::steady_clock::duration{}};
        }
        else
        {
            return std::pair{std::forward<F>(func)(),
                             std::chrono::steady_clock::duration{}};
        }
    }

    const auto start = std::chrono::steady_clock::now();
    if constexpr (std::is_void_v<std::invoke_result_t<F>>)
    {
        std::forward<F>(func)();
        const auto elapsed = std::chrono::steady_clock::now() - start;
        return std::pair{std::monostate{}, elapsed};
    }
    else
    {
        auto result = std::forward<F>(func)();
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
inline std::size_t write_stdout(std::string_view sv)
{
    if (sv.empty()) return 0;
    const char* pstr = sv.data();
    std::size_t len = sv.length();

#if defined(_WIN32)
    DWORD written = 0;
    HANDLE h = ::GetStdHandle(STD_OUTPUT_HANDLE);
    ::WriteFile(h, pstr, static_cast<DWORD>(len), &written, nullptr);
    return written;
#else
    return ::write(1, pstr, len); // fd 1 = stdout
#endif
}

inline std::size_t write_stderr(std::string_view sv)
{
    if (sv.empty()) return 0;
    const char* pstr = sv.data();
    std::size_t len = sv.length();

#if defined(_WIN32)
    DWORD written = 0;
    HANDLE h = ::GetStdHandle(STD_ERROR_HANDLE);
    ::WriteFile(h, pstr, static_cast<DWORD>(len), &written, nullptr);
    return written;
#else
    return ::write(2, pstr, len); // fd 2 = stderr
#endif
}

} // namespace devprompt::utils
