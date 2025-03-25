// Copyright Take Vos 2019-2022.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

#include "unicode_normalization.hpp"
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

TEST_SUITE(unicode_normalization_suite) {

/*! A test defined in NormalizationTests.txt.
 *
 * CONFORMANCE:
 * 1. The following invariants must be true for all conformant implementations
 *
 *    NFC
 *      c2 ==  toNFC(c1) ==  toNFC(c2) ==  toNFC(c3)
 *      c4 ==  toNFC(c4) ==  toNFC(c5)
 *
 *    NFD
 *      c3 ==  toNFD(c1) ==  toNFD(c2) ==  toNFD(c3)
 *      c5 ==  toNFD(c4) ==  toNFD(c5)
 *
 *    NFKC
 *      c4 == toNFKC(c1) == toNFKC(c2) == toNFKC(c3) == toNFKC(c4) == toNFKC(c5)
 *
 *    NFKD
 *      c5 == toNFKD(c1) == toNFKD(c2) == toNFKD(c3) == toNFKD(c4) == toNFKD(c5)
 *
 * 2. For every code point X assigned in this version of Unicode that is not specifically
 *    listed in Part 1, the following invariants must be true for all conformant
 *    implementations:
 *
 *      X == toNFC(X) == toNFD(X) == toNFKC(X) == toNFKD(X)
 */
struct NormalizationTest {
    std::u32string c1;
    std::u32string c2;
    std::u32string c3;
    std::u32string c4;
    std::u32string c5;
    size_t line_nr;
    std::string comment;
};

static std::u32string parseNormalizationTest_column(std::string_view column)
{
    std::u32string r;

    auto codePointStrings = hi::split_string_view(column, " ");
    for (auto const codePointString : codePointStrings) {
        r += hi::char_cast<char32_t>(*hi::from_string<uint32_t>(codePointString, 16));
    }
    return r;
}

static std::optional<NormalizationTest> parseNormalizationTest_line(std::string_view line, size_t line_nr)
{
    auto r = std::optional<NormalizationTest>{};

    auto const split_line = hi::split_string_view(line, "#");
    if (split_line.size() < 2) {
        return r;
    }
    auto const columns = hi::split_string_view(split_line[0], ";");
    if (columns.size() < 6) {
        return r;
    }

    r.emplace(
        parseNormalizationTest_column(columns[0]),
        parseNormalizationTest_column(columns[1]),
        parseNormalizationTest_column(columns[2]),
        parseNormalizationTest_column(columns[3]),
        parseNormalizationTest_column(columns[4]),
        line_nr,
        std::format("{}: {}", line_nr, split_line[1]));

    return r;
}

static hi::generator<NormalizationTest> parseNormalizationTests()
{
    auto const view = hi::file_view(hi::library_test_data_dir() / "NormalizationTest.txt");
    auto const test_data = as_string_view(view);

    size_t line_nr = 0;
    for (auto const line : hi::split_string_view(test_data, "\n")) {
        if (auto const optionalTest = parseNormalizationTest_line(line, ++line_nr)) {
            co_yield *optionalTest;
        }
    }
}

// CONFORMANCE:
// 1. The following invariants must be true for all conformant implementations
//
//    NFC
//      c2 ==  toNFC(c1) ==  toNFC(c2) ==  toNFC(c3)
//      c4 ==  toNFC(c4) ==  toNFC(c5)
//
//    NFD
//      c3 ==  toNFD(c1) ==  toNFD(c2) ==  toNFD(c3)
//      c5 ==  toNFD(c4) ==  toNFD(c5)
//
//    NFKC
//      c4 == toNFKC(c1) == toNFKC(c2) == toNFKC(c3) == toNFKC(c4) == toNFKC(c5)
//
//    NFKD
//      c5 == toNFKD(c1) == toNFKD(c2) == toNFKD(c3) == toNFKD(c4) == toNFKD(c5)
//
// 2. For every code point X assigned in this version of Unicode that is not specifically
//    listed in Part 1, the following invariants must be true for all conformant
//    implementations:
//
//      X == toNFC(X) == toNFD(X) == toNFKC(X) == toNFKD(X)

TEST_CASE(unicode_NFC_colon)
{
    REQUIRE(hi::unicode_normalize(hi::to_u32string("Audio device:")) == hi::to_u32string("Audio device:"));
    REQUIRE(hi::unicode_decompose(hi::to_u32string("Audio device:")) == hi::to_u32string("Audio device:"));
}

TEST_CASE(NFC)
{
    for (auto const& test : parseNormalizationTests()) {
        REQUIRE(hi::unicode_normalize(test.c1) == test.c2, test.comment);
        REQUIRE(hi::unicode_normalize(test.c2) == test.c2, test.comment);
        REQUIRE(hi::unicode_normalize(test.c3) == test.c2, test.comment);
        REQUIRE(hi::unicode_normalize(test.c4) == test.c4, test.comment);
        REQUIRE(hi::unicode_normalize(test.c5) == test.c4, test.comment);
    }
}

TEST_CASE(NFKC)
{
    for (auto const& test : parseNormalizationTests()) {
        REQUIRE(hi::unicode_normalize(test.c1, hi::unicode_normalize_config::NFKC()) == test.c4, test.comment);
        REQUIRE(hi::unicode_normalize(test.c2, hi::unicode_normalize_config::NFKC()) == test.c4, test.comment);
        REQUIRE(hi::unicode_normalize(test.c3, hi::unicode_normalize_config::NFKC()) == test.c4, test.comment);
        REQUIRE(hi::unicode_normalize(test.c4, hi::unicode_normalize_config::NFKC()) == test.c4, test.comment);
        REQUIRE(hi::unicode_normalize(test.c5, hi::unicode_normalize_config::NFKC()) == test.c4, test.comment);
    }
}

TEST_CASE(NFD)
{
    for (auto const& test : parseNormalizationTests()) {
        REQUIRE(hi::unicode_decompose(test.c1) == test.c3, test.comment);
        REQUIRE(hi::unicode_decompose(test.c2) == test.c3, test.comment);
        REQUIRE(hi::unicode_decompose(test.c3) == test.c3, test.comment);
        REQUIRE(hi::unicode_decompose(test.c4) == test.c5, test.comment);
        REQUIRE(hi::unicode_decompose(test.c5) == test.c5, test.comment);
    }
}

TEST_CASE(NFKD)
{
    for (auto const& test : parseNormalizationTests()) {
        REQUIRE(hi::unicode_decompose(test.c1, hi::unicode_normalize_config::NFKD()) == test.c5, test.comment);
        REQUIRE(hi::unicode_decompose(test.c2, hi::unicode_normalize_config::NFKD()) == test.c5, test.comment);
        REQUIRE(hi::unicode_decompose(test.c3, hi::unicode_normalize_config::NFKD()) == test.c5, test.comment);
        REQUIRE(hi::unicode_decompose(test.c4, hi::unicode_normalize_config::NFKD()) == test.c5, test.comment);
        REQUIRE(hi::unicode_decompose(test.c5, hi::unicode_normalize_config::NFKD()) == test.c5, test.comment);
    }
}

#ifdef NDEBUG

TEST_CASE(invariant)
{
    auto previouslyTestedCodePoints = std::vector<bool>(0x11'0000, false);
    for (auto const& test : parseNormalizationTests()) {
        for (auto const& c : test.c1) {
            previouslyTestedCodePoints[c] = true;
        }
        for (auto const& c : test.c2) {
            previouslyTestedCodePoints[c] = true;
        }
        for (auto const& c : test.c3) {
            previouslyTestedCodePoints[c] = true;
        }
        for (auto const& c : test.c4) {
            previouslyTestedCodePoints[c] = true;
        }
        for (auto const& c : test.c5) {
            previouslyTestedCodePoints[c] = true;
        }
    }

    for (char32_t i = 0; i < previouslyTestedCodePoints.size(); i++) {
        if (!previouslyTestedCodePoints[i]) {
            auto const str = std::u32string(1, i);

            REQUIRE(hi::unicode_decompose(str) == str, std::format("NFD code-point: {:x}", static_cast<int>(i)));
            REQUIRE(hi::unicode_normalize(str) == str, std::format("NFC code-point: {:x}", static_cast<int>(i)));
            REQUIRE(hi::unicode_decompose(str, hi::unicode_normalize_config::NFKD()) == str, std::format("NFKD code-point: {:x}", static_cast<int>(i)));
            REQUIRE(hi::unicode_normalize(str, hi::unicode_normalize_config::NFKC()) == str, std::format("NFKC code-point: {:x}", static_cast<int>(i)));
        }
    }
}

#endif

};
