/**
 * @file tests/formatter.cpp
 */

#include <catch2/catch_test_macros.hpp>
#include "VcsFormatter.hpp"
#include "RepoStatus.hpp"
#include "Options.hpp"

using devprompt::VcsModule::format_status;
using devprompt::VcsModule::RepoStatus;


TEST_CASE("format clean repository")
{
    RepoStatus status;
    status.branch = "main";

    REQUIRE(format_status(status, {}) == "main");
}

TEST_CASE("format detached HEAD")
{
    RepoStatus status;
    status.detached = true;
    status.branch = "a1b2c3d";

    REQUIRE(format_status(status, {}) == "@a1b2c3d");
}

TEST_CASE("format unstaged files")
{
    RepoStatus status;
    status.branch = "main";
    status.unstaged = 3;

    REQUIRE(format_status(status, {}) == "main *");
}

TEST_CASE("format staged files")
{
    RepoStatus status;
    status.branch = "main";
    status.staged = 3;

    REQUIRE(format_status(status, {}) == "main +");
}
