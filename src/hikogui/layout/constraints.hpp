// Copyright Take Vos 2024.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "../units/units.hpp"
#include "../utility/utility.hpp"
#include "../macros.hpp"

hi_export_module(hikogui.layout : constraints);

hi_export namespace hi::inline v1 {

struct height_constraints {
    unit::pixels_f minimum = unit::pixels(0.0f);
    unit::pixels_f preferred = unit::pixels(0.0f);
    float weight = 0.0f;
    unit::pixels_f margin_below = unit::pixels(0.0f);
    unit::pixels_f margin_above = unit::pixels(0.0f);
    bool in_use = false;

    constexpr height_constraints() noexcept = default;
    constexpr height_constraints(height_constraints const&) noexcept = default;
    constexpr height_constraints(height_constraints&&) noexcept = default;
    constexpr height_constraints& operator=(height_constraints const&) noexcept = default;
    constexpr height_constraints& operator=(height_constraints&&) noexcept = default;

    constexpr height_constraints(
        unit::pixels_f minimum,
        unit::pixels_f preferred = minimum,
        float weight = 0.0f,
        unit::pixels_f margin_below = unit::pixels(0.0f),
        unit::pixels_f margin_above = unit::pixels(0.0f)) noexcept :
        minimum(minimum),
        preferred(preferred),
        weight(weight),
        margin_below(margin_below),
        margin_above(margin_above),
        in_use(true)
    {
    }

    constexpr void clear() noexcept
    {
        minimum = unit::pixels(0.0f);
        preferred = unit::pixels(0.0f);
        weight = 0.0f;
        margin_below = unit::pixels(0.0f);
        margin_above = unit::pixels(0.0f);
        in_use = false;
    }

    [[nodiscard]] constexpr bool empty() noexcept
    {
        return not in_use;
    }

    [[nodiscard]] constexpr friend height_constraints max(height_constraints const& lhs, height_constraints const& rhs) noexcept
    {
        auto r = height_constraints{};
        r.minimum = std::max(lhs.minimum, rhs.minimum);
        r.preferred = std::max(lhs.preferred, rhs.preferred);
        r.weight = std::max(lhs.weight, rhs.weight);
        r.margin_below = std::max(lhs.margin_below, rhs.margin_below);
        r.margin_above = std::max(lhs.margin_above, rhs.margin_above);
        r.in_use = lhs.in_use or rhs.in_use;

        if (r.minimum > r.preferred) {
            r.preferred = r.minimum;
        }
        return r;
    }

    [[nodiscard]] constexpr friend height_constraints operator+(height_constraints const& lhs, height_constraints const& rhs) noexcept
    {
        auto r = height_constraints{};
        if (lhs.in_use) {
            r.minimum = lhs.minimum + rhs.minimum + std::max(lhs.margin_below, rhs.margin_above);
            r.preferred = lhs.preferred + rhs.preferred + std::max(lhs.margin_below, rhs.margin_above);
            r.weight = lhs.weight + rhs.weight;
            r.margin_above = lhs.margin_above;
            r.margin_below = rhs.margin_below;
            r.in_use = lhs.in_use or rhs.in_use;

            if (r.minimum > r.preferred) {
                r.preferred = r.minimum;
            }

        } else {
            r = rhs;
        }
        return r;
    }
};

} // namespace hi::v1