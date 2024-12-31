

#pragma once

#include "style_properties.hpp"
#include "../parser/parser.hpp"
#include "../container/container.hpp"
#include "../macros.hpp"
#include <iterator>
#include <tuple>

hi_export_module(hikogui.theme : theme_tag);

hi_export namespace hi {
inline namespace v1 {
namespace detail {

template<std::input_iterator It, std::sentinel_for<It> ItEnd>
[[nodiscard]] constexpr expected_optional<std::string, std::string> parse_style_path_id(It& it, ItEnd last)
{
    hi_assert(it != last);

    if (*it != '#') {
        return std::nullopt;
    }

    ++it;
    if (it == last or *it != token::id) {
        return std::unexpected{std::format("{}: Expected a widget-id after '#', got '{}'.", token_location(it, last), *it)};
    }

    auto r = static_cast<std::string>(*it);
    ++it;
    return r;
}

template<std::input_iterator It, std::sentinel_for<It> ItEnd>
[[nodiscard]] constexpr expected_optional<std::string, std::string> parse_style_path_class(It& it, ItEnd last)
{
    hi_assert(it != last);

    if (*it != '.') {
        return std::nullopt;
    }

    ++it;
    if (it == last or *it != token::id) {
        return std::unexpected{std::format("{}: Expected a widget-class after '.', got '{}'.", token_location(it, last), *it)};
    }

    auto r = static_cast<std::string>(*it);
    ++it;
    return r;
}

template<std::input_iterator It, std::sentinel_for<It> ItEnd>
[[nodiscard]] constexpr expected_optional<horizontal_alignment, std::string> parse_style_horizontal_alignment(It& it, ItEnd last)
{
    hi_assert(it != last);

    if (*it != token::id) {
        return std::nullopt;
    }

    if (*it == "flush") {
        ++it;
        return horizontal_alignment::flush;
    } else if (*it == "left") {
        ++it;
        return horizontal_alignment::left;
    } else if (*it == "center") {
        ++it;
        return horizontal_alignment::center;
    } else if (*it == "justified") {
        ++it;
        return horizontal_alignment::justified;
    } else if (*it == "right") {
        ++it;
        return horizontal_alignment::right;
    } else {
        return std::unexpected{std::format("{}: Unknown horizontal alignment {}.", token_location(it, last), static_cast<std::string>(*it))};
    }
}

template<std::input_iterator It, std::sentinel_for<It> ItEnd>
[[nodiscard]] constexpr expected_optional<vertical_alignment, std::string> parse_style_vertical_alignment(It& it, ItEnd last)
{
    hi_assert(it != last);

    if (*it != token::id) {
        return std::nullopt;
    }

    if (*it == "top") {
        ++it;
        return vertical_alignment::top;
    } else if (*it == "middle") {
        ++it;
        return vertical_alignment::middle;
    } else if (*it == "bottom") {
        ++it;
        return vertical_alignment::bottom;
    } else {
        return std::unexpected{std::format("{}: Unknown vertical alignment {}.", token_location(it, last), static_cast<std::string>(*it))};
    }
}

template<std::input_iterator It, std::sentinel_for<It> ItEnd>
[[nodiscard]] constexpr expected_optional<object_fit, std::string> parse_style_object_fit(It& it, ItEnd last)
{
    hi_assert(it != last);

    if (*it != token::id) {
        return std::nullopt;
    }

    if (*it == "none") {
        ++it;
        return object_fit::none;
    } else if (*it == "contain") {
        ++it;
        return object_fit::contain;
    } else if (*it == "cover") {
        ++it;
        return object_fit::cover;
    } else if (*it == "fill") {
        ++it;
        return object_fit::fill;
    } else if (*it == "scale-down") {
        ++it;
        return object_fit::scale_down;
    } else {
        return std::unexpected{std::format("{}: Unknown object-fit {}.", token_location(it, last), static_cast<std::string>(*it))};
    }
}

template<std::input_iterator It, std::sentinel_for<It> ItEnd>
[[nodiscard]] constexpr expected_optional<float, std::string> parse_style_scalar(It& it, ItEnd last)
{
    hi_assert(it != last);

    if (*it != token::integer and *it != token::real) {
        return std::nullopt;
    }

    auto const value = static_cast<float>(*it);
    ++it;

    if (it == last or *it != token::id) {
        // A numeric value without a suffix is the value.
        return value;
    } else if (*it == '%') {
        ++it;
        return value * 0.01f;
    } else {
        // Unknown suffix could be token for another part of the tag.
        return value;
    }
}

template<std::input_iterator It, std::sentinel_for<It> ItEnd>
[[nodiscard]] constexpr expected_optional<unit::length_f, std::string> parse_style_length(It& it, ItEnd last)
{
    hi_assert(it != last);

    if (*it != token::integer and *it != token::real) {
        return std::nullopt;
    }

    auto const value = static_cast<float>(*it);
    ++it;

    if (it == last or *it != token::id) {
        // A numeric value without a suffix is in device independet pixels.
        return unit::dips(value);
    } else if (*it == '%') {
        ++it;
        return value * 0.01f;
    } else if (*it == "px") {
        ++it;
        return unit::pixels(value);
    } else if (*it == "dp" or *it == "dip") {
        ++it;
        return unit::dips(value);
    } else if (*it == "pt") {
        ++it;
        return unit::points(value);
    } else if (*it == "in") {
        ++it;
        return au::inches(value);
    } else if (*it == "cm") {
        ++it;
        return au::centi(au::meters)(value);
    } else {
        // Unknown suffix could be token for another part of the tag.
        return unit::dips(value);
    }
}

template<std::input_iterator It, std::sentinel_for<It> ItEnd>
[[nodiscard]] constexpr expected_optional<color, std::string> parse_style_color(It& it, ItEnd last)
{
    hi_assert(it != last);

    if (*it == token::id and *it == "rgb") {
        ++it;
        if (it == last or *it != '(') {
            return std::unexpected{std::format("{}: Missing '(' after rgb_color.", token_location(it, last))};
        }

        ++it;
        if (it == last or (*it != token::integer and *it != token::real)) {
            return std::unexpected{
                std::format("{}: Expecting a number as first argument to rgb_color.", token_location(it, last))};
        }
        auto const red = static_cast<float>(*it);

        ++it;
        if (it == last or *it != ',') {
            return std::unexpected{
                std::format("{}: Expecting a comma ',' after first argument to rgb_color.", token_location(it, last))};
        }

        ++it;
        if (it == last or (*it != token::integer and *it != token::real)) {
            return std::unexpected{
                std::format("{}: Expecting a number as second argument to rgb_color.", token_location(it, last))};
        }
        auto const green = static_cast<float>(*it);

        ++it;
        if (it == last or *it != ',') {
            return std::unexpected{
                std::format("{}: Expecting a comma ',' after second argument to rgb_color.", token_location(it, last))};
        }

        ++it;
        if (it == last or (*it != token::integer and *it != token::real)) {
            return std::unexpected{
                std::format("{}: Expecting a number as third argument to rgb_color.", token_location(it, last))};
        }
        auto const blue = static_cast<float>(*it);

        ++it;
        if (it == last or *it != ')') {
            return std::unexpected{std::format("{}: Missing ')' after rgb_color arguments.", token_location(it, last))};
        }

        ++it;
        return color{red, green, blue, 1.0f};

    } else if (*it == token::id and *it == "rgba") {
        ++it;
        if (it == last or *it != '(') {
            return std::unexpected{std::format("{}: Missing '(' after rgba_color.", token_location(it, last))};
        }

        ++it;
        if (it == last or (*it != token::integer and *it != token::real)) {
            return std::unexpected{
                std::format("{}: Expecting a number as first argument to rgba_color.", token_location(it, last))};
        }
        auto const red = static_cast<float>(*it);

        ++it;
        if (it == last or *it != ',') {
            return std::unexpected{
                std::format("{}: Expecting a comma ',' after first argument to rgba_color.", token_location(it, last))};
        }

        ++it;
        if (it == last or (*it != token::integer and *it != token::real)) {
            return std::unexpected{
                std::format("{}: Expecting a number as second argument to rgba_color.", token_location(it, last))};
        }
        auto const green = static_cast<float>(*it);

        ++it;
        if (it == last or *it != ',') {
            return std::unexpected{
                std::format("{}: Expecting a comma ',' after second argument to rgba_color.", token_location(it, last))};
        }

        ++it;
        if (it == last or (*it != token::integer and *it != token::real)) {
            return std::unexpected{
                std::format("{}: Expecting a number as third argument to rgba_color.", token_location(it, last))};
        }
        auto const blue = static_cast<float>(*it);

        ++it;
        if (it == last or *it != ',') {
            return std::unexpected{
                std::format("{}: Expecting a comma ',' after third argument to rgba_color.", token_location(it, last))};
        }

        ++it;
        if (it == last or (*it != token::integer and *it != token::real)) {
            return std::unexpected{
                std::format("{}: Expecting a number as forth argument to rgba_color.", token_location(it, last))};
        }
        auto const alpha = static_cast<float>(*it);

        ++it;
        if (it == last or *it != ')') {
            return std::unexpected{std::format("{}: Missing ')' after rgba_color arguments.", token_location(it, last))};
        }

        ++it;
        return color{red, green, blue, alpha};

    } else if (*it == token::id) {
        auto const color_name = static_cast<std::string>(*it);
        ++it;

        if (auto const* color_ptr = color::find(color_name)) {
            return *color_ptr;
        } else {
            return std::unexpected{std::format("{}: Unknown color name '{}'.", token_location(it, last), color_name)};
        }

    } else if (*it == token::sstr or *it == token::dstr) {
        auto const color_name = static_cast<std::string>(*it);
        ++it;

        if (color_name.starts_with("#")) {
            try {
                return color_from_sRGB(color_name);

            } catch (std::exception const &e) {
                return std::unexpected{std::format("{}: Could not parse hex color '{}': {}", token_location(it, last), color_name, e.what())};
            } 

        } else if (auto const* color_ptr = color::find(color_name)) {
            return *color_ptr;
        } else {
            return std::unexpected{std::format("{}: Unknown color name '{}'.", token_location(it, last), color_name)};
        }    

    } else {
        return std::unexpected{std::format("{}: Unknown color value {}.", token_location(it, last), *it)};
    }
}

template<std::input_iterator It, std::sentinel_for<It> ItEnd>
[[nodiscard]] constexpr expected_optional<style_properties, std::string> parse_style_attribute(It& it, ItEnd last)
{
#define HIX_VALUE(VALUE_PARSER, NAME, ATTRIBUTE) \
    if (name == NAME) { \
        if (auto const value = VALUE_PARSER(it, last)) { \
            auto r = style_properties{}; \
            r.set_##ATTRIBUTE(*value, style_priority{style_importance::author, style_specificity::style}); \
            return r; \
        } else if (value.has_error()) { \
            return std::unexpected(value.error()); \
        } else { \
            return std::unexpected(std::format("{}: Unknown value {} for attribute '{}'", token_location(it, last), *it, name)); \
        } \
    } else

    hi_assert(it != last);

    if (it.size() < 3 or it[0] != token::id or it[1] != '=') {
        return std::nullopt;
    }

    auto const name = static_cast<std::string>(it[0]);
    it += 2;

    HIX_VALUE(parse_style_length, "width", width)
    HIX_VALUE(parse_style_length, "height", height)
    HIX_VALUE(parse_style_length, "margin-left", margin_left)
    HIX_VALUE(parse_style_length, "margin-bottom", margin_bottom)
    HIX_VALUE(parse_style_length, "margin-right", margin_right)
    HIX_VALUE(parse_style_length, "margin-top", margin_top)
    HIX_VALUE(parse_style_length, "margin", margin)
    HIX_VALUE(parse_style_length, "padding-left", padding_left)
    HIX_VALUE(parse_style_length, "padding-bottom", padding_bottom)
    HIX_VALUE(parse_style_length, "padding-right", padding_right)
    HIX_VALUE(parse_style_length, "padding-top", padding_top)
    HIX_VALUE(parse_style_length, "padding", padding)
    HIX_VALUE(parse_style_length, "border-width", border_width)
    HIX_VALUE(parse_style_length, "border-bottom-left-radius", border_bottom_left_radius)
    HIX_VALUE(parse_style_length, "border-bottom-right-radius", border_bottom_right_radius)
    HIX_VALUE(parse_style_length, "border-top-left-radius", border_top_left_radius)
    HIX_VALUE(parse_style_length, "border-top-right-radius", border_top_right_radius)
    HIX_VALUE(parse_style_length, "border-radius", border_radius)
    HIX_VALUE(parse_style_color, "color", color)
    HIX_VALUE(parse_style_color, "background-color", background_color)
    HIX_VALUE(parse_style_color, "border-color", border_color)
    HIX_VALUE(parse_style_horizontal_alignment, "horizontal-alignment", horizontal_alignment)
    HIX_VALUE(parse_style_vertical_alignment, "vertical-alignment", vertical_alignment)
    HIX_VALUE(parse_style_object_fit, "object-fit", object_fit)
    HIX_VALUE(parse_style_scalar, "width-weight", width_scalar)
    HIX_VALUE(parse_style_scalar, "height-weight", height_scalar)
    {
        return std::unexpected(std::format("{}: Unknown attribute '{}'.", token_location(it, last), name));
    }

#undef HIX_VALUE
}

} // namespace detail

template<std::input_iterator It, std::sentinel_for<It> ItEnd>
[[nodiscard]] constexpr expected_optional<std::tuple<style_properties, std::string, std::vector<std::string>>, std::string> parse_style(It first, ItEnd last)
{
    constexpr auto config = [] {
        auto r = lexer_config{};
        r.has_double_quote_string_literal = 1;
        r.has_single_quote_string_literal = 1;
        r.filter_white_space = 1;
        r.minus_in_identifier = 1;
        return r;
    }();

    auto lexer_it = lexer<config>.parse(first, last);
    auto token_it = make_lookahead_iterator<4>(lexer_it);

    auto attributes = hi::style_properties{};
    auto id = std::string{};
    auto classes = std::vector<std::string>{};
    while (token_it != std::default_sentinel) {
        if (auto attribute = detail::parse_style_attribute(token_it, std::default_sentinel)) {
            attributes.apply(*attribute);
            continue;

        } else if (attribute.has_error()) {
            return std::unexpected{attribute.error()};
        }

        if (auto new_id = detail::parse_style_path_id(token_it, std::default_sentinel)) {
            if (id.empty()) {
                id = *new_id;
                continue;
            } else {
                return std::unexpected{std::format("{}: Style already has id #{}.", token_location(token_it, std::default_sentinel), id)};
            }

        } else if (new_id.has_error()) {
            return std::unexpected{new_id.error()};
        }

        if (auto new_class = detail::parse_style_path_class(token_it, std::default_sentinel)) {
            classes.push_back(*new_class);
            continue;

        } else if (new_class.has_error()) {
            return std::unexpected{new_class.error()};
        }

        return std::unexpected{std::format("{}: Unexpected token '{}'.", token_location(token_it, std::default_sentinel), *token_it)};
    }

    return std::tuple{attributes, id, classes};
}

[[nodiscard]] constexpr expected_optional<std::tuple<style_properties, std::string, std::vector<std::string>>, std::string> parse_style(std::string_view str)
{
    return parse_style(str.begin(), str.end());
}

} // namespace v1
} // namespace hi::v1
