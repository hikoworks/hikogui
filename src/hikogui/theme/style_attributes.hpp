// Copyright Take Vos 2024.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "style_modify_mask.hpp"
#include "../units/units.hpp"
#include "../color/color.hpp"
#include "../geometry/geometry.hpp"
#include "../macros.hpp"
#include <cstdint>

hi_export_module(hikogui.theme : style_attributes);

hi_export namespace hi {
inline namespace v1 {

class style_attributes {
public:
    constexpr style_attributes() noexcept = default;
    constexpr style_attributes(style_attributes const&) noexcept = default;
    constexpr style_attributes(style_attributes&&) noexcept = default;
    constexpr style_attributes& operator=(style_attributes const&) noexcept = default;
    constexpr style_attributes& operator=(style_attributes&&) noexcept = default;

#define HIX_GETSET(TYPE, NAME, MODIFY_MASK) \
    [[nodiscard]] TYPE NAME() const noexcept \
    { \
        return _##NAME; \
    } \
    style_modify_mask set_##NAME(TYPE NAME, bool important = false) noexcept \
    { \
        auto r = style_modify_mask{}; \
        if (important or not _##NAME##_important) { \
            _##NAME##_important |= static_cast<uint64_t>(important); \
            _##NAME##_valid = 1; \
            r |= _##NAME == NAME ? style_modify_mask::none : MODIFY_MASK; \
            _##NAME = NAME; \
        } \
        return r; \
    }

    HIX_GETSET(hi::unit::length_f, width, style_modify_mask::layout)
    HIX_GETSET(hi::unit::length_f, height, style_modify_mask::layout)
    HIX_GETSET(hi::unit::length_f, margin_left, style_modify_mask::layout)
    HIX_GETSET(hi::unit::length_f, margin_bottom, style_modify_mask::layout)
    HIX_GETSET(hi::unit::length_f, margin_right, style_modify_mask::layout)
    HIX_GETSET(hi::unit::length_f, margin_top, style_modify_mask::layout)
    HIX_GETSET(hi::unit::length_f, padding_left, style_modify_mask::layout)
    HIX_GETSET(hi::unit::length_f, padding_bottom, style_modify_mask::layout)
    HIX_GETSET(hi::unit::length_f, padding_right, style_modify_mask::layout)
    HIX_GETSET(hi::unit::length_f, padding_top, style_modify_mask::layout)
    HIX_GETSET(hi::unit::length_f, border_width, style_modify_mask::layout)
    HIX_GETSET(hi::unit::length_f, border_bottom_left_radius, style_modify_mask::layout)
    HIX_GETSET(hi::unit::length_f, border_bottom_right_radius, style_modify_mask::layout)
    HIX_GETSET(hi::unit::length_f, border_top_left_radius, style_modify_mask::layout)
    HIX_GETSET(hi::unit::length_f, border_top_right_radius, style_modify_mask::layout)
    HIX_GETSET(hi::color, foreground_color, style_modify_mask::color)
    HIX_GETSET(hi::color, background_color, style_modify_mask::color)
    HIX_GETSET(hi::color, border_color, style_modify_mask::color)
    HIX_GETSET(hi::horizontal_alignment, horizontal_alignment, style_modify_mask::layout)
    HIX_GETSET(hi::vertical_alignment, vertical_alignment, style_modify_mask::layout)
#undef HIX_GETSET

    style_modify_mask set_margin(unit::length_f margin, bool important = false) noexcept
    {
        auto r = style_modify_mask{};
        r |= set_margin_left(margin, important);
        r |= set_margin_bottom(margin, important);
        r |= set_margin_right(margin, important);
        r |= set_margin_top(margin, important);
        return r;
    }

    style_modify_mask set_padding(unit::length_f padding, bool important = false) noexcept
    {
        auto r = style_modify_mask{};
        r |= set_padding_left(padding, important);
        r |= set_padding_bottom(padding, important);
        r |= set_padding_right(padding, important);
        r |= set_padding_top(padding, important);
        return r;
    }

    style_modify_mask set_border_radius(unit::length_f border_radius, bool important = false) noexcept
    {
        auto r = style_modify_mask{};
        r |= set_border_bottom_left_radius(border_radius, important);
        r |= set_border_bottom_right_radius(border_radius, important);
        r |= set_border_top_left_radius(border_radius, important);
        r |= set_border_top_right_radius(border_radius, important);
        return r;
    }

    void clear() noexcept
    {
        *this = style_attributes{};
    }

    /** Set all attributes in other as-if they are important.
     *
     * @param other The attributes used to overwrite the current attributes.
     * @return A mask for what kind of values where changed.
     */
    [[nodiscard]] friend style_modify_mask compare(style_attributes const& lhs, style_attributes const& rhs) noexcept
    {
#define HIX_COMPARE(NAME, MODIFY_MASK) \
        r |= lhs._##NAME != rhs._##NAME ? MODIFY_MASK : style_modify_mask{};

        auto r = style_modify_mask{};
        HIX_COMPARE(width, style_modify_mask::size)
        HIX_COMPARE(height, style_modify_mask::size)
        HIX_COMPARE(margin_left, style_modify_mask::margin)
        HIX_COMPARE(margin_bottom, style_modify_mask::margin)
        HIX_COMPARE(margin_right, style_modify_mask::margin)
        HIX_COMPARE(margin_top, style_modify_mask::margin)
        HIX_COMPARE(padding_left, style_modify_mask::margin)
        HIX_COMPARE(padding_bottom, style_modify_mask::margin)
        HIX_COMPARE(padding_right, style_modify_mask::margin)
        HIX_COMPARE(padding_top, style_modify_mask::margin)
        HIX_COMPARE(border_width, style_modify_mask::weight)
        HIX_COMPARE(border_bottom_left_radius, style_modify_mask::weight)
        HIX_COMPARE(border_bottom_right_radius, style_modify_mask::weight)
        HIX_COMPARE(border_top_left_radius, style_modify_mask::weight)
        HIX_COMPARE(border_top_right_radius, style_modify_mask::weight)
        HIX_COMPARE(foreground_color, style_modify_mask::color)
        HIX_COMPARE(background_color, style_modify_mask::color)
        HIX_COMPARE(border_color, style_modify_mask::color)
        HIX_COMPARE(horizontal_alignment, style_modify_mask::alignment)
        HIX_COMPARE(vertical_alignment, style_modify_mask::alignment)
#undef HIX_COMPARE
        return r;
    }

    /** Apply attributes of other on top of the current.
     *
     * @param other The attributes used to overwrite the current attributes.
     * @return A mask for what kind of values where changed.
     */
    style_modify_mask apply(style_attributes const& other) noexcept
    {
#define HIX_APPLY(NAME) \
    if (other._##NAME##_valid) { \
        r |= set_##NAME(other._##NAME, static_cast<bool>(other._##NAME##_important)); \
    }

        auto r = style_modify_mask{};
        HIX_APPLY(width)
        HIX_APPLY(height)
        HIX_APPLY(margin_left)
        HIX_APPLY(margin_bottom)
        HIX_APPLY(margin_right)
        HIX_APPLY(margin_top)
        HIX_APPLY(padding_left)
        HIX_APPLY(padding_bottom)
        HIX_APPLY(padding_right)
        HIX_APPLY(padding_top)
        HIX_APPLY(border_width)
        HIX_APPLY(border_bottom_left_radius)
        HIX_APPLY(border_bottom_right_radius)
        HIX_APPLY(border_top_left_radius)
        HIX_APPLY(border_top_right_radius)
        HIX_APPLY(foreground_color)
        HIX_APPLY(background_color)
        HIX_APPLY(border_color)
        HIX_APPLY(horizontal_alignment)
        HIX_APPLY(vertical_alignment)
#undef HIX_APPLY
        return r;
    }

private:
    hi::unit::length_f _width = unit::points(0.0f);
    hi::unit::length_f _height = unit::points(0.0f);
    hi::unit::length_f _margin_left = unit::points(0.0f);
    hi::unit::length_f _margin_bottom = unit::points(0.0f);
    hi::unit::length_f _margin_right = unit::points(0.0f);
    hi::unit::length_f _margin_top = unit::points(0.0f);
    hi::unit::length_f _padding_left = unit::points(0.0f);
    hi::unit::length_f _padding_bottom = unit::points(0.0f);
    hi::unit::length_f _padding_right = unit::points(0.0f);
    hi::unit::length_f _padding_top = unit::points(0.0f);
    hi::unit::length_f _border_width = unit::points(0.0f);
    hi::unit::length_f _border_bottom_left_radius = unit::points(0.0f);
    hi::unit::length_f _border_bottom_right_radius = unit::points(0.0f);
    hi::unit::length_f _border_top_left_radius = unit::points(0.0f);
    hi::unit::length_f _border_top_right_radius = unit::points(0.0f);
    hi::color _foreground_color = {};
    hi::color _background_color = {};
    hi::color _border_color = {};
    hi::horizontal_alignment _horizontal_alignment = hi::horizontal_alignment::left;
    hi::vertical_alignment _vertical_alignment = hi::vertical_alignment::top;

    uint64_t _width_valid : 1 = 0;
    uint64_t _height_valid : 1 = 0;
    uint64_t _margin_left_valid : 1 = 0;
    uint64_t _margin_bottom_valid : 1 = 0;
    uint64_t _margin_right_valid : 1 = 0;
    uint64_t _margin_top_valid : 1 = 0;
    uint64_t _padding_left_valid : 1 = 0;
    uint64_t _padding_bottom_valid : 1 = 0;
    uint64_t _padding_right_valid : 1 = 0;
    uint64_t _padding_top_valid : 1 = 0;
    uint64_t _border_width_valid : 1 = 0;
    uint64_t _border_bottom_left_radius_valid : 1 = 0;
    uint64_t _border_bottom_right_radius_valid : 1 = 0;
    uint64_t _border_top_left_radius_valid : 1 = 0;
    uint64_t _border_top_right_radius_valid : 1 = 0;
    uint64_t _foreground_color_valid : 1 = 0;
    uint64_t _background_color_valid : 1 = 0;
    uint64_t _border_color_valid : 1 = 0;
    uint64_t _horizontal_alignment_valid : 1 = 0;
    uint64_t _vertical_alignment_valid : 1 = 0;

    uint64_t _width_important : 1 = 0;
    uint64_t _height_important : 1 = 0;
    uint64_t _margin_left_important : 1 = 0;
    uint64_t _margin_bottom_important : 1 = 0;
    uint64_t _margin_right_important : 1 = 0;
    uint64_t _margin_top_important : 1 = 0;
    uint64_t _padding_left_important : 1 = 0;
    uint64_t _padding_bottom_important : 1 = 0;
    uint64_t _padding_right_important : 1 = 0;
    uint64_t _padding_top_important : 1 = 0;
    uint64_t _border_width_important : 1 = 0;
    uint64_t _border_bottom_left_radius_important : 1 = 0;
    uint64_t _border_bottom_right_radius_important : 1 = 0;
    uint64_t _border_top_left_radius_important : 1 = 0;
    uint64_t _border_top_right_radius_important : 1 = 0;
    uint64_t _foreground_color_important : 1 = 0;
    uint64_t _background_color_important : 1 = 0;
    uint64_t _border_color_important : 1 = 0;
    uint64_t _horizontal_alignment_important : 1 = 0;
    uint64_t _vertical_alignment_important : 1 = 0;
};

} // namespace v1
}
