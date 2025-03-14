// Copyright Take Vos 2023.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

#include "markup.hpp"
#include <hikotest/hikotest.hpp>

TEST_SUITE(markup_suite) {

// case 'r': return phrasing::regular;
// case 'e': return phrasing::emphasis;
// case 's': return phrasing::strong;
// case 'c': return phrasing::code;
// case 'a': return phrasing::abbreviation;
// case 'q': return phrasing::quote;
// case 'k': return phrasing::keyboard;
// case 'h': return phrasing::highlight;
// case 'm': return phrasing::math;
// case 'x': return phrasing::example;
// case 'u': return phrasing::unarticulated;
// case 'p': return phrasing::placeholder;
// case 't': return phrasing::title;
// case 'S': return phrasing::success;
// case 'W': return phrasing::warning;
// case 'E': return phrasing::error;

TEST_CASE(phrasing_regular)
{
    auto tmp = hi::apply_markup("a[r]b[.]c");
    REQUIRE(tmp == "abc");
    REQUIRE(tmp[0].phrasing() == hi::phrasing::regular);
    REQUIRE(tmp[1].phrasing() == hi::phrasing::regular);
    REQUIRE(tmp[2].phrasing() == hi::phrasing::regular);
}

TEST_CASE(phrasing_emphasis)
{
    auto tmp = hi::apply_markup("a[e]b[.]c");
    REQUIRE(tmp == "abc");
    REQUIRE(tmp[0].phrasing() == hi::phrasing::regular);
    REQUIRE(tmp[1].phrasing() == hi::phrasing::emphasis);
    REQUIRE(tmp[2].phrasing() == hi::phrasing::regular);
}

TEST_CASE(phrasing_strong)
{
    auto tmp = hi::apply_markup("a[s]b[.]c");
    REQUIRE(tmp == "abc");
    REQUIRE(tmp[0].phrasing() == hi::phrasing::regular);
    REQUIRE(tmp[1].phrasing() == hi::phrasing::strong);
    REQUIRE(tmp[2].phrasing() == hi::phrasing::regular);
}

TEST_CASE(phrasing_code)
{
    auto tmp = hi::apply_markup("a[c]b[.]c");
    REQUIRE(tmp == "abc");
    REQUIRE(tmp[0].phrasing() == hi::phrasing::regular);
    REQUIRE(tmp[1].phrasing() == hi::phrasing::code);
    REQUIRE(tmp[2].phrasing() == hi::phrasing::regular);
}

TEST_CASE(phrasing_abbreviation)
{
    auto tmp = hi::apply_markup("a[a]b[.]c");
    REQUIRE(tmp == "abc");
    REQUIRE(tmp[0].phrasing() == hi::phrasing::regular);
    REQUIRE(tmp[1].phrasing() == hi::phrasing::abbreviation);
    REQUIRE(tmp[2].phrasing() == hi::phrasing::regular);
}

TEST_CASE(phrasing_quote)
{
    auto tmp = hi::apply_markup("a[q]b[.]c");
    REQUIRE(tmp == "abc");
    REQUIRE(tmp[0].phrasing() == hi::phrasing::regular);
    REQUIRE(tmp[1].phrasing() == hi::phrasing::quote);
    REQUIRE(tmp[2].phrasing() == hi::phrasing::regular);
}

TEST_CASE(phrasing_keyboard)
{
    auto tmp = hi::apply_markup("a[k]b[.]c");
    REQUIRE(tmp == "abc");
    REQUIRE(tmp[0].phrasing() == hi::phrasing::regular);
    REQUIRE(tmp[1].phrasing() == hi::phrasing::keyboard);
    REQUIRE(tmp[2].phrasing() == hi::phrasing::regular);
}

TEST_CASE(phrasing_highlight)
{
    auto tmp = hi::apply_markup("a[h]b[.]c");
    REQUIRE(tmp == "abc");
    REQUIRE(tmp[0].phrasing() == hi::phrasing::regular);
    REQUIRE(tmp[1].phrasing() == hi::phrasing::highlight);
    REQUIRE(tmp[2].phrasing() == hi::phrasing::regular);
}

TEST_CASE(phrasing_math)
{
    auto tmp = hi::apply_markup("a[m]b[.]c");
    REQUIRE(tmp == "abc");
    REQUIRE(tmp[0].phrasing() == hi::phrasing::regular);
    REQUIRE(tmp[1].phrasing() == hi::phrasing::math);
    REQUIRE(tmp[2].phrasing() == hi::phrasing::regular);
}

TEST_CASE(phrasing_example)
{
    auto tmp = hi::apply_markup("a[x]b[.]c");
    REQUIRE(tmp == "abc");
    REQUIRE(tmp[0].phrasing() == hi::phrasing::regular);
    REQUIRE(tmp[1].phrasing() == hi::phrasing::example);
    REQUIRE(tmp[2].phrasing() == hi::phrasing::regular);
}

TEST_CASE(phrasing_placeholder)
{
    auto tmp = hi::apply_markup("a[p]b[.]c");
    REQUIRE(tmp == "abc");
    REQUIRE(tmp[0].phrasing() == hi::phrasing::regular);
    REQUIRE(tmp[1].phrasing() == hi::phrasing::placeholder);
    REQUIRE(tmp[2].phrasing() == hi::phrasing::regular);
}

TEST_CASE(phrasing_unarticulated)
{
    auto tmp = hi::apply_markup("a[u]b[.]c");
    REQUIRE(tmp == "abc");
    REQUIRE(tmp[0].phrasing() == hi::phrasing::regular);
    REQUIRE(tmp[1].phrasing() == hi::phrasing::unarticulated);
    REQUIRE(tmp[2].phrasing() == hi::phrasing::regular);
}

TEST_CASE(phrasing_title)
{
    auto tmp = hi::apply_markup("a[t]b[.]c");
    REQUIRE(tmp == "abc");
    REQUIRE(tmp[0].phrasing() == hi::phrasing::regular);
    REQUIRE(tmp[1].phrasing() == hi::phrasing::title);
    REQUIRE(tmp[2].phrasing() == hi::phrasing::regular);
}

TEST_CASE(phrasing_success)
{
    auto tmp = hi::apply_markup("a[S]b[.]c");
    REQUIRE(tmp == "abc");
    REQUIRE(tmp[0].phrasing() == hi::phrasing::regular);
    REQUIRE(tmp[1].phrasing() == hi::phrasing::success);
    REQUIRE(tmp[2].phrasing() == hi::phrasing::regular);
}

TEST_CASE(phrasing_warning)
{
    auto tmp = hi::apply_markup("a[W]b[.]c");
    REQUIRE(tmp == "abc");
    REQUIRE(tmp[0].phrasing() == hi::phrasing::regular);
    REQUIRE(tmp[1].phrasing() == hi::phrasing::warning);
    REQUIRE(tmp[2].phrasing() == hi::phrasing::regular);
}

TEST_CASE(phrasing_error)
{
    auto tmp = hi::apply_markup("a[E]b[.]c");
    REQUIRE(tmp == "abc");
    REQUIRE(tmp[0].phrasing() == hi::phrasing::regular);
    REQUIRE(tmp[1].phrasing() == hi::phrasing::error);
    REQUIRE(tmp[2].phrasing() == hi::phrasing::regular);
}

TEST_CASE(phrasing_unknown)
{
    auto tmp = hi::apply_markup("a[Z]b[.]c");
    REQUIRE(tmp == "a[Z]bc");
}

TEST_CASE(language_nl)
{
    auto tmp = hi::apply_markup("a[nl]b[.]c");
    REQUIRE(tmp == "abc");
    REQUIRE(to_string(tmp[0].language_tag()) == "en-Latn-US");
    REQUIRE(to_string(tmp[1].language_tag()) == "nl-Latn-NL");
    REQUIRE(to_string(tmp[2].language_tag()) == "en-Latn-US");
}

TEST_CASE(language_unknown)
{
    auto tmp = hi::apply_markup("a[no-lang]b[.]c");
    REQUIRE(tmp == "a[no-lang]bc");
}

};
