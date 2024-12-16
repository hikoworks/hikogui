// Copyright Take Vos 2020-2022.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

#include "unicode_bidi.hpp"
#include "../file/file.hpp"
#include "../path/path.hpp"
#include "../utility/utility.hpp"
#include "../algorithm/algorithm.hpp"
#include <hikotest/hikotest.hpp>
#include <iostream>
#include <string>
#include <string_view>
#include <span>
#include <format>
#include <ranges>
#include <generator>

TEST_SUITE(unicode_bidi) {

struct unicode_bidi_test {
    std::vector<int> levels;
    std::vector<int> reorder;
    int line_nr;

    std::vector<hi::unicode_bidi_class> input;
    bool test_for_LTR = false;
    bool test_for_RTL = false;
    bool test_for_auto = false;

    [[nodiscard]] unicode_bidi_test(std::vector<int> const& levels, std::vector<int> const& reorder, int line_nr) noexcept :
        levels(levels), reorder(reorder), line_nr(line_nr)
    {
    }

    [[nodiscard]] std::vector<hi::unicode_bidi_class> get_paragraph_directions() const noexcept
    {
        auto r = std::vector<hi::unicode_bidi_class>{};

        if (test_for_LTR) {
            r.push_back(hi::unicode_bidi_class::L);
        }
        if (test_for_RTL) {
            r.push_back(hi::unicode_bidi_class::R);
        }
        if (test_for_auto) {
            r.push_back(hi::unicode_bidi_class::B);
        }

        return r;
    }
};

[[nodiscard]] static std::vector<int> parse_bidi_test_levels(std::string_view line) noexcept
{
    auto r = std::vector<int>{};
    for (auto const value : hi::split_string(hi::strip(line), " ")) {
        if (value == "x") {
            r.push_back(-1);
        } else {
            r.push_back(*hi::from_string<int>(value));
        }
    }
    return r;
}

[[nodiscard]] static std::vector<int> parse_bidi_test_reorder(std::string_view line) noexcept
{
    auto r = std::vector<int>{};
    for (auto const value : hi::split_string(hi::strip(line), " ")) {
        if (value.empty()) {
            ;
        } else if (value == "x") {
            r.push_back(-1);
        } else {
            r.push_back(*hi::from_string<int>(value));
        }
    }
    return r;
}

[[nodiscard]] static unicode_bidi_test parse_bidi_test_data_line(
    std::string_view line,
    std::vector<int> const& levels,
    std::vector<int> const& reorder,
    int level_nr) noexcept
{
    auto r = unicode_bidi_test{levels, reorder, level_nr};

    auto line_s = hi::split_string(line, ";");

    for (auto bidi_class_str : hi::split_string(hi::strip(line_s[0]), " ")) {
        r.input.push_back(hi::unicode_bidi_class_from_string(bidi_class_str));
    }

    auto bitset = hi::from_string<int>(hi::strip(line_s[1]), 16);
    r.test_for_auto = (*bitset & 1) != 0;
    r.test_for_LTR = (*bitset & 2) != 0;
    r.test_for_RTL = (*bitset & 4) != 0;

    return r;
}

std::generator<unicode_bidi_test> parse_bidi_test(int test_line_nr = -1)
{
    auto const view = hi::file_view(hi::library_test_data_dir() / "BidiTest.txt");
    auto const test_data = as_string_view(view);

    auto levels = std::vector<int>{};
    auto reorder = std::vector<int>{};

    int line_nr = 1;
    for (auto const line_view : std::views::split(test_data, std::string_view{"\n"})) {
        auto const line = hi::strip(std::string_view{line_view.begin(), line_view.end()});
        if (line.empty() || line.starts_with("#")) {
            // Comment and empty lines.
        } else if (line.starts_with("@Levels:")) {
            levels = parse_bidi_test_levels(line.substr(8));
        } else if (line.starts_with("@Reorder:")) {
            reorder = parse_bidi_test_reorder(line.substr(9));
        } else {
            assert(reorder.size() <= levels.size());
            auto data = parse_bidi_test_data_line(line, levels, reorder, line_nr);
            if (test_line_nr == -1 || line_nr == test_line_nr) {
                co_yield data;
            }
        }

        if (line_nr == test_line_nr) {
            break;
        }

        line_nr++;
    }
}

TEST_CASE(bidi_test)
{
    auto bidi = hi::unicode_bidi{};

    for (auto const test : parse_bidi_test()) {
        //if (test.line_nr < 21875) {
        //    continue;
        //}

        for (auto paragraph_direction : test.get_paragraph_directions()) {
            // This test will:
            // - Pretend that the input is a single paragraph.
            // - Pretend that the input is a single line.
            // - Pretend that the input contains no paired brackets.

            auto const lines = std::vector<size_t>{test.input.size()};

            bidi.set_classes(test.input, paragraph_direction);
            bidi.set_lines(lines);

            auto const levels = bidi.embedding_levels();
            auto const ordering = bidi.ordering();

            // We are using the index from the iterator to find embedded levels
            // in input-order. We ignore all elements that where removed by X9.
            REQUIRE(levels.size() == test.levels.size());
            for (auto i = 0; i != levels.size(); ++i) {
                if (test.levels[i] != -1) {
                    REQUIRE(levels[i] == test.levels[i]);
                }
            }

            REQUIRE(ordering.size() >= test.reorder.size());
            auto t_i = size_t{0};
            for (auto r_i = size_t{0}; r_i != ordering.size(); ++r_i) {
                auto const index = ordering[r_i];
                if (test.levels[index] != -1) {
                    REQUIRE(index == test.reorder[t_i++]);
                }
            }
        }

#ifndef NDEBUG
        // The full test with debugging takes 51 seconds.
        if (test.line_nr > 10'000) {
            break;
        }
#endif
    }
}

struct unicode_bidi_character_test {
    int line_nr;
    std::vector<char32_t> characters;
    hi::unicode_bidi_class paragraph_direction;
    int8_t resolved_paragraph_embedding_level;
    std::vector<int8_t> resolved_levels;
    std::vector<size_t> resolved_order;
};

[[nodiscard]] static unicode_bidi_character_test parse_bidi_character_test_line(std::string_view line, int line_nr)
{
    auto const split_line = hi::split_string(line, ";");
    auto const hex_characters = hi::split_string(split_line[0], " ");
    auto const paragraph_direction = hi::from_string<int>(split_line[1]);
    auto const resolved_paragraph_embedding_level = hi::from_string<int8_t>(split_line[2]);
    auto const int_resolved_levels = hi::split_string(split_line[3], " ");
    auto const int_resolved_order = hi::split_string(split_line[4], " ");

    auto r = unicode_bidi_character_test{};
    r.line_nr = line_nr;
    std::transform(begin(hex_characters), end(hex_characters), std::back_inserter(r.characters), [](auto const& x) {
        return hi::char_cast<char32_t>(*hi::from_string<uint32_t>(x, 16));
    });

    r.paragraph_direction = paragraph_direction == 0 ? hi::unicode_bidi_class::L :
        paragraph_direction == 1                     ? hi::unicode_bidi_class::R :
                                                       hi::unicode_bidi_class::B;

    assert(resolved_paragraph_embedding_level.has_value());
    r.resolved_paragraph_embedding_level = *resolved_paragraph_embedding_level;

    std::transform(
        begin(int_resolved_levels), end(int_resolved_levels), std::back_inserter(r.resolved_levels), [](auto const& x) -> int8_t {
            if (x == "x") {
                return -1;
            } else {
                return *hi::from_string<int8_t>(x);
            }
        });

    std::transform(begin(int_resolved_order), end(int_resolved_order), std::back_inserter(r.resolved_order), [](auto const& x) {
        return *hi::from_string<size_t>(x);
    });

    assert(r.characters.size() == r.resolved_levels.size());
    assert(r.resolved_order.size() <= r.resolved_levels.size());
    return r;
}

std::generator<unicode_bidi_character_test> parse_bidi_character_test(int test_line_nr = -1)
{
    auto const view = hi::file_view(hi::library_test_data_dir() / "BidiCharacterTest.txt");
    auto const test_data = as_string_view(view);

    int line_nr = 1;
    for (auto const line_view : std::views::split(test_data, std::string_view{"\n"})) {
        auto const line = hi::strip(std::string_view{line_view.begin(), line_view.end()});
        if (line.empty() || line.starts_with("#")) {
            // Comment and empty lines.
        } else {
            auto data = parse_bidi_character_test_line(line, line_nr);
            if (test_line_nr == -1 || line_nr == test_line_nr) {
                co_yield data;
            }
        }

        if (line_nr == test_line_nr) {
            break;
        }

        line_nr++;
    }
}

TEST_CASE(bidi_character_test)
{
    auto bidi = hi::unicode_bidi{};

    for (auto test : parse_bidi_character_test()) {
        //if (test.line_nr < 254) {
        //    continue;
        //}

        auto line_sizes = std::vector<size_t>{test.characters.size()};

        bidi.set_text(test.characters, test.paragraph_direction);
        bidi.set_lines(line_sizes);

        auto const paragraph_embedding_levels = bidi.paragraph_embedding_levels();
        auto const levels = bidi.embedding_levels();
        auto const ordering = bidi.ordering();

        REQUIRE(paragraph_embedding_levels.size() == 1);
        REQUIRE(paragraph_embedding_levels[0] == test.resolved_paragraph_embedding_level);

        REQUIRE(levels.size() == test.characters.size());
        for (auto i = 0; i != levels.size(); ++i) {
            if (test.resolved_levels[i] != -1) {
                REQUIRE(levels[i] == test.resolved_levels[i]);
            }
        }

        REQUIRE(ordering.size() >= test.resolved_order.size());
        auto t_i = size_t{0};
        for (auto r_i = size_t{0}; r_i != ordering.size(); ++r_i) {
            auto const index = ordering[r_i];
            if (test.resolved_levels[index] != -1) {
                REQUIRE(index == test.resolved_order[t_i++]);
            }
        }
    }
}

}; // TEST_SUITE(unicode_bidi)
