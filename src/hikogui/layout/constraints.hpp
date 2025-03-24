// Copyright Take Vos 2024.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "baseline_priority.hpp"
#include "../units/units.hpp"
#include "../utility/utility.hpp"
#include "../macros.hpp"

hi_export_module(hikogui.layout : constraints);

hi_export namespace hi::inline v1::layout {

/** Constraints for the layout of a widget.
 *
 */
struct constraints {
    extent2 size = {};
    hi::margins margins = {};
    float baseline_offset = {};
    hi::layout::baseline_priority baseline_priority = baseline_priority_type::none;
    vertical_alignment vertical_alignment = vertical_alignment::middle;
    bool expand_width = false;
    bool expand_height = false;
    
    constexpr constraints() noexcept = default;
    constexpr constraints(constraints const&) noexcept = default;
    constexpr constraints(constraints&&) noexcept = default;
    constexpr constraints& operator=(constraints const&) noexcept = default;
    constexpr constraints& operator=(constraints&&) noexcept = default;

    constexpr constraints(
        extent2 size,
        bool expand_width,
        bool expand_height,
        hi::margins margins,
        float baseline_offset,
        hi::layout::baseline_priority baseline_priority,
        hi::vertical_alignment vertical_alignment) noexcept :
        size(size),
        margins(margins),
        baseline_offset(baseline_offset),
        baseline_priority(baseline_priority),
        vertical_alignment(vertical_alignment),
        expand_width(expand_width),
        expand_height(expand_height)
        {}

    constexpr constraints(
        extent2 size,
        hi::margins margins,
        float baseline_offset,
        hi::layout::baseline_priority baseline_priority,
        hi::vertical_alignment vertical_alignment) noexcept :
        constraints(size, false, false, margins, baseline_offset, baseline_priority, vertical_alignment)
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
            extent2{a.size.width() + b.size.width(), std::max(a.size.height(), b.size.height())},
            a.expand_width | b.expand_width,
            a.expand_height | b.expand_height,
            hi::margins{
                a.margins.left() + b.margins.left(),
                std::max(a.margins.bottom(), b.margins.bottom()),
                a.margins.right() + b.margins.right(),
                std::max(a.margins.top(), b.margins.top()),
            },
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
            extent2{std::max(a.size.width(), b.size.width()), a.size.height() + b.size.height()},
            a.expand_width | b.expand_width,
            a.expand_height | b.expand_height,
            hi::margins{
                std::max(a.margins.left(), b.margins.left()),
                a.margins.bottom() + b.margins.bottom(),
                std::max(a.margins.right(), b.margins.right()),
                a.margins.top() + b.margins.top(),
            },
            unit::pixels_f{},
            baseline_priority_type::none,
            vertical_alignment::middle    
        };
    }

};

} // namespace hi::v1
