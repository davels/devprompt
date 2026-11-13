/**
 * @file tests/jsonparser.cpp
 */

#include <catch2/catch_test_macros.hpp>
#include <string_view>
#include "jsonparser.hpp"

using devprompt::json::get_version;

TEST_CASE("get_version finds top-level version")
{
    const auto json = R"({
        "name": "my-program",
        "version": "1.2.5",
        "bin": {
            "my-program": "path/to/program"
        }
    })";

    CHECK(get_version(json) == "1.2.5");
}

TEST_CASE("get_version handles whitespace")
{
    CHECK(get_version(R"({"version":"1.2.5"})") == "1.2.5");

    CHECK(get_version(R"(
        {
            "version" : "2.0.0"
        }
    )") == "2.0.0");
}

TEST_CASE("get_version finds version regardless of property order")
{
    CHECK(get_version(R"({
        "description": "test",
        "name": "foo",
        "version": "3.4.5"
    })") == "3.4.5");

    CHECK(get_version(R"({
        "version": "3.4.5",
        "name": "foo",
        "description": "test"
    })") == "3.4.5");
}

TEST_CASE("get_version ignores nested version properties")
{
    const auto json = R"({
        "metadata": {
            "version": "nested"
        },
        "version": "1.2.3"
    })";

    CHECK(get_version(json) == "1.2.3");
}

TEST_CASE("get_version ignores version properties in arrays")
{
    const auto json = R"({
        "packages": [
            {
                "version": "nested-1"
            },
            {
                "version": "nested-2"
            }
        ],
        "version": "1.2.3"
    })";

    CHECK(get_version(json) == "1.2.3");
}

TEST_CASE("get_version handles nested arrays and objects")
{
    const auto json = R"({
        "data": [
            1,
            2,
            {
                "values": [
                    {
                        "version": "nested"
                    }
                ]
            }
        ],
        "version": "4.5.6"
    })";

    CHECK(get_version(json) == "4.5.6");
}

TEST_CASE("get_version handles structural characters inside strings")
{
    const auto json = R"({
        "description": "text containing { } [ ] and \"quotes\"",
        "version": "1.2.3"
    })";

    CHECK(get_version(json) == "1.2.3");
}

TEST_CASE("get_version handles escaped characters in skipped strings")
{
    const auto json = R"({
        "description": "line one\nline two\twith \"quotes\"",
        "version": "1.2.3"
    })";

    CHECK(get_version(json) == "1.2.3");
}

TEST_CASE("get_version returns empty when version is missing")
{
    CHECK(get_version(R"({
        "name": "my-program",
        "description": "test"
    })").empty());
}

TEST_CASE("get_version returns empty for non-string version")
{
    CHECK(get_version(R"({"version": 123})").empty());
    CHECK(get_version(R"({"version": true})").empty());
    CHECK(get_version(R"({"version": null})").empty());
    CHECK(get_version(R"({"version": {}})").empty());
    CHECK(get_version(R"({"version": []})").empty());
}

TEST_CASE("get_version does not match similarly named properties")
{
    CHECK(get_version(R"({
        "versions": "1.2.3",
        "versioned": "4.5.6"
    })").empty());
}

TEST_CASE("get_version handles empty version")
{
    CHECK(get_version(R"({"version": ""})" ).empty());
}

TEST_CASE("get_version handles empty and minimal JSON")
{
    CHECK(get_version("{}").empty());
    CHECK(get_version(R"({"name":"foo"})").empty());
}

TEST_CASE("get_version returns a view into the input")
{
    const std::string_view json = R"({"version":"1.2.3"})";

    const auto version = get_version(json);

    CHECK(version == "1.2.3");
    CHECK(version.data() > json.data());
    CHECK(version.data() < json.data() + json.size());
}

TEST_CASE("get_version handles realistic package configuration")
{
    const auto json = R"({
        "name": "@example/my-program",
        "version": "1.7.12",
        "private": true,
        "description": "Example application",
        "scripts": {
            "build": "cmake --build build",
            "test": "ctest --test-dir build"
        },
        "dependencies": {
            "foo": "^1.2.3",
            "bar": "^4.5.6"
        },
        "config": {
            "version": "not-the-package-version"
        }
    })";

    CHECK(get_version(json) == "1.7.12");
}

TEST_CASE("get_version handles realistic package configuration with version at bottom")
{
    const auto json = R"({
        "name": "@example/my-program",
        "private": true,
        "description": "Example application",
        "scripts": {
            "build": "cmake --build build",
            "test": "ctest --test-dir build"
        },
        "dependencies": {
            "foo": "^1.2.3",
            "bar": "^4.5.6"
        },
        "version": "1.7.12",
        "config": {
            "version": "not-the-package-version"
        }
    })";

    CHECK(get_version(json) == "1.7.12");
}
