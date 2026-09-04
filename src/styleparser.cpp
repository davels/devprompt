/**
 * @file styleparser.cpp
 */

#include "styleparser.hpp"
#include <cstddef>
#include <string_view>
#include <algorithm>
#include "Palette.hpp"
#include "utils.hpp"

namespace devprompt
{

namespace
{
constexpr bool is_hex_color(std::string_view color)
{
    return (color.size() == 7 && color[0] == '#');
}

constexpr ansi::Color ansi_color(std::string_view color)
{
    using ansi::Color;
    
    if (color == "black")          return Color::Black;
    if (color == "red")            return Color::Red;
    if (color == "green")          return Color::Green;
    if (color == "yellow")         return Color::Yellow;
    if (color == "blue")           return Color::Blue;
    if (color == "magenta")        return Color::Magenta;
    if (color == "cyan")           return Color::Cyan;
    if (color == "white")          return Color::White;

    if (color == "bright-black")   return Color::BrightBlack;
    if (color == "bright-red")     return Color::BrightRed;
    if (color == "bright-green")   return Color::BrightGreen;
    if (color == "bright-yellow")  return Color::BrightYellow;
    if (color == "bright-blue")    return Color::BrightBlue;
    if (color == "bright-magenta") return Color::BrightMagenta;
    if (color == "bright-cyan")    return Color::BrightCyan;
    if (color == "bright-white")   return Color::BrightWhite;

    return Color::None;
}

} // namespace


Expected<Style,std::string> parse_style(const Palette* palette, std::string_view stylestr)
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
        bool stop = false;
        if (decl == "/") {
            style.reset();
            stop = true;
        }
        else if (palette)
        {
            if (const auto palette_style = palette->lookup(decl))
            {
                style.merge(palette_style->get());
                stop = true;
            }
        }
        if (stop) 
        {}
        else if (decl.starts_with("fg:"))
        {
            const auto color_name = decl.substr(3);
            if (is_hex_color(color_name))
            {
                style.fg(color_name);
            }
            else
            {
                const auto color_code = ansi_color(color_name);
                if (color_code == ansi::Color::None)
                {
                    return Unexpected("invalid foreground color: " +
                                      std::string{color_name});
                }
                style.fg(color_code);
            }
        }
        else if (decl.starts_with("bg:"))
        {
            const auto color_name = decl.substr(3);
            if (is_hex_color(color_name))
            {
                style.bg(color_name);
            }
            else
            {
                const auto color_code = ansi_color(color_name);
                if (color_code == ansi::Color::None)
                {
                    return Unexpected("invalid background color: " +
                                      std::string{color_name});
                }
                style.bg(color_code);
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

Expected<Palette, std::string> palette_from_string(std::string_view palette_str)
{
    Palette palette;
    if (palette_str.empty())
        return palette;
    
    auto sep_count = std::ranges::count(palette_str, ';');
    if (sep_count % 2 == 0)
        return Unexpected("palette has unbalanced ';' separators");
    
    palette.reserve((sep_count +1) / 2);

    while (!palette_str.empty())
    {
        auto sep = palette_str.find(';');
        if (sep == std::string_view::npos)
            return Unexpected("palette style missing");
        std::string_view name = utils::trimmed(palette_str.substr(0, sep));
        if (name.empty())
            return Unexpected("palette has empty name");
        ++sep;  // skip separator
        auto end = palette_str.find(';', sep);
        // (end-sep) works even if end=npos
        auto style = parse_style(palette_str.substr(sep, end-sep));
        if (!style)
            return Unexpected(std::move(style).error());
        if (style->empty())
            return Unexpected("palette style missing");
        palette.add(std::string{name}, std::move(*style));

        if (end == std::string_view::npos)
            break;
        palette_str.remove_prefix(end + 1);
    }

    return palette;
}


} // namespace devprompt

