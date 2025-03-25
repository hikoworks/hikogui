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
#include <expected>

hi_export_module(hikogui.font : otype_feature_list);

hi_export namespace hi::inline v1 {
/** Search the open type feature-list-table for a feature.
 *
 * @param bytes The bytes of the feature-list-table.
 * @param feature_indices A list of indices to the feature-list-table for the
 *                        current script and language.
 * @param optional_feature_tags The feature to search for.
 * @return A list of offsets to the feature-tables.
 */
template<std::ranges::input_range FeatureIndices>
[[nodiscard]] inline generator<uint16_t> otype_feature_list_search_offsets(
    std::span<std::byte const> bytes,
    FeatureIndices&& feature_indices,
    std::span<uint32_t> optional_feature_tags)
{
    auto feature_indices_it = std::ranges::begin(feature_indices);
    auto const feature_indices_end = std::ranges::end(feature_indices);
    assert(feature_indices_it != feature_indices_end);

    struct feature_list_header_type {
        big_uint16_buf_t feature_count;
    };

    struct feature_record_type {
        uint8_t feature_tag[4];
        big_uint16_buf_t feature_offset;
    };

    auto offset = size_t{0};
    auto const feature_list_header = implicit_cast<feature_list_header_type>(offset, bytes);

    auto const feature_records = implicit_cast<feature_record_type>(offset, bytes, *feature_list_header.feature_count);

    // Add the required feature.
    auto const required_feature_index = *feature_indices_it++;
    if (required_feature_index != 0xffff and required_feature_index < feature_records.size()) {
        co_yield *feature_records[required_feature_index].feature_offset;
    }

    // Add the optional selected features.
    for (; feature_indices_it != feature_indices_end; ++feature_indices_it) {
        auto const feature_index = *feature_indices_it;
        if (feature_index >= feature_records.size()) {
            throw otype_file_error("feature_index out of range");
        }

        auto const& feature_record = feature_records[feature_index];
        for (auto const tag : optional_feature_tags) {
            if (load<uint32_t, std::endian::big>(feature_record.feature_tag) == tag) {
                co_yield *feature_record.feature_offset;
                break;
            }
        }
    }
}

/** Append the lookup-indices to a list.
 *
 * @param bytes The bytes of the feature-table.
 * @return A list of lookup-indices.
 */
[[nodiscard]] inline generator<uint16_t> otype_feature_table_lookup_indices(std::span<std::byte const> bytes)
{
    struct feature_table_header_type {
        big_uint16_buf_t feature_params_offset;
        big_uint16_buf_t lookup_index_count;
    };

    auto offset = size_t{0};
    auto const feature_table_header = implicit_cast<feature_table_header_type>(offset, bytes);

    auto const lookup_indices = implicit_cast<big_uint16_buf_t>(offset, bytes, *feature_table_header.lookup_index_count);
    for (auto const& lookup_index : lookup_indices) {
        co_yield *lookup_index;
    }
}

/** Collect the lookup-indices for features.
 *
 * The optional_feature_tags are the features you want to apply here are some
 * examples of features:
 * - 'kern' Kerning
 * - 'liga' Standard Ligatures
 * - 'mark' Mark Positioning
 *
 * @param bytes The bytes of the feature-table.
 * @param feature_indices A list of indices to the feature-list-table for the
 *                        current script and language. The first index is the
 *                        required feature, the rest are optional features.
 * @param optional_feature_tags The features you want to apply to the glyphs.
 * @return A list of indices to the lookup-table. The indices are sorted and unique.
 */
template<std::ranges::input_range FeatureIndices>
[[nodiscard]] inline lean_vector<uint16_t> otype_feature_table_search(
    std::span<std::byte const> bytes,
    FeatureIndices&& feature_indices,
    std::span<uint32_t> optional_feature_tags)
{
    auto r = lean_vector<uint16_t>{};
    for (auto const feature_table_offset :
         otype_feature_list_search_offsets(bytes, std::forward<FeatureIndices>(feature_indices), optional_feature_tags)) {
        auto const feature_table_bytes = bytes.subspan(gsl::narrow_cast<size_t>(feature_table_offset) * 2);

        for (auto const lookup_index : otype_feature_table_lookup_indices(feature_table_bytes)) {
            r.push_back(lookup_index);
        }
    }

    // The list of lookup-indices should be sorted and unique.
    // So that the lookup tables are used in the correct order on the glyphs.
    std::sort(r.begin(), r.end());
    r.erase(std::unique(r.begin(), r.end()), r.end());
    return r;
}
}
