// Copyright Take Vos 2022.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "text_shaper_char.hpp"
#include "../font/font.hpp"
#include "../unicode/unicode.hpp"
#include "../geometry/geometry.hpp"
#include "../macros.hpp"
#include <vector>

hi_export_module(hikogui.text.text_shaper_line);

hi_export namespace hi::inline v1 {

class text_shaper_line {
public:
    using iterator = std::vector<text_shaper_char>::iterator;
    using const_iterator = std::vector<text_shaper_char>::const_iterator;
    using column_vector = std::vector<iterator>;

    /** The first character in the line, in logical order.
     */
    iterator first;

    /** One beyond the last character in the line, in logical order.
     */
    iterator last;

    /** Iterators to the characters in the text.
     *
     * The Iterators are in display-order.
     */
    column_vector columns;

    /** The maximum metrics of the font of each glyph on this line.
     */
    font_metrics_px metrics;

    /** The maximum line spacing for each grapheme on this line.
    */
    float line_spacing = 1.0f;

    /** The maximum paragraph spacing for each grapheme on this line.
    */
    float paragraph_spacing = 1.5f;

    /** The line number of this line, counted from top to bottom.
     */
    size_t line_nr;

    /** Position of the base-line of this line.
     */
    float y;

    /** The rectangle of the line.
     *
     * The attributes of the rectangle are:
     *  - left: The rectangle.left() of the first character on the line.
     *  - right: The rectangle.right() of the last visible character on the line.
     *  - top: At the ascender of the line.
     *  - bottom: At the descender of the line.
     */
    aarectangle rectangle;

    /** The width of this line, excluding trailing white space, glyph morphing and kerning.
     */
    float width;

    /** Category of the last character on the line.
     *
     * Use to determine if this line ends in:
     *  - Zp: An explicit paragraph separator.
     *  - Zl: An explicit line separator.
     *  - *: A word-wrapped line. Need to add line-separators into the stream for bidi-algorithm.
     */
    unicode_general_category last_category;

    /** The writing direction of the paragraph.
     *
     * This value will be set the same on each line of a paragraph.
     */
    unicode_bidi_class paragraph_direction;

    /** Construct a line.
     *
     * @param line_nr The line number counting from top to bottom.
     * @param begin The first character of the text.
     * @param first The first character of the line.
     * @param last One beyond the last character of the line.
     * @param width The width of the line.
     * @param metrics The initial line metrics.
     */
    text_shaper_line(
        size_t line_nr,
        const_iterator begin,
        iterator first,
        iterator last,
        float width,
        font_metrics_px const& metrics) noexcept :
        first(first), last(last), columns(), metrics(metrics), line_nr(line_nr), y(0.0f), width(width), last_category()
    {
        auto last_visible_it = first;
        for (auto it = first; it != last; ++it) {
            // Reset the trailing white space marker.
            it->is_trailing_white_space = false;

            // Only calculate line metrics based on visible characters.
            // For example a paragraph separator is seldom available in a font.
            if (is_visible(it->general_category)) {
                this->metrics = max(metrics, it->font_metrics());
                this->line_spacing = std::max(this->line_spacing, it->style.line_spacing());
                this->paragraph_spacing = std::max(this->paragraph_spacing, it->style.paragraph_spacing());
                last_visible_it = it;
            }
        }

        if (first != last) {
            // Mark trailing whitespace as such
            for (auto it = last_visible_it + 1; it != last; ++it) {
                it->is_trailing_white_space = true;
            }

            last_category = (last - 1)->general_category;
        } else {
            last_category = unicode_general_category::Cn;
        }
    }

    [[nodiscard]] constexpr size_t size() const noexcept
    {
        return columns.size();
    }

    [[nodiscard]] constexpr iterator front() const noexcept
    {
        return columns.front();
    }

    [[nodiscard]] constexpr iterator back() const noexcept
    {
        return columns.back();
    }

    iterator operator[](size_t index) const noexcept
    {
        hi_assert_bounds(index, columns);
        return columns[index];
    }

    void layout(horizontal_alignment alignment, float min_x, float max_x, float sub_pixel_width) noexcept
    {
        // Reset the position and advance the glyphs.
        advance_glyphs(columns, y);

        // Calculate the precise width of the line.
        auto const[visible_width, num_internal_white_space] = calculate_precise_width(columns, paragraph_direction);

        // Align the glyphs for a given width. But keep the left side at x=0.0.
        align_glyphs(columns, alignment, paragraph_direction, max_x - min_x, visible_width, num_internal_white_space);

        // Move the glyphs to where the left side is.
        move_glyphs(columns, min_x);

        // Round the glyphs to sub-pixels to improve sharpness of rendered glyphs.
        round_glyph_positions(columns, sub_pixel_width);

        // Create the bounding rectangles around each glyph, for use to draw selection boxes/cursors and handle mouse control.
        create_bounding_rectangles(columns, y, metrics.ascender.in(unit::pixels), metrics.descender.in(unit::pixels));

        // Create a bounding rectangle around the visible part of the line.
        if (columns.empty()) {
            rectangle = {point2{0.0f, y - metrics.descender.in(unit::pixels)}, point2{1.0f, y + metrics.ascender.in(unit::pixels)}};
        } else {
            rectangle = columns.front()->rectangle | columns.back()->rectangle;
        }
    }

    /** Get the character nearest to position.
     *
     * @return An iterator to the character, and true if the position is after the character.
     */
    [[nodiscard]] std::pair<const_iterator, bool> get_nearest(point2 position) const noexcept
    {
        if (columns.empty()) {
            // This is the last line, so return an the iterator to the end-of-document.
            return {last, false};
        }

        auto column_it = std::lower_bound(columns.begin(), columns.end(), position.x(), [](auto const& char_it, auto const& x) {
            return char_it->rectangle.right() < x;
        });
        if (column_it == columns.end()) {
            column_it = columns.end() - 1;
        }

        auto char_it = *column_it;
        if (is_Zp_or_Zl(char_it->general_category)) {
            // Do not put the cursor on a paragraph separator or line separator.
            if (paragraph_direction == unicode_bidi_class::L) {
                if (column_it != columns.begin()) {
                    char_it = *--column_it;
                } else {
                    // If there is only a paragraph separator, place the cursor before it.
                    return {char_it, false};
                }
            } else {
                if (column_it + 1 != columns.end()) {
                    char_it = *++column_it;
                } else {
                    // If there is only a paragraph separator, place the cursor before it.
                    return {char_it, false};
                }
            }
        }

        auto const after = (char_it->direction == unicode_bidi_class::L) == position.x() > char_it->rectangle.center();
        return {char_it, after};
    }

private:
    static void advance_glyphs_run(
        point2& p,
        text_shaper_line::column_vector::iterator first,
        text_shaper_line::column_vector::iterator last) noexcept
    {
        hi_axiom(first != last);

        auto const char_it = *first;
        auto const font = char_it->glyphs.font;
        auto const script = char_it->script;
        auto const language = iso_639{};

        auto run = gstring{};
        run.reserve(std::distance(first, last));
        for (auto it = first; it != last; ++it) {
            run += (*it)->grapheme;
        }

        auto result = font->shape_run(language, script, run);
        result.scale_and_offset(char_it->scaled_font_size.in(unit::pixels_per_em));
        hi_axiom(result.advances.size() == run.size());
        hi_axiom(result.glyph_count.size() == run.size());

        auto grapheme_index = 0_uz;
        for (auto it = first; it != last; ++it, ++grapheme_index) {
            (*it)->position = p;

            p += vector2{result.advances[grapheme_index], 0.0f};
        }
    }

    /**
     */
    static void advance_glyphs(text_shaper_line::column_vector& columns, float y) noexcept
    {
        if (columns.empty()) {
            return;
        }

        auto p = point2{0.0f, y};

        auto run_start = columns.begin();
        for (auto it = run_start + 1; it != columns.end(); ++it) {
            auto const start_char_it = *run_start;
            auto const char_it = *it;

            auto const same_font = start_char_it->glyphs.font == char_it->glyphs.font;
            auto const same_style = start_char_it->style == char_it->style;
            auto const same_size = start_char_it->font_size == char_it->font_size;
            auto const same_language = true;
            auto const same_script = start_char_it->script == char_it->script;

            if (not(same_font and same_style and same_size and same_language and same_script)) {
                advance_glyphs_run(p, run_start, it);
                run_start = it;
            }
        }
        advance_glyphs_run(p, run_start, columns.end());
    }

    [[nodiscard]] static std::pair<float, size_t>
    calculate_precise_width(text_shaper_line::column_vector& columns, unicode_bidi_class paragraph_direction)
    {
        if (columns.empty()) {
            return {0.0f, 0_uz};
        }

        auto it = columns.begin();
        for (; it != columns.end(); ++it) {
            if (not(*it)->is_trailing_white_space) {
                break;
            }
        }
        auto const left_x = (*it)->position.x();

        auto right_x = left_x;
        auto num_white_space = 0_uz;
        for (; it != columns.end(); ++it) {
            if ((*it)->is_trailing_white_space) {
                // Stop at the first trailing white space.
                break;
            }

            right_x = (*it)->position.x() + (*it)->metrics.advance;
            if (not is_visible((*it)->general_category)) {
                ++num_white_space;
            }
        }

        auto const width = right_x - left_x;

        // Adjust the offset to left align on the first visible character.
        for (auto& char_it : columns) {
            char_it->position.x() -= left_x;
        }

        return {width, num_white_space};
    }

    static void move_glyphs(text_shaper_line::column_vector& columns, float offset) noexcept
    {
        for (auto const& char_it : columns) {
            char_it->position.x() += offset;
        }
    }

    [[nodiscard]] static bool align_glyphs_justified(
        text_shaper_line::column_vector& columns,
        float max_line_width,
        float visible_width,
        size_t num_internal_white_space) noexcept
    {
        if (num_internal_white_space == 0) {
            return false;
        }

        auto const extra_space = max_line_width - visible_width;
        if (extra_space > max_line_width * 0.25f) {
            return false;
        }

        auto const extra_space_per_whitespace = extra_space / num_internal_white_space;
        auto offset = 0.0f;
        for (auto const& char_it : columns) {
            char_it->position.x() += offset;

            // Add extra space for each white space in the visible part of the line. Leave the
            // sizes of trailing white space normal.
            if (not char_it->is_trailing_white_space and not is_visible(char_it->general_category)) {
                offset += extra_space_per_whitespace;
            }
        }

        return true;
    }

    static void align_glyphs(
        text_shaper_line::column_vector& columns,
        horizontal_alignment alignment,
        unicode_bidi_class paragraph_direction,
        float max_line_width,
        float visible_width,
        size_t num_internal_white_space) noexcept
    {
        if (alignment == horizontal_alignment::justified) {
            if (align_glyphs_justified(columns, max_line_width, visible_width, num_internal_white_space)) {
                return;
            }
        }

        if (alignment == horizontal_alignment::flush or alignment == horizontal_alignment::justified) {
            alignment = paragraph_direction == unicode_bidi_class::R ? horizontal_alignment::right : horizontal_alignment::left;
        }

        // clang-format off
    auto const offset =
        alignment == horizontal_alignment::left ? 0.0f :
        alignment == horizontal_alignment::right ? max_line_width - visible_width :
        (max_line_width - visible_width) * 0.5f;
        // clang-format on

        return move_glyphs(columns, offset);
    }

    static void round_glyph_positions(text_shaper_line::column_vector& columns, float sub_pixel_width) noexcept
    {
        auto const rcp_sub_pixel_width = 1.0f / sub_pixel_width;
        for (auto it : columns) {
            it->position.x() = std::round(it->position.x() * rcp_sub_pixel_width) * sub_pixel_width;
        }
    }

    static void
    create_bounding_rectangles(text_shaper_line::column_vector& columns, float y, float ascender, float descender) noexcept
    {
        for (auto it = columns.begin(); it != columns.end(); ++it) {
            auto const next_it = it + 1;
            auto const char_it = *it;
            if (next_it == columns.end()) {
                char_it->rectangle = {
                    point2{char_it->position.x(), y - descender},
                    point2{char_it->position.x() + char_it->metrics.advance, y + ascender}};
            } else {
                auto const next_char_it = *next_it;

                if (next_char_it->position.x() <= char_it->position.x()) {
                    // Somehow the next character is overlapping with the current character, use the advance instead.
                    char_it->rectangle = {
                        point2{char_it->position.x(), y - descender},
                        point2{char_it->position.x() + char_it->metrics.advance, y + ascender}};
                } else {
                    char_it->rectangle = {
                        point2{char_it->position.x(), y - descender}, point2{next_char_it->position.x(), y + ascender}};
                }
            }
        }
    }
};

} // namespace hi::inline v1
