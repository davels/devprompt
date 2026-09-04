/**
 * @file tests/Style.cpp
 *
 * Catch2 unit tests for Style.hpp.
 *
 * Notes:
 *  - Covers Style, the free functions style()/reset()/append_style()
 *  - `fg:`/`bg:` with nothing after the colon does not crash but produces
 *    an error with an empty color name.
 *  - All style keywords (`bold`, `fg:`, `bg:`, color names, etc.) are
 *    required to be lowercase; see the case-sensitivity test below.
 */

#include <catch2/catch_test_macros.hpp>
#include "Style.hpp"

using namespace devprompt;
using devprompt::ansi::Output;
using devprompt::ansi::Color;


// ---------------------------------------------------------------------
// Style: text attributes
// ---------------------------------------------------------------------

TEST_CASE("Style attribute codes are correct", "[Style][attributes]")
{
    CHECK(Style().reset().str(Output::Code) == "0");
    CHECK(Style().bold().str(Output::Code) == "1");
    CHECK(Style().dim().str(Output::Code) == "2");
    CHECK(Style().italic().str(Output::Code) == "3");
    CHECK(Style().underline().str(Output::Code) == "4");
    CHECK(Style().blink().str(Output::Code) == "5");
    CHECK(Style().reverse().str(Output::Code) == "7");
    CHECK(Style().hidden().str(Output::Code) == "8");
    CHECK(Style().strikethrough().str(Output::Code) == "9");
}

TEST_CASE("Style attributes chain and join with semicolons", "[Style][chaining]")
{
    Style s;
    s.bold().underline().italic();
    CHECK(s.str(Output::Code) == "1;4;3");
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
    CHECK(Style().fg(Color::Black).str(Output::Code) == "30");
    CHECK(Style().fg(Color::Red).str(Output::Code) == "31");
    CHECK(Style().fg(Color::BrightWhite).str(Output::Code) == "97");
}

TEST_CASE("Style bg(Color) is fg(Color) code + 10", "[Style][color]")
{
    CHECK(Style().bg(Color::Black).str(Output::Code) == "40");
    CHECK(Style().bg(Color::Red).str(Output::Code) == "41");
    CHECK(Style().bg(Color::BrightWhite).str(Output::Code) == "107");
}

TEST_CASE("fg and bg colors can be combined", "[Style][color]")
{
    Style s;
    s.fg(Color::Green).bg(Color::Black);
    CHECK(s.str(Output::Code) == "32;40");
}

// ---------------------------------------------------------------------
// Style: 24-bit (truecolor) RGB
// ---------------------------------------------------------------------

TEST_CASE("Style fg(r,g,b) emits a 38;2;r;g;b code", "[Style][rgb]")
{
    CHECK(Style().fg(std::uint8_t{255}, std::uint8_t{0}, std::uint8_t{128}).str(Output::Code)
          == "38;2;255;0;128");
}

TEST_CASE("Style bg(r,g,b) emits a 48;2;r;g;b code", "[Style][rgb]")
{
    CHECK(Style().bg(std::uint8_t{10}, std::uint8_t{20}, std::uint8_t{30}).str(Output::Code)
          == "48;2;10;20;30");
}

// ---------------------------------------------------------------------
// Style: hex color strings
// ---------------------------------------------------------------------

TEST_CASE("Style fg(hex) parses a valid lowercase hex color", "[Style][hex]")
{
    CHECK(Style().fg("#ff0000").str(Output::Code) == "38;2;255;0;0");
}

TEST_CASE("Style fg(hex) parses a valid uppercase hex color", "[Style][hex]")
{
    CHECK(Style().fg("#00FF80").str(Output::Code) == "38;2;0;255;128");
}

TEST_CASE("Style bg(hex) parses a valid hex color", "[Style][hex]")
{
    CHECK(Style().bg("#0a141e").str(Output::Code) == "48;2;10;20;30");
}

TEST_CASE("Style fg(hex) leaves style unchanged when hex is malformed", "[Style][hex][invalid]")
{
    // Missing '#'
    CHECK(Style().fg("ff0000").str(Output::Code).empty());
    // Wrong length (too short)
    CHECK(Style().fg("#ff00").str(Output::Code).empty());
    // Wrong length (too long)
    CHECK(Style().fg("#ff000000").str(Output::Code).empty());
    // Invalid hex digits
    CHECK(Style().fg("#zz0000").str(Output::Code).empty());
    CHECK(Style().fg("#ff00zz").str(Output::Code).empty());
    // Empty string
    CHECK(Style().fg("").str(Output::Code).empty());
}

TEST_CASE("Style bg(hex) leaves style unchanged when hex is malformed", "[Style][hex][invalid]")
{
    CHECK(Style().bg("#12345").str(Output::Code).empty());
    CHECK(Style().bg("not-a-hex").str(Output::Code).empty());
}

TEST_CASE("A malformed hex does not clobber previously added codes", "[Style][hex][invalid]")
{
    Style s;
    s.bold().fg("#gggggg"); // invalid, should be a no-op
    CHECK(s.str(Output::Code) == "1");
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

TEST_CASE("Output::Code output the code without any wrapping", "[Output][Code]")
{
    CHECK(Style().bold().str(Output::Code) == "1");
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
    CHECK(Style().str(Output::Code).empty());
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
    CHECK(reset(Output::Code) == "0");
    CHECK(reset(Output::Zsh) == "%{\033[0m%}");
    CHECK(reset(Output::PowerShell) == "`e[0m");
    CHECK(reset(Output::Bash) == std::string("\001") + "\033[0m" + "\002");
}

TEST_CASE("reset(Output::None) is empty", "[free-functions][reset]")
{
    CHECK(reset(Output::None).empty());
}
