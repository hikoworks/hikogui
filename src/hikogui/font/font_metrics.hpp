// Copyright Take Vos 2021.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "../macros.hpp"
#include "../units/units.hpp"
#include <algorithm>
#include <cmath>

hi_export_module(hikogui.font.font_metrics);

hi_export namespace hi::inline v1 {
/** The metrics of a font.
 *
 * These are the metrics that are used for the font as a whole.
 * Inside the font these are in 'em' units, outside the font they
 * may have been scaled to 'points' or 'dp'.
 */
template<typename Unit, typename T>
hi_export struct font_metrics {
    /** Distance from baseline of highest ascender.
     */
    au::Quantity<Unit, T> ascender;

    /** Distance from baseline of lowest descender.
     *
     * @note positive downward.
     */
    au::Quantity<Unit, T> descender;

    /** Distance between lines.
     *
     * The distance between the descender of a line and the ascender of the next line.
     */
    au::Quantity<Unit, T> line_gap;

    /** Height of capital letter, or height of the letter 'H'.
     */
    au::Quantity<Unit, T> cap_height;

    /** Height of lower case characters without ascenders or descenders, or the small letter 'x'.
     */
    au::Quantity<Unit, T> x_height;

    /** The advance for digits, specifically the digit '8'.
     *
     * @note: All digits in a font should have the same advance.
     */
    au::Quantity<Unit, T> digit_advance;

    ~font_metrics() = default;
    constexpr font_metrics() noexcept = default;
    constexpr font_metrics(font_metrics const&) noexcept = default;
    constexpr font_metrics(font_metrics&&) noexcept = default;
    constexpr font_metrics& operator=(font_metrics const&) noexcept = default;
    constexpr font_metrics& operator=(font_metrics&&) noexcept = default;
    [[nodiscard]] constexpr friend bool operator==(font_metrics const&, font_metrics const&) noexcept = default;

    /** Scale the metrics by a scalar value.
     */
    template<typename LhsUnit, typename LhsT>
    [[nodiscard]] constexpr friend auto operator*(au::Quantity<LhsUnit, LhsT> const& lhs, font_metrics const& rhs) noexcept requires std::same_as<Unit, unit::EmSquares>
    {
        // clang-format off
        using result_unit =
            std::conditional_t<std::is_same_v<LhsUnit, unit::PixelsPerEm>, unit::Pixels,
            std::conditional_t<std::is_same_v<LhsUnit, unit::PointsPerEm>, unit::Points,
            std::conditional_t<std::is_same_v<LhsUnit, unit::DipsPerEm>, unit::Dips,
            void>>>;
        // clang-format on

        auto r = font_metrics<result_unit, std::common_type_t<LhsT, T>>{};
        r.ascender = lhs * rhs.ascender;
        r.descender = lhs * rhs.descender;
        r.line_gap = lhs * rhs.line_gap;
        r.cap_height = lhs * rhs.cap_height;
        r.x_height = lhs * rhs.x_height;
        r.digit_advance = lhs * rhs.digit_advance;
        return r;
    }

    [[nodiscard]] constexpr friend font_metrics max(font_metrics const& a, font_metrics const& b) noexcept
    {
        auto r = font_metrics{};
        r.ascender = std::max(a.ascender, b.ascender);
        r.descender = std::max(a.descender, b.descender);
        r.line_gap = std::max(a.line_gap, b.line_gap);
        r.cap_height = std::max(a.cap_height, b.cap_height);
        r.x_height = std::max(a.x_height, b.x_height);
        r.digit_advance = std::max(a.digit_advance, b.digit_advance);
        return r;
    }
};

using font_metrics_em = font_metrics<unit::EmSquares, float>;
using font_metrics_pt = font_metrics<unit::Points, float>;
using font_metrics_px = font_metrics<unit::Pixels, float>;

} // namespace hi::inline v1
