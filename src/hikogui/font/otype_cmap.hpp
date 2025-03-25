// Copyright Take Vos 2023.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "otype_utilities.hpp"
#include "font_char_map.hpp"
#include "../macros.hpp"
#include <gsl/gsl>
#include <algorithm>
#include <span>
#include <cstddef>
#include <cstdint>

hi_export_module(hikogui.font.otype_cmap);

hi_export namespace hi { inline namespace v1 {

[[nodiscard]] inline glyph_id otype_cmap_find_map_4(std::span<std::byte const> over_sized_bytes, char32_t code_point) noexcept
{
    struct header_type {
        big_uint16_buf_t format;
        big_uint16_buf_t length;
        big_uint16_buf_t language;
        big_uint16_buf_t seg_count_x2;
        big_uint16_buf_t search_range;
        big_uint16_buf_t entry_selector;
        big_uint16_buf_t range_shift;
    };

    if (code_point > 0xffff) {
        return glyph_id{};
    }

    auto offset = 0_uz;
    auto const& header = implicit_cast<header_type>(offset, over_sized_bytes);
    assert(*header.format == 4);

    auto const length = *header.length;
    if (length > over_sized_bytes.size()) {
        return glyph_id{};
    }
    auto const bytes = over_sized_bytes.subspan(0, length);

    auto const seg_count = *header.seg_count_x2 / 2;

    auto const end_codes = implicit_cast<big_uint16_buf_t>(offset, bytes, seg_count);
    offset += sizeof(uint16_t); // reservedPad

    auto const end_code_it = std::lower_bound(end_codes.begin(), end_codes.end(), gsl::narrow_cast<uint16_t>(code_point), [](auto const& a, auto b) {
        return *a < b;
    });
    if (end_code_it == end_codes.end()) {
        return glyph_id{};
    }
    auto const segment_index = gsl::narrow_cast<size_t>(std::distance(end_codes.begin(), end_code_it));

    auto const start_codes = implicit_cast<big_uint16_buf_t>(offset, bytes, seg_count);
    auto const start_code = *start_codes[segment_index];

    if (code_point < start_code) {
        return glyph_id{};
    }

    auto const id_deltas = implicit_cast<big_uint16_buf_t>(offset, bytes, seg_count);
    auto id_delta = *id_deltas[segment_index];

    auto const id_range_offsets = implicit_cast<big_uint16_buf_t>(offset, bytes, seg_count);
    auto id_range_offset = static_cast<size_t>(*id_range_offsets[segment_index]);

    if (id_range_offset == 0) {
        id_delta += gsl::narrow_cast<uint16_t>(code_point);
        return glyph_id{id_delta};
    }

    auto const glyph_id_array_count = (bytes.size() - offset) / sizeof(big_uint16_buf_t);
    auto const glyph_id_array = implicit_cast<big_uint16_buf_t>(offset, bytes, glyph_id_array_count);

    id_range_offset /= 2;
    id_range_offset += code_point;
    id_range_offset -= start_code;
    id_range_offset -= seg_count;
    id_range_offset += segment_index;

    if (id_range_offset >= glyph_id_array_count) {
        return glyph_id{};
    }
    return glyph_id{*glyph_id_array[id_range_offset]};
}

[[nodiscard]] inline glyph_id otype_cmap_find_map_6(std::span<std::byte const> over_sized_bytes, char32_t code_point) noexcept
{
    struct header_type {
        big_uint16_buf_t format;
        big_uint16_buf_t length;
        big_uint16_buf_t language;
        big_uint16_buf_t first_code;
        big_uint16_buf_t entry_count;
    };

    if (code_point > 0xffff) {
        return glyph_id{};
    }

    auto offset = 0_uz;
    auto const& header = implicit_cast<header_type>(offset, over_sized_bytes);
    assert(*header.format == 6);

    auto const length = *header.length;
    if (length > over_sized_bytes.size()) {
        return glyph_id{};
    }
    auto const bytes = over_sized_bytes.subspan(0, length);

    auto const first_code = *header.entry_count;
    if (code_point < first_code) {
        return glyph_id{};
    }

    auto const index = gsl::narrow_cast<size_t>(code_point - first_code);
    auto const entry_count = *header.entry_count;
    if (index >= entry_count) {
        return glyph_id{};
    }

    auto const entries = implicit_cast<big_uint16_buf_t>(offset, bytes, entry_count);
    return glyph_id{*entries[index]};
}

[[nodiscard]] inline glyph_id otype_cmap_find_map_12(std::span<std::byte const> over_sized_bytes, char32_t code_point) noexcept
{
    struct header_type {
        big_uint16_buf_t format;
        big_uint16_buf_t reserved;
        big_uint32_buf_t length;
        big_uint32_buf_t language;
        big_uint32_buf_t num_groups;
    };

    struct entry_type {
        big_uint32_buf_t start_char_code;
        big_uint32_buf_t end_char_code;
        big_uint32_buf_t start_glyph_id;
    };

    struct entry_type_end_first {
        big_uint32_buf_t end_char_code;
        big_uint32_buf_t start_glyph_id;
        big_uint32_buf_t dummy;
    };

    auto offset = 0_uz;
    auto const& header = implicit_cast<header_type>(offset, over_sized_bytes);
    assert(*header.format == 12);

    auto const length = *header.length;
    if (length > over_sized_bytes.size()) {
        return glyph_id{};
    }

    auto const num_groups = gsl::narrow_cast<size_t>(*header.num_groups);
    if (num_groups == 0) {
        return glyph_id{};
    }

    auto const bytes = over_sized_bytes.subspan(0, length);

    auto const entries = implicit_cast<entry_type>(offset, bytes, *header.num_groups);
    auto const it = std::lower_bound(entries.begin(), entries.end(), code_point, [](auto const& a, auto b) {
        return *a.end_char_code < b;
    });
    if (it == entries.end()) {
        return glyph_id{};
    }

    auto const start_char_code = *it->start_char_code;
    auto const end_char_code = *it->end_char_code;
    auto const start_glyph_id = *it->start_glyph_id;

    if (code_point < start_char_code or code_point > end_char_code) {
        return glyph_id{};
    }

    auto id = start_glyph_id;
    id += code_point;
    id -= start_char_code;
    return glyph_id{id};
}

/** Lookup a code-point in a sub-map.
 *
 * @param bytes A span arround the complete sub-map.
 * @param code_point The code-point to lookup.
 * @return The glyph_id found, or empty on failure.
 */
[[nodiscard]] inline glyph_id otype_cmap_find_map(std::span<std::byte const> bytes, char32_t code_point) noexcept
{
    // The first 16 bits of a cmap sub-table always contain the format.
    auto const format = *implicit_cast<big_uint16_buf_t>(bytes);

    switch (format) {
    case 4:
        return otype_cmap_find_map_4(bytes, code_point);
    case 6:
        return otype_cmap_find_map_6(bytes, code_point);
    case 12:
        return otype_cmap_find_map_12(bytes, code_point);
    default:
        return glyph_id{};
    }
}

/** Find a character in the 'cmap'-table of a open-type font file.
 *
 * @param bytes A span arround the complete 'cmap'-table.
 * @param code_point The code-point to look up in the table.
 * @return The glyph_id found, or empty on failure.
 */
[[nodiscard]] inline glyph_id otype_cmap_find(std::span<std::byte const> bytes, char32_t code_point) noexcept
{
    struct header_type {
        big_uint16_buf_t version;
        big_uint16_buf_t num_tables;
    };

    struct entry_type {
        big_uint16_buf_t platform_id;
        big_uint16_buf_t platform_specific_id;
        big_uint32_buf_t offset;
    };

    enum class command_type : uint8_t{
        nop,
        assign_bmp,
        return_bmp,
        return_full
    };

    constexpr auto command_table = [&] {
        auto r = std::array<command_type, 256>{};
        // Unicode / BMP (can handle BMP)
        r[0b10'00'0011] = command_type::assign_bmp;
        // Windows / BMP (can handle BMP)
        r[0b10'11'0001] = command_type::assign_bmp;

        // Unicode / full (can handle BMP)
        r[0b10'00'0100] = command_type::return_full;
        // Unicode / full (cannot handle BMP)
        r[0b00'00'0100] = command_type::return_full;
        // Windows / full (can handle BMP)
        r[0b10'11'1010] = command_type::return_full;
        // Windows / full (cannot handle BMP)
        r[0b00'11'1010] = command_type::return_full;
        // Windows / BMP (found BMP)
        r[0b11'11'0001] = command_type::return_bmp;
        // Windows / full (found BMP)
        r[0b11'11'1010] = command_type::return_bmp;
        return r;
    }();

    std::size_t offset = 0;

    auto const& header = implicit_cast<header_type>(offset, bytes);
    if (bytes.size() < sizeof(header_type) or *header.version != 0) {
        return glyph_id{};
    }

    auto const maps = implicit_cast<entry_type>(offset, bytes, *header.num_tables);

    auto const map = [&] {
        auto const can_use_bmp = static_cast<uint8_t>(code_point <= 0xffff) << 7;

        entry_type const *bmp_map = nullptr;
        for (auto const& map : maps) {
            auto const platform_id = *map.platform_id;
            if (platform_id > 0b11) {
                continue;
            }
            auto const platform_specific_id = *map.platform_specific_id;
            if (platform_specific_id > 0b1111) {
                continue;
            }

            auto platform = can_use_bmp;
            platform |= static_cast<uint8_t>(bmp_map != nullptr) << 6;
            platform |= platform_id << 4;
            platform |= platform_specific_id;

            auto const command = command_table[platform];
            switch (command) {
            case command_type::nop:
                break;
            case command_type::assign_bmp:
                bmp_map = &map;
                break;
            case command_type::return_bmp:
                return bmp_map;
            case command_type::return_full:
                return &map;
            default:
                std::unreachable();
            }
        }
        return bmp_map;
    }();

    if (map == nullptr) {
        return glyph_id{};
    }

    return otype_cmap_find_map(bytes.subspan(*map->offset), code_point);
}


}} // namespace hi::v1
