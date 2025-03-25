// Copyright Take Vos 2024.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "../geometry/geometry.hpp"
#include "../utility/utility.hpp"
#include "../units/units.hpp"
#include "../macros.hpp"
#include <utility>

hi_export_module(hikogui.widgets : utility);

hi_export namespace hi::inline v1 {

[[nodiscard]] inline aarectangle align(
    aarectangle dst,
    extent2 src,
    hi::horizontal_alignment horizontal_alignment,
    hi::vertical_alignment vertical_alignment,
    unit::pixels_f cap_height)
{
    auto x = dst.left();
    auto y = dst.bottom();
    auto width = src.width();
    auto height = src.height();

    switch (horizontal_alignment) {
    case hi::horizontal_alignment::left:
        break;
    case hi::horizontal_alignment::center:
        x += (dst.width() - width) / 2.0f;
        break;
    case hi::horizontal_alignment::right:
        x += dst.width() - width;
        break;
    default:
        std::unreachable();
    }

    switch (vertical_alignment) {
    case hi::vertical_alignment::top:
        y += dst.height() - height / 2.0f;
        y -= cap_height.in(unit::pixels) / 2.0f;
        break;
    case hi::vertical_alignment::middle:
        y += (dst.height() - height) / 2.0f;
        break;
    case hi::vertical_alignment::bottom:
        y -= height / 2.0f;
        y += cap_height.in(unit::pixels) / 2.0f;
        break;
    default:
        std::unreachable();
    }

    x = std::round(x);
    y = std::round(y);
    return aarectangle{x, y, width, height};
}

[[nodiscard]] inline aarectangle
align(aarectangle dst, extent2 src, hi::alignment alignment, unit::pixels_f cap_height)
{
    return align(dst, src, alignment.horizontal(), alignment.vertical(), cap_height);
}

} // namespace hi::v1
