// Copyright Take Vos 2022.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

#include "unicode_word_break.hpp"
#include "unicode_sentence_break.hpp"
#include "unicode_line_break.hpp"
#include "unicode_grapheme_cluster_break.hpp"
#include "unicode_description.hpp"
#include "../file/file.hpp"
#include "../path/path.hpp"
#include "../algorithm/algorithm.hpp"
#include "../utility/utility.hpp"
#include "../macros.hpp"
#include <hikotest/hikotest.hpp>
#include <iostream>
#include <string>
#include <span>
#include <format>
#include <ranges>

TEST_SUITE(unicode_break_suite) {

struct test_type {
    std::u32string code_points;
    std::vector<hi::unicode_break_opportunity> expected;
    std::string comment;
    int line_nr;
};

static std::optional<test_type> parse_test_line(std::string_view line, int line_nr)
{
    auto r = test_type{};

    auto const split_line = hi::split_string(line, "\t", "#");
    if (split_line.size() < 2) {
        return {};
    }
    r.comment = std::format("{}: {}", line_nr, split_line[1]);
    r.line_nr = line_nr;

    auto const columns = hi::split_string(split_line[0], " ");
    if (columns.size() < 2) {
        return {};
    }

    for (auto const column : columns) {
        if (column == "") {
            // Empty.
        } else if (column == "\xc3\xb7") {
            r.expected.push_back(hi::unicode_break_opportunity::yes);
        } else if (column == "\xc3\x97") {
            r.expected.push_back(hi::unicode_break_opportunity::no);
        } else {
            auto const code_point = std::stoi(std::string(column), nullptr, 16);
            r.code_points += hi::char_cast<char32_t>(code_point);
        }
    }

    return {std::move(r)};
}

static hi::generator<test_type> parse_tests(std::filesystem::path filename)
{
    auto const view = hi::file_view(filename);
    auto const test_data = as_string_view(view);

    int line_nr = 1;
    for (auto const line_view : std::views::split(test_data, std::string_view{"\n"})) {
        auto const line = std::string_view{line_view.begin(), line_view.end()};
        if (auto const optional_test = parse_test_line(line, line_nr)) {
            co_yield *optional_test;
        }
        line_nr++;
    }
}

TEST_CASE(grapheme_break)
{
    for (auto const& test : parse_tests(hi::library_test_data_dir() / "GraphemeBreakTest.txt")) {
        auto const result = hi::unicode_grapheme_break(test.code_points.begin(), test.code_points.end());

        REQUIRE(test.expected == result, test.comment);
    }
}

TEST_CASE(word_break)
{
    for (auto const& test : parse_tests(hi::library_test_data_dir() / "WordBreakTest.txt")) {
        auto const result =
            hi::unicode_word_break(test.code_points.begin(), test.code_points.end(), [](auto const code_point) -> decltype(auto) {
                return code_point;
            });

        REQUIRE(test.expected == result, test.comment);
    }
}

TEST_CASE(sentence_break)
{
    for (auto const& test : parse_tests(hi::library_test_data_dir() / "SentenceBreakTest.txt")) {
        auto const result =
            hi::unicode_sentence_break(test.code_points.begin(), test.code_points.end(), [](auto const code_point) -> decltype(auto) {
                return code_point;
            });

        REQUIRE(test.expected == result, test.comment);
    }
}

TEST_CASE(line_break)
{
    auto lb = hi::unicode_line_break{};

    for (auto const& test : parse_tests(hi::library_test_data_dir() / "LineBreakTest.txt")) {
        //if (test.line_nr < 29) {
        //    continue;
        //}

        lb.set_text(test.code_points);

        auto result = std::ranges::to<std::vector<hi::unicode_break_opportunity>>(lb.opportunities());

        // The algorithm produces mandatory-break in the result, but LineBreakTest.txt only has break/no-break.
        for (auto& x : result) {
            if (x == hi::unicode_break_opportunity::mandatory) {
                x = hi::unicode_break_opportunity::yes;
            }
        }

        REQUIRE(test.expected == result, test.comment);
    }
}

};
