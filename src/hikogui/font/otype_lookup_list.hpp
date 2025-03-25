// Copyright Take Vos 2024.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "otype_error.hpp"
#include "../utility/utility.hpp"
#include "../macros.hpp"
#include <optional>
#include <span>
#include <cstdint>

hi_export_module(hikogui.font : otype_script_list);

hi_export namespace hi::inline v1 {
template<std::ranges::input_range LookupIndices>
[[nodiscard]] inline generator<uint16_t>
otype_lookup_list_search_offsets(std::span<std::byte const> bytes, LookupIndices&& lookup_indices)
{
    struct header_type {
        big_uint16_buf_t lookup_count;
    };

    auto offset = size_t{0};
    auto const header = implicit_cast<header_type>(offset, bytes);

    auto const lookup_offsets = implicit_cast<big_uint16_buf_t>(offset, bytes, *header.lookup_count);

    for (auto const lookup_index : lookup_indices) {
        if (lookup_index >= *header.lookup_count) {
            throw otype_file_error("lookup_index out of range");
        }
        co_yield *lookup_offsets[lookup_index];
    }
}

/** Execute a function for each lookup-sub-table in the lookup-list.
 *
 * @tparam F The function type to execute for each lookup-sub-table. The
 *         function must have the following signature: `void(std::span<std::byte
 *         const> bytes, uint16_t lookup_type, uint16_t lookup_flag, uint16_t
 *         mark_filtering_set)`
 * @param bytes The bytes of the lookup-list.
 * @param lookup_indices A list of indices to the lookup-list-table.
 * @param f The function to execute for each lookup-sub-table. This function
 *        should handle substitution or positioning of a run of glyphs.
 *        pass the glyphs as a capture.
 */
template<std::ranges::forward_range LookupIndices, std::invocable<std::span<std::byte const>, uint16_t, uint16_t, uint16_t> F>
inline void otype_lookup_list_execute(std::span<std::byte const> bytes, LookupIndices&& lookup_indices, F const& f)
{
    struct lookup_table_header_type {
        big_uint16_buf_t lookup_type;
        big_uint16_buf_t lookup_flag;
        big_uint16_buf_t sub_table_count;
    };

    for (auto const lookup_table_offset : otype_lookup_list_search_offsets(bytes, std::forward<LookupIndices>(lookup_indices))) {
        auto const lookup_table_bytes = bytes.subspan(gsl::narrow_cast<size_t>(lookup_table_offset) * 2);

        auto offset = size_t{0};
        auto const lookup_table = implicit_cast<lookup_table_header_type>(offset, lookup_table_bytes);
        auto const lookup_table_sub_table_offsets =
            implicit_cast<big_uint16_buf_t>(offset, lookup_table_bytes, *lookup_table.sub_table_count);
        auto const mark_filtering_set = *implicit_cast<big_uint16_buf_t>(offset, lookup_table_bytes);

        for (auto const lookup_sub_table_offset : lookup_table_sub_table_offsets) {
            auto const lookup_sub_table_bytes = lookup_table_bytes.subspan(gsl::narrow_cast<size_t>(*lookup_sub_table_offset) * 2);

            f(lookup_sub_table_bytes, *lookup_table.lookup_type, *lookup_table.lookup_flag, mark_filtering_set);
        }
    }
}
}
