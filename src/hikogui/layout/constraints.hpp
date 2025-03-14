// Copyright Take Vos 2024.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "../units/units.hpp"
#include "../utility/utility.hpp"
#include "../macros.hpp"

hi_export_module(hikogui.layout : constraints);

hi_export namespace hi::inline v1::layout {

/** Constraints for the layout of a widget.
 *
 */
struct constraints {
    unit::pixels_f width = {};
    unit::pixels_f height = {};
    float width_weight = 0.0f;
    float height_weight = 0.0f;
    unit::pixels_f left = {};
    unit::pixels_f right = {};
    unit::pixels_f top = {};
    unit::pixels_f bottom = {};
    unit::pixels_f baseline_offset = {};
    baseline_priority_type baseline_priority = baseline_priority_type::none;
    vertical_alignment vertical_alignment = vertical_alignment::middle;
    
    /**
     * @brief Enumeration representing the priority levels for baselines.
     *
     * The baseline_priority enumeration defines the priority levels for baselines.
     * Each priority level represents a different alignment preference for widgets
     * when negotiating the baseline position.
     */
    enum class baseline_priority_type : unsigned int {
        none = 0, //< No priority.
        label = 1, //< Priority for labels.
        small_widget = 10, //< Priority for small widgets.
        large_widget = 100, //< Priority for large widgets.
    };

    constexpr constraints() noexcept = default;
    constexpr constraints(constraints const&) noexcept = default;
    constexpr constraints(constraints&&) noexcept = default;
    constexpr constraints& operator=(constraints const&) noexcept = default;
    constexpr constraints& operator=(constraints&&) noexcept = default;

    /** Construct layout constraints.
     *
     * @param width The width of the widget.
     * @param height The height of the widget.
     * @param width_weight The weight used to distribute extra width. When the
     *                     width has a weight of 0.0f, the width of the widget
     *                     will be the minimum width.
     * @param height_weight The weight used to distribute extra height. When the
     *                      height has a weight of 0.0f, the height of the widget
     *                      will be the minimum height.
     * @param left The left margin of the widget.
     * @param right The right margin of the widget.
     * @param top The top margin of the widget.
     * @param bottom The bottom margin of the widget.
     * @param baseline_offset The baseline offset of the widget.
     * @param baseline_priority The priority of the baseline.
     * @param vertical_alignment The vertical alignment of the widget.
     */
    constexpr constraints(
        unit::pixels_f width,
        unit::pixels_f height,
        float width_weight,
        float height_weight,
        unit::pixels_f left,
        unit::pixels_f right,
        unit::pixels_f top,
        unit::pixels_f bottom,
        unit::pixels_f baseline_offset,
        baseline_priority_type baseline_priority,
        vertical_alignment vertical_alignment) noexcept :
        width(width),
        height(height),
        width_weight(width_weight),
        height_weight(height_weight),
        left(left),
        right(right),
        top(top),
        bottom(bottom),
        baseline_offset(baseline_offset),
        baseline_priority(baseline_priority),
        vertical_alignment(vertical_alignment)
    {
    }

    constexpr constraints(
        extent2 size,
        margins margins,
        unit::pixels_f baseline_offset,
        baseline_priority_type baseline_priority,
        vertical_alignment vertical_alignment) noexcept :
        constraints(
            unit::pixels(size.width()),
            unit::pixels(size.height()),
            0.0f,
            0.0f,
            unit::pixels(margins.left()),
            unit::pixels(margins.right()),
            unit::pixels(margins.top()),
            unit::pixels(margins.bottom()),
            baseline_offset,
            baseline_priority,
            vertical_alignment)
        {}

    constexpr constraints(
        extent2 size,
        extent2 weight,
        margins margins,
        unit::pixels_f baseline_offset,
        baseline_priority_type baseline_priority) noexcept :
        constraints(
            unit::pixels(size.width()),
            unit::pixels(size.height()),
            weight.width(),
            weight.height(),
            unit::pixels(margins.left()),
            unit::pixels(margins.right()),
            unit::pixels(margins.top()),
            unit::pixels(margins.bottom()),
            baseline_offset,
            baseline_priority,
            vertical_alignment::middle)
        {}

    

    /** Merge the constraints of two widgets in a row.
     *
     * All horizontal constraints are added together. All vertical constraints
     * are maximized. The baseline offset is calculated based on the priority of
     * the baselines.
     *
     * @param a The constraints of the first widget.
     * @param b The constraints of the second widget.
     * @return The constraints of the two widgets in a row combined.
     */
    [[nodiscard]] constexpr friend constraints merge_row(constraints const& a, constraints const& b) noexcept
    {
        auto const new_baseline_offset = [&] {
            if (a.baseline_priority > b.baseline_priority) {
                return a.baseline_offset;
            } else if (a.baseline_priority < b.baseline_priority) {
                return b.baseline_offset;
            } else if (a.vertical_alignment == b.vertical_alignment) {
                switch (a.vertical_alignment) {
                case vertical_alignment::top:
                    return std::min(a.baseline_offset, b.baseline_offset);
                case vertical_alignment::middle:
                    return (a.baseline_offset + b.baseline_offset) * 0.5f;
                case vertical_alignment::bottom:
                    return std::max(a.baseline_offset, b.baseline_offset);
                }
            } else {
                return a.baseline_offset;
            }
        }();

        return constraints{
            a.width + b.width,
            std::max(a.height, b.height),
            a.width_weight + b.width_weight,
            std::max(a.height_weight, b.height_weight),
            a.left + b.left,
            a.right + b.right,
            std::max(a.top, b.top),
            std::max(a.bottom, b.bottom),
            new_baseline_offset,
            max(a.baseline_priority, b.baseline_priority),
            a.vertical_alignment    
        };
    }

    /** Merge the constraints of two widgets in a column.
     * 
     * All vertical constraints are added together. All horizontal constraints
     * are maximized. The baseline is not calculated and should not be used.
     * 
     * @param a The constraints of the first widget.
     * @param b The constraints of the second widget.
     * @return The constraints of the two widgets in a column combined.
     */
    [[nodiscard]] constexpr friend constraints merge_column(constraints const& a, constraints const& b) noexcept
    {
        return constraints{
            std::max(a.width, b.width),
            a.height + b.height,
            std::max(a.width_weight, b.width_weight),
            a.height_weight + b.height_weight,
            std::max(a.left, b.left),
            std::max(a.right, b.right),
            a.top + b.top,
            a.bottom + b.bottom,
            unit::pixels_f{},
            baseline_priority_type::none,
            vertical_alignment::middle    
        };
    }

};

} // namespace hi::v1
