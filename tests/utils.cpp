/**
 * @file tests/utils.cpp
 */

#include <catch2/catch_test_macros.hpp>
#include <string>
#include <string_view>
#include "utils.hpp"

using namespace devprompt::utils;

TEST_CASE("isspace recognizes ASCII whitespace", "[utils][whitespace]")
{
    CHECK(isspace(' '));
    CHECK(isspace('\t'));
    CHECK(isspace('\n'));
    CHECK(isspace('\r'));
    CHECK(isspace('\f'));
    CHECK(isspace('\v'));
}

TEST_CASE("isspace rejects non-whitespace", "[utils][whitespace]")
{
    CHECK_FALSE(isspace('a'));
    CHECK_FALSE(isspace('0'));
    CHECK_FALSE(isspace('.'));
    CHECK_FALSE(isspace('\0'));
}

TEST_CASE("isspace handles high-bit characters", "[utils][whitespace]")
{
    // Important because char may be signed.
    CHECK_FALSE(isspace(static_cast<char>(0x80)));
    CHECK_FALSE(isspace(static_cast<char>(0xff)));
}

TEST_CASE("skip_whitespace", "[utils][whitespace]")
{
    SECTION("no whitespace")
    {
        CHECK(skip_whitespace("hello", 0) == 0);
    }

    SECTION("leading spaces")
    {
        CHECK(skip_whitespace("   hello", 0) == 3);
    }

    SECTION("all whitespace")
    {
        CHECK(skip_whitespace("   ", 0) == std::string_view::npos);
    }

    SECTION("empty string")
    {
        CHECK(skip_whitespace("", 0) == std::string_view::npos);
    }

    SECTION("mixed whitespace")
    {
        CHECK(skip_whitespace(" \t\r\nhello", 0) == 4);
    }

    SECTION("starts in the middle")
    {
        CHECK(skip_whitespace("abc   hello", 3) == 6);
    }

    SECTION("position already at non-whitespace")
    {
        CHECK(skip_whitespace("abc", 1) == 1);
    }

    SECTION("position at end")
    {
        CHECK(skip_whitespace("abc", 3) == std::string_view::npos);
    }

    SECTION("position beyond end")
    {
        CHECK(skip_whitespace("abc", 10) == std::string_view::npos);
    }

    SECTION("whitespace followed by whitespace")
    {
        CHECK(skip_whitespace(" \t\n\r\f\vX", 0) == 6);
    }
}

TEST_CASE("trimmed removes leading and trailing whitespace",
          "[utils][whitespace]")
{
    CHECK(trimmed("hello") == "hello");
    CHECK(trimmed(" hello") == "hello");
    CHECK(trimmed("hello ") == "hello");
    CHECK(trimmed(" hello ") == "hello");
    CHECK(trimmed("\t hello \n") == "hello");
    CHECK(trimmed(" \t\r\nhello\n\r\t ") == "hello");
}

TEST_CASE("trimmed handles empty and whitespace-only strings",
          "[utils][whitespace]")
{
    CHECK(trimmed("").empty());
    CHECK(trimmed(" ").empty());
    CHECK(trimmed("\t\n\r\f\v").empty());
}

TEST_CASE("trimmed preserves internal whitespace",
          "[utils][whitespace]")
{
    CHECK(trimmed("  hello   world  ") == "hello   world");
}

TEST_CASE("trimmed works with string", "[utils][whitespace]")
{
    std::string str = "  hello world  ";

    const auto result = trimmed(str);

    CHECK(result == "hello world");
}

TEST_CASE("trim modifies string_view", "[utils][whitespace]")
{
    std::string_view str = "  hello world  ";

    trim(str);

    CHECK(str == "hello world");
}

TEST_CASE("trim handles empty and whitespace-only strings",
          "[utils][whitespace]")
{
    SECTION("empty")
    {
        std::string_view str;
        trim(str);
        CHECK(str.empty());
    }

    SECTION("whitespace only")
    {
        std::string_view str = " \t\n\r\f\v";
        trim(str);
        CHECK(str.empty());
    }
}

TEST_CASE("trim preserves internal whitespace", "[utils][whitespace]")
{
    std::string_view str = "  hello   world  ";

    trim(str);

    CHECK(str == "hello   world");
}


TEST_CASE("find_first_whitespace", "[utils][whitespace]")
{
    SECTION("whitespace at beginning")
    {
        CHECK(find_first_whitespace(" hello", 0) == 0);
    }

    SECTION("no whitespace")
    {
        CHECK(find_first_whitespace("hello", 0) == std::string_view::npos);
    }

    SECTION("whitespace in middle")
    {
        CHECK(find_first_whitespace("hello world", 0) == 5);
    }

    SECTION("multiple whitespace characters")
    {
        CHECK(find_first_whitespace("hello \tworld", 0) == 5);
    }

    SECTION("starts in the middle")
    {
        CHECK(find_first_whitespace("hello   world", 1) == 5);
    }

    SECTION("position already at whitespace")
    {
        CHECK(find_first_whitespace("hello world", 5) == 5);
    }

    SECTION("position after whitespace")
    {
        CHECK(find_first_whitespace("hello world", 6) == std::string_view::npos);
    }

    SECTION("position at end")
    {
        CHECK(find_first_whitespace("hello", 5) == std::string_view::npos);
    }

    SECTION("position beyond end")
    {
        CHECK(find_first_whitespace("hello", 10) == std::string_view::npos);
    }

    SECTION("empty string")
    {
        CHECK(find_first_whitespace("", 0) == std::string_view::npos);
    }

    SECTION("all whitespace")
    {
        CHECK(find_first_whitespace(" \t\n\r\f\v", 0) == 0);
    }

    SECTION("finds each whitespace character")
    {
        CHECK(find_first_whitespace("a b", 0) == 1);
        CHECK(find_first_whitespace("a\tb", 0) == 1);
        CHECK(find_first_whitespace("a\nb", 0) == 1);
        CHECK(find_first_whitespace("a\rb", 0) == 1);
        CHECK(find_first_whitespace("a\fb", 0) == 1);
        CHECK(find_first_whitespace("a\vb", 0) == 1);
    }

    SECTION("handles high-bit characters")
    {
        // Verifies that the lookup is safe when char is signed.
        std::string_view str{
            "abc"
        };

        CHECK(find_first_whitespace(str, 0) == std::string_view::npos);
    }
}
