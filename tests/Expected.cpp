/**
 * @file Expected.cpp
 */

#include <catch2/catch_test_macros.hpp>
#include "Expected.hpp"

#include <string>

using devprompt::Expected;
using devprompt::Unexpected;

TEST_CASE("Expected constructed with value")
{
    Expected<int, std::string> e(42);

    REQUIRE(e.has_value());
    REQUIRE(static_cast<bool>(e));
    REQUIRE(e.value() == 42);
    REQUIRE(*e == 42);
}

TEST_CASE("Expected constructed with Unexpected")
{
    Expected<int, std::string> e = Unexpected(std::string{"bad thing"});

    REQUIRE_FALSE(e.has_value());
    REQUIRE_FALSE(static_cast<bool>(e));
    REQUIRE(e.error() == "bad thing");
}

TEST_CASE("Unexpected deduces from const char* literal")
{
    Expected<int, std::string> e = Unexpected("bad thing");

    REQUIRE_FALSE(e.has_value());
    REQUIRE(e.error() == std::string{"bad thing"});
}

TEST_CASE("Unexpected deduces from concatenated temporary std::string")
{
    std::string color = "puce";
    Expected<int, std::string> e = Unexpected("invalid color: " + color);

    REQUIRE_FALSE(e.has_value());
    REQUIRE(e.error() == "invalid color: puce");
}

TEST_CASE("value() throws when Expected holds an error")
{
    Expected<int, std::string> e = Unexpected(std::string{"bad thing"});

    REQUIRE_THROWS_AS(e.value(), std::bad_variant_access);
}

TEST_CASE("error() throws when Expected holds a value")
{
    Expected<int, std::string> e(42);

    REQUIRE_THROWS_AS(e.error(), std::bad_variant_access);
}

TEST_CASE("operator-> accesses members of contained value")
{
    struct Point { int x; int y; };

    Expected<Point, std::string> e(Point{1, 2});

    REQUIRE(e->x == 1);
    REQUIRE(e->y == 2);
}

TEST_CASE("Expected value can be mutated through non-const access")
{
    Expected<int, std::string> e(1);

    *e = 99;

    REQUIRE(e.value() == 99);
}
