/**
 * @file tests/FormatString.cpp
 */

#include <catch2/catch_test_macros.hpp>
#include <string>
#include "FormatString.hpp"

using namespace devprompt;

namespace
{

std::string expand(const std::string& format)
{
    FormatString fs{format};

    REQUIRE(fs.compile());

    std::string out;
    fs.expand(ansi::Output::None, out);
    return out;
}

} // namespace


TEST_CASE("FormatString: literal text", "[FormatString]")
{
    CHECK(expand("hello world") == "hello world");
    CHECK(expand("") == "");
    CHECK(expand("123456789") == "123456789");
}


TEST_CASE("FormatString: UTF-8 literal text", "[FormatString]")
{
    // U+E46F, encoded as three UTF-8 bytes.
    const std::string format = "abcd\xEE\xB1\xAF"
                               "123456";

    CHECK(expand(format) == format);
}


TEST_CASE("FormatString: UTF-8 character followed by text", "[FormatString]")
{
    // This is specifically a regression test for the signed-char
    // SPECIAL_CHARS lookup bug.
    const std::string format = "abcd\xEE\xB1\xAF"
                               "123456";

    FormatString fs{format};

    REQUIRE(fs.compile());

    std::string out;
    fs.expand(ansi::Output::None, out);

    CHECK(out.size() == format.size());
    CHECK(out == format);
}


TEST_CASE("FormatString: ASCII text around UTF-8 character", "[FormatString]")
{
    const std::string format = "a\xEE\xB1\xAF"
                               "b";

    CHECK(expand(format) == format);
}


TEST_CASE("FormatString: escaped characters", "[FormatString]")
{
    CHECK(expand(R"(hello\%world)") == "hello%world");
    CHECK(expand(R"(hello\[world)") == "hello[world");
    CHECK(expand(R"(hello\]world)") == "hello]world");
    CHECK(expand(R"(hello\{world)") == "hello{world");
    CHECK(expand(R"(hello\}world)") == "hello}world");
    CHECK(expand(R"(hello\\world)") == R"(hello\world)");
}


TEST_CASE("FormatString: escaped newline", "[FormatString]")
{
    CHECK(expand(R"(hello\nworld)") == "hello\nworld");
}


TEST_CASE("FormatString: trailing escape", "[FormatString]")
{
    CHECK(expand("hello\\") == "hello\\");
}


TEST_CASE("FormatString: multiple literal tokens", "[FormatString]")
{
    // Escaping '%' forces the parser to finish the current literal
    // and create another one.
    CHECK(expand(R"(abc\%def)") == "abc%def");
}


TEST_CASE("FormatString: missing variable close", "[FormatString]")
{
    FormatString fs{"hello %foo"};

    auto result = fs.compile();

    CHECK_FALSE(result);
}


TEST_CASE("FormatString: unknown variable", "[FormatString]")
{
    FormatString fs{"%this_variable_does_not_exist%"};

    auto result = fs.compile();

    CHECK_FALSE(result);
}


TEST_CASE("FormatString: mismatched conditional close", "[FormatString]")
{
    FormatString fs{"hello]"};

    auto result = fs.compile();

    CHECK_FALSE(result);
}


TEST_CASE("FormatString: unterminated style", "[FormatString]")
{
    FormatString fs{"hello {bold"};

    auto result = fs.compile();

    CHECK_FALSE(result);
}


TEST_CASE("FormatString: assign clears compiled tokens", "[FormatString]")
{
    FormatString fs{"first"};

    REQUIRE(fs.compile());

    std::string out;
    fs.expand(ansi::Output::None, out);
    CHECK(out == "first");

    fs.assign("second");

    out.clear();
    // assign() clears m_tokens, so the new format must be compiled.
    fs.compile();
    fs.expand(ansi::Output::None, out);

    CHECK(out == "second");
}
