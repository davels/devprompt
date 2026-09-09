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
        pos = utils::skip_whitespace(stylestr, pos);
        if (pos == std::string_view::npos)
            break;

        // Find the end of the style element.
        const auto end = utils::find_first_whitespace(stylestr, pos);
        const auto elem = stylestr.substr(pos,
                                          end == std::string_view::npos
                                          ? std::string_view::npos
                                          : end - pos);
        bool stop = false;
        if (elem == "/")
        {
            style.reset();
            stop = true;
        }
        else if (palette)
        {
            if (const auto palette_style = palette->lookup(elem))
            {
                style.merge(palette_style->get());
                stop = true;
            }
        }
        if (stop) 
        {}
        else if (elem.starts_with("fg:"))
        {
            const auto color_name = elem.substr(3);
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
        else if (elem.starts_with("bg:"))
        {
            const auto color_name = elem.substr(3);
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
        else if (elem == "bold") {
            style.bold();
        }
        else if (elem == "italic") {
            style.italic();
        }
        else if (elem == "underline") {
            style.underline();
        }
        // else if (elem == "dimmed") {
        //     style.dimmed();
        // }
        // else if (elem == "inverted") {
        //     style.inverted();
        // }
        else if (elem == "blink") {
            style.blink();
        }
        else if (elem == "hidden") {
            style.hidden();
        }
        else if (elem == "strikethrough") {
            style.strikethrough();
        }
        else
        {
            return Unexpected("invalid style element: " +
                              std::string{elem});
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

