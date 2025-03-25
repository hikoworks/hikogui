// Copyright Take Vos 2019, 2021-2022.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

/** @file alignment.hpp types and utilities for alignment.
 * @ingroup geometry
 */

#pragma once

#include "../utility/utility.hpp"
#include "../macros.hpp"
#include <optional>
#include <algorithm>
#include <exception>
#include <compare>

hi_export_module(hikogui.geometry : alignment);

hi_export namespace hi { inline namespace v1 {

/** Vertical alignment.
 * @ingroup geometry
 */
enum class vertical_alignment : uint8_t {
    /** Align to the top.
     */
    top = 1,

    /** Align to the vertical-middle.
     */
    middle = 2,

    /** Align to the bottom.
     */
    bottom = 3
};

/** Create a guideline between two points.
 *
 * The vertical guideline is mostly used to create a baseline; in this
 * case the guideline_width is set to the cap-height of a font.
 *
 *  - aligned-top: The top of the guideline will be flush with the top-padding.
 *  - aligned-bottom: The bottom of the guideline will be flush with the bottom-padding.
 *  - aligned-middle: The middle of the guideline will be in the middle between bottom and top; clamped by the padding.
 *  - aligned-none: nullopt.
 *
 * @ingroup geometry
 * @param alignment The vertical alignment how to place the guideline.
 * @param bottom The y-coordinate of the bottom.
 * @param top The y-coordinate of the top.
 * @param guideline_width The thickness of the guideline
 * @return The y-coordinate of the bottom of the guideline.
 * @retval nullopt No alignment, or guideline does not fit in the space.
 */
[[nodiscard]] constexpr std::optional<float> make_guideline(
    vertical_alignment alignment,
    float bottom,
    float top,
    float guideline_width)
{
    hi_axiom(bottom <= top);
    hi_axiom(guideline_width >= 0.0f);

    auto const guideline_bottom = bottom;
    auto const guideline_top = top - guideline_width;
    auto const guideline_middle = (bottom + top - guideline_width) / 2.0f;

    switch (alignment) {
    case vertical_alignment::top:
        if (guideline_bottom <= top) {
            return guideline_top;
        } else {
            return {};
        }
    case vertical_alignment::bottom:
        if (guideline_top >= bottom) {
            return guideline_top;
        } else {
            return {};
        }
    case vertical_alignment::middle:
        if (guideline_bottom <= guideline_top) {
            return std::clamp(guideline_middle, guideline_bottom, guideline_top);
        } else {
            return {};
        }
    }
    hi_no_default();
}

/** Horizontal alignment.
 * @ingroup geometry
 */
enum class horizontal_alignment : uint8_t {
    /** Align the text naturally based on the writing direction of each paragraph.
     *
     * This will act as flush_left if the paragraph is in left-to-right direction,
     * and as flush_right if the paragraph is in right-to-left direction.
     */
    flush = 1,

    /** Align the text to the left side
     *
     * The text will be flush-left independent of the writing direction.
     */
    left = 2,

    /** Align the text in the center.
     *
     * Since the text is centered, the writing direction is unimportant.
     */
    center = 3,

    /** Stretch the text to be flush to both sides.
     *
     * Since the text is flush on both sides, the writing direction is unimportant.
     */
    justified = 4,

    /** Align the text to the right side
     *
     * The text will be flush-left independent of the writing direction.
     */
    right = 5,
};

/** Create a guideline between two points.
 * @ingroup geometry
 *
 * The horizontal guideline is may used to create alignment for text or numerics. The guideline_width
 * should probably be set to zero.
 *
 *  - aligned-left: The left of the guideline will be flush with the left-padding.
 *  - aligned-right: The right of the guideline will be flush with the right-padding.
 *  - aligned-center: The center of the guideline will be in the center between left and right; clamped by the padding.
 *  - aligned-none: nullopt.
 *
 * @note The padding is a soft-constraint and may be ignored if needed.
 * @param alignment The horizontal alignment where to put the guideline.
 * @param left The x-coordinate of the left.
 * @param right The x-coordinate of the right.
 * @param guideline_width The thickness of the guideline
 * @return The x-coordinate of the left of the guideline.
 * @retval std::nullopt No alignment, or guideline does not fit in the space.
 */
[[nodiscard]] constexpr std::optional<float> make_guideline(
    horizontal_alignment alignment,
    float left,
    float right,
    float guideline_width = 0.0f)
{
    hi_axiom(left <= right);
    hi_axiom(guideline_width >= 0.0f);

    auto const guideline_left = left;
    auto const guideline_right = right - guideline_width;
    auto const guideline_center = (left + right - guideline_width) / 2.0f;

    switch (alignment) {
    case horizontal_alignment::left:
        if (guideline_left <= right) {
            return guideline_left;
        } else {
            return {};
        }
    case horizontal_alignment::right:
        if (guideline_right >= left) {
            return guideline_right;
        } else {
            return {};
        }

    case horizontal_alignment::center:
        if (guideline_left <= guideline_right) {
            return std::clamp(guideline_center, guideline_left, guideline_right);
        } else {
            return {};
        }

    case horizontal_alignment::flush:
        hi_no_default();

    case horizontal_alignment::justified:
        hi_no_default();
    }
    hi_no_default();
}

/** Mirror the horizontal alignment.
 */
[[nodiscard]] constexpr horizontal_alignment mirror(horizontal_alignment const& rhs) noexcept
{
    if (rhs == horizontal_alignment::left) {
        return horizontal_alignment::right;
    } else if (rhs == horizontal_alignment::right) {
        return horizontal_alignment::left;
    } else {
        return rhs;
    }
}

/** Mirror the horizontal alignment.
 */
[[nodiscard]] constexpr horizontal_alignment mirror(horizontal_alignment const& rhs, bool left_to_right) noexcept
{
    if (left_to_right) {
        return rhs;
    } else {
        return mirror(rhs);
    }
}

[[nodiscard]] constexpr horizontal_alignment resolve(horizontal_alignment const& rhs, bool left_to_right) noexcept
{
    if (rhs == horizontal_alignment::flush or rhs == horizontal_alignment::justified) {
        return left_to_right ? horizontal_alignment::left : horizontal_alignment::right;
    } else {
        return rhs;
    }
}

[[nodiscard]] constexpr horizontal_alignment resolve_mirror(horizontal_alignment const& rhs, bool left_to_right) noexcept
{
    return resolve(mirror(rhs, left_to_right), left_to_right);
}

/** Horizontal/Vertical alignment combination.
 * @ingroup geometry
 */
class alignment {
public:
    constexpr alignment() noexcept : _value(0) {}
    constexpr alignment(alignment const&) noexcept = default;
    constexpr alignment(alignment&&) noexcept = default;
    constexpr alignment& operator=(alignment const&) noexcept = default;
    constexpr alignment& operator=(alignment&&) noexcept = default;

    constexpr explicit alignment(uint8_t value) noexcept : _value(value) {}

    constexpr alignment(horizontal_alignment t, vertical_alignment v = vertical_alignment::top) noexcept :
        _value((std::to_underlying(v) << 4) | std::to_underlying(t))
    {
        hi_axiom(std::to_underlying(v) <= 0xf);
        hi_axiom(std::to_underlying(t) <= 0xf);
    }

    constexpr alignment(vertical_alignment v, horizontal_alignment h = horizontal_alignment::left) noexcept :
        _value((std::to_underlying(v) << 4) | std::to_underlying(h))
    {
        hi_axiom(std::to_underlying(v) <= 0xf);
        hi_axiom(std::to_underlying(h) <= 0xf);
    }

    [[nodiscard]] constexpr static alignment top_flush() noexcept
    {
        return {horizontal_alignment::flush, vertical_alignment::top};
    }

    [[nodiscard]] constexpr static alignment top_left() noexcept
    {
        return {horizontal_alignment::left, vertical_alignment::top};
    }

    [[nodiscard]] constexpr static alignment top_center() noexcept
    {
        return {horizontal_alignment::center, vertical_alignment::top};
    }

    [[nodiscard]] constexpr static alignment top_justified() noexcept
    {
        return {horizontal_alignment::justified, vertical_alignment::top};
    }

    [[nodiscard]] constexpr static alignment top_right() noexcept
    {
        return {horizontal_alignment::right, vertical_alignment::top};
    }

    [[nodiscard]] constexpr static alignment middle_flush() noexcept
    {
        return {horizontal_alignment::flush, vertical_alignment::middle};
    }

    [[nodiscard]] constexpr static alignment middle_left() noexcept
    {
        return {horizontal_alignment::left, vertical_alignment::middle};
    }

    [[nodiscard]] constexpr static alignment middle_center() noexcept
    {
        return {horizontal_alignment::center, vertical_alignment::middle};
    }

    [[nodiscard]] constexpr static alignment middle_justified() noexcept
    {
        return {horizontal_alignment::justified, vertical_alignment::middle};
    }

    [[nodiscard]] constexpr static alignment middle_right() noexcept
    {
        return {horizontal_alignment::right, vertical_alignment::middle};
    }

    [[nodiscard]] constexpr static alignment bottom_left() noexcept
    {
        return {horizontal_alignment::left, vertical_alignment::bottom};
    }

    [[nodiscard]] constexpr static alignment bottom_center() noexcept
    {
        return {horizontal_alignment::center, vertical_alignment::bottom};
    }

    [[nodiscard]] constexpr static alignment bottom_right() noexcept
    {
        return {horizontal_alignment::right, vertical_alignment::bottom};
    }

    [[nodiscard]] constexpr horizontal_alignment horizontal() const noexcept
    {
        return static_cast<horizontal_alignment>(_value & 0xf);
    }

    [[nodiscard]] constexpr vertical_alignment vertical() const noexcept
    {
        return static_cast<vertical_alignment>(_value >> 4);
    }

    [[nodiscard]] constexpr friend bool operator==(alignment const& lhs, alignment const& rhs) noexcept = default;

    [[nodiscard]] constexpr friend bool operator==(alignment const& lhs, horizontal_alignment const& rhs) noexcept
    {
        return lhs.horizontal() == rhs;
    }

    [[nodiscard]] constexpr friend bool operator==(horizontal_alignment const& lhs, alignment const& rhs) noexcept
    {
        return lhs == rhs.horizontal();
    }

    [[nodiscard]] constexpr friend bool operator==(alignment const& lhs, vertical_alignment const& rhs) noexcept
    {
        return lhs.vertical() == rhs;
    }

    [[nodiscard]] constexpr friend bool operator==(vertical_alignment const& lhs, alignment const& rhs) noexcept
    {
        return lhs == rhs.vertical();
    }

    [[nodiscard]] constexpr friend alignment mirror(alignment const& rhs) noexcept
    {
        return alignment{mirror(rhs.horizontal()), rhs.vertical()};
    }

    [[nodiscard]] constexpr friend alignment mirror(alignment const& rhs, bool left_to_right) noexcept
    {
        return alignment{mirror(rhs.horizontal(), left_to_right), rhs.vertical()};
    }

    [[nodiscard]] constexpr friend alignment resolve(alignment const& rhs, bool left_to_right) noexcept
    {
        return alignment{resolve(rhs.horizontal(), left_to_right), rhs.vertical()};
    }

    [[nodiscard]] constexpr friend alignment resolve_mirror(alignment const& rhs, bool left_to_right) noexcept
    {
        return alignment{resolve_mirror(rhs.horizontal(), left_to_right), rhs.vertical()};
    }

private:
    /** The combined vertical- and text-alignment
     *
     * [7:5] vertical_alignment
     * [4:0] horizontal_alignment
     */
    uint8_t _value;
};

/** Combine vertical and horizontal alignment.
 *
 * @param lhs A text alignment.
 * @param rhs A vertical alignment.
 * @return A combined vertical and horizontal alignment.
 */
constexpr alignment operator|(horizontal_alignment lhs, vertical_alignment rhs) noexcept
{
    return alignment{lhs, rhs};
}

/** Combine vertical and horizontal alignment.
 *
 * @param lhs A text alignment.
 * @param rhs A vertical alignment.
 * @return A combined vertical and horizontal alignment.
 */
constexpr alignment operator|(vertical_alignment lhs, horizontal_alignment rhs) noexcept
{
    return alignment{lhs, rhs};
}

}} // namespace hi::v1
