// Copyright Take Vos 2022.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "text_style_set.hpp"
#include "text_style.hpp"
#include "../font/font.hpp"
#include "../unicode/unicode.hpp"
#include "../geometry/geometry.hpp"
#include "../units/units.hpp"
#include "../macros.hpp"

hi_export_module(hikogui.text.text_shaper_char);

hi_export namespace hi::inline v1 {

class text_shaper_char {
public:
    /** The grapheme.
     */
    hi::grapheme grapheme;

    unit::font_size_f font_size;

    /** The style of how to display the grapheme.
     */
    hi::text_style style;

    /** The scale to resize the font's size to match the physical display.
     */
    unit::pixel_density pixel_density;

    /** The glyph representing one or more graphemes.
     * The glyph will change during shaping of the text:
     *  1. The starter glyph, used for determining the width of the grapheme
     *     and the folding algorithm.
     *  2. The glyph representing a bracket may be replaced with a mirrored bracket
     *     by the bidi-algorithm.
     *  3. The glyph may be replaced by the font using the glyph-morphing algorithms
     *     for better continuation of cursive text and merging of graphemes into
     *     a ligature.
     */
    hi::font_glyph_ids glyphs;

    /** The glyph metrics of the current starter glyph.
     *
     * The metrics are scaled by `scale`.
     */
    hi::glyph_metrics metrics;

    /** The line number where this character is located, counting from top to bottom line.
     */
    size_t line_nr;

    /** The column number where the character is located on the line, counting from left to right in display order.
     */
    size_t column_nr;

    /** Position of the character.
     *
     * For a non-ligature this is the origin of the glyph, where the actual glyph
     * is located at `position + metrics.bounding_rectangle`.
     * For ligatures the position is moved based on the advance of each character within the ligature.
     */
    point2 position;

    /** The rectangle for this character.
     *
     * The rectangle is used for:
     *  - creating a selection box around the character.
     *  - creating cursors before, after and on the character.
     *  - converting mouse-position to character.
     *
     * The attributes of the rectangle are:
     *  - left side is equal to the position.x
     *  - The width is the advance of the character within the ligature.
     *    Or if the glyph is not a ligature the width is the same as the advance.
     *  - The bottom is at the descender
     *  - The top is at the ascender
     *
     * When multiple characters are converted to a ligature, the
     * rectangle of each of those characters occupies a
     * subsection of the ligature-glyph. In this case the left most
     * character will contain the ligature-glyph, and the rest of
     * the characters of the ligature will have empty glyphs.
     */
    aarectangle rectangle;

    /** The general category of this grapheme.
     */
    unicode_general_category general_category;

    /** The text direction for this glyph.
     *
     * This is needed to figure out where the location of the insert cursor is compared to the character.
     */
    unicode_bidi_class direction;

    /** The script of this character.
     * The script of the character is based on:
     * - The actual script of this unicode character, or if `unicode_script::Common`;
     * - The script of characters before/after this character in the same word, or if `unicode_script::Common`;
     * - The script passed during construction of the text_shaper.
     */
    iso_15924 script;

    /** The font size in pixels, rounded so that the x-height is rounded to the nearest pixel.
     */
    au::Quantity<unit::PixelsPerEm, float> scaled_font_size;

    /** The width used for this grapheme when folding lines.
     *
     * This width is based on the initial glyph's advance after converting the grapheme
     * using the text-style into a glyph. This width excludes kerning and glyph-morphing.
     */
    float width = 0.0f;

    /** Set to true if this glyph is a white space at the end of a line.
     */
    bool is_trailing_white_space = false;

    /** The glyph is the initial glyph.
     *
     * This flag is set to true after loading the initial glyph.
     * This flag is set to false when the glyph is replaced by the bidi-algorithm
     * or glyph-morphing.
     */
    bool glyph_is_initial = false;

    [[nodiscard]] text_shaper_char(hi::grapheme const& grapheme, unit::font_size_f const &font_size, text_style_set const& style, unit::pixel_density pixel_density) noexcept :
        grapheme(grapheme),
        font_size(font_size),
        style(style[grapheme.attributes()]),
        pixel_density(pixel_density),
        line_nr(std::numeric_limits<size_t>::max()),
        column_nr(std::numeric_limits<size_t>::max()),
        general_category(ucd_get_general_category(grapheme.starter()))
    {
    }

    /** Initialize the glyph based on the grapheme.
     *
     * @note The glyph is only initialized when `glyph_is_initial == false`.
     * @post `glyph`, `metrics` and `width` are modified. `glyph_is_initial` is set to true.
     */
    void initialize_glyph(hi::font_id font) noexcept
    {
        if (not glyph_is_initial) {
            set_glyph(find_glyph(font, grapheme));

            width = metrics.advance;
            glyph_is_initial = true;
        }
    }

    /** Initialize the glyph based on the grapheme.
     *
     * @note The glyph is only initialized when `glyph_is_initial == false`.
     * @post `glyph`, `metrics` and `width` are modified. `glyph_is_initial` is set to true.
     */
    void initialize_glyph() noexcept
    {
        return initialize_glyph(style.font_chain()[0]);
    }

    /** Called by the bidi-algorithm to mirror glyphs.
     *
     * The glyph is replaced with a glyph from the same font using the given code-point.
     *
     * @pre `glyph.num_grapheme == 1`.
     * @post `glyph` and `metrics` are modified. `glyph_is_initial` is set to false.
     * @note The `width` remains based on the original glyph.
     */
    void replace_glyph(char32_t code_point) noexcept
    {
        set_glyph(find_glyph(glyphs.font, code_point));
        glyph_is_initial = false;
    }

    /** Get the scaled font metrics for this character.
     */
    [[nodiscard]] font_metrics_px font_metrics() const noexcept
    {
        hi_axiom(not glyphs.font.empty());
        return scaled_font_size * glyphs.font->metrics;
    }

    [[nodiscard]] friend bool operator==(text_shaper_char const& lhs, char32_t const& rhs) noexcept
    {
        return lhs.grapheme == rhs;
    }

    [[nodiscard]] friend bool operator==(text_shaper_char const& lhs, char const& rhs) noexcept
    {
        return lhs.grapheme == rhs;
    }

private:
    /** Load metrics based on the loaded glyph.
     */
    void set_glyph(hi::font_glyph_ids&& new_glyphs) noexcept
    {
        glyphs = std::move(new_glyphs);
        hi_axiom(not glyphs.font.empty());
        scaled_font_size = round(font_size * style.scale() * pixel_density, glyphs.font_metrics().x_height);
        metrics = scaled_font_size.in(unit::pixels_per_em) * glyphs.front_glyph_metrics();
    }
};

} // namespace hi::inline v1
