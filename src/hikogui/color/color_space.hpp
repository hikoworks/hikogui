// Copyright Take Vos 2021.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

#pragma once

/** @file color/color_space.hpp Functions to create color conversion matrices.
 * @ingroup color
 */

#include "../geometry/geometry.hpp"
#include "../macros.hpp"
#include <compare>

hi_export_module(hikogui.color.color_space);

hi_export namespace hi {
inline namespace v1 {

struct color_primaries {
    float wx;
    float wy;
    float rx;
    float ry;
    float gx;
    float gy;
    float bx;
    float by;
};

/** Create a color space conversion matrix.
 *
 * Coordinates for color primaries and white-point are in the CIE xy chromaticity
 * coordinate system.
 *
 * @ingroup color
 * @param wx x-coord for the white point.
 * @param wy y-coord for the white point.
 * @param rx x-coord for the red primary.
 * @param ry y-coord for the red primary.
 * @param gx x-coord for the green primary.
 * @param gy y-coord for the green primary.
 * @param bx x-coord for the blue primary.
 * @param by y-coord for the blue primary.
 */
[[nodiscard]] constexpr matrix3
color_primaries_to_RGBtoXYZ(float wx, float wy, float rx, float ry, float gx, float gy, float bx, float by) noexcept
{
    auto const w = vector3{wx, wy, 1.0f - wx - wy};
    auto const r = vector3{rx, ry, 1.0f - rx - ry};
    auto const g = vector3{gx, gy, 1.0f - gx - gy};
    auto const b = vector3{bx, by, 1.0f - bx - by};

    // Calculate white point's tristimulus values from coordinates
    auto const W = vector3{1.0f * (w.x() / w.y()), 1.0f, 1.0f * (w.z() / w.y())};

    // C is the chromaticity matrix.
    auto const C = matrix3{r, g, b};

    // solve tristimulus sums.
    auto const S = scale3{~C * W};

    return C * S;
}

[[nodiscard]] constexpr matrix3 color_primaries_to_RGBtoXYZ(color_primaries const &cp) noexcept
{
    return color_primaries_to_RGBtoXYZ(cp.wx, cp.wy, cp.rx, cp.ry, cp.gx, cp.gy, cp.bx, cp.by);
}

}} // namespace hi::inline v1
