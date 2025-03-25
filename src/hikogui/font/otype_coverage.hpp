// Copyright Take Vos 2022.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "otype_error.hpp"
#include "../geometry/geometry.hpp"
#include "../telemetry/telemetry.hpp"
#include "../utility/utility.hpp"
#include "../parser/parser.hpp"
#include <cstddef>
#include <span>

hi_export_module(hikogui.font.otype_coverage);

hi_export namespace hi::inline v1 {

[[nodiscard]] inline std::optional<uint16_t> otype_coverage_1_search(std::span<std::byte const> bytes, glyph_id id)
{
    struct header_type {
        big_uint16_buf_t format;
        big_uint16_buf_t glyph_count;
    };

    auto offset = size_t{0};
    auto const header = implicit_cast<header_type>(offset, bytes);
    assert(*header.format == 1);

    auto const glyph_ids = implicit_cast<big_uint16_buf_t>(offset, bytes, *header.glyph_count);
    auto const it = std::lower_bound(glyph_ids.begin(), glyph_ids.end(), id, [](auto const& a, auto b) {
        return *a < b.value();
    });
    if (it == glyph_ids.end() or **it != id.value()) {
        return std::nullopt;
    }
    return gsl::narrow<uint16_t>(std::distance(glyph_ids.begin(), it));
}

[[nodiscard]] inline std::optional<uint16_t> otype_coverage_2_search(std::span<std::byte const> bytes, glyph_id id)
{
    struct header_type {
        big_uint16_buf_t format;
        big_uint16_buf_t range_count;
    };

    struct range_record_type {
        big_uint16_buf_t start_glyph_id;
        big_uint16_buf_t end_glyph_id;
        big_uint16_buf_t start_coverage_index;
    };

    auto offset = size_t{0};
    auto const header = implicit_cast<header_type>(offset, bytes);
    assert(*header.format == 2);

    auto const records = implicit_cast<range_record_type>(offset, bytes, *header.range_count);
    auto const it = std::lower_bound(records.begin(), records.end(), id, [](auto const& a, auto b) {
        return *a.end_glyph_id < b.value();
    });
    if (it == records.end() or id.value() < *it->start_glyph_id) {
        return std::nullopt;
    }

    return *it->start_coverage_index + (id.value() - *it->start_glyph_id);
}

[[nodiscard]] inline std::optional<uint16_t> otype_coverage_search(std::span<std::byte const> bytes, glyph_id id)
{
    struct header_type {
        big_uint16_buf_t coverage_format;
    };

    auto const header = implicit_cast<header_type>(bytes);

    switch (*header.coverage_format) {
    case 1:
        return otype_coverage_1_search(bytes, id);
    case 2:
        return otype_coverage_2_search(bytes, id);
    default:
        throw otype_file_error("unsupported coverage format");
    }
}

} // namespace hi::inline v1
