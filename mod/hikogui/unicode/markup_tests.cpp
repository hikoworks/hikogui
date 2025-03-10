// Copyright Take Vos 2023.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

#include "markup.hpp"
#include "../macros.hpp"
#include <gtest/gtest.h>


// case 'r': return phrasing::regular;
// case 'e': return phrasing::emphesis;
// case 's': return phrasing::strong;
// case 'c': return phrasing::code;
// case 'a': return phrasing::abbreviation;
// case 'q': return phrasing::quote;
// case 'k': return phrasing::keyboard;
// case 'h': return phrasing::highlight;
// case 'm': return phrasing::math;
// case 'x': return phrasing::example;
// case 'u': return phrasing::unarticulated;
// case 't': return phrasing::title;
// case 'S': return phrasing::success;
// case 'W': return phrasing::warning;
// case 'E': return phrasing::error;

TEST(markup, phrasing_regular)
{
    auto tmp = hi::apply_markup("a[r]b[.]c");
    ASSERT_EQ(tmp, "abc");
    ASSERT_EQ(tmp[0].phrasing(), hi::phrasing::regular);
    ASSERT_EQ(tmp[1].phrasing(), hi::phrasing::regular);
    ASSERT_EQ(tmp[2].phrasing(), hi::phrasing::regular);
}

TEST(markup, phrasing_emphasis)
{
    auto tmp = hi::apply_markup("a[e]b[.]c");
    ASSERT_EQ(tmp, "abc");
    ASSERT_EQ(tmp[0].phrasing(), hi::phrasing::regular);
    ASSERT_EQ(tmp[1].phrasing(), hi::phrasing::emphesis);
    ASSERT_EQ(tmp[2].phrasing(), hi::phrasing::regular);
}

TEST(markup, phrasing_strong)
{
    auto tmp = hi::apply_markup("a[s]b[.]c");
    ASSERT_EQ(tmp, "abc");
    ASSERT_EQ(tmp[0].phrasing(), hi::phrasing::regular);
    ASSERT_EQ(tmp[1].phrasing(), hi::phrasing::strong);
    ASSERT_EQ(tmp[2].phrasing(), hi::phrasing::regular);
}

TEST(markup, phrasing_code)
{
    auto tmp = hi::apply_markup("a[c]b[.]c");
    ASSERT_EQ(tmp, "abc");
    ASSERT_EQ(tmp[0].phrasing(), hi::phrasing::regular);
    ASSERT_EQ(tmp[1].phrasing(), hi::phrasing::code);
    ASSERT_EQ(tmp[2].phrasing(), hi::phrasing::regular);
}

TEST(markup, phrasing_abbreviation)
{
    auto tmp = hi::apply_markup("a[a]b[.]c");
    ASSERT_EQ(tmp, "abc");
    ASSERT_EQ(tmp[0].phrasing(), hi::phrasing::regular);
    ASSERT_EQ(tmp[1].phrasing(), hi::phrasing::abbreviation);
    ASSERT_EQ(tmp[2].phrasing(), hi::phrasing::regular);
}

TEST(markup, phrasing_quote)
{
    auto tmp = hi::apply_markup("a[q]b[.]c");
    ASSERT_EQ(tmp, "abc");
    ASSERT_EQ(tmp[0].phrasing(), hi::phrasing::regular);
    ASSERT_EQ(tmp[1].phrasing(), hi::phrasing::quote);
    ASSERT_EQ(tmp[2].phrasing(), hi::phrasing::regular);
}

TEST(markup, phrasing_keyboard)
{
    auto tmp = hi::apply_markup("a[k]b[.]c");
    ASSERT_EQ(tmp, "abc");
    ASSERT_EQ(tmp[0].phrasing(), hi::phrasing::regular);
    ASSERT_EQ(tmp[1].phrasing(), hi::phrasing::keyboard);
    ASSERT_EQ(tmp[2].phrasing(), hi::phrasing::regular);
}

TEST(markup, phrasing_highlight)
{
    auto tmp = hi::apply_markup("a[h]b[.]c");
    ASSERT_EQ(tmp, "abc");
    ASSERT_EQ(tmp[0].phrasing(), hi::phrasing::regular);
    ASSERT_EQ(tmp[1].phrasing(), hi::phrasing::highlight);
    ASSERT_EQ(tmp[2].phrasing(), hi::phrasing::regular);
}

TEST(markup, phrasing_math)
{
    auto tmp = hi::apply_markup("a[m]b[.]c");
    ASSERT_EQ(tmp, "abc");
    ASSERT_EQ(tmp[0].phrasing(), hi::phrasing::regular);
    ASSERT_EQ(tmp[1].phrasing(), hi::phrasing::math);
    ASSERT_EQ(tmp[2].phrasing(), hi::phrasing::regular);
}

TEST(markup, phrasing_example)
{
    auto tmp = hi::apply_markup("a[x]b[.]c");
    ASSERT_EQ(tmp, "abc");
    ASSERT_EQ(tmp[0].phrasing(), hi::phrasing::regular);
    ASSERT_EQ(tmp[1].phrasing(), hi::phrasing::example);
    ASSERT_EQ(tmp[2].phrasing(), hi::phrasing::regular);
}

TEST(markup, phrasing_unarticulated)
{
    auto tmp = hi::apply_markup("a[u]b[.]c");
    ASSERT_EQ(tmp, "abc");
    ASSERT_EQ(tmp[0].phrasing(), hi::phrasing::regular);
    ASSERT_EQ(tmp[1].phrasing(), hi::phrasing::unarticulated);
    ASSERT_EQ(tmp[2].phrasing(), hi::phrasing::regular);
}

TEST(markup, phrasing_title)
{
    auto tmp = hi::apply_markup("a[t]b[.]c");
    ASSERT_EQ(tmp, "abc");
    ASSERT_EQ(tmp[0].phrasing(), hi::phrasing::regular);
    ASSERT_EQ(tmp[1].phrasing(), hi::phrasing::title);
    ASSERT_EQ(tmp[2].phrasing(), hi::phrasing::regular);
}

TEST(markup, phrasing_success)
{
    auto tmp = hi::apply_markup("a[S]b[.]c");
    ASSERT_EQ(tmp, "abc");
    ASSERT_EQ(tmp[0].phrasing(), hi::phrasing::regular);
    ASSERT_EQ(tmp[1].phrasing(), hi::phrasing::success);
    ASSERT_EQ(tmp[2].phrasing(), hi::phrasing::regular);
}

TEST(markup, phrasing_warning)
{
    auto tmp = hi::apply_markup("a[W]b[.]c");
    ASSERT_EQ(tmp, "abc");
    ASSERT_EQ(tmp[0].phrasing(), hi::phrasing::regular);
    ASSERT_EQ(tmp[1].phrasing(), hi::phrasing::warning);
    ASSERT_EQ(tmp[2].phrasing(), hi::phrasing::regular);
}

TEST(markup, phrasing_error)
{
    auto tmp = hi::apply_markup("a[E]b[.]c");
    ASSERT_EQ(tmp, "abc");
    ASSERT_EQ(tmp[0].phrasing(), hi::phrasing::regular);
    ASSERT_EQ(tmp[1].phrasing(), hi::phrasing::error);
    ASSERT_EQ(tmp[2].phrasing(), hi::phrasing::regular);
}

TEST(markup, phrasing_unknown)
{
    auto tmp = hi::apply_markup("a[Z]b[.]c");
    ASSERT_EQ(tmp, "a[Z]bc");
}

TEST(markup, language_nl)
{
    auto tmp = hi::apply_markup("a[nl]b[.]c");
    ASSERT_EQ(tmp, "abc");
    ASSERT_EQ(to_string(tmp[0].language_tag()), "en-Latn-US");
    ASSERT_EQ(to_string(tmp[1].language_tag()), "nl-Latn-NL");
    ASSERT_EQ(to_string(tmp[2].language_tag()), "en-Latn-US");
}

TEST(markup, language_unknown)
{
    auto tmp = hi::apply_markup("a[no-lang]b[.]c");
    ASSERT_EQ(tmp, "a[no-lang]bc");
}
