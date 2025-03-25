// Copyright Take Vos 2019-2022.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "font_font.hpp"
#include "otype_utilities.hpp"
#include "otype_sfnt.hpp"
#include "otype_cmap.hpp"
#include "otype_glyf.hpp"
#include "otype_head.hpp"
#include "otype_hhea.hpp"
#include "otype_hmtx.hpp"
#include "otype_kern.hpp"
#include "otype_loca.hpp"
#include "otype_maxp.hpp"
#include "otype_name.hpp"
#include "otype_os2.hpp"
#include "otype_GSUB.hpp"
#include "font_char_map.hpp"
#include "../file/file_view.hpp"
#include "../graphic_path/graphic_path.hpp"
#include "../telemetry/telemetry.hpp"
#include "../utility/utility.hpp"
#include <memory>
#include <filesystem>

hi_export_module(hikogui.font.true_type_font);

hi_export namespace hi::inline v1 {
hi_export class true_type_font final : public font {
public:
    true_type_font(font_id id, std::filesystem::path const& path) : font(id), _path(path), _view(file_view{path})
    {
        ++global_counter<"ttf:map">;
        try {
            _bytes = as_span<std::byte const>(_view);
            parse_font_directory(_bytes);

            // Clear the view to reclaim resources.
            _view = {};
            _bytes = {};
            ++global_counter<"ttf:unmap">;

        } catch (std::exception const& e) {
            throw parse_error(std::format("{}: Could not parse font directory.\n{}", path.string(), e.what()));
        }
    }

    true_type_font() = delete;
    true_type_font(true_type_font const& other) = delete;
    true_type_font& operator=(true_type_font const& other) = delete;
    true_type_font(true_type_font&& other) = delete;
    true_type_font& operator=(true_type_font&& other) = delete;
    ~true_type_font() = default;

    [[nodiscard]] bool loaded() const noexcept override
    {
        return to_bool(_view);
    }

    [[nodiscard]] glyph_id find_glyph(char32_t c) const noexcept override
    {
        load_view();
        return otype_cmap_find(_cmap_table_bytes, c);
    }

    [[nodiscard]] graphic_path get_path(hi::glyph_id glyph_id) const override
    {
        load_view();

        hi_check(*glyph_id < num_glyphs, "glyph_id is not valid in this font.");

        auto const glyph_bytes = otype_loca_get(_loca_table_bytes, _glyf_table_bytes, glyph_id, _loca_is_offset32);

        if (otype_glyf_is_compound(glyph_bytes)) {
            auto r = graphic_path{};

            for (auto const& component : otype_glyf_get_compound(glyph_bytes, _em_scale)) {
                auto component_path = component.scale * get_path(component.glyph_id);

                if (component.use_points) {
                    auto const compound_point = hi_check_at(r.points, component.compound_point_index).p;
                    auto const component_point = hi_check_at(component_path.points, component.component_point_index).p;
                    auto const offset = translate2{compound_point - component_point};
                    component_path = offset * component_path;
                } else {
                    component_path = translate2{component.offset} * component_path;
                }

                r += component_path;
            }
            return r;

        } else {
            return otype_glyf_get_path(glyph_bytes, _em_scale);
        }
    }

    [[nodiscard]] unit::em_squares_f get_advance(hi::glyph_id glyph_id) const override
    {
        load_view();

        hi_check(*glyph_id < num_glyphs, "glyph_id is not valid in this font.");
        auto const [advance_width, left_side_bearing] =
            otype_hmtx_get(_hmtx_table_bytes, glyph_id, _num_horizontal_metrics, _em_scale);
        return advance_width;
    }

    [[nodiscard]] glyph_metrics get_metrics(hi::glyph_id glyph_id) const override
    {
        load_view();

        hi_check(*glyph_id < num_glyphs, "glyph_id is not valid in this font.");

        auto const glyph_bytes = otype_loca_get(_loca_table_bytes, _glyf_table_bytes, glyph_id, _loca_is_offset32);

        if (otype_glyf_is_compound(glyph_bytes)) {
            for (auto const& component : otype_glyf_get_compound(glyph_bytes, _em_scale)) {
                if (component.use_for_metrics) {
                    return get_metrics(component.glyph_id);
                }
            }
        }

        auto r = glyph_metrics{};
        r.bounding_rectangle = otype_glyf_get_bounding_box(glyph_bytes, _em_scale);
        auto const [advance_width, left_side_bearing] =
            otype_hmtx_get(_hmtx_table_bytes, glyph_id, _num_horizontal_metrics, _em_scale);

        r.advance = advance_width.in(unit::em_squares);
        r.left_side_bearing = left_side_bearing.in(unit::em_squares);
        r.right_side_bearing =
            (advance_width - (left_side_bearing + unit::em_squares(r.bounding_rectangle.width()))).in(unit::em_squares);
        return r;
    }

    [[nodiscard]] shape_run_result_type shape_run(iso_639 language, iso_15924 script, gstring run) const override
    {
        auto r = shape_run_basic(run);

        // Glyphs should be morphed only once.
        // auto morphed = false;
        // Glyphs should be positioned only once.
        auto positioned = false;

        if (not positioned and not _kern_table_bytes.empty()) {
            try {
                shape_run_kern(r);
                positioned = true;
            } catch (std::exception const& e) {
                hi_log_error("Turning off invalid 'kern' table in font '{} {}': {}", family_name, sub_family_name, e.what());
                _kern_table_bytes = {};
            }
        }

        return r;
    }

private:
    /** The url to retrieve the view.
     */
    std::filesystem::path _path;

    /** The resource view of the font-file.
     *
     * This view may be reset if there is a path available.
     */
    mutable file_view _view;

    float OS2_x_height = 0;
    float OS2_cap_height = 0;

    float _em_scale;

    uint16_t _num_horizontal_metrics;

    int num_glyphs;
    mutable std::span<std::byte const> _bytes;
    mutable std::span<std::byte const> _loca_table_bytes;
    mutable std::span<std::byte const> _glyf_table_bytes;
    mutable std::span<std::byte const> _hmtx_table_bytes;
    mutable std::span<std::byte const> _kern_table_bytes;
    mutable std::span<std::byte const> _GSUB_table_bytes;
    mutable std::span<std::byte const> _cmap_table_bytes;
    bool _loca_is_offset32;

    void cache_tables(std::span<std::byte const> bytes) const
    {
        _loca_table_bytes = otype_sfnt_search<"loca">(bytes);
        _glyf_table_bytes = otype_sfnt_search<"glyf">(bytes);
        _hmtx_table_bytes = otype_sfnt_search<"hmtx">(bytes);
        _cmap_table_bytes = otype_sfnt_search<"cmap">(bytes);

        // Optional tables.
        _kern_table_bytes = otype_sfnt_search<"kern">(bytes);
        _GSUB_table_bytes = otype_sfnt_search<"GSUB">(bytes);
    }

    void load_view() const noexcept
    {
        if (_view) {
            [[likely]] return;
        }

        _view = file_view{_path};
        _bytes = as_span<std::byte const>(_view);
        ++global_counter<"ttf:map">;
        cache_tables(_bytes);
    }

    /** Parses the directory table of the font file.
     *
     * This function is called by the constructor to set up references
     * inside the file for each table.
     */
    void parse_font_directory(std::span<std::byte const> bytes)
    {
        if (auto head_bytes = otype_sfnt_search<"head">(bytes); not head_bytes.empty()) {
            auto head = otype_head_parse(head_bytes);
            _loca_is_offset32 = head.loca_is_offset32;
            _em_scale = head.em_scale;
        }

        if (auto name_bytes = otype_sfnt_search<"name">(bytes); not name_bytes.empty()) {
            auto names = otype_name_get_family(name_bytes);
            family_name = std::move(names.family_name);
            sub_family_name = std::move(names.sub_family_name);
        }

        if (auto maxp_bytes = otype_sfnt_search<"maxp">(bytes); not maxp_bytes.empty()) {
            auto maxp = otype_maxp_parse(maxp_bytes);
            num_glyphs = maxp.num_glyphs;
        }

        if (auto hhea_bytes = otype_sfnt_search<"hhea">(bytes); not hhea_bytes.empty()) {
            auto hhea = otype_hhea_parse(hhea_bytes, _em_scale);
            metrics.ascender = unit::em_squares(hhea.ascender);
            metrics.descender = unit::em_squares(-hhea.descender);
            metrics.line_gap = unit::em_squares(hhea.line_gap);
            _num_horizontal_metrics = hhea.number_of_h_metrics;
        }

        if (auto os2_bytes = otype_sfnt_search<"OS/2">(bytes); not os2_bytes.empty()) {
            auto os2 = otype_parse_os2(os2_bytes, _em_scale);
            weight = os2.weight;
            condensed = os2.condensed;
            serif = os2.serif;
            monospace = os2.monospace;
            style = os2.italic ? font_style::italic : font_style::normal;
            OS2_x_height = os2.x_height;
            OS2_cap_height = os2.cap_height;
        }

        // Tables need to be cached, before querying them. Such as the
        // find_glyph() function below.
        cache_tables(bytes);

        // Parsing the weight, italic and other features from the sub-family-name
        // is much more reliable than the explicit data in the OS/2 table.
        // Only use the OS/2 data as a last resort.
        auto name_lower = to_lower(family_name + " " + sub_family_name);
        if (name_lower.find("italic") != std::string::npos) {
            style = font_style::italic;
        }

        if (name_lower.find("oblique") != std::string::npos) {
            style = font_style::oblique;
        }

        if (name_lower.find("condensed") != std::string::npos) {
            condensed = true;
        }

        if (name_lower.find("mono") != std::string::npos or name_lower.find("console") != std::string::npos or
            name_lower.find("code") != std::string::npos) {
            monospace = true;
        }

        if (name_lower.find("sans") != std::string::npos) {
            serif = false;
        } else if (name_lower.find("serif") != std::string::npos) {
            serif = true;
        }

        if (name_lower.find("regular") != std::string::npos or name_lower.find("medium") != std::string::npos) {
            weight = font_weight::regular;
        } else if (
            name_lower.find("extra light") != std::string::npos or name_lower.find("extra-light") != std::string::npos or
            name_lower.find("extralight") != std::string::npos) {
            weight = font_weight::extra_light;
        } else if (
            name_lower.find("extra black") != std::string::npos or name_lower.find("extra-black") != std::string::npos or
            name_lower.find("extrablack") != std::string::npos) {
            weight = font_weight::extra_black;
        } else if (
            name_lower.find("extra bold") != std::string::npos or name_lower.find("extra-bold") != std::string::npos or
            name_lower.find("extrabold") != std::string::npos) {
            weight = font_weight::extra_bold;
        } else if (name_lower.find("thin") != std::string::npos) {
            weight = font_weight::thin;
        } else if (name_lower.find("light") != std::string::npos) {
            weight = font_weight::light;
        } else if (name_lower.find("bold") != std::string::npos) {
            weight = font_weight::bold;
        } else if (name_lower.find("black") != std::string::npos) {
            weight = font_weight::black;
        }

        // Figure out the features.
        features.clear();
        if (not _kern_table_bytes.empty()) {
            features += "kern,";
        }
        if (not _GSUB_table_bytes.empty()) {
            features += "GSUB,";
        }

        if (OS2_x_height > 0.0f) {
            metrics.x_height = unit::em_squares(OS2_x_height);
        } else {
            auto const glyph_id = find_glyph('x');
            if (glyph_id) {
                metrics.x_height = unit::em_squares(get_metrics(glyph_id).bounding_rectangle.height());
            }
        }

        if (OS2_cap_height > 0.0f) {
            metrics.cap_height = unit::em_squares(OS2_cap_height);
        } else {
            auto const glyph_id = find_glyph('H');
            if (glyph_id) {
                metrics.cap_height = unit::em_squares(get_metrics(glyph_id).bounding_rectangle.height());
            }
        }

        auto const glyph_id = find_glyph('8');
        if (glyph_id) {
            metrics.digit_advance = unit::em_squares(get_metrics(glyph_id).advance);
        }
    }

    /** Shape the given text with very basic rules.
     */
    [[nodiscard]] font::shape_run_result_type shape_run_basic(gstring run) const
    {
        auto r = font::shape_run_result_type{};
        r.reserve(run.size());

        for (auto const grapheme : run) {
            auto const glyphs = find_glyphs(grapheme);

            // At this point ligature substitution has not been done. So we should
            // have at least one glyph per grapheme.
            hi_axiom(not glyphs.empty());
            auto const base_glyph_id = glyphs.front();
            auto const base_glyph_metrics = get_metrics(base_glyph_id);

            r.advances.push_back(base_glyph_metrics.advance);
            r.glyph_count.push_back(glyphs.size());

            // Store information of the base-glyph
            r.glyphs.push_back(base_glyph_id);
            r.glyph_positions.push_back(point2{});
            r.glyph_rectangles.push_back(base_glyph_metrics.bounding_rectangle);

            // Position the mark-glyphs.
            auto glyph_position = point2{base_glyph_metrics.advance, 0.0f};
            for (auto i = 1_uz; i != glyphs.size(); ++i) {
                auto const glyph_id = glyphs[i];

                auto const glyph_metrics = get_metrics(glyph_id);

                r.glyphs.push_back(glyph_id);
                r.glyph_positions.push_back(glyph_position);
                r.glyph_rectangles.push_back(glyph_metrics.bounding_rectangle);

                glyph_position.x() += glyph_metrics.advance;
            }
        }
        return r;
    }

    void shape_run_kern(font::shape_run_result_type& shape_result) const
    {
        auto const num_graphemes = shape_result.advances.size();

        auto prev_base_glyph_id = hi::glyph_id{};
        auto glyph_index = 0_uz;
        for (auto grapheme_index = 0_uz; grapheme_index != num_graphemes; ++grapheme_index) {
            // Kerning is done between base-glyphs of consecutive graphemes.
            // Marks should be handled by the Unicode mark positioning algorithm.
            // Or by the more stateful GPOS table.
            auto const base_glyph_id = shape_result.glyphs[glyph_index];

            if (prev_base_glyph_id) {
                auto const kerning = otype_kern_find(_kern_table_bytes, prev_base_glyph_id, base_glyph_id, _em_scale);

                hi_axiom(grapheme_index != 0);
                shape_result.advances[grapheme_index - 1] += kerning.x();
            }

            glyph_index += shape_result.glyph_count[grapheme_index];
        }
    }
};

} // namespace hi::inline v1
