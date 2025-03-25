// Copyright Take Vos 2024.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "../i18n/i18n.hpp"
#include "../utility/utility.hpp"
#include "../macros.hpp"
#include <optional>
#include <span>
#include <cstdint>

hi_export_module(hikogui.font : otype_script_list);

hi_export namespace hi::inline v1 {
/** Search the script-list-table for a script.
 *
 * @param bytes The bytes of the script-list-table.
 * @param script The script to search for.
 * @return The word-offset of the script-table, or NULL when not found.
 */
[[nodiscard]] inline uint16_t otype_script_list_search(std::span<std::byte const> bytes, iso_15924 script)
{
    struct script_header_type {
        big_uint16_buf_t script_count;
    };

    struct script_record_type {
        // Because records need to be aligned to 16-bit boundaries, we need to
        // use a 8-bit buffer to read the 32-bit value.
        uint8_t script_tag[4];
        big_uint16_buf_t script_offset;
    };

    auto const script_tag = script.code4_open_type();
    constexpr auto default_tag = fourcc<"DFLT">();

    auto offset = size_t{0};
    auto const script_header = implicit_cast<script_header_type>(offset, bytes);

    // Do a linear search, because there should be only a few scripts.
    // And linear search is faster than binary search for small numbers.
    auto const script_records = implicit_cast<script_record_type>(offset, bytes, *script_header.script_count);

    auto default_offset = uint16_t{0};
    for (auto const& script_record : script_records) {
        auto const tag = load<uint32_t, std::endian::big>(script_record.script_tag);

        if (tag == script_tag) {
            return *script_record.script_offset;
        } else if (tag == default_tag) {
            default_offset = *script_record.script_offset;
        }
    }
    return default_offset;
}

/** Search the script table for a language system.
 *
 * @param bytes The bytes of the script-table.
 * @param language The language system to search for.
 * @return The word-offset of the language-system-table, from the start of the
 *         script-table, or NULL when not found.
 */
[[nodiscard]] inline uint16_t otype_script_table_search(std::span<std::byte const> bytes, iso_639 language)
{
    struct script_table_header_type {
        big_uint16_buf_t default_language_system_offset;
        big_uint16_buf_t language_system_count;
    };

    struct language_system_record_type {
        uint8_t language_tag[4];
        big_uint16_buf_t language_system_offset;
    };

    auto const language_tag = language.open_type_code();

    auto offset = size_t{0};
    auto const script_table_header = implicit_cast<script_table_header_type>(offset, bytes);

    auto const language_system_records =
        implicit_cast<language_system_record_type>(offset, bytes, *script_table_header.language_system_count);

    auto const it = std::lower_bound(
        language_system_records.begin(), language_system_records.end(), language_tag, [&](auto const& a, auto b) {
            return load<uint32_t, std::endian::big>(a.language_tag) < b;
        });
    if (it == language_system_records.end() or load<uint32_t, std::endian::big>(it->language_tag) != language_tag) {
        return *script_table_header.default_language_system_offset;
    }
    return *it->language_system_offset;
}

/** Search the language-system-table for a feature.
 *
 * @param bytes The bytes of the language-system-table.
 * @return A list of indices to the feature-list-table, the first index is the
 *         required feature, the rest are optional features.
 *         the required feature may be 0xffff when there is no required feature.
 */
[[nodiscard]] inline generator<uint16_t> otype_language_system_table_search(std::span<std::byte const> bytes)
{
    struct language_system_table_header_type {
        big_uint16_buf_t lookup_order_offset;
        big_uint16_buf_t required_feature_index;
        big_uint16_buf_t feature_index_count;
    };

    auto offset = size_t{0};
    auto const language_system_table_header = implicit_cast<language_system_table_header_type>(offset, bytes);

    auto r = lean_vector<uint16_t>{};
    r.reserve(*language_system_table_header.feature_index_count + 1);
    r.push_back(*language_system_table_header.required_feature_index);

    auto const feature_indices =
        implicit_cast<big_uint16_buf_t>(offset, bytes, *language_system_table_header.feature_index_count);
    for (auto const& feature_index : feature_indices) {
        co_yield *feature_index;
    }
}

/** Search the open type script-list-table for a script and language.
 *
 * @param bytes The bytes of the script-list-table.
 * @param script The script to search for.
 * @param language The language to search for.
 * @return A list of indices to the feature-list-table, the first index is the
 *         required feature, the rest are optional features.
 *         the required feature may be 0xffff when there is no required feature.
 */
[[nodiscard]] inline generator<uint16_t>
otype_script_list_search(std::span<std::byte const> bytes, iso_15924 script, iso_639 language)
{
    auto const script_offset = otype_script_list_search(bytes, script);
    if (script_offset == 0) {
        // Script not found, no required features.
        co_yield 0xffff;
        co_return;
    }

    auto const script_table_bytes = bytes.subspan(gsl::narrow_cast<size_t>(script_offset) * 2);
    auto const script_table_offset = otype_script_table_search(script_table_bytes, language);
    if (script_table_offset == 0) {
        // Language not found, no required features.
        co_yield 0xffff;
        co_return;
    }

    auto const language_system_table_bytes = script_table_bytes.subspan(gsl::narrow_cast<size_t>(script_table_offset) * 2);
    for (auto const feature_index : otype_language_system_table_search(language_system_table_bytes)) {
        co_yield feature_index;
    }
}


}
