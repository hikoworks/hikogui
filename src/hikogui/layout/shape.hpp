// Copyright Take Vos 2022.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "baseline.hpp"
#include "../geometry/geometry.hpp"
#include "../utility/utility.hpp"
#include "../macros.hpp"
#include <limits>
#include <optional>

hi_export_module(hikogui.layout.shape);

hi_export namespace hi::inline v1::layout {

struct shape {
    aarectangle rectangle;
    float baseline;

    shape() noexcept = default;
    shape(shape const&) noexcept = default;
    shape(shape&&) noexcept = default;
    shape& operator=(shape const&) noexcept = default;
    shape& operator=(shape&&) noexcept = default;

    explicit shape(aarectangle rectangle, hi::baseline baseline = {}) noexcept : rectangle(rectangle), baseline(baseline) {}

    [[nodiscard]] float x() const noexcept
    {
        return rectangle.x();
    }

    [[nodiscard]] float y() const noexcept
    {
        return rectangle.y();
    }

    [[nodiscard]] extent2 size() const noexcept
    {
        return rectangle.size();
    }

    [[nodiscard]] float width() const noexcept
    {
        return rectangle.width();
    }

    [[nodiscard]] float height() const noexcept
    {
        return rectangle.height();
    }

    [[nodiscard]] float left() const noexcept
    {
        return rectangle.left();
    }

    [[nodiscard]] float right() const noexcept
    {
        return rectangle.right();
    }

    [[nodiscard]] float bottom() const noexcept
    {
        return rectangle.bottom();
    }

    [[nodiscard]] float top() const noexcept
    {
        return rectangle.top();
    }
};

} // namespace v1
} // namespace hi::v1
