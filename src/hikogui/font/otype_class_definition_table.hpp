// Copyright Take Vos 2024.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "otype_utilities.hpp"
#include "font_char_map.hpp"
#include "../macros.hpp"

hi_export_module(hikogui.font : otype_class_definition_table);

hi_export namespace hi::inline v1 {

[[nodiscard]] inline uint16_t otype_class_definition_table_search_format1(std::span<std::byte const> bytes, glyph_id id)
{
    struct header_type {
        big_uint16_buf_t format;
        big_uint16_buf_t start_glyph_id;
        big_uint16_buf_t glyph_count;
    };

    auto offset = size_t{0};
    auto const header = implicit_cast<header_type>(offset, bytes);
    assert(*header.format == 1);

    auto const glyph_count = *header.glyph_count;
    auto const start_glyph_id = *header.start_glyph_id;

    if (id < start_glyph_id) {
        return 0;
    }

    auto const index = id.value() - start_glyph_id;
    if (index >= glyph_count) {
        return 0;
    }
    auto const class_values = implicit_cast<big_uint16_buf_t>(offset, bytes, glyph_count);
    return *class_values[index];
}

[[nodiscard]] inline uint16_t otype_class_definition_table_search_format2(std::span<std::byte const> bytes, glyph_id id)
{
    struct header_type {
        big_uint16_buf_t format;
        big_uint16_buf_t class_range_count;
    };

    struct class_range_type {
        big_uint16_buf_t start_glyph_id;
        big_uint16_buf_t end_glyph_id;
        big_uint16_buf_t class_value;
    };

    auto offset = size_t{0};
    auto const header = implicit_cast<header_type>(offset, bytes);
    assert(*header.format == 2);

    auto const class_range_count = *header.class_range_count;
    auto const class_ranges = implicit_cast<class_range_type>(offset, bytes, class_range_count);

    auto const it = std::lower_bound(class_ranges.begin(), class_ranges.end(), id, [](auto const& lhs, auto rhs) {
        return *lhs.end_glyph_id < rhs;
    });

    if (it == class_ranges.end() or id < *it->start_glyph_id) {
        return 0;
    }
    assert(id <= *it->end_glyph_id);

    return *it->class_value;
}

[[nodiscard]] inline uint16_t otype_class_definition_table_search(std::span<std::byte const> bytes, glyph_id id)
{
    auto offset = size_t{0};
    auto const format = implicit_cast<big_uint16_buf_t>(offset, bytes);

    switch (*format) {
    case 1:
        return otype_class_definition_table_search_format1(bytes, id);
    case 2:
        return otype_class_definition_table_search_format2(bytes, id);
    default:
        throw otype_file_error("class definition table format not supported");
    }
}

}