// Copyright Take Vos 2024.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "otype_error.hpp"
#include "otype_script_list.hpp"
#include "otype_feature_list.hpp"
#include "otype_lookup_list.hpp"
#include "otype_coverage.hpp"
#include "otype_GDEF.hpp"
#include "../i18n/i18n.hpp"
#include "../geometry/geometry.hpp"
#include "../parser/parser.hpp"
#include "../utility/utility.hpp"
#include <cstddef>
#include <span>

hi_export_module(hikogui.font.otype_GSUB);

hi_export namespace hi::inline v1 {
inline void otype_GSUB_type1_format1_apply(
    std::span<std::byte const> bytes,
    std::span<std::byte const> GDEF_bytes,
    std::span<font_glyph_ids> run,
    uint16_t lookup_flag,
    uint16_t mark_filtering_set)
{
    struct header_type {
        big_uint16_buf_t format;
        big_uint16_buf_t coverage_offset;
        big_uint16_buf_t delta_glyph_id;
    };

    auto offset = size_t{0};
    auto const header = implicit_cast<header_type>(offset, bytes);
    assert(*header.format == 1);

    auto const coverage_bytes = bytes.subspan(gsl::narrow_cast<size_t>(*header.coverage_offset) * 2);

    for (auto& font_glyph_ids : run) {
        for (auto i = size_t{0}; i != font_glyph_ids.size(); ++i) {
            auto& glyph_id = font_glyph_ids[i];

            if (otype_GDEF_filter_glyph(GDEF_bytes, glyph_id, i, lookup_flag, mark_filtering_set)) {
                continue;
            }

            if (auto coverage_index = otype_coverage_search(coverage_bytes, glyph_id)) {
                // Use 16-bit modulo to wrap around the glyph_id.
                auto value = glyph_id.value();
                value += *header.delta_glyph_id;
                glyph_id = value;
            }
        }
    }
}

inline void otype_GSUB_type1_format2_apply(
    std::span<std::byte const> bytes,
    std::span<std::byte const> GDEF_bytes,
    std::span<font_glyph_ids> run,
    uint16_t lookup_flag,
    uint16_t mark_filtering_set)
{
    struct header_type {
        big_uint16_buf_t format;
        big_uint16_buf_t coverage_offset;
        big_uint16_buf_t glyph_count;
    };

    auto offset = size_t{0};
    auto const header = implicit_cast<header_type>(offset, bytes);
    assert(*header.format == 2);

    auto const coverage_bytes = bytes.subspan(gsl::narrow_cast<size_t>(*header.coverage_offset) * 2);
    auto const substitute_glyph_ids = implicit_cast<big_uint16_buf_t>(offset, bytes, *header.glyph_count);

    for (auto& font_glyph_ids : run) {
        for (auto i = size_t{0}; i != font_glyph_ids.size(); ++i) {
            auto& glyph_id = font_glyph_ids[i];

            if (otype_GDEF_filter_glyph(GDEF_bytes, glyph_id, i, lookup_flag, mark_filtering_set)) {
                continue;
            }

            if (auto coverage_index = otype_coverage_search(coverage_bytes, glyph_id)) {
                glyph_id = *substitute_glyph_ids[*coverage_index];
            }
        }
    }
}

inline void otype_GSUB_type4_format1_apply(
    std::span<std::byte const> bytes,
    std::span<std::byte const> GDEF_bytes,
    std::span<font_glyph_ids> run,
    uint16_t lookup_flag,
    uint16_t mark_filtering_set)
{
    struct header_type {
        big_uint16_buf_t format;
        big_uint16_buf_t coverage_offset;
        big_uint16_buf_t ligature_set_count;
    };

    struct ligature_type {
        big_uint16_buf_t ligature_glyph;
        big_uint16_buf_t component_count;
    };

    auto const advance_glyph = [&](size_t& grapheme_i, size_t& glyph_i) {
        do {
            if (++glyph_i == run[grapheme_i].size()) {
                if (++grapheme_i == run.size()) {
                    return false;
                }
                glyph_i = 0;
            }
        } while (
            otype_GDEF_filter_glyph(GDEF_bytes, run[grapheme_i][glyph_i], glyph_i, lookup_flag, mark_filtering_set));
        return true;
    };

    auto const first_glyph = [&]() -> std::pair<size_t, size_t> {
        auto grapheme_i = size_t{0};
        auto glyph_i = size_t{0};
        if (otype_GDEF_filter_glyph(GDEF_bytes, run[grapheme_i][glyph_i], glyph_i, lookup_flag, mark_filtering_set)) {
            advance_glyph(grapheme_i, glyph_i);
        }
        return {grapheme_i, glyph_i};
    };

    auto offset = size_t{0};
    auto const header = implicit_cast<header_type>(offset, bytes);
    assert(*header.format == 4);

    auto const coverage_bytes = bytes.subspan(gsl::narrow_cast<size_t>(*header.coverage_offset) * 2);
    auto const ligature_offsets = implicit_cast<big_uint16_buf_t>(offset, bytes, *header.ligature_set_count);

    for (auto [grapheme_i, glyph_i] = first_glyph(); grapheme_i != run.size(); advance_glyph(grapheme_i, glyph_i)) {
        assert(not run[grapheme_i].empty());
        auto& glyph_id = run[grapheme_i][glyph_i];

        if (glyph_id.empty()) {
            // Empty glyphs where snuffed out by a ligature, possibly from this
            // algorithm and can be skipped.
            continue;
        }

        auto const coverage_index = otype_coverage_search(coverage_bytes, glyph_id);
        if (not coverage_index) {
            // No ligature for this glyph.
            continue;
        }

        auto const ligature_set_offset = ligature_offsets[*coverage_index];
        auto const ligature_set_bytes = bytes.subspan(gsl::narrow_cast<size_t>(*ligature_set_offset) * 2);
        auto offset = size_t{0};
        auto const ligature_count = implicit_cast<big_uint16_buf_t>(offset, ligature_set_bytes);
        auto const ligature_offsets = implicit_cast<big_uint16_buf_t>(offset, ligature_set_bytes, *ligature_count);

        for (auto ligature_offset : ligature_offsets) {
            auto offset = size_t{0};

            auto const ligature_bytes = ligature_set_bytes.subspan(gsl::narrow_cast<size_t>(*ligature_offset) * 2);
            auto const ligature = implicit_cast<ligature_type>(offset, ligature_bytes);
            if (*ligature.component_count == 0) {
                throw otype_file_error("GSUB type 4 format 1 ligature has no components");
            }
            auto const components = implicit_cast<big_uint16_buf_t>(offset, ligature_bytes, *ligature.component_count - 1);

            auto const ligature_matches = [&]() {
                auto grapheme_j = grapheme_i;
                auto glyph_j = glyph_i;
                for (auto component : components) {
                    if (not advance_glyph(grapheme_j, glyph_j)) {
                        return false;
                    }
                    if (run[grapheme_j][glyph_j] != *component) {
                        return false;
                    }
                }
                // All components matched.
                return true;
            }();

            if (ligature_matches) {
                // The ligature matched, replace the glyphs.
                glyph_id = *ligature.ligature_glyph;

                // Replace all components that took part in the ligature
                // with empty glyphs, so that we can keep track of the position
                // of the original graphemes, and keep track of the position
                // of the marks over the ligature.
                auto grapheme_j = grapheme_i;
                auto glyph_j = glyph_i;
                for (auto component : components) {
                    advance_glyph(grapheme_j, glyph_j);
                    run[grapheme_j][glyph_j] = {};
                }
                break;
            }
        }
    }
}

inline void otype_GSUB_apply_sub_table(
    std::span<std::byte const> bytes,
    std::span<std::byte const> GDEF_bytes,
    std::span<font_glyph_ids> run,
    uint16_t lookup_type,
    uint16_t lookup_flag,
    uint16_t mark_filtering_set)
{
    auto const format = *implicit_cast<big_uint16_buf_t>(bytes);

    switch (lookup_type) {
    case 1:
        switch (format) {
        case 1:
            otype_GSUB_type1_format1_apply(bytes, GDEF_bytes, run, lookup_flag, mark_filtering_set);
            break;
        case 2:
            otype_GSUB_type1_format2_apply(bytes, GDEF_bytes, run, lookup_flag, mark_filtering_set);
            break;
        default:
            throw otype_file_error("GSUB type 1 format not supported");
        }
        break;

    case 4:
        switch (format) {
        case 1:
            otype_GSUB_type4_format1_apply(bytes, GDEF_bytes, run, lookup_flag, mark_filtering_set);
            break;
        default:
            throw otype_file_error("GSUB type 4 format not supported");
        }
        break;

    default:
        throw otype_file_error("GSUB lookup type not supported");
    }
}

/** Apply the GSUB table to a run of glyphs.
 *
 * @param bytes The GSUB table.
 * @param GDEF_bytes The GDEF table.
 * @param run The run of glyphs to apply the GSUB table to. The glyphs are
 *            modified in place. When glyphs are replaced by a ligature the
 *            deleted glyphs are replaced by empty glyphs.
 * @param script The script of the run.
 * @param language The language of the run.
 * @param feature_tags The feature tags to apply.
 */
inline void otype_GSUB_apply(
    std::span<std::byte const> bytes,
    std::span<std::byte const> GDEF_bytes,
    std::span<font_glyph_ids> run,
    iso_15924 script,
    iso_639 language,
    std::span<uint32_t> feature_tags)
{
    struct GSUB_version_1_0 {
        big_uint16_buf_t major_version;
        big_uint16_buf_t minor_version;
        big_uint16_buf_t script_list_offset;
        big_uint16_buf_t feature_list_offset;
        big_uint16_buf_t lookup_list_offset;
    };

    auto offset = size_t{0};
    auto const header = implicit_cast<GSUB_version_1_0>(offset, bytes);
    if (*header.major_version != 1 and (*header.minor_version != 0 or *header.minor_version != 1)) {
        throw otype_file_error("GSUB table version not supported");
    }

    auto const script_list_bytes = bytes.subspan(gsl::narrow_cast<size_t>(*header.script_list_offset) * 2);
    auto feature_indices = otype_script_list_search(script_list_bytes, script, language);

    auto const feature_list_bytes = bytes.subspan(gsl::narrow_cast<size_t>(*header.feature_list_offset) * 2);
    auto const lookup_indices = otype_feature_table_search(feature_list_bytes, std::move(feature_indices), feature_tags);

    auto const lookup_list_bytes = bytes.subspan(gsl::narrow_cast<size_t>(*header.lookup_list_offset) * 2);

    otype_lookup_list_execute(
        lookup_list_bytes, lookup_indices, [&](std::span<std::byte const> sub_table_bytes, uint16_t lookup_type, uint16_t lookup_flag, uint16_t mark_filtering_set) {
            otype_GSUB_apply_sub_table(sub_table_bytes, GDEF_bytes, run, lookup_type, lookup_flag, mark_filtering_set);
        });
}
}
