// Copyright Take Vos 2023.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "../utility/utility.hpp"
#include "../color/color.hpp"
#include "../macros.hpp"
#include <format>
#include <filesystem>
#include <iterator>

hi_export_module(hikogui.parser.token);

hi_export namespace hi { inline namespace v1 {

hi_export struct token {
    enum class kind_type : uint8_t {
        none,
        error_unexepected_character,
        error_invalid_digit,
        error_incomplete_exponent,
        error_incomplete_string,
        error_incomplete_comment,
        error_after_lt_bang,
        integer,
        real,
        sstr,
        dstr,
        bstr,
        istr,
        color,
        lcomment,
        bcomment,
        ws,
        id,
        other
    };

    // clang-format off
    constexpr static auto kind_type_metadata = enum_metadata{
        kind_type::none, "none",
        kind_type::error_unexepected_character, "error:unexpected character",
        kind_type::error_invalid_digit, "error:invalid digit",
        kind_type::error_incomplete_exponent, "error:incomplete exponent",
        kind_type::error_incomplete_string, "error:incomplete string",
        kind_type::error_incomplete_comment, "error:incomplete comment",
        kind_type::error_after_lt_bang, "error:after_lt_bang",
        kind_type::integer, "integer",
        kind_type::real, "read",
        kind_type::sstr, "single-quote string",
        kind_type::dstr, "double-quote string",
        kind_type::bstr, "back-quote string",
        kind_type::istr, "ini string",
        kind_type::color, "color",
        kind_type::lcomment, "line comment",
        kind_type::bcomment, "block comment",
        kind_type::ws, "ws",
        kind_type::id, "id",
        kind_type::other, "other"
    };
    // clang-format on

    constexpr static auto none = kind_type::none;
    constexpr static auto error_unexepected_character = kind_type::error_unexepected_character;
    constexpr static auto error_invalid_digit = kind_type::error_invalid_digit;
    constexpr static auto error_incomplete_exponent = kind_type::error_incomplete_exponent;
    constexpr static auto error_incomplete_string = kind_type::error_incomplete_string;
    constexpr static auto error_incomplete_comment = kind_type::error_incomplete_comment;
    constexpr static auto error_after_lt_bang = kind_type::error_after_lt_bang;
    constexpr static auto integer = kind_type::integer;
    constexpr static auto real = kind_type::real;
    constexpr static auto sstr = kind_type::sstr;
    constexpr static auto dstr = kind_type::dstr;
    constexpr static auto bstr = kind_type::bstr;
    constexpr static auto istr = kind_type::istr;
    constexpr static auto color = kind_type::color;
    constexpr static auto lcomment = kind_type::lcomment;
    constexpr static auto bcomment = kind_type::bcomment;
    constexpr static auto ws = kind_type::ws;
    constexpr static auto id = kind_type::id;
    constexpr static auto other = kind_type::other;

    std::vector<char> capture = {};
    size_t line_nr = 0;
    size_t column_nr = 0;
    kind_type kind = kind_type::none;

    constexpr token() noexcept = default;
    constexpr token(token const&) noexcept = default;
    constexpr token(token&&) noexcept = default;
    constexpr token& operator=(token const&) noexcept = default;
    constexpr token& operator=(token&&) noexcept = default;

    constexpr token(kind_type kind, std::string_view capture, size_t column_nr) noexcept :
        kind(kind), capture(), line_nr(0), column_nr(column_nr)
    {
        std::copy(capture.begin(), capture.end(), std::back_inserter(this->capture));
    }

    constexpr token(kind_type kind, std::string_view capture, size_t line_nr, size_t column_nr) noexcept :
        kind(kind), capture(), line_nr(line_nr), column_nr(column_nr)
    {
        std::copy(capture.begin(), capture.end(), std::back_inserter(this->capture));
    }

    [[nodiscard]] constexpr friend bool operator==(token const&, token const&) noexcept = default;

    [[nodiscard]] constexpr bool operator==(kind_type rhs) const noexcept
    {
        return kind == rhs;
    }

    [[nodiscard]] constexpr bool operator==(std::string_view rhs) const noexcept
    {
        return static_cast<std::string_view>(*this) == rhs;
    }

    [[nodiscard]] constexpr bool operator==(char rhs) const noexcept
    {
        return kind == kind_type::other and capture.size() == 1 and capture.front() == rhs;
    }

    constexpr operator std::string() const noexcept
    {
        return std::string{capture.data(), capture.size()};
    }

    template<std::integral T>
    constexpr operator T() const
    {
        if (auto value = from_string<T>(static_cast<std::string_view>(*this))) {
            return *value;
        } else {
            throw parse_error(std::format("Can not convert string to integer: {}", static_cast<std::string>(*this)));
        }
    }

    template<std::floating_point T>
    operator T() const
    {
        return from_string<T>(static_cast<std::string_view>(*this));
    }

    operator ::hi::color() const
    {
        hi_axiom(kind == kind_type::color);
        return color_from_sRGB(static_cast<std::string_view>(*this));
    }

private:
    constexpr operator std::string_view() const noexcept
    {
        return std::string_view{capture.data(), capture.size()};
    }
};

/** Create a location string for error messages.
 *
 * @param it An iterator that dereferences to a `hi::token`.
 * @param last The sentinel for @a it.
 * @param path The filename of the file being parsed.
 * @return A string with the location of the token.
 */
hi_export template<std::input_iterator It, std::sentinel_for<It> ItEnd>
[[nodiscard]] constexpr std::string token_location(It& it, ItEnd last, std::string_view path) noexcept
{
    if (it == last) {
        return std::format("{}:eof", path);
    } else {
        return std::format("{}:{}:{}", path, it->line_nr + 1, it->column_nr + 1);
    }
}

hi_export template<std::input_iterator It>
[[nodiscard]] constexpr std::string token_location(It& it, std::string_view path) noexcept
{
    return token_location(it, std::default_sentinel, path);
}

/** Create a location string for error messages.
 *
 * @param it An iterator that dereferences to a `hi::token`.
 * @param last The sentinel for @a it.
 * @return A string with the location of the token.
 */
hi_export template<std::input_iterator It, std::sentinel_for<It> ItEnd>
[[nodiscard]] constexpr std::string token_location(It& it, ItEnd last) noexcept
{
    if (it == last) {
        return std::format("eof");
    } else {
        return std::format("{}:{}", it->line_nr + 1, it->column_nr + 1);
    }
}

hi_export template<std::input_iterator It>
[[nodiscard]] constexpr std::string token_location(It& it) noexcept
{
    return token_location(it, std::default_sentinel);
}

}} // namespace hi::v1

// XXX #617 MSVC bug does not handle partial specialization in modules.
hi_export template<>
struct std::formatter<hi::token, char> : std::formatter<std::string, char> {
    auto format(hi::token const& t, auto& fc) const
    {
        return std::formatter<std::string, char>::format(
            std::format(
                "{} \"{}\" {}:{}", hi::token::kind_type_metadata[t.kind], static_cast<std::string>(t), t.line_nr, t.column_nr),
            fc);
    }
};

hi_export namespace hi { inline namespace v1 {

hi_export inline std::ostream& operator<<(std::ostream& lhs, token const& rhs)
{
    return lhs << std::format("{}", rhs);
}

}} // namespace hi::v1
