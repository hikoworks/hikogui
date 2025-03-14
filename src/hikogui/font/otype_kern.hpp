// Copyright Take Vos 2023.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "otype_utilities.hpp"
#include "../utility/utility.hpp"
#include "../macros.hpp"
#include <span>
#include <cstddef>

hi_export_module(hikogui.font.otype_kern);

hi_export namespace hi { inline namespace v1 {

[[nodiscard]] inline std::optional<float>
otype_kern_sub0_find(size_t &offset, std::span<std::byte const> bytes, glyph_id first_glyph_id, glyph_id second_glyph_id, float em_scale)
{
    struct header_type {
        big_uint16_buf_t num_pairs;
        big_uint16_buf_t search_range;
        big_uint16_buf_t entry_selector;
        big_uint16_buf_t range_shift;
    };

    struct entry_type {
        big_uint16_buf_t left;
        big_uint16_buf_t right;
        otype_fword_buf_t value;
    };

    auto const& header = implicit_cast<header_type>(offset, bytes);
    auto const entries = implicit_cast<entry_type>(offset, bytes, *header.num_pairs);

    auto const key = (wide_cast<uint32_t>(*first_glyph_id) << 16) | wide_cast<uint32_t>(*second_glyph_id);
    if (auto const entry_ptr = fast_binary_search_eq<std::endian::big>(entries, key)) {
        return entry_ptr->value * em_scale;
    } else {
        return std::nullopt;
    }
}

/** 'kern' version 0 find.
 *
 * 'kern' version 0 is used by Microsoft and is not in use anymore by Apple.
 * However it is part of open-type.
 */
[[nodiscard]] inline vector2
otype_kern_v0_find(std::span<std::byte const> bytes, glyph_id first_glyph_id, glyph_id second_glyph_id, float em_scale)
{
    struct header_type {
        big_uint16_buf_t version;
        big_uint16_buf_t num_tables;
    };

    struct entry_type {
        big_uint16_buf_t version;
        big_uint16_buf_t length;
        big_uint16_buf_t coverage;
    };

    auto offset = 0_uz;
    auto const& header = implicit_cast<header_type>(offset, bytes);
    hi_check(*header.version == 0, "'kern' table expect version to be version 0.");
    auto const num_tables = *header.num_tables;

    // The kerning is additive when multiple sub-tables are involved.
    auto r = vector2{};
    for (auto i = 0_uz; i != num_tables; ++i) {
        auto const& entry = implicit_cast<entry_type>(offset, bytes);
        hi_check(*entry.version == 0, "'kern' expect sub-table version to be 0.");

        // The entry length field is broken, due to being only 16-bits.
        // There are Windows system fonts where the 'kern' table is 0x13b60 in length,
        // with only a single sub-table with an entry-length of 0x3b5c. As you see the
        // top bits of the entry-length are simply truncated.
        //
        // This means we have to abort unknown coverage and unknown sub-tables as we
        // are unable to skip over those.

        auto const entry_coverage = *entry.coverage;

        auto const cross_stream = to_bool(entry_coverage & 0x0004);
        hi_check(not cross_stream, "'kern' this font contains cross-stream kerning which is unsupported.");

        auto const format = entry_coverage >> 8;
        hi_check(format == 0, "'kern' this font contains a unsupported subtable.");

        auto const kerning = otype_kern_sub0_find(offset, bytes, first_glyph_id, second_glyph_id, em_scale);
        if (kerning) {
            auto const horizontal = to_bool(entry_coverage & 0x0001);
            auto const minimum = to_bool(entry_coverage & 0x0002);
            auto const overwrite = to_bool(entry_coverage & 0x0008);

            if (overwrite) {
                r = horizontal ? vector2{*kerning, 0.0f} : vector2{0.0f, *kerning};

            } else if (minimum) {
                if (horizontal) {
                    r.x() = std::min(r.x(), *kerning);
                } else {
                    r.y() = std::min(r.y(), *kerning);
                }

            } else {
                r += horizontal ? vector2{*kerning, 0.0f} : vector2{0.0f, *kerning};
            }
        }
    }
    return r;
}

[[nodiscard]] inline vector2
otype_kern_v1_find(std::span<std::byte const> bytes, glyph_id first_glyph_id, glyph_id second_glyph_id, float em_scale)
{
    struct header_type {
        big_uint32_buf_t version;
        big_uint32_buf_t num_tables;
    };

    struct entry_type {
        big_uint32_buf_t length;
        big_uint16_buf_t coverage;
        big_uint16_buf_t tuple_index;
    };

    auto offset = 0_uz;
    auto const& header = implicit_cast<header_type>(offset, bytes);
    hi_check(*header.version == 0x00010000, "'kern' table expect version to be version 0x00010000.");
    auto const num_tables = *header.num_tables;

    // The kerning is additive when multiple sub-tables are involved.
    auto r = vector2{};
    for (auto i = 0_uz; i != num_tables; ++i) {
        auto sub_table_offset = offset;
        auto const& entry = implicit_cast<entry_type>(sub_table_offset, bytes);

        auto const entry_length = *entry.length;
        hi_check(entry_length >= sizeof(entry_type), "'kern' subtable length is invalid.");

        // Calculate the offset to the table now, because we are going to filter out sub-tables
        // based on their coverage.
        offset += entry_length;

        // The Microsoft OpenType documentation uses bit indicies left to right, i.e. msb=0 and lsb=15.
        auto const entry_coverage = *entry.coverage;

        auto const cross_stream = to_bool(entry_coverage & 0x4000);
        if (cross_stream) {
            // The cross-stream specification is broken as the sub-table data
            // may contain the value 0x8000 to turn off cross-streaming.
            // However the specification also says to use binary-search in this
            // data so we will never see 0x8000 in reality.
            continue;
        }

        auto const variation = to_bool(entry_coverage & 0x2000);
        if (variation or *entry.tuple_index != 0) {
            // We do not support variation fonts.
            continue;
        }

        auto const format = entry_coverage & 0xff;

        auto const kerning = [&]() -> std::optional<float> {
            if (format == 0) {
                return otype_kern_sub0_find(sub_table_offset, bytes, first_glyph_id, second_glyph_id, em_scale);
            } else {
                return std::nullopt;
            }
        }();

        if (kerning) {
            auto const vertical = to_bool(entry_coverage & 0x8000);

            auto const kerning_2D = vertical ? vector2{0.0f, *kerning} : vector2{*kerning, 0.0f};
            r += kerning_2D;
        }
    }
    return r;
}

[[nodiscard]] inline vector2
otype_kern_find(std::span<std::byte const> bytes, glyph_id first_glyph_id, glyph_id second_glyph_id, float em_scale)
{
    if (bytes.empty()) {
        // If the 'kern' table doesn't exist there are no kern adjustment.
        return {};
    }

    auto const version = *implicit_cast<big_uint16_buf_t>(bytes);
    if (version == 0) {
        return otype_kern_v0_find(bytes, first_glyph_id, second_glyph_id, em_scale);
    } else {
        return otype_kern_v1_find(bytes, first_glyph_id, second_glyph_id, em_scale);
    }
}

}} // namespace hi::v1
