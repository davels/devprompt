/**
 * @file tests/utils_split_view.cpp
 */

#include <string>
#include <string_view>
#include <ranges>
#include <vector>
#include "utils.hpp"
#include <catch2/catch_test_macros.hpp>

using devprompt::utils::split_view;

namespace {

// Local helper (the project's own split_view has no vector convenience
// function right now) -- just materializes a split_view into a vector so
// the tests below can compare against expected token lists directly.
std::vector<std::string_view> collect(auto splitter) {
    std::vector<std::string_view> result;
    for (std::string_view piece : splitter) result.push_back(piece);
    return result;
}

} // namespace


TEST_CASE("splits on a plain char separator", "[split_view]") {
    auto tokens = collect(split_view("a,b,c", ','));
    CHECK(tokens == std::vector<std::string_view>{"a", "b", "c"});
}

TEST_CASE("splits on a multi-character string_view separator", "[split_view]") {
    auto tokens = collect(split_view("one::two::three", "::"));
    CHECK(tokens == std::vector<std::string_view>{"one", "two", "three"});
}

TEST_CASE("split with an empty separator returns input string as one token", "[split_view]") {
    auto tokens = collect(split_view("one::two::three", ""));
    CHECK(tokens == std::vector<std::string_view>{"one::two::three"});
}

TEST_CASE("a single-character string literal separator still deduces string_view, not char",
          "[split_view]") {
    // "-" is a `const char*` / string literal, distinct from '-' (a char).
    // This exercises the CTAD deduction guide rather than the char overload.
    auto tokens = collect(split_view("a-b-c", "-"));
    CHECK(tokens == std::vector<std::string_view>{"a", "b", "c"});
}

TEST_CASE("trailing separator produces a trailing empty token", "[split_view]") {
    auto tokens = collect(split_view("a,b,", ','));
    CHECK(tokens == std::vector<std::string_view>{"a", "b", ""});
}

TEST_CASE("leading separator produces a leading empty token", "[split_view]") {
    auto tokens = collect(split_view(",a,b", ','));
    CHECK(tokens == std::vector<std::string_view>{"", "a", "b"});
}

TEST_CASE("consecutive separators produce empty tokens between them", "[split_view]") {
    auto tokens = collect(split_view("a,,b", ','));
    CHECK(tokens == std::vector<std::string_view>{"a", "", "b"});
}

TEST_CASE("separator not present in input yields a single token", "[split_view]") {
    auto tokens = collect(split_view("hello", ','));
    CHECK(tokens == std::vector<std::string_view>{"hello"});
}

TEST_CASE("empty input yields a single empty token", "[split_view]") {
    auto tokens = collect(split_view(std::string_view{}, ','));
    CHECK(tokens == std::vector<std::string_view>{""});
}

TEST_CASE("input that is only separators yields all-empty tokens", "[split_view]") {
    auto tokens = collect(split_view(",,", ','));
    CHECK(tokens == std::vector<std::string_view>{"", "", ""});
}

TEST_CASE("input equal to the separator itself yields two empty tokens", "[split_view]") {
    auto tokens = collect(split_view("::", "::"));
    CHECK(tokens == std::vector<std::string_view>{"", ""});
}

TEST_CASE("multi-char separator longer than the remaining input is not matched", "[split_view]") {
    // "ab::" split on "::" -- the trailing "::" after "ab" is the only match.
    // Regression guard against off-by-one substring-length bugs.
    auto tokens = collect(split_view("ab::", "::"));
    CHECK(tokens == std::vector<std::string_view>{"ab", ""});
}

TEST_CASE("produced tokens are views into the original buffer, not copies", "[split_view]") {
    std::string_view src = "hello,world";
    auto tokens = collect(split_view(src, ','));
    REQUIRE(tokens.size() == 2);
    CHECK(tokens[0].data() == src.data());
    CHECK(tokens[1].data() == src.data() + 6);
}

TEST_CASE("iterator is a multipass forward iterator (copies advance independently)", "[split_view]") {
    split_view sv("a,b,c", ',');
    auto it1 = sv.begin();
    auto it2 = it1; // copy
    ++it1;          // advance only the the original so it1 and it2 diverge
    CHECK(*it2 == "a"); // it2 unaffected by advancing it1
    CHECK(*it1 == "b");
}

TEST_CASE("works with range-based for", "[split_view]") {
    std::size_t count = 0;
    for (std::string_view piece : split_view("x=1;y=2;z=3", ';')) {
        (void)piece;
        ++count;
    }
    CHECK(count == 3);
}

TEST_CASE("interoperates with classic <algorithm>", "[split_view]") {
    split_view sv("a,b,c,d", ',');
    CHECK(std::distance(sv.begin(), sv.end()) == 4);

    auto it = std::find(sv.begin(), sv.end(), std::string_view("c"));
    REQUIRE(it != sv.end());
    CHECK(*it == "c");

    auto missing = std::find(sv.begin(), sv.end(), std::string_view("z"));
    CHECK(missing == sv.end());
}

TEST_CASE("is a proper C++20 range/view", "[split_view]") {
    STATIC_REQUIRE(std::ranges::forward_range<split_view<char>>);
    STATIC_REQUIRE(std::ranges::forward_range<split_view<std::string_view>>);
    STATIC_REQUIRE(std::ranges::view<split_view<char>>);
}

TEST_CASE("view_interface provides front() and empty() for free", "[split_view]") {
    split_view sv("a,b,c", ',');
    CHECK_FALSE(sv.empty());
    CHECK(sv.front() == "a");
}

TEST_CASE("composes with std::views adaptors", "[split_view]") {
    split_view sv("a,b,c,d,e", ',');
    auto tokens = collect(sv | std::views::take(3));
    CHECK(tokens == std::vector<std::string_view>{"a", "b", "c"});
}

TEST_CASE("dereferencing a temporary iterator is safe", "[split_view]") {
    // Mirrors the shape of view_interface::front()/back(): the range and its iterator are
    // both temporaries, dereferenced and used entirely within one expression. Build this
    // test under -fsanitize=address to have it actually catch a reference-returning
    // operator* -- under plain ASan-less builds this would still pass since UB doesn't
    // always crash.
    auto first_token = [](std::string_view text, char sep) -> std::string_view {
        return *split_view(text, sep).begin();
    };
    CHECK(first_token("a,b,c", ',') == "a");
}

TEST_CASE("a value obtained from operator* is independent of later iterator state",
          "[split_view]") {
    split_view sv("a,b,c", ',');
    auto it = sv.begin();
    auto first = *it;
    ++it;
    CHECK(first == "a"); // must NOT have silently become "b"
    CHECK(*it == "b");
}

// Compile-time-only check: split_view<T> must be rejected for T that is
// neither char nor std::string_view. This should fail to compile
// 
//devprompt::utils::split_view<int> bad("text", 5); // should fail to compile


