// Copyright Take Vos 2023.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

#include "lexer.hpp"
#include <hikotest/hikotest.hpp>

TEST_SUITE(lexer_suite) {

TEST_CASE(integer_literal)
{
    constexpr auto c_lexer = hi::detail::lexer<hi::lexer_config::c_style()>{};

    auto it = c_lexer.parse("42");
    REQUIRE(*it == hi::token(hi::token::integer, "42", 0));
    ++it;
    REQUIRE(it == std::default_sentinel);
}

TEST_CASE(integer_literal_e)
{
    constexpr auto c_lexer = hi::detail::lexer<hi::lexer_config::c_style()>{};

    auto it = c_lexer.parse("42e");
    REQUIRE(*it == hi::token(hi::token::integer, "42", 0));
    ++it;
    // Due to 'e' maybe being exponent the column-nr was advanced already.
    REQUIRE(*it == hi::token(hi::token::id, "e", 3));
    ++it;
    REQUIRE(it == std::default_sentinel);
}

TEST_CASE(integer_literal_em)
{
    constexpr auto c_lexer = hi::detail::lexer<hi::lexer_config::c_style()>{};

    auto it = c_lexer.parse("42em");
    REQUIRE(*it == hi::token(hi::token::integer, "42", 0));
    ++it;
    // Do to 'e' maybe being exponent the column-nr was advanced already.
    REQUIRE(*it == hi::token(hi::token::id, "em", 3));
    ++it;
    REQUIRE(it == std::default_sentinel);
}

TEST_CASE(integer_literal_E_a31)
{
    constexpr auto c_lexer = hi::detail::lexer<hi::lexer_config::c_style()>{};

    auto it = c_lexer.parse("42Eö");
    REQUIRE(*it == hi::token(hi::token::integer, "42", 0));
    ++it;
    // Do to 'e' maybe being exponent the column-nr was advanced already.
    REQUIRE(*it == hi::token(hi::token::id, "Eö", 3));
    ++it;
    REQUIRE(it == std::default_sentinel);
}

TEST_CASE(integer_literal_sigma)
{
    constexpr auto c_lexer = hi::detail::lexer<hi::lexer_config::c_style()>{};

    auto it = c_lexer.parse("42∑");
    REQUIRE(*it == hi::token(hi::token::integer, "42", 0));
    ++it;
    REQUIRE(*it == hi::token(hi::token::other, "∑", 2));
    ++it;
    REQUIRE(it == std::default_sentinel);
}


TEST_CASE(integer_literal_digit_separator)
{
    constexpr auto c_lexer = hi::detail::lexer<hi::lexer_config::c_style()>{};

    auto it = c_lexer.parse("4'2");
    REQUIRE(*it == hi::token(hi::token::integer, "42", 0));
    ++it;
    REQUIRE(it == std::default_sentinel);
}

TEST_CASE(leading_zero_integer_literal)
{
    constexpr auto c_lexer = hi::detail::lexer<hi::lexer_config::c_style()>{};

    auto it = c_lexer.parse("042");
    REQUIRE(*it == hi::token(hi::token::integer, "042", 0));
    ++it;
    REQUIRE(it == std::default_sentinel);
}

TEST_CASE(leading_zero_integer_literal_invalid_digit1)
{
    constexpr auto c_lexer = hi::detail::lexer<hi::lexer_config::c_style()>{};

    auto it = c_lexer.parse("084");
    REQUIRE(*it == hi::token(hi::token::error_invalid_digit, "0", 0));
}

TEST_CASE(leading_zero_integer_literal_invalid_digit2)
{
    constexpr auto c_lexer = hi::detail::lexer<hi::lexer_config::c_style()>{};

    auto it = c_lexer.parse("048");
    REQUIRE(*it == hi::token(hi::token::error_invalid_digit, "04", 0));
}

TEST_CASE(hex_integer_literal1)
{
    constexpr auto c_lexer = hi::detail::lexer<hi::lexer_config::c_style()>{};

    auto it = c_lexer.parse("0x4f");
    REQUIRE(*it == hi::token(hi::token::integer, "0x4f", 0));
    ++it;
    REQUIRE(it == std::default_sentinel);
}

TEST_CASE(hex_integer_literal2)
{
    constexpr auto c_lexer = hi::detail::lexer<hi::lexer_config::c_style()>{};

    auto it = c_lexer.parse("0X4f");
    REQUIRE(*it == hi::token(hi::token::integer, "0X4f", 0));
    ++it;
    REQUIRE(it == std::default_sentinel);
}

TEST_CASE(hex_integer_literal_sigma)
{
    constexpr auto c_lexer = hi::detail::lexer<hi::lexer_config::c_style()>{};

    auto it = c_lexer.parse("0X4f∑");
    REQUIRE(*it == hi::token(hi::token::integer, "0X4f", 0));
    ++it;
    REQUIRE(*it == hi::token(hi::token::other, "∑", 4));
    ++it;
    REQUIRE(it == std::default_sentinel);
}

TEST_CASE(integer_literal_d)
{
    constexpr auto c_lexer = hi::detail::lexer<hi::lexer_config::c_style()>{};

    auto it = c_lexer.parse("0dp");
    REQUIRE(*it == hi::token(hi::token::integer, "0", 0));
    ++it;
    // Due to 'd' maybe being decimal-indicator the column-nr was advanced already.
    REQUIRE(*it == hi::token(hi::token::id, "dp", 2));
    ++it;
    REQUIRE(it == std::default_sentinel);
}

TEST_CASE(dec_integer_literal1)
{
    constexpr auto c_lexer = hi::detail::lexer<hi::lexer_config::c_style()>{};

    auto it = c_lexer.parse("0d42");
    REQUIRE(*it == hi::token(hi::token::integer, "0d42", 0));
    ++it;
    REQUIRE(it == std::default_sentinel);
}

TEST_CASE(dec_integer_literal2)
{
    constexpr auto c_lexer = hi::detail::lexer<hi::lexer_config::c_style()>{};

    auto it = c_lexer.parse("0D42");
    REQUIRE(*it == hi::token(hi::token::integer, "0D42", 0));
    ++it;
    REQUIRE(it == std::default_sentinel);
}

TEST_CASE(oct_integer_literal1)
{
    constexpr auto c_lexer = hi::detail::lexer<hi::lexer_config::c_style()>{};

    auto it = c_lexer.parse("0o42");
    REQUIRE(*it == hi::token(hi::token::integer, "0o42", 0));
    ++it;
    REQUIRE(it == std::default_sentinel);
}

TEST_CASE(oct_integer_literal2)
{
    constexpr auto c_lexer = hi::detail::lexer<hi::lexer_config::c_style()>{};

    auto it = c_lexer.parse("0O42");
    REQUIRE(*it == hi::token(hi::token::integer, "0O42", 0));
    ++it;
    REQUIRE(it == std::default_sentinel);
}

TEST_CASE(oct_integer_literal_invalid_digit)
{
    constexpr auto c_lexer = hi::detail::lexer<hi::lexer_config::c_style()>{};

    auto it = c_lexer.parse("0O82");
    REQUIRE(*it == hi::token(hi::token::error_invalid_digit, "0O", 0));
}

TEST_CASE(bin_integer_literal1)
{
    constexpr auto c_lexer = hi::detail::lexer<hi::lexer_config::c_style()>{};

    auto it = c_lexer.parse("0b0101");
    REQUIRE(*it == hi::token(hi::token::integer, "0b0101", 0));
    ++it;
    REQUIRE(it == std::default_sentinel);
}

TEST_CASE(bin_integer_literal2)
{
    constexpr auto c_lexer = hi::detail::lexer<hi::lexer_config::c_style()>{};

    auto it = c_lexer.parse("0B0101");
    REQUIRE(*it == hi::token(hi::token::integer, "0B0101", 0));
    ++it;
    REQUIRE(it == std::default_sentinel);
}

TEST_CASE(bin_integer_literal_invalid_digit)
{
    constexpr auto c_lexer = hi::detail::lexer<hi::lexer_config::c_style()>{};

    auto it = c_lexer.parse("0B0201");
    REQUIRE(*it == hi::token(hi::token::error_invalid_digit, "0B0", 0));
}

TEST_CASE(float_literal)
{
    constexpr auto c_lexer = hi::detail::lexer<hi::lexer_config::c_style()>{};

    auto it = c_lexer.parse("4.2");
    REQUIRE(*it == hi::token(hi::token::real, "4.2", 0));
    ++it;
    REQUIRE(it == std::default_sentinel);
}

TEST_CASE(float_literal_start_with_zero)
{
    constexpr auto c_lexer = hi::detail::lexer<hi::lexer_config::c_style()>{};

    auto it = c_lexer.parse("0.42");
    REQUIRE(*it == hi::token(hi::token::real, "0.42", 0));
    ++it;
    REQUIRE(it == std::default_sentinel);
}


TEST_CASE(float_literal_only_fractional)
{
    constexpr auto c_lexer = hi::detail::lexer<hi::lexer_config::c_style()>{};

    auto it = c_lexer.parse(".2");
    REQUIRE(*it == hi::token(hi::token::real, ".2", 0));
    ++it;
    REQUIRE(it == std::default_sentinel);
}

TEST_CASE(float_literal_only_integral)
{
    constexpr auto c_lexer = hi::detail::lexer<hi::lexer_config::c_style()>{};

    auto it = c_lexer.parse("4.");
    REQUIRE(*it == hi::token(hi::token::real, "4.", 0));
    ++it;
    REQUIRE(it == std::default_sentinel);
}

TEST_CASE(float_literal_integral_and_exponent)
{
    constexpr auto c_lexer = hi::detail::lexer<hi::lexer_config::c_style()>{};

    auto it = c_lexer.parse("4e2");
    REQUIRE(*it == hi::token(hi::token::real, "4e2", 0));
    ++it;
    REQUIRE(it == std::default_sentinel);
}

TEST_CASE(float_literal_integral_and_positive_exponent)
{
    constexpr auto c_lexer = hi::detail::lexer<hi::lexer_config::c_style()>{};

    auto it = c_lexer.parse("4e+2");
    REQUIRE(*it == hi::token(hi::token::real, "4e+2", 0));
    ++it;
    REQUIRE(it == std::default_sentinel);
}

TEST_CASE(float_literal_integral_and_negative_exponent)
{
    constexpr auto c_lexer = hi::detail::lexer<hi::lexer_config::c_style()>{};

    auto it = c_lexer.parse("4e-2");
    REQUIRE(*it == hi::token(hi::token::real, "4e-2", 0));
    ++it;
    REQUIRE(it == std::default_sentinel);
}

TEST_CASE(float_literal_integral_dot_and_exponent)
{
    constexpr auto c_lexer = hi::detail::lexer<hi::lexer_config::c_style()>{};

    auto it = c_lexer.parse("4.e2");
    REQUIRE(*it == hi::token(hi::token::real, "4.e2", 0));
    ++it;
    REQUIRE(it == std::default_sentinel);
}

TEST_CASE(float_literal_fractional_and_exponent)
{
    constexpr auto c_lexer = hi::detail::lexer<hi::lexer_config::c_style()>{};

    auto it = c_lexer.parse(".4e2");
    REQUIRE(*it == hi::token(hi::token::real, ".4e2", 0));
    ++it;
    REQUIRE(it == std::default_sentinel);
}

TEST_CASE(float_literal_e)
{
    constexpr auto c_lexer = hi::detail::lexer<hi::lexer_config::c_style()>{};

    auto it = c_lexer.parse("4.2e");
    REQUIRE(*it == hi::token(hi::token::real, "4.2", 0));
    ++it;
    // Do to 'e' maybe being exponent the column-nr was advanced already.
    REQUIRE(*it == hi::token(hi::token::id, "e", 4));
    ++it;
    REQUIRE(it == std::default_sentinel);
}

TEST_CASE(float_literal_em)
{
    constexpr auto c_lexer = hi::detail::lexer<hi::lexer_config::c_style()>{};

    auto it = c_lexer.parse("4.2em");
    REQUIRE(*it == hi::token(hi::token::real, "4.2", 0));
    ++it;
    // Do to 'e' maybe being exponent the column-nr was advanced already.
    REQUIRE(*it == hi::token(hi::token::id, "em", 4));
    ++it;
    REQUIRE(it == std::default_sentinel);
}

TEST_CASE(float_literal_E_a31)
{
    constexpr auto c_lexer = hi::detail::lexer<hi::lexer_config::c_style()>{};

    auto it = c_lexer.parse("4.2Eö");
    REQUIRE(*it == hi::token(hi::token::real, "4.2", 0));
    ++it;
    // Do to 'e' maybe being exponent the column-nr was advanced already.
    REQUIRE(*it == hi::token(hi::token::id, "Eö", 4));
    ++it;
    REQUIRE(it == std::default_sentinel);
}

TEST_CASE(float_literal_incomplete_exponent)
{
    constexpr auto c_lexer = hi::detail::lexer<hi::lexer_config::c_style()>{};

    auto it = c_lexer.parse("4e+");
    REQUIRE(*it == hi::token(hi::token::error_incomplete_exponent, "4e+", 0));
}

TEST_CASE(hex_float_literal)
{
    constexpr auto c_lexer = hi::detail::lexer<hi::lexer_config::c_style()>{};

    auto it = c_lexer.parse("0x4.2");
    REQUIRE(*it == hi::token(hi::token::real, "0x4.2", 0));
    ++it;
    REQUIRE(it == std::default_sentinel);
}

TEST_CASE(hex_float_literal_only_fractional)
{
    constexpr auto c_lexer = hi::detail::lexer<hi::lexer_config::c_style()>{};

    auto it = c_lexer.parse("0x.2");
    REQUIRE(*it == hi::token(hi::token::real, "0x.2", 0));
    ++it;
    REQUIRE(it == std::default_sentinel);
}

TEST_CASE(hex_float_literal_only_integral)
{
    constexpr auto c_lexer = hi::detail::lexer<hi::lexer_config::c_style()>{};

    auto it = c_lexer.parse("0x4.");
    REQUIRE(*it == hi::token(hi::token::real, "0x4.", 0));
    ++it;
    REQUIRE(it == std::default_sentinel);
}

TEST_CASE(hex_float_literal_integral_and_exponent)
{
    constexpr auto c_lexer = hi::detail::lexer<hi::lexer_config::c_style()>{};

    auto it = c_lexer.parse("0x4p2");
    REQUIRE(*it == hi::token(hi::token::real, "0x4p2", 0));
    ++it;
    REQUIRE(it == std::default_sentinel);
}

TEST_CASE(hex_float_literal_integral_and_positive_exponent)
{
    constexpr auto c_lexer = hi::detail::lexer<hi::lexer_config::c_style()>{};

    auto it = c_lexer.parse("0x4p+2");
    REQUIRE(*it == hi::token(hi::token::real, "0x4p+2", 0));
    ++it;
    REQUIRE(it == std::default_sentinel);
}

TEST_CASE(hex_float_literal_integral_and_negative_exponent)
{
    constexpr auto c_lexer = hi::detail::lexer<hi::lexer_config::c_style()>{};

    auto it = c_lexer.parse("0x4p-2");
    REQUIRE(*it == hi::token(hi::token::real, "0x4p-2", 0));
    ++it;
    REQUIRE(it == std::default_sentinel);
}

TEST_CASE(hex_float_literal_integral_dot_and_exponent)
{
    constexpr auto c_lexer = hi::detail::lexer<hi::lexer_config::c_style()>{};

    auto it = c_lexer.parse("0x4.p2");
    REQUIRE(*it == hi::token(hi::token::real, "0x4.p2", 0));
    ++it;
    REQUIRE(it == std::default_sentinel);
}

TEST_CASE(hex_float_literal_fractional_and_exponent)
{
    constexpr auto c_lexer = hi::detail::lexer<hi::lexer_config::c_style()>{};

    auto it = c_lexer.parse("0x.4p2");
    REQUIRE(*it == hi::token(hi::token::real, "0x.4p2", 0));
    ++it;
    REQUIRE(it == std::default_sentinel);
}

TEST_CASE(hex_float_literal_incomplete_exponent1)
{
    constexpr auto c_lexer = hi::detail::lexer<hi::lexer_config::c_style()>{};

    auto it = c_lexer.parse("0x4p");
    REQUIRE(*it == hi::token(hi::token::error_incomplete_exponent, "0x4p", 0));
}

TEST_CASE(hex_float_literal_incomplete_exponent2)
{
    constexpr auto c_lexer = hi::detail::lexer<hi::lexer_config::c_style()>{};

    auto it = c_lexer.parse("0x4p+");
    REQUIRE(*it == hi::token(hi::token::error_incomplete_exponent, "0x4p+", 0));
}

TEST_CASE(dqstring_literal)
{
    constexpr auto c_lexer = hi::detail::lexer<hi::lexer_config::c_style()>{};

    auto it = c_lexer.parse("\"foo\"");
    REQUIRE(*it == hi::token(hi::token::dstr, "foo", 0));
    ++it;
    REQUIRE(it == std::default_sentinel);
}

TEST_CASE(sqstring_literal)
{
    constexpr auto c_lexer = hi::detail::lexer<hi::lexer_config::c_style()>{};

    auto it = c_lexer.parse("\'foo\'");
    REQUIRE(*it == hi::token(hi::token::sstr, "foo", 0));
    ++it;
    REQUIRE(it == std::default_sentinel);
}

TEST_CASE(dqstring_literal_empty)
{
    constexpr auto c_lexer = hi::detail::lexer<hi::lexer_config::c_style()>{};

    auto it = c_lexer.parse("\"\"");
    REQUIRE(*it == hi::token(hi::token::dstr, "", 0));
    ++it;
    REQUIRE(it == std::default_sentinel);
}

TEST_CASE(dqstring_literal_escaped_dquote)
{
    constexpr auto c_lexer = hi::detail::lexer<hi::lexer_config::c_style()>{};

    auto it = c_lexer.parse("\"foo\\\"bar\"");
    REQUIRE(*it == hi::token(hi::token::dstr, "foo\\\"bar", 0));
    ++it;
    REQUIRE(it == std::default_sentinel);
}

TEST_CASE(dqstring_literal_unicode)
{
    constexpr auto c_lexer = hi::detail::lexer<hi::lexer_config::c_style()>{};

    auto it = c_lexer.parse("\"föö\"");
    REQUIRE(*it == hi::token(hi::token::dstr, "föö", 0));
    ++it;
    REQUIRE(it == std::default_sentinel);
}

TEST_CASE(dqstring_literal_incomplete)
{
    constexpr auto c_lexer = hi::detail::lexer<hi::lexer_config::c_style()>{};

    auto it = c_lexer.parse("\"foo");
    REQUIRE(*it == hi::token(hi::token::error_incomplete_string, "foo", 0));
}

TEST_CASE(line_comment)
{
    constexpr auto c_lexer = hi::detail::lexer<hi::lexer_config::c_style()>{};

    auto it = c_lexer.parse("//foo\n");
    REQUIRE(*it == hi::token(hi::token::lcomment, "foo", 0));
    ++it;
    REQUIRE(it == std::default_sentinel);
}

TEST_CASE(line_comment_unicode)
{
    constexpr auto c_lexer = hi::detail::lexer<hi::lexer_config::c_style()>{};

    auto it = c_lexer.parse("//föö\n");
    REQUIRE(*it == hi::token(hi::token::lcomment, "föö", 0));
    ++it;
    REQUIRE(it == std::default_sentinel);
}


TEST_CASE(line_comment_eof)
{
    constexpr auto c_lexer = hi::detail::lexer<hi::lexer_config::c_style()>{};

    auto it = c_lexer.parse("//foo");
    REQUIRE(*it == hi::token(hi::token::lcomment, "foo", 0));
    ++it;
    REQUIRE(it == std::default_sentinel);
}

TEST_CASE(line_comment_eof_unicode)
{
    constexpr auto c_lexer = hi::detail::lexer<hi::lexer_config::c_style()>{};

    auto it = c_lexer.parse("//föö");
    REQUIRE(*it == hi::token(hi::token::lcomment, "föö", 0));
    ++it;
    REQUIRE(it == std::default_sentinel);
}

TEST_CASE(block_comment)
{
    constexpr auto c_lexer = hi::detail::lexer<hi::lexer_config::c_style()>{};

    auto it = c_lexer.parse("/*foo*/");
    REQUIRE(*it == hi::token(hi::token::bcomment, "foo", 0));
    ++it;
    REQUIRE(it == std::default_sentinel);
}

TEST_CASE(block_comment_unicode)
{
    constexpr auto c_lexer = hi::detail::lexer<hi::lexer_config::c_style()>{};

    auto it = c_lexer.parse("/*föö*/");
    REQUIRE(*it == hi::token(hi::token::bcomment, "föö", 0));
    ++it;
    REQUIRE(it == std::default_sentinel);
}


TEST_CASE(block_comment_multi_line)
{
    constexpr auto c_lexer = hi::detail::lexer<hi::lexer_config::c_style()>{};

    auto it = c_lexer.parse("/*foo\nbar*/");
    REQUIRE(*it == hi::token(hi::token::bcomment, "foo\nbar", 0));
    ++it;
    REQUIRE(it == std::default_sentinel);
}

TEST_CASE(block_comment_star)
{
    constexpr auto c_lexer = hi::detail::lexer<hi::lexer_config::c_style()>{};

    auto it = c_lexer.parse("/*foo*bar*/");
    REQUIRE(*it == hi::token(hi::token::bcomment, "foo*bar", 0));
    ++it;
    REQUIRE(it == std::default_sentinel);
}

TEST_CASE(block_comment_star_end)
{
    constexpr auto c_lexer = hi::detail::lexer<hi::lexer_config::c_style()>{};

    auto it = c_lexer.parse("/*foo**/");
    REQUIRE(*it == hi::token(hi::token::bcomment, "foo*", 0));
    ++it;
    REQUIRE(it == std::default_sentinel);
}

TEST_CASE(block_comment_incomplete)
{
    constexpr auto c_lexer = hi::detail::lexer<hi::lexer_config::c_style()>{};

    auto it = c_lexer.parse("/*foo");
    REQUIRE(*it == hi::token(hi::token::error_incomplete_comment, "foo", 0));
}

TEST_CASE(identifier)
{
    constexpr auto c_lexer = hi::detail::lexer<hi::lexer_config::c_style()>{};

    auto it = c_lexer.parse("foo");
    REQUIRE(*it == hi::token(hi::token::id, "foo", 0));
    ++it;
    REQUIRE(it == std::default_sentinel);
}

TEST_CASE(identifier_with_number)
{
    constexpr auto c_lexer = hi::detail::lexer<hi::lexer_config::c_style()>{};

    auto it = c_lexer.parse("f42");
    REQUIRE(*it == hi::token(hi::token::id, "f42", 0));
    ++it;
    REQUIRE(it == std::default_sentinel);
}

TEST_CASE(identifier_with_a31)
{
    constexpr auto c_lexer = hi::detail::lexer<hi::lexer_config::c_style()>{};

    auto it = c_lexer.parse("föö");
    REQUIRE(*it == hi::token(hi::token::id, "föö", 0));
    ++it;
    REQUIRE(it == std::default_sentinel);
}

TEST_CASE(identifier_start_a31)
{
    constexpr auto c_lexer = hi::detail::lexer<hi::lexer_config::c_style()>{};

    auto it = c_lexer.parse("ööf");
    REQUIRE(*it == hi::token(hi::token::id, "ööf", 0));
    ++it;
    REQUIRE(it == std::default_sentinel);
}

TEST_CASE(other_slash)
{
    constexpr auto c_lexer = hi::detail::lexer<hi::lexer_config::c_style()>{};

    auto it = c_lexer.parse("/");
    REQUIRE(*it == hi::token(hi::token::other, "/", 0));
    ++it;
    REQUIRE(it == std::default_sentinel);
}

TEST_CASE(other_pattern_syntax)
{
    constexpr auto c_lexer = hi::detail::lexer<hi::lexer_config::c_style()>{};

    auto it = c_lexer.parse("∑");
    REQUIRE(*it == hi::token(hi::token::other, "∑", 0));
    ++it;
    REQUIRE(it == std::default_sentinel);
}

TEST_CASE(ini_assignment_ini_string)
{
    constexpr auto c_lexer = hi::detail::lexer<hi::lexer_config::ini_style()>{};

    auto it = c_lexer.parse("foo = bar");
    REQUIRE(*it == hi::token(hi::token::id, "foo", 0));
    ++it;
    REQUIRE(*it == hi::token(hi::token::other, "=", 4));
    ++it;
    REQUIRE(*it == hi::token(hi::token::istr, "bar", 6));
    ++it;
    REQUIRE(it == std::default_sentinel);
}

TEST_CASE(multiple_tokens)
{
    constexpr auto c_lexer = hi::detail::lexer<hi::lexer_config::c_style()>{};

    auto it = c_lexer.parse("1234, 1.23, \"hello\", foo;");
    REQUIRE(*it == hi::token(hi::token::integer, "1234", 0));
    ++it;
    REQUIRE(*it == hi::token(hi::token::other, ",", 4));
    ++it;
    REQUIRE(*it == hi::token(hi::token::real, "1.23", 6));
    ++it;
    REQUIRE(*it == hi::token(hi::token::other, ",", 10));
    ++it;
    REQUIRE(*it == hi::token(hi::token::dstr, "hello", 12));
    ++it;
    REQUIRE(*it == hi::token(hi::token::other, ",", 19));
    ++it;
    REQUIRE(*it == hi::token(hi::token::id, "foo", 21));
    ++it;
    REQUIRE(*it == hi::token(hi::token::other, ";", 24));
    ++it;
    REQUIRE(it == std::default_sentinel);
}

TEST_CASE(operator_eq)
{
    constexpr auto c_lexer = hi::detail::lexer<hi::lexer_config::c_style()>{};

    auto it = c_lexer.parse("foo = bar");
    REQUIRE(*it == hi::token(hi::token::id, "foo", 0));
    ++it;
    REQUIRE(*it == hi::token(hi::token::other, "=", 4));
    ++it;
    REQUIRE(*it == hi::token(hi::token::id, "bar", 6));
    ++it;
    REQUIRE(it == std::default_sentinel);
}

TEST_CASE(operator_eq_eq)
{
    constexpr auto c_lexer = hi::detail::lexer<hi::lexer_config::c_style()>{};

    auto it = c_lexer.parse("foo == bar");
    REQUIRE(*it == hi::token(hi::token::id, "foo", 0));
    ++it;
    REQUIRE(*it == hi::token(hi::token::other, "==", 4));
    ++it;
    REQUIRE(*it == hi::token(hi::token::id, "bar", 7));
    ++it;
    REQUIRE(it == std::default_sentinel);
}

};
