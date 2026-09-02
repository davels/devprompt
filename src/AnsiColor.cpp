/**
 * @file AnsiColor.cpp
 */

#include "AnsiColor.hpp"
#include <cstddef>

namespace devprompt::ansi
{

namespace
{
constexpr bool is_hex_color(std::string_view color)
{
    return (color.size() == 7 && color[0] == '#');
}

constexpr int ansi_color(std::string_view color)
{
    if (color == "black")          return 30;
    if (color == "red")            return 31;
    if (color == "green")          return 32;
    if (color == "yellow")         return 33;
    if (color == "blue")           return 34;
    if (color == "magenta")        return 35;
    if (color == "cyan")           return 36;
    if (color == "white")          return 37;

    if (color == "bright-black")   return 90;
    if (color == "bright-red")     return 91;
    if (color == "bright-green")   return 92;
    if (color == "bright-yellow")  return 93;
    if (color == "bright-blue")    return 94;
    if (color == "bright-magenta") return 95;
    if (color == "bright-cyan")    return 96;
    if (color == "bright-white")   return 97;

    return -1;
}
} // namespace


Expected<Style,std::string> parse_style(std::string_view stylestr)
{
    Style style;
    constexpr std::string_view WHITESPACE = " \t\n\r\f\v";
    std::size_t pos = 0;

    // short-circuit a common case, reset tag
    if (stylestr == "/")
    {
        style.reset();
        return style;
    }
    
    while (true)
    {
        // skip whitespace
        pos = stylestr.find_first_not_of(WHITESPACE, pos);
        if (pos == std::string_view::npos)
            break;

        // Find the end of the decl.
        const auto end = stylestr.find_first_of(WHITESPACE, pos);
        const auto decl = stylestr.substr(pos,
                                          end == std::string_view::npos
                                          ? std::string_view::npos
                                          : end - pos);
        if (decl == "/") {
            style.reset();
        }        
        else if (decl.starts_with("fg:"))
        {
            const auto color = decl.substr(3);
            if (is_hex_color(color))
            {
                style.fg(color);
            }
            else
            {
                const auto ansi = ansi_color(color);
                if (ansi < 0)
                {
                    return Unexpected("invalid foreground color: " +
                                      std::string{color});
                }
                style.fg(static_cast<ansi::Color>(ansi));
            }
        }
        else if (decl.starts_with("bg:"))
        {
            const auto color = decl.substr(3);
            if (is_hex_color(color))
            {
                style.bg(color);
            }
            else
            {
                const auto ansi = ansi_color(color);
                if (ansi < 0) {
                    return Unexpected("invalid background color: " +
                                      std::string{color});
                }
                style.bg(static_cast<ansi::Color>(ansi));
            }
        }
        else if (decl == "bold") {
            style.bold();
        }
        else if (decl == "italic") {
            style.italic();
        }
        else if (decl == "underline") {
            style.underline();
        }
        // else if (decl == "dimmed") {
        //     style.dimmed();
        // }
        // else if (decl == "inverted") {
        //     style.inverted();
        // }
        else if (decl == "blink") {
            style.blink();
        }
        else if (decl == "hidden") {
            style.hidden();
        }
        else if (decl == "strikethrough") {
            style.strikethrough();
        }
        else
        {
            return Unexpected("invalid style component: " +
                              std::string{decl});
        }

        if (end == std::string_view::npos)
            break;        
        pos = end;
    }

    return style;
}

    

} // namespace devprompt::ansi

