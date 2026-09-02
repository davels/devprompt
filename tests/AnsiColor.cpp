/**
 * @file test_AnsiColor.cpp
 *
 * Catch2 unit tests for AnsiColor.hpp.
 *
 * Notes:
 *  - Covers Style, the free functions style()/reset()/append_style(), and
 *    parse_style() (AnsiColor.cpp).
 *  - The parse_style() tests assume `Expected<T,E>` exposes `.error()` to
 *    read the error value, mirroring std::expected's interface. Adjust if
 *    your Expected type names it differently.
 *  - `is_hex_color()` requires both `size == 7` and a leading '#'. One
 *    remaining quirk (tagged "[quirk]" below) falls out of `Style::fg(hex)`
 *    /`bg(hex)` themselves: a well-formed-length but unparsable hex string
 *    (e.g. "#zzzzzz") is silently dropped -- no color code, no error --
 *    rather than reported as invalid.
 *  - `fg:`/`bg:` with nothing after the colon does not crash but produces
 *    an error with an empty color name.
 *  - All style keywords (`bold`, `fg:`, `bg:`, color names, etc.) are
 *    required to be lowercase; see the case-sensitivity test below.
 *
 * Build (Catch2 v3 example):
 *   g++ -std=c++20 -I<path-to-catch2>/include test_AnsiColor.cpp \
 *       AnsiColor.cpp -lCatch2Main -lCatch2 -o test_AnsiColor
 *   ./test_AnsiColor
 */

#include <catch2/catch_test_macros.hpp>
#include "AnsiColor.hpp"

using namespace devprompt::ansi;

// ---------------------------------------------------------------------
// Style: text attributes
// ---------------------------------------------------------------------

TEST_CASE("Style attribute codes are correct", "[Style][attributes]")
{
    CHECK(Style().reset().str(Output::Raw) == "\033[0m");
    CHECK(Style().bold().str(Output::Raw) == "\033[1m");
    CHECK(Style().dim().str(Output::Raw) == "\033[2m");
    CHECK(Style().italic().str(Output::Raw) == "\033[3m");
    CHECK(Style().underline().str(Output::Raw) == "\033[4m");
    CHECK(Style().blink().str(Output::Raw) == "\033[5m");
    CHECK(Style().reverse().str(Output::Raw) == "\033[7m");
    CHECK(Style().hidden().str(Output::Raw) == "\033[8m");
    CHECK(Style().strikethrough().str(Output::Raw) == "\033[9m");
}

TEST_CASE("Style attributes chain and join with semicolons", "[Style][chaining]")
{
    Style s;
    s.bold().underline().italic();
    CHECK(s.str(Output::Raw) == "\033[1;4;3m");
}

TEST_CASE("Style supports fluent chaining that returns the same object", "[Style][chaining]")
{
    Style s;
    Style& ref = s.bold();
    CHECK(&ref == &s);
}

// ---------------------------------------------------------------------
// Style: standard (named) colors
// ---------------------------------------------------------------------

TEST_CASE("Style fg(Color) emits the correct SGR code", "[Style][color]")
{
    CHECK(Style().fg(Color::Black).str(Output::Raw) == "\033[30m");
    CHECK(Style().fg(Color::Red).str(Output::Raw) == "\033[31m");
    CHECK(Style().fg(Color::BrightWhite).str(Output::Raw) == "\033[97m");
}

TEST_CASE("Style bg(Color) is fg(Color) code + 10", "[Style][color]")
{
    CHECK(Style().bg(Color::Black).str(Output::Raw) == "\033[40m");
    CHECK(Style().bg(Color::Red).str(Output::Raw) == "\033[41m");
    CHECK(Style().bg(Color::BrightWhite).str(Output::Raw) == "\033[107m");
}

TEST_CASE("fg and bg colors can be combined", "[Style][color]")
{
    Style s;
    s.fg(Color::Green).bg(Color::Black);
    CHECK(s.str(Output::Raw) == "\033[32;40m");
}

// ---------------------------------------------------------------------
// Style: 24-bit (truecolor) RGB
// ---------------------------------------------------------------------

TEST_CASE("Style fg(r,g,b) emits a 38;2;r;g;b code", "[Style][rgb]")
{
    CHECK(Style().fg(std::uint8_t{255}, std::uint8_t{0}, std::uint8_t{128}).str(Output::Raw)
          == "\033[38;2;255;0;128m");
}

TEST_CASE("Style bg(r,g,b) emits a 48;2;r;g;b code", "[Style][rgb]")
{
    CHECK(Style().bg(std::uint8_t{10}, std::uint8_t{20}, std::uint8_t{30}).str(Output::Raw)
          == "\033[48;2;10;20;30m");
}

// ---------------------------------------------------------------------
// Style: hex color strings
// ---------------------------------------------------------------------

TEST_CASE("Style fg(hex) parses a valid lowercase hex color", "[Style][hex]")
{
    CHECK(Style().fg("#ff0000").str(Output::Raw) == "\033[38;2;255;0;0m");
}

TEST_CASE("Style fg(hex) parses a valid uppercase hex color", "[Style][hex]")
{
    CHECK(Style().fg("#00FF80").str(Output::Raw) == "\033[38;2;0;255;128m");
}

TEST_CASE("Style bg(hex) parses a valid hex color", "[Style][hex]")
{
    CHECK(Style().bg("#0a141e").str(Output::Raw) == "\033[48;2;10;20;30m");
}

TEST_CASE("Style fg(hex) leaves style unchanged when hex is malformed", "[Style][hex][invalid]")
{
    // Missing '#'
    CHECK(Style().fg("ff0000").str(Output::Raw).empty());
    // Wrong length (too short)
    CHECK(Style().fg("#ff00").str(Output::Raw).empty());
    // Wrong length (too long)
    CHECK(Style().fg("#ff000000").str(Output::Raw).empty());
    // Invalid hex digits
    CHECK(Style().fg("#zz0000").str(Output::Raw).empty());
    CHECK(Style().fg("#ff00zz").str(Output::Raw).empty());
    // Empty string
    CHECK(Style().fg("").str(Output::Raw).empty());
}

TEST_CASE("Style bg(hex) leaves style unchanged when hex is malformed", "[Style][hex][invalid]")
{
    CHECK(Style().bg("#12345").str(Output::Raw).empty());
    CHECK(Style().bg("not-a-hex").str(Output::Raw).empty());
}

TEST_CASE("A malformed hex does not clobber previously added codes", "[Style][hex][invalid]")
{
    Style s;
    s.bold().fg("#gggggg"); // invalid, should be a no-op
    CHECK(s.str(Output::Raw) == "\033[1m");
}

// ---------------------------------------------------------------------
// Output formatting (via Style::str)
// ---------------------------------------------------------------------

TEST_CASE("Output::None always produces an empty string", "[Output][None]")
{
    CHECK(Style().bold().str(Output::None).empty());
    CHECK(Style().fg(Color::Red).bg(Color::Blue).str(Output::None).empty());
}

TEST_CASE("Output::Raw wraps codes in a plain ESC[...m sequence", "[Output][Raw]")
{
    CHECK(Style().bold().str(Output::Raw) == "\033[1m");
}

TEST_CASE("Output::Text wraps codes in double brackets", "[Output][Text]")
{
    CHECK(Style().bold().str(Output::Text) == "[[1]]");
}

TEST_CASE("Output::Bash wraps codes for readline non-printing markers", "[Output][Bash]")
{
    const std::string expected = std::string("\001") + "\033[1m" + "\002";
    CHECK(Style().bold().str(Output::Bash) == expected);
}

TEST_CASE("Output::Zsh wraps codes in %{...%}", "[Output][Zsh]")
{
    CHECK(Style().bold().str(Output::Zsh) == "%{\033[1m%}");
}

TEST_CASE("Output::PowerShell wraps codes with a backtick escape", "[Output][PowerShell]")
{
    CHECK(Style().bold().str(Output::PowerShell) == "`e[1m");
}

TEST_CASE("An empty Style produces an empty string regardless of Output",
          "[Output][empty]")
{
    CHECK(Style().str(Output::Raw).empty());
    CHECK(Style().str(Output::Text).empty());
    CHECK(Style().str(Output::Bash).empty());
    CHECK(Style().str(Output::Zsh).empty());
    CHECK(Style().str(Output::PowerShell).empty());
    CHECK(Style().str(Output::None).empty());
}

// ---------------------------------------------------------------------
// Free functions: style() and reset()
// ---------------------------------------------------------------------

TEST_CASE("style() returns a default-constructed, empty Style", "[free-functions][style]")
{
    CHECK(style().str(Output::Raw).empty());
}

TEST_CASE("reset(Output) wraps the '0' code directly, bypassing Style", "[free-functions][reset]")
{
    CHECK(reset(Output::Raw) == "\033[0m");
    CHECK(reset(Output::Text) == "[[0]]");
    CHECK(reset(Output::Zsh) == "%{\033[0m%}");
    CHECK(reset(Output::PowerShell) == "`e[0m");
    CHECK(reset(Output::Bash) == std::string("\001") + "\033[0m" + "\002");
}

TEST_CASE("reset(Output::None) is empty", "[free-functions][reset]")
{
    CHECK(reset(Output::None).empty());
}

// ---------------------------------------------------------------------
// append_style: header-defined early-return behavior
// ---------------------------------------------------------------------

TEST_CASE("append_style does nothing when the style string is empty", "[free-functions][append_style]")
{
    std::string out = "unchanged";
    append_style("", Output::Raw, out);
    CHECK(out == "unchanged");
}

// ---------------------------------------------------------------------
// parse_style: whitespace / empty handling
// ---------------------------------------------------------------------

TEST_CASE("parse_style on an empty string yields an empty (no-op) style", "[parse_style][whitespace]")
{
    auto result = parse_style("");
    REQUIRE(static_cast<bool>(result));
    CHECK(result->str(Output::Raw).empty());
}

TEST_CASE("parse_style on whitespace-only input yields an empty style", "[parse_style][whitespace]")
{
    auto result = parse_style("   \t \n  ");
    REQUIRE(static_cast<bool>(result));
    CHECK(result->str(Output::Raw).empty());
}

TEST_CASE("parse_style tolerates leading, trailing, and repeated whitespace between words",
          "[parse_style][whitespace]")
{
    auto result = parse_style("  bold   fg:red  bg:blue  ");
    REQUIRE(static_cast<bool>(result));
    CHECK(result->str(Output::Raw) == "\033[1;31;44m");
}

// ---------------------------------------------------------------------
// parse_style: single-word attribute tokens
// ---------------------------------------------------------------------

TEST_CASE("parse_style recognizes each attribute keyword", "[parse_style][attributes]")
{
    CHECK(parse_style("bold")->str(Output::Raw) == "\033[1m");
    CHECK(parse_style("italic")->str(Output::Raw) == "\033[3m");
    CHECK(parse_style("underline")->str(Output::Raw) == "\033[4m");
    CHECK(parse_style("blink")->str(Output::Raw) == "\033[5m");
    CHECK(parse_style("hidden")->str(Output::Raw) == "\033[8m");
    CHECK(parse_style("strikethrough")->str(Output::Raw) == "\033[9m");
    CHECK(parse_style("/")->str(Output::Raw) == "\033[0m"); // "/" maps to Style::reset()
}

TEST_CASE("parse_style combines multiple attribute words in the given order",
          "[parse_style][attributes]")
{
    auto result = parse_style("bold italic underline");
    REQUIRE(static_cast<bool>(result));
    CHECK(result->str(Output::Raw) == "\033[1;3;4m");
}

TEST_CASE("parse_style is case-sensitive and rejects uppercase keywords",
          "[parse_style][case-sensitivity]")
{
    auto result = parse_style("BOLD");
    REQUIRE_FALSE(static_cast<bool>(result));
    CHECK(result.error() == "invalid style component: BOLD");
}

TEST_CASE("Unimplemented attribute keywords are rejected as unknown components",
          "[parse_style][attributes][not-yet-implemented]")
{
    // "dimmed" and "inverted" are commented out in AnsiColor.cpp; until they
    // are implemented they must be treated as unknown style components.
    auto dimmed = parse_style("dimmed");
    REQUIRE_FALSE(static_cast<bool>(dimmed));
    CHECK(dimmed.error() == "invalid style component: dimmed");

    auto inverted = parse_style("inverted");
    REQUIRE_FALSE(static_cast<bool>(inverted));
    CHECK(inverted.error() == "invalid style component: inverted");
}

// ---------------------------------------------------------------------
// parse_style: unknown components
// ---------------------------------------------------------------------

TEST_CASE("parse_style rejects an unrecognized style component", "[parse_style][invalid]")
{
    auto result = parse_style("sparkle");
    REQUIRE_FALSE(static_cast<bool>(result));
    CHECK(result.error() == "invalid style component: sparkle");
}

TEST_CASE("parse_style stops at the first invalid component even if valid ones follow",
          "[parse_style][invalid]")
{
    auto result = parse_style("bold sparkle underline");
    REQUIRE_FALSE(static_cast<bool>(result));
    CHECK(result.error() == "invalid style component: sparkle");
}

// ---------------------------------------------------------------------
// parse_style: fg:/bg: named colors
// ---------------------------------------------------------------------

TEST_CASE("parse_style resolves every named foreground color", "[parse_style][color][named]")
{
    CHECK(parse_style("fg:black")->str(Output::Raw) == "\033[30m");
    CHECK(parse_style("fg:red")->str(Output::Raw) == "\033[31m");
    CHECK(parse_style("fg:green")->str(Output::Raw) == "\033[32m");
    CHECK(parse_style("fg:yellow")->str(Output::Raw) == "\033[33m");
    CHECK(parse_style("fg:blue")->str(Output::Raw) == "\033[34m");
    CHECK(parse_style("fg:magenta")->str(Output::Raw) == "\033[35m");
    CHECK(parse_style("fg:cyan")->str(Output::Raw) == "\033[36m");
    CHECK(parse_style("fg:white")->str(Output::Raw) == "\033[37m");
    CHECK(parse_style("fg:bright-black")->str(Output::Raw) == "\033[90m");
    CHECK(parse_style("fg:bright-red")->str(Output::Raw) == "\033[91m");
    CHECK(parse_style("fg:bright-green")->str(Output::Raw) == "\033[92m");
    CHECK(parse_style("fg:bright-yellow")->str(Output::Raw) == "\033[93m");
    CHECK(parse_style("fg:bright-blue")->str(Output::Raw) == "\033[94m");
    CHECK(parse_style("fg:bright-magenta")->str(Output::Raw) == "\033[95m");
    CHECK(parse_style("fg:bright-cyan")->str(Output::Raw) == "\033[96m");
    CHECK(parse_style("fg:bright-white")->str(Output::Raw) == "\033[97m");
}

TEST_CASE("parse_style resolves named background colors (offset by 10)",
          "[parse_style][color][named]")
{
    CHECK(parse_style("bg:red")->str(Output::Raw) == "\033[41m");
    CHECK(parse_style("bg:bright-cyan")->str(Output::Raw) == "\033[106m");
}

TEST_CASE("parse_style combines fg: and bg: named colors with attributes",
          "[parse_style][color][named]")
{
    auto result = parse_style("bold fg:green bg:black");
    REQUIRE(static_cast<bool>(result));
    CHECK(result->str(Output::Raw) == "\033[1;32;40m");
}

TEST_CASE("parse_style rejects an unrecognized named foreground color",
          "[parse_style][color][invalid]")
{
    // "crimson7" is 8 characters, so it does not accidentally match the
    // (buggy) is_hex_color() length check -- it correctly falls through to
    // the named-color lookup and fails there.
    auto result = parse_style("fg:crimson7");
    REQUIRE_FALSE(static_cast<bool>(result));
    CHECK(result.error() == "invalid foreground color: crimson7");
}

TEST_CASE("parse_style rejects an unrecognized named background color",
          "[parse_style][color][invalid]")
{
    auto result = parse_style("bg:crimson7");
    REQUIRE_FALSE(static_cast<bool>(result));
    CHECK(result.error() == "invalid background color: crimson7");
}

TEST_CASE("parse_style reports an empty color name for a bare 'fg:'/'bg:' token",
          "[parse_style][color][invalid][edge-case]")
{
    auto fgResult = parse_style("fg:");
    REQUIRE_FALSE(static_cast<bool>(fgResult));
    CHECK(fgResult.error() == "invalid foreground color: ");

    auto bgResult = parse_style("bg:");
    REQUIRE_FALSE(static_cast<bool>(bgResult));
    CHECK(bgResult.error() == "invalid background color: ");
}

// ---------------------------------------------------------------------
// parse_style: fg:/bg: hex colors
// ---------------------------------------------------------------------

TEST_CASE("parse_style resolves a valid hex foreground color", "[parse_style][color][hex]")
{
    CHECK(parse_style("fg:#ff00ff")->str(Output::Raw) == "\033[38;2;255;0;255m");
}

TEST_CASE("parse_style resolves a valid hex background color", "[parse_style][color][hex]")
{
    CHECK(parse_style("bg:#0a141e")->str(Output::Raw) == "\033[48;2;10;20;30m");
}

TEST_CASE("parse_style silently drops a malformed hex color that is 7 chars and '#'-prefixed",
          "[parse_style][color][hex][quirk]")
{
    // "#zzzzzz" is 7 chars AND starts with '#', so is_hex_color() routes it
    // to Style::fg(hex), which silently no-ops when detail::parse_hex()
    // fails -- no error is returned and no color code is emitted. This is
    // a property of Style::fg(hex)/bg(hex) itself (see the Style-level
    // "leaves style unchanged when hex is malformed" tests above), not of
    // is_hex_color(), so it holds regardless of how is_hex_color() is
    // implemented.
    auto result = parse_style("fg:#zzzzzz");
    REQUIRE(static_cast<bool>(result));
    CHECK(result->str(Output::Raw).empty());
}

TEST_CASE("parse_style rejects an unrecognized color word that happens to be 7 characters",
          "[parse_style][color]")
{
    // "orange1" is a made-up, unrecognized color name that is exactly 7
    // characters long but does not start with '#'. is_hex_color() requires
    // both the length check AND a leading '#', so this correctly falls
    // through to the named-color lookup and is reported as invalid, the
    // same as any other unrecognized name (c.f. the "crimson7" tests).
    auto result = parse_style("fg:orange1");
    REQUIRE_FALSE(static_cast<bool>(result));
    CHECK(result.error() == "invalid foreground color: orange1");
}

// ---------------------------------------------------------------------
// append_style
// ---------------------------------------------------------------------

TEST_CASE("append_style appends the rendered style to an existing string",
          "[append_style]")
{
    std::string out = "prefix:";
    append_style("bold", Output::Raw, out);
    CHECK(out == "prefix:\033[1m");
}

TEST_CASE("append_style respects the requested Output format", "[append_style]")
{
    std::string out;
    append_style("bold", Output::Text, out);
    CHECK(out == "[[1]]");
}

TEST_CASE("append_style leaves the output untouched when the style string is invalid",
          "[append_style][invalid]")
{
    std::string out = "unchanged";
    append_style("sparkle", Output::Raw, out);
    CHECK(out == "unchanged");
}

TEST_CASE("append_style can be called multiple times to accumulate styling",
          "[append_style]")
{
    std::string out;
    append_style("bold", Output::Raw, out);
    append_style("fg:red", Output::Raw, out);
    CHECK(out == "\033[1m\033[31m");
}
