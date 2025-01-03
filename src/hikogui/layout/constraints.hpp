// Copyright Take Vos 2024.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "../units/units.hpp"
#include "../utility/utility.hpp"
#include "../macros.hpp"

hi_export_module(hikogui.layout : constraints);

hi_export namespace hi::inline v1 {

/** Constraints for the layout of a widget. 
 * 
 * The constraints are used to calculate the size of a widget in a layout.
 * 
 * The layout gives priority to the height of a widget first, once the minimum
 * and preferred height are calculated for the layout of the window as a whole,
 * the minimum and preferred widths are calculated as a function of these
 * heights. This is the reason why there are no minimum or preferred widths in
 * this struct.
 */
struct layout_constraints {
    /** The minimum height of the widget.
     * 
     * A widget will never be laid out smaller than this height.
     * This height is used to determine the minimum height of the window.
     */
    unit::pixels_f minimum_height = unit::pixels(0.0f);

    /** The preferred height of the widget.
     * 
     * A widget will be laid out at this height if there is enough space.
     * This height is used to determine the initial height of the window.
     * 
     * The preferred height should be greater or equal to the minimum height.
     */
    unit::pixels_f preferred_height = unit::pixels(0.0f);

    /** The weight for distributing extra width among widgets in the same row.
     *
     * When a widget has a weight of zero, then it will not grow in width when
     * there is extra space available. Except when all widgets in the row have a
     * weight of zero, then the extra space will be distributed evenly among the
     * widgets.
     * 
     * If the layout as a whole has a weight of zero, then the window will not
     * be allowed to be resized beyond the preferred width of the whole layout.
     */
    float weight_width = 0.0f;

    /** The weight for distributing extra height among widgets in the same column.
     * 
     * When a widget has a weight of zero, then it will not grow in height when
     * there is extra space available. Except when all widgets in the column have
     * a weight of zero, then the extra space will be distributed evenly among the
     * widgets.
     * 
     * If the layout as a whole has a weight of zero, then the window will not
     * be allowed to be resized beyond the preferred height of the whole layout.
     */
    float weight_height = 0.0f;

    /** The left margin of the widget.
     * 
     * The margin is the space between the widget and the widget to the left, or
     * the left edge of the window or left edge of a container.
     * 
     * Widgets should return the actual left margin, even when the layout is
     * right-to-left. This means that in a right-to-left layout the left margin
     * is by default taken from the theme's right margin.
     */
    unit::pixels_f margin_left = unit::pixels(0.0f);

    /** The right margin of the widget.
     * 
     * The margin is the space between the widget and the widget to the right, or
     * the right edge of the window or right edge of a container.
     * 
     * Widgets should return the actual right margin, even when the layout is
     * right-to-left. This means that in a right-to-left layout the right margin
     * is by default taken from the theme's left margin.
     */
    unit::pixels_f margin_right = unit::pixels(0.0f);

    /** The bottom margin of the widget.
     * 
     * The margin is the space between the widget and the widget below, or
     * the bottom edge of the window or bottom edge of a container.
     */
    unit::pixels_f margin_below = unit::pixels(0.0f);

    /** The top margin of the widget.
     * 
     * The margin is the space between the widget and the widget above, or
     * the top edge of the window or top edge of a container.
     */
    unit::pixels_f margin_above = unit::pixels(0.0f);

    constexpr layout_constraints() noexcept = default;
    constexpr layout_constraints(layout_constraints const&) noexcept = default;
    constexpr layout_constraints(layout_constraints&&) noexcept = default;
    constexpr layout_constraints& operator=(layout_constraints const&) noexcept = default;
    constexpr layout_constraints& operator=(layout_constraints&&) noexcept = default;

    [[nodiscard]] constexpr bool holds_invariant() const noexcept
    {
        return minimum_height <= preferred_height;
    }

    [[nodiscard]] constexpr friend layout_constraints max(layout_constraints const& lhs, layout_constraints const& rhs) noexcept
    {
        auto r = layout_constraints{};
        r.minimum_height = std::max(lhs.minimum_height, rhs.minimum_height);
        r.preferred_height = std::max(lhs.preferred_height, rhs.preferred_height);
        r.weight_width = std::max(lhs.weight_width, rhs.weight_width);
        r.weight_height = std::max(lhs.weight_height, rhs.weight_height);
        r.margin_left = std::max(lhs.margin_left, rhs.margin_left);
        r.margin_right = std::max(lhs.margin_right, rhs.margin_right);
        r.margin_below = std::max(lhs.margin_below, rhs.margin_below);
        r.margin_above = std::max(lhs.margin_above, rhs.margin_above);

        if (r.minimum_height > r.preferred_height) {
            r.preferred_height = r.minimum_height;
        }
        assert(r.holds_invariant());
        return r;
    }
};

struct layout_width_constraints {
    unit::pixels_f minimum = unit::pixels(0.0f);
    unit::pixels_f preferred = unit::pixels(0.0f);
};

} // namespace hi::v1