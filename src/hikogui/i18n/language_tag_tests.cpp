// Copyright Take Vos 2022.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

#include "language_tag.hpp"
#include <hikotest/hikotest.hpp>

hi_warning_push();
hi_warning_ignore_msvc(4834);

TEST_SUITE(language_tag_suite)
{
    using tag = hi::language_tag;

TEST_CASE(parse_test)
{
    REQUIRE(to_string(tag::parse("")) == "");

    REQUIRE(to_string(tag::parse("nl")) == "nl");
    REQUIRE(to_string(tag::parse("nl-NL")) == "nl-NL");
    REQUIRE(to_string(tag::parse("nl-Cyrl-NL")) == "nl-Cyrl-NL");
    REQUIRE(to_string(tag::parse("nl-BE")) == "nl-BE");
    REQUIRE(to_string(tag::parse("NL-be")) == "nl-BE");
    REQUIRE(to_string(tag::parse("nl-56")) == "nl-BE");
    REQUIRE(to_string(tag::parse("nl-056")) == "nl-BE");
    // "foo" language extension is ignored.
    REQUIRE(to_string(tag::parse("nl-foo-056")) == "nl-BE");
    // "foo" language is accepted even though it doesn't exist and has no default script.
    REQUIRE(to_string(tag::parse("foo-056")) == "foo-BE");
    // "abcde" variant is ignored.
    REQUIRE(to_string(tag::parse("nl-Cyrl-NL-abcde")) == "nl-Cyrl-NL");
    // "a-bcde" extension is ignored.
    REQUIRE(to_string(tag::parse("nl-Cyrl-NL-a-bcde")) == "nl-Cyrl-NL");
    // "x-bcde" user-defined extension is ignored.
    REQUIRE(to_string(tag::parse("nl-Cyrl-NL-x-bcde")) == "nl-Cyrl-NL");

    REQUIRE_THROWS(tag::parse("x-NL"), hi::parse_error);
    REQUIRE_THROWS(tag::parse("xxxx-NL"), hi::parse_error);
    // "Food" script does not exist.
    REQUIRE_THROWS(tag::parse("nl-Food-NL"), hi::parse_error);
    // Region "AA" does not exist.
    REQUIRE_THROWS(tag::parse("nl-Latn-AA"), hi::parse_error);
}

TEST_CASE(construct_test)
{
    REQUIRE(to_string(tag{""}) == "");

    REQUIRE(to_string(tag{"nl"}) == "nl-Latn-NL");
    REQUIRE(to_string(tag{"nl-NL"}) == "nl-Latn-NL");
    REQUIRE(to_string(tag{"nl-Cyrl-NL"}) == "nl-Cyrl-NL");
    REQUIRE(to_string(tag{"nl-BE"}) == "nl-Latn-BE");
    REQUIRE(to_string(tag{"NL-be"}) == "nl-Latn-BE");
    REQUIRE(to_string(tag{"nl-56"}) == "nl-Latn-BE");
    REQUIRE(to_string(tag{"nl-056"}) == "nl-Latn-BE");
    // "foo" language extension is ignored.
    REQUIRE(to_string(tag{"nl-foo-056"}) == "nl-Latn-BE");
    // "foo" language is accepted even though it doesn't exist and has no default script.
    REQUIRE(to_string(tag{"foo-056"}) == "foo-BE");
    // "abcde" variant is ignored.
    REQUIRE(to_string(tag{"nl-Cyrl-NL-abcde"}) == "nl-Cyrl-NL");
    // "a-bcde" extension is ignored.
    REQUIRE(to_string(tag{"nl-Cyrl-NL-a-bcde"}) == "nl-Cyrl-NL");
    // "x-bcde" user-defined extension is ignored.
    REQUIRE(to_string(tag{"nl-Cyrl-NL-x-bcde"}) == "nl-Cyrl-NL");

    REQUIRE_THROWS(tag{"x-NL"}, hi::parse_error);
    REQUIRE_THROWS(tag{"xxxx-NL"}, hi::parse_error);
    // "Food" script does not exist.
    REQUIRE_THROWS(tag{"nl-Food-NL"}, hi::parse_error);
    // Region "AA" does not exist.
    REQUIRE_THROWS(tag{"nl-Latn-AA"}, hi::parse_error);
}

TEST_CASE(shrink_test)
{
    REQUIRE(to_string(tag{"nl"}.shrink()) == "nl");
    REQUIRE(to_string(tag{"nl-NL"}.shrink()) == "nl");
    REQUIRE(to_string(tag{"nl-Latn"}.shrink()) == "nl");
    REQUIRE(to_string(tag{"nl-Latn-NL"}.shrink()) == "nl");
    REQUIRE(to_string(tag{"nl-BE"}.shrink()) == "nl-BE");
    REQUIRE(to_string(tag{"nl-Latn-BE"}.shrink()) == "nl-BE");

    REQUIRE(to_string(tag{"nl-Cyrl"}.shrink()) == "nl-Cyrl");
    REQUIRE(to_string(tag{"nl-Cyrl-NL"}.shrink()) == "nl-Cyrl");
    REQUIRE(to_string(tag{"nl-Cyrl-BE"}.shrink()) == "nl-Cyrl-BE");
}

TEST_CASE(expand_test)
{
    REQUIRE(to_string(tag::parse("nl").expand()) == "nl-Latn-NL");
    REQUIRE(to_string(tag::parse("nl-NL").expand()) == "nl-Latn-NL");
    REQUIRE(to_string(tag::parse("nl-Latn").expand()) == "nl-Latn-NL");
    REQUIRE(to_string(tag::parse("nl-Latn-NL").expand()) == "nl-Latn-NL");
    REQUIRE(to_string(tag::parse("nl-BE").expand()) == "nl-Latn-BE");
    REQUIRE(to_string(tag::parse("nl-Latn-BE").expand()) == "nl-Latn-BE");

    REQUIRE(to_string(tag::parse("nl-Cyrl").expand()) == "nl-Cyrl-NL");
    REQUIRE(to_string(tag::parse("nl-Cyrl-NL").expand()) == "nl-Cyrl-NL");
    REQUIRE(to_string(tag::parse("nl-Cyrl-BE").expand()) == "nl-Cyrl-BE");

    REQUIRE(to_string(tag::parse("en").expand()) == "en-Latn-US");
    REQUIRE(to_string(tag::parse("en-US").expand()) == "en-Latn-US");
    REQUIRE(to_string(tag::parse("en-Latn").expand()) == "en-Latn-US");
    REQUIRE(to_string(tag::parse("en-Latn-US").expand()) == "en-Latn-US");
    REQUIRE(to_string(tag::parse("en-GB").expand()) == "en-Latn-GB");
    REQUIRE(to_string(tag::parse("en-Latn-GB").expand()) == "en-Latn-GB");

    REQUIRE(to_string(tag::parse("en-Cyrl").expand()) == "en-Cyrl-US");
    REQUIRE(to_string(tag::parse("en-Cyrl-GB").expand()) == "en-Cyrl-GB");
}

TEST_CASE(variants_test)
{
    auto nl_Latn_NL_expected =
        std::vector<tag>{tag::parse("nl-Latn-NL"), tag::parse("nl-NL"), tag::parse("nl-Latn"), tag::parse("nl")};
    REQUIRE(std::ranges::to<std::vector>(tag::parse("nl-Latn-NL").variants()) == nl_Latn_NL_expected);

    auto nl_NL_expected = std::vector<tag>{tag::parse("nl-NL"), tag::parse("nl")};
    REQUIRE(std::ranges::to<std::vector>(tag::parse("nl-NL").variants()) == nl_NL_expected);

    auto nl_Latn_expected = std::vector<tag>{tag::parse("nl-Latn"), tag::parse("nl")};
    REQUIRE(std::ranges::to<std::vector>(tag::parse("nl-Latn").variants()) == nl_Latn_expected);

    auto nl_expected = std::vector<tag>{tag::parse("nl")};
    REQUIRE(std::ranges::to<std::vector>(tag::parse("nl").variants()) == nl_expected);
}

TEST_CASE(canonical_variants_test)
{
    auto nl_Latn_NL_expected =
        std::vector<tag>{tag::parse("nl-Latn-NL"), tag::parse("nl-NL"), tag::parse("nl-Latn"), tag::parse("nl")};
    REQUIRE(std::ranges::to<std::vector>(tag::parse("nl-Latn-NL").canonical_variants()) == nl_Latn_NL_expected);

    auto nl_Latn_BE_expected = std::vector<tag>{tag::parse("nl-Latn-BE"), tag::parse("nl-BE")};
    REQUIRE(std::ranges::to<std::vector>(tag::parse("nl-Latn-BE").canonical_variants()) == nl_Latn_BE_expected);
}

TEST_CASE(all_variants_test)
{
    auto nl_Latn_NL_expected =
        std::vector<tag>{tag::parse("nl-Latn-NL"), tag::parse("nl-NL"), tag::parse("nl-Latn"), tag::parse("nl")};
    auto nl_latn_NL = tag::parse("nl-Latn-NL");
    auto nl_latn_NL_result = nl_latn_NL.all_variants();
    REQUIRE(nl_latn_NL_result == nl_Latn_NL_expected);

    auto nl_Latn_BE_expected = std::vector<tag>{
        tag::parse("nl-Latn-BE"),
        tag::parse("nl-BE"),
        tag::parse("nl-Latn"),
        tag::parse("nl"),
        tag::parse("nl-Latn-NL"),
        tag::parse("nl-NL")};
    REQUIRE(tag::parse("nl-Latn-BE").all_variants() == nl_Latn_BE_expected);
}

TEST_CASE(variant_vector_test)
{
    // Language order often used in the Netherlands:
    // - English with the Netherlands' local
    // - Fallback 1: English as spoken in Great-Britain.
    // - Fallback 2: Dutch as spoken in the Netherlands.
    auto test = std::vector<tag>{tag::parse("en-Latn-NL"), tag::parse("en-Latn-GB"), tag::parse("nl-Latn-NL")};

    // In the result we also automatically add a fallback to English as spoken in the United States, after
    // all the originally specified English tags.
    auto expected = std::vector<tag>{
        tag::parse("en-Latn-NL"),
        tag::parse("en-NL"),
        tag::parse("en-Latn-GB"),
        tag::parse("en-GB"),
        tag::parse("en-Latn"),
        tag::parse("en"),
        tag::parse("en-Latn-US"),
        tag::parse("en-US"),
        tag::parse("nl-Latn-NL"),
        tag::parse("nl-NL"),
        tag::parse("nl-Latn"),
        tag::parse("nl")};
    REQUIRE(variants(test) == expected);
}

};

hi_warning_pop();
