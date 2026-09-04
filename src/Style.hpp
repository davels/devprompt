/**
 * @file Style.hpp
 */

#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <cassert>

namespace devprompt
{

namespace ansi
{

/// output targets
enum class Output
{
    None = 0,    // output nothing
    Code,        // just the code
    Raw,         // raw ansi wrapped codes
    Bash,        // bash compatible ansi code
    Zsh,         // zsh
    PowerShell,  // powershell 
    _Count
};

/// ansi color codes
enum class Color
{
    None          = 0,
    Black         = 30,
    Red           = 31,
    Green         = 32,
    Yellow        = 33,
    Blue          = 34,
    Magenta       = 35,
    Cyan          = 36,
    White         = 37,
    BrightBlack   = 90,
    BrightRed     = 91,
    BrightGreen   = 92,
    BrightYellow  = 93,
    BrightBlue    = 94,
    BrightMagenta = 95,
    BrightCyan    = 96,
    BrightWhite   = 97
};

} // namespace ansi

namespace detail
{

using namespace ansi;

struct OutputWrapper
{
    std::string_view open;
    std::string_view close;
};

static constexpr auto OUTPUT_ENUM_COUNT = static_cast<std::size_t>(Output::_Count);
static constexpr std::array<OutputWrapper, OUTPUT_ENUM_COUNT> OUTPUT_WRAPPERS
{{
    { "",          ""     },   // None
    { "",          ""     },   // Code
    { "\033[",     "m"    },   // Raw
    { "\001\033[", "m\002"},   // Bash
    { "%{\033[",   "m%}"  },   // Zsh
    { "`e[",       "m"    },   // PowerShell
}};

constexpr const OutputWrapper& get_wrapper(Output output)
{
    auto const index = static_cast<std::size_t>(output);
    assert(index < OUTPUT_WRAPPERS.size());
    return OUTPUT_WRAPPERS[index];
}

constexpr int color_code(Color color) noexcept
{
    return static_cast<int>(color);
}

constexpr std::optional<std::array<std::uint8_t, 3>>
parse_hex(std::string_view hex)
{
    if (hex.size() != 7 || hex[0] != '#')
        return std::nullopt;

    auto hex_digit = [](char c) -> int
    {
        if (c >= '0' && c <= '9')
            return c - '0';
        if (c >= 'a' && c <= 'f')
            return c - 'a' + 10;
        if (c >= 'A' && c <= 'F')
            return c - 'A' + 10;

        return -1;
    };

    std::array<std::uint8_t, 3> rgb{};
    for (int i = 0; i < 3; ++i)
    {
        const int high = hex_digit(hex[1 + i * 2]);
        const int low  = hex_digit(hex[2 + i * 2]);
        if (high < 0 || low < 0)
            return std::nullopt;
        rgb[i] = static_cast<std::uint8_t>((high << 4) | low);
    }
    return rgb;
}

inline std::string wrap(std::string_view codes, Output output)
{
    std::string result;
    if (output == Output::None)  // output nothing
        return result;
    assert(static_cast<std::size_t>(output) < OUTPUT_ENUM_COUNT);
    const auto& wrap = detail::get_wrapper(output);
    result += wrap.open;
    result += codes;
    result += wrap.close;
    return result;
}

} // namespace detail


class Style
{
  public:
    Style& reset()
    {
        add("0");
        return *this;
    }

    Style& bold()
    {
        add("1");
        return *this;
    }

    Style& dim()
    {
        add("2");
        return *this;
    }

    Style& italic()
    {
        add("3");
        return *this;
    }

    Style& underline()
    {
        add("4");
        return *this;
    }

    Style& blink()
    {
        add("5");
        return *this;
    }

    Style& reverse()
    {
        add("7");
        return *this;
    }

    Style& hidden()
    {
        add("8");
        return *this;
    }

    Style& strikethrough()
    {
        add("9");
        return *this;
    }

    // --------------------------------------------------------------
    // Standard colors
    // --------------------------------------------------------------

    Style& fg(ansi::Color color)
    {
        add(std::to_string(detail::color_code(color)));
        return *this;
    }

    Style& bg(ansi::Color color)
    {
        add(std::to_string(detail::color_code(color) + 10));
        return *this;
    }

    // --------------------------------------------------------------
    // 24-bit colors
    // --------------------------------------------------------------

    Style& fg(std::uint8_t r,
              std::uint8_t g,
              std::uint8_t b)
    {
        std::string code = "38;2;";  // rgb fg code
        code += std::to_string(r);
        code += ';';
        code += std::to_string(g);
        code += ';';
        code += std::to_string(b);
        add(code);

        return *this;
    }

    Style& bg(std::uint8_t r,
              std::uint8_t g,
              std::uint8_t b)
    {
        std::string code = "48;2;";  // rgb bg code
        code += std::to_string(r);
        code += ';';
        code += std::to_string(g);
        code += ';';
        code += std::to_string(b);
        add(code);

        return *this;
    }

    Style& fg(std::string_view hex)
    {
        if (auto rgb = detail::parse_hex(hex))
            return fg((*rgb)[0], (*rgb)[1], (*rgb)[2]);

        return *this;
    }

    Style& bg(std::string_view hex)
    {
        if (auto rgb = detail::parse_hex(hex))
            return bg((*rgb)[0], (*rgb)[1], (*rgb)[2]);

        return *this;
    }

    bool empty() const
    {
        return m_codes.empty();
    }
    
    Style& merge(const Style& s)
    {
        if (!s.empty())
            add(s.m_codes);
        return *this;
    }
    
    // --------------------------------------------------------------
    // Generate the shell-specific escape sequence
    // --------------------------------------------------------------

    [[nodiscard]]
    std::string str(ansi::Output output) const
    {
        if (m_codes.empty())
            return {};
        return detail::wrap(m_codes, output);
    }

    void write(ansi::Output output, std::string& out) const
    {
        if (m_codes.empty())
            return;
        const auto wrap = detail::get_wrapper(output);
        out += wrap.open;
        out += m_codes;
        out += wrap.close;
    }

  private:
    void add(std::string code)
    {
        if (!m_codes.empty())
            m_codes += ';';
        m_codes += code;
    }

    std::string m_codes;
}; // class Style


inline Style style()
{
    return Style();
}

// shorthand for style().reset().str(output)
inline std::string reset(ansi::Output output)
{
    return detail::wrap("0", output);
}


} // namespace devprompt::ansi
