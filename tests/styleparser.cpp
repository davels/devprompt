/**
 * @file tests/styleoarser.cpp
 *
 * Catch2 unit tests for styleparser.hpp
 * parse_style
 * palette_from_string
 */


#include <catch2/catch_test_macros.hpp>
#include "styleparser.hpp"
#include "Style.hpp"
#include "Palette.hpp"

using namespace devprompt;
using devprompt::ansi::Output;


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
    CHECK(parse_style("bold")->str(Output::Code) == "1");
    CHECK(parse_style("italic")->str(Output::Code) == "3");
    CHECK(parse_style("underline")->str(Output::Code) == "4");
    CHECK(parse_style("blink")->str(Output::Code) == "5");
    CHECK(parse_style("hidden")->str(Output::Code) == "8");
    CHECK(parse_style("strikethrough")->str(Output::Code) == "9");
    CHECK(parse_style("/")->str(Output::Code) == "0"); // "/" maps to Style::reset()
}

TEST_CASE("parse_style combines multiple attribute words in the given order",
          "[parse_style][attributes]")
{
    auto result = parse_style("bold italic underline");
    REQUIRE(static_cast<bool>(result));
    CHECK(result->str(Output::Code) == "1;3;4");
}

TEST_CASE("parse_style is case-sensitive and rejects uppercase keywords",
          "[parse_style][case-sensitivity]")
{
    auto result = parse_style("BOLD");
    REQUIRE_FALSE(static_cast<bool>(result));
    CHECK(result.error() == "invalid style element: BOLD");
}

TEST_CASE("Unimplemented attribute keywords are rejected as unknown components",
          "[parse_style][attributes][not-yet-implemented]")
{
    // "dimmed" and "inverted" are commented out in AnsiColor.cpp; until they
    // are implemented they must be treated as unknown style components.
    auto dimmed = parse_style("dimmed");
    REQUIRE_FALSE(static_cast<bool>(dimmed));
    CHECK(dimmed.error() == "invalid style element: dimmed");

    auto inverted = parse_style("inverted");
    REQUIRE_FALSE(static_cast<bool>(inverted));
    CHECK(inverted.error() == "invalid style element: inverted");
}

// ---------------------------------------------------------------------
// parse_style: unknown components
// ---------------------------------------------------------------------

TEST_CASE("parse_style rejects an unrecognized style component", "[parse_style][invalid]")
{
    auto result = parse_style("sparkle");
    REQUIRE_FALSE(static_cast<bool>(result));
    CHECK(result.error() == "invalid style element: sparkle");
}

TEST_CASE("parse_style stops at the first invalid component even if valid ones follow",
          "[parse_style][invalid]")
{
    auto result = parse_style("bold sparkle underline");
    REQUIRE_FALSE(static_cast<bool>(result));
    CHECK(result.error() == "invalid style element: sparkle");
}

// ---------------------------------------------------------------------
// parse_style: fg:/bg: named colors
// ---------------------------------------------------------------------

TEST_CASE("parse_style resolves every named foreground color", "[parse_style][color][named]")
{
    CHECK(parse_style("fg:black")->str(Output::Code) == "30");
    CHECK(parse_style("fg:red")->str(Output::Code) == "31");
    CHECK(parse_style("fg:green")->str(Output::Code) == "32");
    CHECK(parse_style("fg:yellow")->str(Output::Code) == "33");
    CHECK(parse_style("fg:blue")->str(Output::Code) == "34");
    CHECK(parse_style("fg:magenta")->str(Output::Code) == "35");
    CHECK(parse_style("fg:cyan")->str(Output::Code) == "36");
    CHECK(parse_style("fg:white")->str(Output::Code) == "37");
    CHECK(parse_style("fg:bright-black")->str(Output::Code) == "90");
    CHECK(parse_style("fg:bright-red")->str(Output::Code) == "91");
    CHECK(parse_style("fg:bright-green")->str(Output::Code) == "92");
    CHECK(parse_style("fg:bright-yellow")->str(Output::Code) == "93");
    CHECK(parse_style("fg:bright-blue")->str(Output::Code) == "94");
    CHECK(parse_style("fg:bright-magenta")->str(Output::Code) == "95");
    CHECK(parse_style("fg:bright-cyan")->str(Output::Code) == "96");
    CHECK(parse_style("fg:bright-white")->str(Output::Code) == "97");
}

TEST_CASE("parse_style resolves named background colors (offset by 10)",
          "[parse_style][color][named]")
{
    CHECK(parse_style("bg:red")->str(Output::Code) == "41");
    CHECK(parse_style("bg:bright-cyan")->str(Output::Code) == "106");
}

TEST_CASE("parse_style combines fg: and bg: named colors with attributes",
          "[parse_style][color][named]")
{
    auto result = parse_style("bold fg:green bg:black");
    REQUIRE(static_cast<bool>(result));
    CHECK(result->str(Output::Code) == "1;32;40");
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
    CHECK(parse_style("fg:#ff00ff")->str(Output::Code) == "38;2;255;0;255");
}

TEST_CASE("parse_style resolves a valid hex background color", "[parse_style][color][hex]")
{
    CHECK(parse_style("bg:#0a141e")->str(Output::Code) == "48;2;10;20;30");
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
// palette_from_string
// ---------------------------------------------------------------------

TEST_CASE("palette_from_string parses a single style", "[palette]")
{
    auto result = palette_from_string("prompt;bold");

    REQUIRE(result);
    const auto& palette = *result;

    REQUIRE_FALSE(palette.empty());

    auto style = palette.lookup("prompt");
    REQUIRE(style);
    CHECK(style->get().str(ansi::Output::Code) == "1");
}

TEST_CASE("palette_from_string parses multiple styles", "[palette]")
{
    auto result = palette_from_string("prompt;bold;branch;fg:green;error;fg:red");

    REQUIRE(result);

    const auto& palette = *result;

    auto prompt = palette.lookup("prompt");
    REQUIRE(prompt);
    CHECK(prompt->get().str(ansi::Output::Code) == "1");

    auto branch = palette.lookup("branch");
    REQUIRE(branch);
    CHECK(branch->get().str(ansi::Output::Code) == "32");

    auto error = palette.lookup("error");
    REQUIRE(error);
    CHECK(error->get().str(ansi::Output::Code) == "31");
}

TEST_CASE("palette_from_string parses multiple style tokens", "[palette]")
{
    auto result = palette_from_string("prompt;bold italic fg:red bg:blue");

    REQUIRE(result);

    auto style = result->lookup("prompt");
    REQUIRE(style);

    CHECK(style->get().str(ansi::Output::Code) == "1;3;31;44");
}

TEST_CASE("palette_from_string ignores whitespace around separators", "[palette]")
{
    auto result = palette_from_string(" prompt ; bold ; branch ; fg:green ");

    REQUIRE(result);

    const auto& palette = *result;

    auto prompt = palette.lookup("prompt");
    REQUIRE(prompt);
    CHECK(prompt->get().str(ansi::Output::Code) == "1");

    auto branch = palette.lookup("branch");
    REQUIRE(branch);
    CHECK(branch->get().str(ansi::Output::Code) == "32");
}

TEST_CASE("palette_from_string trims whitespace from names", "[palette]")
{
    auto result = palette_from_string(
        "  prompt  ;bold;  branch  ;fg:green");

    REQUIRE(result);

    const auto& palette = *result;

    CHECK(palette.lookup("prompt"));
    CHECK(palette.lookup("branch"));
    CHECK_FALSE(palette.lookup("  prompt  "));
    CHECK_FALSE(palette.lookup("  branch  "));
}

TEST_CASE("palette_from_string rejects unbalanced separators", "[palette]")
{
    SECTION("no separator")
    {
        auto result = palette_from_string("prompt");

        REQUIRE_FALSE(result);
        CHECK(result.error() == "palette has unbalanced ';' separators");
    }

    SECTION("missing style separator")
    {
        auto result = palette_from_string("prompt;bold;branch");

        REQUIRE_FALSE(result);
        CHECK(result.error() == "palette has unbalanced ';' separators");
    }
}

TEST_CASE("palette_from_string rejects missing style", "[palette]")
{
    auto result = palette_from_string("prompt;");

    REQUIRE_FALSE(result);
    CHECK(result.error() == "palette style missing");
}

TEST_CASE("palette_from_string rejects empty name", "[palette]")
{
    SECTION("empty name")
    {
        auto result = palette_from_string(";bold");

        REQUIRE_FALSE(result);
        CHECK(result.error() == "palette has empty name");
    }

    SECTION("whitespace-only name")
    {
        auto result = palette_from_string("   ;bold");

        REQUIRE_FALSE(result);
        CHECK(result.error() == "palette has empty name");
    }

    SECTION("empty name between styles")
    {
        auto result = palette_from_string("prompt;bold;;red");

        REQUIRE_FALSE(result);
        CHECK(result.error() == "palette has empty name");
    }
}

TEST_CASE("palette_from_string propagates invalid style error", "[palette]")
{
    auto result = palette_from_string("prompt;not-a-valid-style");

    REQUIRE_FALSE(result);
    CHECK_FALSE(result.error().empty());
}

TEST_CASE("palette_from_string accepts empty input", "[palette]")
{
    auto result = palette_from_string("");

    REQUIRE(result);
    CHECK(result->empty());
}

TEST_CASE("palette_from_string parses bright colors", "[palette]")
{
    auto result = palette_from_string("prompt;bold fg:bright-green bg:bright-red");

    REQUIRE(result);

    auto style = result->lookup("prompt");
    REQUIRE(style);

    CHECK(style->get().str(ansi::Output::Code) == "1;92;101");
}

TEST_CASE("palette_from_string parses hex colors", "[palette]")
{
    auto result = palette_from_string("prompt;fg:#aa2277 bg:#123456");

    REQUIRE(result);

    auto style = result->lookup("prompt");
    REQUIRE(style);

    CHECK(style->get().str(ansi::Output::Code) == "38;2;170;34;119;48;2;18;52;86");
}
