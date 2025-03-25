// Copyright Take Vos 2023.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "otype_utilities.hpp"
#include "../utility/utility.hpp"
#include "../units/units.hpp"
#include "../macros.hpp"
#include <span>
#include <cstddef>

hi_export_module(hikogui.font.otype_htmx);

hi_export namespace hi {
inline namespace v1 {

[[nodiscard]] inline auto
otype_hmtx_get(std::span<std::byte const> bytes, hi::glyph_id glyph_id, uint16_t num_horizontal_metrics, float em_scale)
{
    struct entry_type {
        otype_fuword_buf_t advance_width;
        otype_fword_buf_t left_side_bearing;
    };

    struct return_type {
        unit::em_squares_f advance_width;
        unit::em_squares_f left_side_bearing;
    };

    hi_axiom(num_horizontal_metrics >= 1);

    auto offset = 0_uz;
    auto const horizontal_metrics = implicit_cast<entry_type>(offset, bytes, num_horizontal_metrics);

    if (*glyph_id < num_horizontal_metrics) {
        auto const& entry = horizontal_metrics[*glyph_id];
        return return_type{
            unit::em_squares(entry.advance_width * em_scale), unit::em_squares(entry.left_side_bearing * em_scale)};
    }

    // In mono-type fonts the advance_width is repeating from the last entry and only the left_side_bearing is needed.
    auto const advance_width = unit::em_squares(horizontal_metrics.back().advance_width * em_scale);

    // The rest of the bytes in this table form the left_side_bearings.
    auto const num_left_side_bearing = (bytes.size() - offset) / sizeof(otype_fword_buf_t);
    auto const left_side_bearings = implicit_cast<otype_fword_buf_t>(offset, bytes, num_left_side_bearing);

    auto const left_side_bearing_index = *glyph_id - num_horizontal_metrics;
    auto const left_side_bearing = unit::em_squares(hi_check_at(left_side_bearings, left_side_bearing_index) * em_scale);
    return return_type{advance_width, left_side_bearing};
}

} // namespace v1
} // namespace hi::v1
