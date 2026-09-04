/**
 * @file stylelparser.hpp
 */

#pragma once

#include <string>
#include "Expected.hpp"
#include "Style.hpp"
#include "Palette.hpp"

namespace devprompt
{

// Precondition: stylestr only contains lowercase ASCII characters.
[[nodiscard]]
Expected<Style, std::string> parse_style(const Palette* palette, std::string_view stylestr);

[[nodiscard]]
inline Expected<Style, std::string> parse_style(std::string_view stylestr)
{ return parse_style(nullptr, stylestr); }

// Construct a style Palette from a compacted palette string: "name;style;name;style;..."
Expected<Palette, std::string> palette_from_string(std::string_view palette);

} // namespace devprompt
