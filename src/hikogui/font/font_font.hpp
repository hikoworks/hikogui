// Copyright Take Vos 2019-2022.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "glyph_metrics.hpp"
#include "glyph_atlas_info.hpp"
#include "font_weight.hpp"
#include "font_variant.hpp"
#include "font_metrics.hpp"
#include "font_char_map.hpp"
#include "font_id.hpp"
#include "../unicode/unicode.hpp"
#include "../i18n/i18n.hpp"
#include "../graphic_path/graphic_path.hpp"
#include "../utility/utility.hpp"
#include "../container/container.hpp"
#include <span>
#include <vector>
#include <map>
#include <string>

hi_export_module(hikogui.font : font);

hi_export namespace hi::inline v1 {

/*! A font.
 * This class has information on how to shape text and
 * get glyphs consisting of bezier contours.
 */
hi_export class font {
public:
    /** The family name as parsed from the font file.
     *
     * Examples: "Helvetica", "Times New Roman"
     */
    std::string family_name;

    /** The sub-family name as parsed from the font file.
     *
     * Examples: "Regular", "ItalicBold"
     */
    std::string sub_family_name;

    bool monospace = false;
    bool serif = false;
    font_style style = font_style::normal;
    bool condensed = false;
    font_weight weight = font_weight::regular;
    float optical_size = 12.0;

    /** A string representing the features of a font.
     * This will be a comma separated list of features, mostly tables like 'kern' and 'GPOS'.
     */
    std::string features;

    /** The metrics of a font.
     */
    font_metrics_em metrics;

    font() = default;
    virtual ~font() = default;
    font(font const&) = delete;
    font& operator=(font const&) = delete;
    font(font&&) = delete;
    font& operator=(font&&) = delete;

    /** Return if the font is loaded.
     *
     * @return true if the font is fully loaded, false if only metadata of the font is available.
     */
    [[nodiscard]] virtual bool loaded() const noexcept = 0;

    /** Get the glyph for a code-point.
     * @return glyph-id, or invalid when not found or error.
     */
    [[nodiscard]] virtual glyph_id find_glyph(char32_t c) const noexcept = 0;

    /** Get the glyphs for a grapheme.
     * 
     * @param g The grapheme to look up the glyph for.
     * @return a set of glyph-ids, or empty when the grapheme is not found in
     *         the font.
     */
    [[nodiscard]] lean_vector<glyph_id> find_glyphs(grapheme g) const
    {
        // Create a glyph_ids object for a single grapheme.
        auto r = lean_vector<glyph_id>{};

        // First try composed normalization
        for (auto const c : g.composed()) {
            if (auto const glyph_id = find_glyph(c)) {
                r.push_back(glyph_id);
            } else {
                r.clear();
                break;
            }
        }

        if (r.empty()) {
            // Now try decomposed normalization
            for (auto const c : g.decomposed()) {
                if (auto const glyph_id = find_glyph(c)) {
                    r.push_back(glyph_id);
                } else {
                    r.clear();
                    break;
                }
            }
        }

        return r;
    }

    /** Get glyphs of a run of graphemes.
     *
     * When a run of text has a relatively unique grapheme it may not be found
     * in the font. Therefor this function needs to be fast so it can be tried
     * on multiple fonts. This is why this function returns the glyphs as an
     * output argument, being able to reuse the allocation.
     * 
     * @param text The text to get the glyphs for from the same font.
     * @param out Glyphs for each grapheme in the text.
     * @return true on success, false when not all graphemes could be found in
     *         the font.
     */
    bool find_glyphs(gstring_view text, std::vector<lean_vector<glyph_id>> &out) const
    {
        out.clear();
        out.reserve(text.size());

        for (auto g : text) {
            if (auto glyph_ids = find_glyphs(g); not glyph_ids.empty()) {
                out.push_back(std::move(glyph_ids));
            } else {
                return false;
            }
        }

        assert(text.size() == out.size());
        return true;
    }

    /** Substitute a run glyphs in logical order.
     *
     * This function will change glyphs to better match the given script and language.
     * Some glyphs may be replaced, or combined (ligature) into a single glyph.
     * Glyphs that are removed will be replaced with empty glyphs (0xffff).
     *
     * The position of the glyphs will remain the same however, so that it is still
     * possible to match the glyphs with the original grapheme, and know which glyphs
     * are marks.
     *
     * @param inout A span of glyphs which may be replaced with glyphs that fit better
     *              with the script and language.
     * @param script The script of the glyph-run.
     * @param language The language of the glyph-run.
     */
    //virtual bool substitute_glyphs(std::span<lean_vector<glyph_id>> glyphs, iso script, iso language) const
    //{
    //}
//
    //virtual bool position_glyphs(std::vector<glyph_position> &out, std::span<lean_vector<glyph_id> const> glyphs, iso script, iso language) const
    //{
    //}

    /** Load a glyph into a path.
     * The glyph is directly loaded from the font file.
     *
     * @param glyph_id the id of a glyph inside the font.
     * @return The path loaded from the font file.
     * @throws std::exception If there was an error while loading the path.
     *         Recommend to disable the font on error.
     */
    [[nodiscard]] virtual graphic_path get_path(hi::glyph_id glyph_id) const = 0;

    /** Get the advance for a glyph.
     *
     * @param glyph_id The glyph to look up the advance for.
     * @return The advance for the glyph.
     * @throws std::exception If there was an error looking up the glyph.
     */
    [[nodiscard]] virtual unit::em_squares_f get_advance(hi::glyph_id glyph_id) const = 0;

    /** Load a glyph into a path.
     * The glyph is directly loaded from the font file.
     *
     * @param glyph_id the id of a glyph inside the font.
     * @param metrics The metrics constructed by the loader.
     * @param lookahead_glyph_id The id of a glyph to the right, needed for kerning.
     * @return true on success, false on error.
     */
    [[nodiscard]] virtual glyph_metrics get_metrics(hi::glyph_id glyph_id) const = 0;

    struct shape_run_result_type {
        /** The horizontal advance of each grapheme.
         */
        std::vector<float> advances;

        /** The number of glyphs used by each grapheme.
         */
        std::vector<size_t> glyph_count;

        /** The glyphs representing all the graphemes.
         *
         * There may be zero or more glyphs for each grapheme.
         * The difference may be due to having to add accent-glyphs
         * or merging glyphs into ligatures.
         */
        std::vector<glyph_id> glyphs;

        /** Position of each glyph, relative to the grapheme
         *
         * There is exactly one position for each glyph.
         */
        std::vector<point2> glyph_positions;

        /** The bounding rectangle for each glyph, relative to glyph_position.
         *
         * There is exactly one bounding rectangle for each glyph.
         */
        std::vector<aarectangle> glyph_rectangles;

        void reserve(size_t count) noexcept
        {
            advances.reserve(count);
            glyph_count.reserve(count);
            glyphs.reserve(count);
            glyph_positions.reserve(count);
            glyph_rectangles.reserve(count);
        }

        /** Scale and position the result of the run.
         *
         * @param s The font-size to scale by
         * @param x The horizontal position, calculated after scaling
         */
        void scale_and_offset(float s) noexcept
        {
            auto S = scale2{s};

            for (auto& tmp : advances) {
                tmp = s * tmp;
            }
            for (auto& tmp : glyph_positions) {
                tmp = S * tmp;
            }
            for (auto& tmp : glyph_rectangles) {
                tmp = S * tmp;
            }
        }
    };

    /** Shape a run of graphemes.
     *
     * A run of graphemes is a piece of text that is:
     * - from the same style,
     * - from the same font,
     * - from the same language and script, and
     * - on the same line.
     *
     * A run needs to be shaped by the font-file itself as it handles:
     * - language/script depended glyph substitution for ligatures, accents and cursive text.
     * - language/script depended glyph positioning for kerning, accents and cursive text.
     *
     * @param language The language of this run of graphemes.
     * @param script The script of this run of graphemes.
     * @param run The run of graphemes.
     * @return The glyphs and coordinates to display, and coordinates of grapheme for interaction.
     */
    [[nodiscard]] virtual shape_run_result_type shape_run(iso_639 language, iso_15924 script, gstring run) const = 0;

    glyph_atlas_info& atlas_info(glyph_id glyph) const
    {
        if (*glyph >= _glyph_atlas_table.size()) [[unlikely]] {
            _glyph_atlas_table.resize(*glyph + 1);
        }

        hi_axiom_bounds(*glyph, _glyph_atlas_table);
        return _glyph_atlas_table[*glyph];
    }

    [[nodiscard]] font_variant font_variant() const noexcept
    {
        return {weight, style};
    }

    [[nodiscard]] friend std::string to_string(font const& rhs) noexcept
    {
        return std::format(
            "{} - {}: style={}{}{}{}{}{}, features={}",
            rhs.family_name,
            rhs.sub_family_name,
            rhs.monospace ? 'M' : '_',
            rhs.serif ? 'S' : '_',
            rhs.style == font_style::italic ? 'I' : '_',
            rhs.condensed ? 'C' : '_',
            to_char(rhs.weight),
            rhs.optical_size,
            rhs.features);
    }

private:
    mutable std::vector<glyph_atlas_info> _glyph_atlas_table;
};

} // namespace hi::inline v1

// XXX #617 MSVC bug does not handle partial specialization in modules.
hi_export template<>
struct std::formatter<hi::font, char> : formatter<std::string_view, char> {
    auto format(hi::font const& t, auto& fc) const
    {
        return formatter<string_view, char>::format(to_string(t), fc);
    }
};
