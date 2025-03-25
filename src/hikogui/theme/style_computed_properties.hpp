// Copyright Take Vos 2024.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "style_modify_mask.hpp"
#include "../text/text.hpp"
#include "../units/units.hpp"
#include "../color/color.hpp"
#include "../geometry/geometry.hpp"
#include "../macros.hpp"
#include <cstdint>
#include <cassert>

hi_export_module(hikogui.theme : style_computed_properties);

hi_export namespace hi::inline v1 {
struct style_computed_properties {
    unit::length_f width = {};
    unit::length_f height = {};
    unit::pixels_per_em_f font_size = {};
    unit::pixels_f margin_left = {};
    unit::pixels_f margin_bottom = {};
    unit::pixels_f margin_right = {};
    unit::pixels_f margin_top = {};
    unit::pixels_f padding_left = {};
    unit::pixels_f padding_bottom = {};
    unit::pixels_f padding_right = {};
    unit::pixels_f padding_top = {};
    unit::pixels_f border_width = {};
    unit::pixels_f border_bottom_left_radius = {};
    unit::pixels_f border_bottom_right_radius = {};
    unit::pixels_f border_top_left_radius = {};
    unit::pixels_f border_top_right_radius = {};
    hi::horizontal_alignment horizontal_alignment = {};
    hi::vertical_alignment vertical_alignment = {};
    hi::object_fit object_fit = {};
    hi::color color = {};
    hi::color background_color = {};
    hi::color border_color = {};
    hi::color accent_color = {};
    text_style_set text_style = {};
    hi::baseline_priority baseline_priority = {};
    float width_weight = 0.0f;
    float height_weight = 0.0f;

    bool _width_inherit : 1 = false;
    bool _height_inherit : 1 = false;
    bool _font_size_inherit : 1 = false;
    bool _margin_left_inherit : 1 = false;
    bool _margin_bottom_inherit : 1 = false;
    bool _margin_right_inherit : 1 = false;
    bool _margin_top_inherit : 1 = false;
    bool _padding_left_inherit : 1 = false;
    bool _padding_bottom_inherit : 1 = false;
    bool _padding_right_inherit : 1 = false;
    bool _padding_top_inherit : 1 = false;
    bool _border_width_inherit : 1 = false;
    bool _border_bottom_left_radius_inherit : 1 = false;
    bool _border_bottom_right_radius_inherit : 1 = false;
    bool _border_top_left_radius_inherit : 1 = false;
    bool _border_top_right_radius_inherit : 1 = false;
    bool _horizontal_alignment_inherit : 1 = false;
    bool _vertical_alignment_inherit : 1 = false;
    bool _object_fit_inherit : 1 = false;
    bool _color_inherit : 1 = false;
    bool _background_color_inherit : 1 = false;
    bool _border_color_inherit : 1 = false;
    bool _accent_color_inherit : 1 = false;
    bool _text_style_inherit : 1 = false;
    bool _baseline_priority_inherit : 1 = false;
    bool _width_weight_inherit : 1 = false;
    bool _height_weight_inherit : 1 = false;

    void inherit(style_computed_properties const& rhs) noexcept
    {
#define HIX_INHERIT(NAME) \
    if (_##NAME##_inherit) { \
        NAME = rhs.NAME; \
        assert(not rhs._##NAME##_inherit); \
        _##NAME##_inherit = false; \
    }

        HIX_INHERIT(width);
        HIX_INHERIT(height);
        HIX_INHERIT(font_size);
        HIX_INHERIT(margin_left);
        HIX_INHERIT(margin_bottom);
        HIX_INHERIT(margin_right);
        HIX_INHERIT(margin_top);
        HIX_INHERIT(padding_left);
        HIX_INHERIT(padding_bottom);
        HIX_INHERIT(padding_right);
        HIX_INHERIT(padding_top);
        HIX_INHERIT(border_width);
        HIX_INHERIT(border_bottom_left_radius);
        HIX_INHERIT(border_bottom_right_radius);
        HIX_INHERIT(border_top_left_radius);
        HIX_INHERIT(border_top_right_radius);
        HIX_INHERIT(horizontal_alignment);
        HIX_INHERIT(vertical_alignment);
        HIX_INHERIT(object_fit);
        HIX_INHERIT(color);
        HIX_INHERIT(background_color);
        HIX_INHERIT(border_color);
        HIX_INHERIT(accent_color);
        HIX_INHERIT(text_style);
        HIX_INHERIT(baseline_priority);
        HIX_INHERIT(width_weight);
        HIX_INHERIT(height_weight);

#undef HIX_INHERIT
    }

    void set_properties(style_computed_properties const& rhs, style_modify_mask mask = style_modify_mask::all) noexcept
    {
        if (to_bool(mask & style_modify_mask::color)) {
            color = rhs.color;
            background_color = rhs.background_color;
            border_color = rhs.border_color;
            accent_color = rhs.accent_color;
        }

        if (to_bool(mask & style_modify_mask::size)) {
            width = rhs.width;
            height = rhs.height;
            font_size = rhs.font_size;
            text_style = rhs.text_style;
            object_fit = rhs.object_fit;
            width_weight = rhs.width_weight;
            height_weight = rhs.height_weight;
        }

        if (to_bool(mask & style_modify_mask::margin)) {
            margin_left = rhs.margin_left;
            margin_bottom = rhs.margin_bottom;
            margin_right = rhs.margin_right;
            margin_top = rhs.margin_top;
            padding_left = rhs.padding_left;
            padding_bottom = rhs.padding_bottom;
            padding_right = rhs.padding_right;
            padding_top = rhs.padding_top;
        }

        if (to_bool(mask & style_modify_mask::weight)) {
            border_width = rhs.border_width;
            border_bottom_left_radius = rhs.border_bottom_left_radius;
            border_bottom_right_radius = rhs.border_bottom_right_radius;
            border_top_left_radius = rhs.border_top_left_radius;
            border_top_right_radius = rhs.border_top_right_radius;
        }

        if (to_bool(mask & style_modify_mask::alignment)) {
            horizontal_alignment = rhs.horizontal_alignment;
            vertical_alignment = rhs.vertical_alignment;
            baseline_priority = rhs.baseline_priority;
        }
    }

    /** Set all attributes in other as-if they are important.
     * 
     * @param other The attributes used to overwrite the current attributes.
     * @return A mask for what kind of values where changed.
     */
    [[nodiscard]] friend style_modify_mask compare(style_computed_properties const& lhs, style_computed_properties const& rhs) noexcept
    {
#define HIX_COMPARE(NAME, MODIFY_MASK) \
        r |= lhs.NAME != rhs.NAME ? MODIFY_MASK : style_modify_mask{};

        auto r = style_modify_mask{};
        HIX_COMPARE(width, style_modify_mask::size)
        HIX_COMPARE(height, style_modify_mask::size)
        HIX_COMPARE(font_size, style_modify_mask::size)
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
        HIX_COMPARE(color, style_modify_mask::color)
        HIX_COMPARE(background_color, style_modify_mask::color)
        HIX_COMPARE(border_color, style_modify_mask::color)
        HIX_COMPARE(accent_color, style_modify_mask::color)
        HIX_COMPARE(horizontal_alignment, style_modify_mask::alignment)
        HIX_COMPARE(vertical_alignment, style_modify_mask::alignment)
        HIX_COMPARE(object_fit, style_modify_mask::size)
        HIX_COMPARE(text_style, style_modify_mask::size)
        HIX_COMPARE(baseline_priority, style_modify_mask::alignment)
        HIX_COMPARE(width_weight, style_modify_mask::size)
        HIX_COMPARE(height_weight, style_modify_mask::size)
#undef HIX_COMPARE
        return r;
    }
};

}
