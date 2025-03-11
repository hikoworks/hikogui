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
class constraints {
public:
    constexpr constraints() noexcept = default;
    constexpr constraints(constraints const&) noexcept = default;
    constexpr constraints(constraints&&) noexcept = default;
    constexpr constraints& operator=(constraints const&) noexcept = default;
    constexpr constraints& operator=(constraints&&) noexcept = default;

    /** Construct layout constraints.
     *
     * @param size The size of the widget.
     * @param weight The weight used to distribute extra size. When an axis has
     *               a weight of 0.0f, the size of the widget will be the
     *               minimum size.
     * @param margins The margins around the widget.
     * @param baseline The baseline of the widget.
     */
    constexpr constraints(extent2 size, extent2 weight, hi::margins margins, hi::baseline baseline) noexcept :
        _size(size), _weight(weight), _margins(margins), _baseline(baseline)
    {
    }

    /** Construct layout constraints for embedding a single widget.
     *
     * @param child The layout constraints of the child widget.
     * @param padding The padding around the child widget.
     * @param margins The margins around the current widget.
     * @return The layout constraints of the current widget.
     */
    [[nodiscard]] constraints constexpr static embed(
        constraints const& child,
        hi::margins padding,
        hi::margins margins) noexcept
    {
        auto const child_padding = max(padding, child._margins);

        auto r = constraints{};
        r._size = child._size + child_padding.size();
        r._weight = child._weight;
        r._margins = margins;
        r._baseline = hi::baseline::embed(child._baseline, child_padding);
        return r;
    }

    /** Construct layout constraints for embedding a single widget.
     *
     * @param child The layout constraints of the child widget.
     * @param padding The padding around the child widget.
     * @param margins The margins around the current widget.
     * @param baseline_priority The priority of the baseline. When embedding a
     *                          widget the priority should probably be
     *                          increased.
     * @return The layout constraints of the current widget.
     */
    [[nodiscard]] constraints constexpr static embed(
        constraints const& child,
        hi::margins padding,
        hi::margins margins,
        hi::baseline_priority baseline_priority) noexcept
    {
        auto r = embed(child, padding, margins);
        r._baseline.set_priority(baseline_priority);
        return r;
    }

private:
    extent2 _size = {};
    extent2 _weight = {};
    hi::margins _margins = {};
    hi::baseline _baseline = {};
};

} // namespace hi::v1
