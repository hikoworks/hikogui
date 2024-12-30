// Copyright Take Vos 2022.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "baseline.hpp"
#include "../geometry/geometry.hpp"
#include "../utility/utility.hpp"
#include "../units/units.hpp"
#include "../macros.hpp"
#include <cstdint>
#include <limits>
#include <concepts>

hi_export_module(hikogui.layout : box_constraints);

hi_export namespace hi {
inline namespace v1 {

/** 2D constraints.
 *
 * This type holds multiple possible sizes that an 2D object may be.
 * We need multiple sizes in case there is a non-linear relation between the width and height of an object.
 *
 */
struct box_constraints {
    enum class priority_type : uint8_t { low = 0, medium = 1, high = 2 };

    constexpr static auto low = priority_type::low;
    constexpr static auto medium = priority_type::medium;
    constexpr static auto high = priority_type::high;

    using size_function = std::function<unit::pixels_f(unit::pixels_f)>;

    constexpr static auto null_size_function = [](unit::pixels_f) {
        return unit::pixels(0.0f);
    };

    /** This is the minimum-height for the object.
     *
     * This is the smallest a widget may be drawn, or for text the height of
     * the text without word-wrapping.
     *
     * The layout system will never make the object smaller than this size.
     */
    unit::pixels_f minimum_height = unit::pixels(0.0f);

    /** This is the preferred-height for the object.
     *
     * This is the height that the object would like to be, based on the theme's
     * height for the object, or the height of the text with word-wrapping
     * (based on the theme's width for the text).
     *
     * The layout system will try to make the object this size.
     */
    unit::pixels_f preferred_height = unit::pixels(0.0f);

    /** The minimum width of the object, given a height.
     *
     * Initially the minimum-height of a row of objects is calculated by taking
     * the maximum of all the minimum-heights of all the objects. The width
     * function is then called with the row's minimum-height; this becomes the
     * minimum-width of the object.
     *
     * The minimum width is the smallest width the object may be, given the
     * minimum height. For text this is the width of the text with word-wrapping
     * to fit the minimum height.
     */
    size_function minimum_width = null_size_function;

    /** The preferred width of the object, given a height.
     *
     * Initially the preferred-height of a row of objects is calculated by taking
     * the maximum of all the preferred-heights of all the objects. The width
     * function is then called with the row's preferred-height; this becomes the
     * preferred-width of the object.
     *
     * The preferred width is the width that the object would like to be, based
     * on the theme's width for the object, or the width of the text with
     * word-wrapping (based on the theme's height for the text).
     */
    size_function preferred_width = null_size_function;

    /** The margins around the object.
     *
     * The margins are the space around the object that is not part of the
     * object itself. The margins are used to separate objects from each other.
     *
     * The margins of two neighboring objects overlap, the largest margin is
     * used.
     */
    hi::margins margins = hi::margins{};

    /** The baseline of the object, given a height.
     *
     * The baseline is the vertical position of line of text or a drawing,
     * specifically text/drawing between objects in the same row.
     */
    size_function baseline = null_size_function;

    /** Priority of the baseline.
     *
     * The priority of the baseline is used to determine which baseline to use
     * when there are multiple baselines in a row of objects.
     */
    priority_type baseline_priority = priority_type::low;

    /** A weight to determine how extra height is distributed.
     *
     * If there is extra height beyond the preferred height, the extra height is
     * distributed to rows based on the maximum height-weight of the objects in
     * that row.
     *
     * The height-weight is a positive number, the higher the number the more
     * extra height is distributed to the object. If the height-weight is zero
     * then the object will not receive any extra height.
     */
    float height_weight = 0.0f;

    /** A weight to determine how extra width is distributed.
     *
     * If there is extra width beyond the preferred width, the extra width is
     * distributed to columns based on the maximum width-weight of the objects
     * in that column.
     *
     * The width-weight is a positive number, the higher the number the more
     * extra width is distributed to the object. If the width-weight is zero
     * then the object will not receive any extra width.
     */
    float width_weight = 0.0f;

    box_constraints() noexcept = default;
    box_constraints(box_constraints const&) noexcept = default;
    box_constraints(box_constraints&&) noexcept = default;
    box_constraints& operator=(box_constraints const&) noexcept = default;
    box_constraints& operator=(box_constraints&&) noexcept = default;

    box_constraints(
        unit::pixels_f minimum_height,
        unit::pixels_f preferred_height,
        size_function minimum_width,
        size_function preferred_width,
        hi::margins margins = hi::margins{},
        size_function baseline = null_size_function,
        priority_type baseline_priority = low,
        float height_weight = 0.0f,
        float width_weight = 0.0f) noexcept :
        minimum_height(minimum_height),
        preferred_height(preferred_height),
        minimum_width(std::move(minimum_width)),
        preferred_width(std::move(preferred_width)),
        margins(margins),
        baseline(std::move(baseline)),
        baseline_priority(baseline_priority),
        height_weight(height_weight),
        width_weight(width_weight)
    {
    }

    box_constraints(
        unit::pixels_f width,
        unit::pixels_f height,
        hi::margins margins = hi::margins{},
        unit::pixels_f baseline = unit::pixels(0.0f),
        priority_type baseline_priority = low,
        float height_weight = 0.0f,
        float width_weight = 0.0f) noexcept :
        box_constraints(
            height,
            height,
            [width](unit::pixels_f) {
                return width;
            },
            [width](unit::pixels_f) {
                return width;
            },
            margins,
            [baseline](unit::pixels_f) {
                return baseline;
            },
            baseline_priority,
            height_weight,
            width_weight)
    {
    }

    [[nodiscard]] friend box_constraints
    embed(box_constraints const& lhs, unit::pixels_f left, unit::pixels_f bottom, unit::pixels_f right, unit::pixels_f top)
    {
        auto r = box_constraints{};

        r.minimum_height = lhs.minimum_height + bottom + top;
        r.preferred_height = lhs.preferred_height + bottom + top;
        r.minimum_width = [embed_width = lhs.minimum_width, width = left + right](unit::pixels_f height) {
            return embed_width(height) + width;
        };
        r.preferred_width = [embed_width = lhs.preferred_width, width = left + right](unit::pixels_f height) {
            return embed_width(height) + width;
        };
        r.margins = hi::margins{};
        r.baseline = [embed_baseline = lhs.baseline, bottom](unit::pixels_f height) {
            return embed_baseline(height) + bottom;
        };

        return r;
    }

    
};

} // namespace v1
} // namespace hi::v1
