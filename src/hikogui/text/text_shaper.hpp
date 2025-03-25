// Copyright Take Vos 2021-2022.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "text_shaper_char.hpp"
#include "text_shaper_line.hpp"
#include "text_cursor.hpp"
#include "text_style_set.hpp"
#include "../layout/layout.hpp"
#include "../font/font.hpp"
#include "../geometry/geometry.hpp"
#include "../unicode/unicode.hpp"
#include "../units/units.hpp"
#include "../macros.hpp"
#include <vector>
#include <tuple>
#include <coroutine>

hi_export_module(hikogui.text.text_shaper);

hi_export namespace hi::inline v1 {
/** Text shaper.
 *
 * This class takes text as a set of graphemes attributed with font, size, style and color.
 *
 * Steps:
 *  1. Load default glyphs and metrics scaled to the font-size of each glyph.
 *  2. Fold default glyphs to a certain width by inserting line-separators.
 *  3. Run unicode bidirectional algorithm.
 *  4. Reload glyphs and metrics of any brackets.
 *  5. Morph glyphs.
 *  6. Position glyphs including kerning and justification.
 *
 */
class text_shaper {
public:
    using char_vector = std::vector<text_shaper_char>;
    using char_iterator = char_vector::iterator;
    using char_const_iterator = char_vector::const_iterator;
    using char_reference = char_vector::reference;
    using char_const_reference = char_vector::const_reference;
    using line_vector = std::vector<text_shaper_line>;
    using line_iterator = line_vector::iterator;
    using line_const_iterator = line_vector::const_iterator;

    constexpr text_shaper() noexcept = default;
    constexpr text_shaper(text_shaper const&) noexcept = default;
    constexpr text_shaper(text_shaper&&) noexcept = default;
    constexpr text_shaper& operator=(text_shaper const&) noexcept = default;
    constexpr text_shaper& operator=(text_shaper&&) noexcept = default;

    /** Construct a text_shaper with a text and alignment.
     *
     * The constructor will load all the default glyphs for the text.
     *
     * Horizontal alignment is done for each line independent of the writing direction.
     * This allows labels to remain aligned in the same direction on the user-interface
     * even when the labels have translations in different languages.
     *
     * Label widgets should flip the alignment passed to the text shaper when the
     * user interface is mirrored.
     *
     * Text edit fields may want to change the alignment of the text depending on the
     * dominant writing direction, for more natural typing.
     *
     * Vertical alignment of the text determines what y=0 means:
     *  - top: y = 0 is the base-line of the first line, all other lines are at y < 0.
     *  - bottom: y = 0 is the base-line of the last line, all other lines are at y > 0.
     *  - middle, odd: y = 0 is the base-line of the middle line.
     *  - middle, even: y = 0 is half way between the base-lines of the middle two lines.
     *
     * @param text The text as a vector of attributed graphemes.
     *             Use U+2029 as paragraph separator, and if needed U+2028 as line separator.
     * @param style The initial text-style to use to display the text.
     * @param pixel_density The pixel density of the current display.
     * @param alignment The alignment how to align the text.
     * @param text_direction The default text direction when it can not be deduced from the text.
     * @param script The script of the text.
     */
    [[nodiscard]] text_shaper(
        gstring const& text,
        unit::font_size_f const& font_size,
        text_style_set const& style,
        unit::pixel_density pixel_density,
        hi::horizontal_alignment alignment,
        bool left_to_right,
        iso_15924 script = iso_15924{"Zyyy"}) noexcept :
        _bidi_context(left_to_right ? unicode_bidi_class::L : unicode_bidi_class::R),
        _pixel_density(pixel_density),
        _alignment(alignment),
        _script(script)
    {
        auto const font = style.front().font_chain()[0];
        _initial_line_metrics = font_size * style.front().scale() * _pixel_density * font->metrics;
        _line_spacing = style.front().line_spacing();
        _paragraph_spacing = style.front().paragraph_spacing();

        _text.reserve(text.size());
        for (auto const& c : text) {
            auto const clean_c = c == '\n' ? grapheme{unicode_PS} : c;

            auto& tmp = _text.emplace_back(clean_c, font_size, style, _pixel_density);
            tmp.initialize_glyph(font);
        }

        _text_direction = unicode_bidi_direction(
            _text.begin(),
            _text.end(),
            [](text_shaper::char_const_reference it) {
                return it.grapheme.starter();
            },
            _bidi_context);

        _line_break_opportunities = unicode_line_break(_text.begin(), _text.end(), [](auto const& c) -> decltype(auto) {
            return c.grapheme.starter();
        });

        _line_break_widths.reserve(text.size());
        for (auto const& c : _text) {
            _line_break_widths.push_back(is_visible(c.general_category) ? c.width : -c.width);
        }

        _word_break_opportunities = unicode_word_break(_text.begin(), _text.end(), [](auto const& c) -> decltype(auto) {
            return c.grapheme.starter();
        });

        _sentence_break_opportunities = unicode_sentence_break(_text.begin(), _text.end(), [](auto const& c) -> decltype(auto) {
            return c.grapheme.starter();
        });

        resolve_script();
    }

    [[nodiscard]] text_shaper(
        std::string_view text,
        unit::font_size_f const& font_size,
        text_style_set const& style,
        unit::pixel_density pixel_density,
        hi::horizontal_alignment alignment,
        bool left_to_right,
        iso_15924 script = iso_15924{"Zyyy"}) noexcept :
        text_shaper(to_gstring(text), font_size, style, pixel_density, alignment, left_to_right, script)
    {
    }

    [[nodiscard]] bool empty() const noexcept
    {
        return _text.empty();
    }

    [[nodiscard]] size_t size() const noexcept
    {
        return _text.size();
    }

    [[nodiscard]] char_iterator begin() noexcept
    {
        return _text.begin();
    }

    [[nodiscard]] char_const_iterator begin() const noexcept
    {
        return _text.begin();
    }

    [[nodiscard]] char_const_iterator cbegin() const noexcept
    {
        return _text.cbegin();
    }

    [[nodiscard]] char_iterator end() noexcept
    {
        return _text.end();
    }

    [[nodiscard]] char_const_iterator end() const noexcept
    {
        return _text.end();
    }

    [[nodiscard]] char_const_iterator cend() const noexcept
    {
        return _text.cend();
    }

    auto const& lines() const noexcept
    {
        return _lines;
    }

    

    struct bounds_result {
        // p0: x=position of the left most glyph x = 0.0
        // p0: y=position of the baseline of the bottom-line of text. y <= 0
        // p3: x=position of right most glyphs + advance
        // p3: y=position of the cap-height of the top-line of text. y > 0
        // y=0.0 is the baseline of the top-line of text.
        aarectangle bounds;

        // Position of the baseline of the middle-line of text.
        // Or the average baseline of the two middle lines.
        float middle_baseline;

        // Cap-height of the middle-line of text.
        float middle_cap_height;

        // Length of the ascender of the top-line of text.
        float top_ascender;

        // Length of the descender of the bottom-line of text, positive number.
        float bottom_descender;
    };

    /** Get bounding rectangle.
     *
     * It will estimate the width and height based on the glyphs before
     * glyph-morphing and kerning and fold the lines using the unicode line
     * breaking algorithm to the @a max_line_width.
     *
     * The returned values from this function are used to determine the
     * locations of features of the shaped text:
     *
     * @param maximum_line_width The maximum line width allowed, this may be
     *                           infinite to determine the natural text size
     *                           without folding.
     * @return bounding box, middle-baseline, middle-capheight, top-ascender,
     * bottom-descender.
     */
    [[nodiscard]] bounds_result bounds(float maximum_line_width) noexcept
    {
        auto const rectangle = aarectangle{
            point2{0.0f, std::numeric_limits<float>::lowest()}, point2{maximum_line_width, std::numeric_limits<float>::max()}};

        auto const lines = make_lines(rectangle, 0.0f, extent2{1.0f, 1.0f});
        assert(not lines.empty());

        auto max_width = 0.0f;
        for (auto& line : lines) {
            inplace_max(max_width, line.width);
        }
        max_width = std::ceil(max_width);

        auto r = bounds_result{};
        r.bounds =
            aarectangle{point2{0.0f, lines.back().y}, point2{max_width, ceil_in(unit::pixels, lines.front().metrics.cap_height)}};

        if (lines.size() % 2 == 1) {
            auto const& line = lines[lines.size() / 2];
            r.middle_baseline = line.y;
            r.middle_cap_height = ceil_in(unit::pixels, line.metrics.cap_height);
        } else {
            assert(lines.size() >= 2);
            r.middle_baseline = std::round((lines[lines.size() / 2].y + lines[lines.size() / 2 - 1].y) / 2.0f);
            r.middle_cap_height = ceil_in(unit::pixels, (lines[lines.size() / 2].metrics.cap_height + lines[lines.size() / 2 - 1].metrics.cap_height) / 2.0f);
        }

        r.top_ascender = ceil_in(unit::pixels, lines.front().metrics.ascender);
        r.bottom_descender = ceil_in(unit::pixels, lines.back().metrics.descender);
        return r;
    }

    /** Layout the lines of the text.
     *
     * It will estimate the width and height based on the glyphs before glyph-morphing and kerning
     * and fold the lines using the unicode line breaking algorithm to the width of the @a rectangle.
     *
     * @post The lines have been laid out.
     * @param rectangle The rectangle to position the glyphs in.
     * @param baseline The position of the recommended base-line.
     * @param sub_pixel_size The size of a sub-pixel in device-independent-pixels.
     * @param line_spacing The scaling of the spacing between lines (default: 1.0).
     * @param paragraph_spacing The scaling of the spacing between paragraphs (default: 1.5).
     */
    void layout(aarectangle rectangle, float baseline, extent2 sub_pixel_size) noexcept
    {
        _rectangle = rectangle;
        _lines = make_lines(rectangle, baseline, sub_pixel_size);
        hi_assert(not _lines.empty());
        position_glyphs(rectangle, sub_pixel_size);
    }

    /** The rectangle used when laying out the text.
     */
    [[nodiscard]] aarectangle rectangle() const noexcept
    {
        return _rectangle;
    }

    /** Get the text-direction as a whole.
     */
    [[nodiscard]] unicode_bidi_class text_direction() const noexcept
    {
        return _text_direction;
    }

    /** Get the resolved alignment of the text.
     *
     * This is the alignment when taking into account the direction of the text
     * and the direction of the selected language.
     */
    [[nodiscard]] alignment resolved_alignment() const noexcept
    {
        return resolve(_alignment, _text_direction == unicode_bidi_class::L);
    }

    /** Get the character at index in logical order.
     *
     * @note This function checks for underflow and overflow of index and always returns an iterator
     *       between `begin()` and `end()` inclusive.
     * @param index The index in the text.
     * @return Iterator to the character.
     */
    [[nodiscard]] char_const_iterator get_it(size_t index) const noexcept
    {
        if (static_cast<ptrdiff_t>(index) < 0) {
            return begin();
        } else if (index >= size()) {
            return end();
        }

        return begin() + index;
    }

    /** Get the character at the cursor.
     *
     * @note This function checks for underflow and overflow of cursor and always returns an iterator
     *       between `begin()` and `end()` inclusive.
     * @param cursor The cursor in the text.
     * @return Iterator to the character.
     */
    [[nodiscard]] char_const_iterator get_it(text_cursor cursor) const noexcept
    {
        return get_it(cursor.index());
    }

    /** Get the character at column and row in display order.
     *
     * @note This function checks for underflow and overflow of column and row and always returns an iterator
     *       between `begin()` and `end()` inclusive.
     * @param column_nr The column
     * @param line_nr The row
     * @return Iterator to the character.
     */
    [[nodiscard]] char_const_iterator get_it(size_t column_nr, size_t line_nr) const noexcept
    {
        hi_assert(not _lines.empty());

        if (static_cast<ptrdiff_t>(line_nr) < 0) {
            return begin();
        } else if (line_nr >= _lines.size()) {
            return end();
        }

        auto const left_of_line = static_cast<ptrdiff_t>(column_nr) < 0;
        auto const right_of_line = column_nr >= _lines[line_nr].size();

        if (left_of_line or right_of_line) {
            auto const ltr = _lines[line_nr].paragraph_direction == unicode_bidi_class::L;
            auto const go_up = left_of_line == ltr;
            if (go_up) {
                // Go to line above.
                if (static_cast<ptrdiff_t>(--line_nr) < 0) {
                    return begin();
                } else {
                    // Go to end of line above.
                    return _lines[line_nr].paragraph_direction == unicode_bidi_class::L ? _lines[line_nr].back() :
                                                                                          _lines[line_nr].front();
                }

            } else {
                // Go to the line below.
                if (++line_nr >= _lines.size()) {
                    return end();
                } else {
                    // Go to begin of line below.
                    return _lines[line_nr].paragraph_direction == unicode_bidi_class::L ? _lines[line_nr].front() :
                                                                                          _lines[line_nr].back();
                }
            }
        }

        return _lines[line_nr][column_nr];
    }

    /** Get the character at column and row in display order.
     *
     * @note This function checks for underflow and overflow of column and row and always returns an iterator
     *       between `begin()` and `end()` inclusive.
     * @param column_row The column, row packed in a `std::pair`.
     * @return Iterator to the character.
     */
    [[nodiscard]] char_const_iterator get_it(std::pair<size_t, size_t> column_row) const noexcept
    {
        return get_it(column_row.first, column_row.second);
    }

    /** Get the column and line of a character.
     *
     * @param it The iterator to the character, or `end()`.
     * @return The (column, row) packed in a `std::pair`.
     */
    [[nodiscard]] std::pair<size_t, size_t> get_column_line(text_shaper::char_const_iterator it) const noexcept
    {
        if (it != end()) {
            return {it->column_nr, it->line_nr};
        } else {
            hi_assert(not _lines.empty());
            return {_lines.size() - 1, _lines.back().size()};
        }
    }

    /** Get the column and line of a character.
     *
     * @param index The index of the character in logical order.
     * @return The (column, row) packed in a `std::pair`.
     */
    [[nodiscard]] std::pair<size_t, size_t> get_column_line(size_t index) const noexcept
    {
        return get_column_line(get_it(index));
    }

    /** Get the column and line of a character.
     *
     * @param cursor The cursor to the character.
     * @return The (column, row) packed in a `std::pair`.
     */
    [[nodiscard]] std::pair<size_t, size_t> get_column_line(text_cursor cursor) const noexcept
    {
        return get_column_line(cursor.index());
    }

    /** Get the index of the character in logical order.
     *
     * @param it The iterator to the character or `end()`.
     * @return The index in logical order.
     */
    [[nodiscard]] size_t get_index(text_shaper::char_const_iterator it) const noexcept
    {
        return narrow_cast<size_t>(std::distance(begin(), it));
    }

    /** Get the cursor at the beginning of the document.
     *
     * @return The cursor at the beginning of the document.
     */
    [[nodiscard]] text_cursor get_begin_cursor() const noexcept
    {
        return {};
    }

    /** Get the cursor at the end of the document.
     *
     * @return The cursor at the end of the document.
     */
    [[nodiscard]] text_cursor get_end_cursor() const noexcept
    {
        return text_cursor{size() - 1, true}.resize(size());
    }

    /** Get the cursor before the character in logical order.
     *
     * @param index The index to the character or one beyond.
     * @return A cursor before the character in logical order.
     */
    [[nodiscard]] text_cursor get_before_cursor(size_t index) const noexcept
    {
        return text_cursor{index, false}.resize(size());
    }

    /** Get the cursor after the character in logical order.
     *
     * @param index The index to the character or one beyond.
     * @return A cursor after the character in logical order.
     */
    [[nodiscard]] text_cursor get_after_cursor(size_t index) const noexcept
    {
        return text_cursor{index, true}.resize(size());
    }

    /** Get the cursor before the character in logical order.
     *
     * @param it The iterator to the character or `end()`.
     * @return A cursor before the character in logical order.
     */
    [[nodiscard]] text_cursor get_before_cursor(text_shaper::char_const_iterator it) const noexcept
    {
        return get_before_cursor(get_index(it));
    }

    /** Get the cursor after the character in logical order.
     *
     * @param it The iterator to the character or `end()`.
     * @return A cursor after the character in logical order.
     */
    [[nodiscard]] text_cursor get_after_cursor(text_shaper::char_const_iterator it) const noexcept
    {
        return get_after_cursor(get_index(it));
    }

    /** Get the cursor left of the character in display order.
     *
     * @param it The iterator to the character or `end()`.
     * @return A cursor left of the character in display order.
     */
    [[nodiscard]] text_cursor get_left_cursor(text_shaper::char_const_iterator it) const noexcept
    {
        if (it != end()) {
            if (it->direction == unicode_bidi_class::L) {
                return get_before_cursor(it);
            } else {
                return get_after_cursor(it);
            }
        } else {
            return get_end_cursor();
        }
    }

    /** Get the cursor right of the character in display order.
     *
     * @param it The iterator to the character or `end()`.
     * @return A cursor right of the character in display order.
     */
    [[nodiscard]] text_cursor get_right_cursor(text_shaper::char_const_iterator it) const noexcept
    {
        if (it != end()) {
            if (it->direction == unicode_bidi_class::L) {
                return get_after_cursor(it);
            } else {
                return get_before_cursor(it);
            }
        } else {
            return get_end_cursor();
        }
    }

    /** Check if the cursor is on the left side of the character in display order.
     *
     * @param cursor The cursor to query.
     * @return True if the cursor is on the left of the character.
     */
    [[nodiscard]] bool is_on_left(text_cursor cursor) const noexcept
    {
        auto const it = get_it(cursor);
        if (it != end()) {
            return (it->direction == unicode_bidi_class::L) == cursor.before();
        } else {
            hi_assert(begin() == end());
            return true;
        }
    }

    /** Check if the cursor is on the right side of the character in display order.
     *
     * @param cursor The cursor to query.
     * @return True if the cursor is on the right of the character.
     */
    [[nodiscard]] bool is_on_right(text_cursor cursor) const noexcept
    {
        auto const it = get_it(cursor);
        if (it != end()) {
            return (it->direction == unicode_bidi_class::L) == cursor.after();
        } else {
            hi_assert(begin() == end());
            return true;
        }
    }

    /** find the nearest character.
     *
     * @param position The point near
     * @return The text_cursor nearest to the point.
     */
    [[nodiscard]] text_cursor get_nearest_cursor(point2 position) const noexcept
    {
        if (_text.empty()) {
            return {};
        }

        auto const line_it = std::ranges::min_element(_lines, std::ranges::less{}, [position](auto const& line) {
            return std::abs(line.y - position.y());
        });

        if (line_it != _lines.end()) {
            auto const [char_it, after] = line_it->get_nearest(position);
            return {narrow_cast<size_t>(std::distance(_text.begin(), char_it)), after};
        } else {
            return {};
        }
    }

    /** Get the selection for the character at the cursor.
     */
    [[nodiscard]] std::pair<text_cursor, text_cursor> select_char(text_cursor cursor) const noexcept
    {
        auto const index = cursor.index();
        return {get_before_cursor(index), get_after_cursor(index)};
    }

    /** Get the selection for the word at the cursor.
     */
    [[nodiscard]] std::pair<text_cursor, text_cursor> select_word(text_cursor cursor) const noexcept
    {
        return get_selection_from_break(cursor, _word_break_opportunities);
    }

    /** Get the selection for the sentence at the cursor.
     */
    [[nodiscard]] std::pair<text_cursor, text_cursor> select_sentence(text_cursor cursor) const noexcept
    {
        return get_selection_from_break(cursor, _sentence_break_opportunities);
    }

    /** Get the selection for a paragraph at the cursor.
     */
    [[nodiscard]] std::pair<text_cursor, text_cursor> select_paragraph(text_cursor cursor) const noexcept
    {
        auto const first_index = [&]() {
            auto i = cursor.index();
            while (i > 0) {
                if (_text[i - 1].general_category == unicode_general_category::Zp) {
                    return i;
                }
                --i;
            }
            return i;
        }();
        auto const last_index = [&]() {
            auto i = cursor.index();
            while (i < _text.size()) {
                if (_text[i].general_category == unicode_general_category::Zp) {
                    return i;
                }
                ++i;
            }
            return i;
        }();

        return {get_before_cursor(first_index), get_after_cursor(last_index)};
    }

    /** Get the selection for a paragraph at the cursor.
     */
    [[nodiscard]] std::pair<text_cursor, text_cursor> select_document(text_cursor cursor) const noexcept
    {
        if (_text.empty()) {
            return {{}, {}};
        }

        return {{}, get_end_cursor()};
    }

    /** Get the character to the left.
     *
     * @param it The iterator to the character.
     * @return The iterator to the character on the left, or empty.
     */
    [[nodiscard]] char_const_iterator move_left_char(char_const_iterator it) const noexcept
    {
        auto const [column_nr, line_nr] = get_column_line(it);
        return get_it(column_nr - 1, line_nr);
    }

    /** Get the character to the right.
     *
     * @param it The iterator to the character.
     * @return The iterator to the character on the left, or empty.
     */
    [[nodiscard]] char_const_iterator move_right_char(char_const_iterator it) const noexcept
    {
        auto const [column_nr, line_nr] = get_column_line(it);
        return get_it(column_nr + 1, line_nr);
    }

    [[nodiscard]] text_cursor move_left_char(text_cursor cursor, bool overwrite_mode) const noexcept
    {
        auto it = get_it(cursor);
        if (overwrite_mode) {
            it = move_left_char(it);
            return get_before_cursor(it);

        } else {
            if (is_on_left(cursor)) {
                // If the cursor is on the left side of a character, then move one character left.
                it = move_left_char(it);
            }

            return get_left_cursor(it);
        }
    }

    [[nodiscard]] text_cursor move_right_char(text_cursor cursor, bool overwrite_mode) const noexcept
    {
        auto it = get_it(cursor);
        if (overwrite_mode) {
            it = move_right_char(it);
            return get_before_cursor(it);

        } else {
            if (is_on_right(cursor)) {
                // If the cursor is on the left side of a character, then move one character left.
                it = move_right_char(it);
            }

            return get_right_cursor(it);
        }
    }

    [[nodiscard]] text_cursor move_down_char(text_cursor cursor, float& x) const noexcept
    {
        if (_text.empty()) {
            return {};
        }

        auto [column_nr, line_nr] = get_column_line(cursor);
        if (++line_nr == _lines.size()) {
            return get_end_cursor();
        }

        if (std::isnan(x)) {
            auto const char_it = get_it(cursor);
            hi_assert(char_it != _text.end());
            x = is_on_left(cursor) ? char_it->rectangle.left() : char_it->rectangle.right();
        }

        auto const [new_char_it, after] = _lines[line_nr].get_nearest(point2{x, 0.0f});
        return get_before_cursor(new_char_it);
    }

    [[nodiscard]] text_cursor move_up_char(text_cursor cursor, float& x) const noexcept
    {
        if (_text.empty()) {
            return {};
        }

        auto [column_nr, line_nr] = get_column_line(cursor);
        if (line_nr-- == 0) {
            return {};
        }

        if (std::isnan(x)) {
            auto char_it = get_it(cursor);
            hi_assert(char_it < _text.end());
            x = is_on_left(cursor) ? char_it->rectangle.left() : char_it->rectangle.right();
        }

        auto const [new_char_it, after] = _lines[line_nr].get_nearest(point2{x, 0.0f});
        return get_before_cursor(new_char_it);
    }

    [[nodiscard]] text_cursor move_left_word(text_cursor cursor, bool overwrite_mode) const noexcept
    {
        cursor = move_left_char(cursor, overwrite_mode).before_neighbor(size());
        auto it = get_it(cursor);
        while (it != end()) {
            if (it->general_category != unicode_general_category::Zs and
                _word_break_opportunities[get_index(it)] != unicode_break_opportunity::no) {
                return get_before_cursor(it);
            }
            it = move_left_char(it);
        }
        return get_end_cursor();
    }

    [[nodiscard]] text_cursor move_right_word(text_cursor cursor, bool overwrite_mode) const noexcept
    {
        cursor = move_right_char(cursor, overwrite_mode).before_neighbor(size());
        auto it = get_it(cursor);
        while (it != end()) {
            if (it->general_category != unicode_general_category::Zs and
                _word_break_opportunities[get_index(it)] != unicode_break_opportunity::no) {
                return get_before_cursor(it);
            }
            it = move_right_char(it);
        }
        return get_end_cursor();
    }

    [[nodiscard]] text_cursor move_begin_line(text_cursor cursor) const noexcept
    {
        auto const [column_nr, line_nr] = get_column_line(cursor);
        auto const& line = _lines[line_nr];
        return get_before_cursor(line.first);
    }

    [[nodiscard]] text_cursor move_end_line(text_cursor cursor) const noexcept
    {
        auto const [column_nr, line_nr] = get_column_line(cursor);
        auto const& line = _lines[line_nr];

        auto it = line.last;
        while (it != line.first) {
            --it;
            if (not it->is_trailing_white_space) {
                break;
            }
        }

        return get_after_cursor(it);
    }

    [[nodiscard]] text_cursor move_begin_sentence(text_cursor cursor) const noexcept
    {
        if (cursor.after()) {
            cursor = {cursor.index(), false};
        } else if (cursor.index() != 0) {
            cursor = {cursor.index() - 1, false};
        }
        auto const [first, last] = select_sentence(cursor);
        return first.before_neighbor(size());
    }

    [[nodiscard]] text_cursor move_end_sentence(text_cursor cursor) const noexcept
    {
        if (cursor.before()) {
            cursor = {cursor.index(), true};
        } else if (cursor.index() != _text.size() - 1) {
            cursor = {cursor.index() + 1, true};
        }
        auto const [first, last] = select_sentence(cursor);
        return last.before_neighbor(size());
    }

    [[nodiscard]] text_cursor move_begin_paragraph(text_cursor cursor) const noexcept
    {
        if (cursor.after()) {
            cursor = {cursor.index(), false};
        } else if (cursor.index() != 0) {
            cursor = {cursor.index() - 1, false};
        }
        auto const [first, last] = select_paragraph(cursor);
        return first.before_neighbor(size());
    }

    [[nodiscard]] text_cursor move_end_paragraph(text_cursor cursor) const noexcept
    {
        if (cursor.before()) {
            cursor = {cursor.index(), true};
        } else if (cursor.index() != _text.size() - 1) {
            cursor = {cursor.index() + 1, true};
        }
        auto const [first, last] = select_paragraph(cursor);
        return last.before_neighbor(size());
    }

    [[nodiscard]] text_cursor move_begin_document(text_cursor cursor) const noexcept
    {
        return {};
    }

    [[nodiscard]] text_cursor move_end_document(text_cursor cursor) const noexcept
    {
        if (_text.empty()) {
            return {};
        }

        return get_end_cursor();
    }

private:
    /** The scaling factor to use to scale a font's size to match the physical pixels on the display.
     */
    unit::pixel_density _pixel_density;

    /** A list of character in logical order.
     *
     * @note Graphemes are not allowed to be typographical-ligatures.
     * @note line-feeds, carriage-returns & form-feeds must be replaced by paragraph-separators or line-separators.
     * @note This variable is marked mutable because make_lines() has to modify the characters in the text.
     */
    char_vector _text;

    float _line_spacing = 1.0;
    float _paragraph_spacing = 1.5;

    hi::horizontal_alignment _alignment;

    /** A list of word break opportunities.
     */
    unicode_line_break_vector _line_break_opportunities;

    /** A list of widths, one for each character in _text.
     */
    std::vector<float> _line_break_widths;

    /** A list of word break opportunities.
     */
    unicode_word_break_vector _word_break_opportunities;

    /** A list of sentence break opportunities.
     */
    unicode_sentence_break_vector _sentence_break_opportunities;

    /** The unicode bidi algorithm context.
     */
    unicode_bidi_context _bidi_context;

    /** Direction of the text as a whole.
     */
    unicode_bidi_class _text_direction;

    /** The default script of the text.
     */
    iso_15924 _script;

    /** A list of lines top-to-bottom order.
     *
     * The characters contained in each line are in display order.
     */
    line_vector _lines;

    /** The font metrics of a line without text.
     */
    font_metrics_px _initial_line_metrics;

    /** The rectangle used for laying out.
     */
    aarectangle _rectangle;

    static void layout_lines_vertical_spacing(text_shaper::line_vector& lines, float sub_pixel_height) noexcept
    {
        hi_assert(not lines.empty());

        auto const rcp_sub_pixel_height = 1.0f / sub_pixel_height;

        auto prev = lines.begin();
        prev->y = 0.0f;
        for (auto it = prev + 1; it != lines.end(); ++it) {
            auto const height =
                prev->metrics.descender + std::max(prev->metrics.line_gap, it->metrics.line_gap) + it->metrics.ascender;

            auto const spacing =
                prev->last_category == unicode_general_category::Zp ? prev->paragraph_spacing : prev->line_spacing;
            // Lines advance downward on the y-axis.
            it->y = std::round((prev->y - spacing * height.in(unit::pixels)) * rcp_sub_pixel_height) * sub_pixel_height;
            prev = it;
        }
    }

    /** Run the bidi-algorithm over the text and replace the columns of each line.
     *
     * @param[in,out] lines The lines to be modified
     * @param[in,out] text The input text. non-const because modifications on the text is required.
     * @param writing_direction The initial writing direction.
     */
    static void
    bidi_algorithm(text_shaper::line_vector& lines, text_shaper::char_vector& text, unicode_bidi_context bidi_context) noexcept
    {
        hi_assert(not lines.empty());

        // Create a list of all character indices.
        auto char_its = std::vector<text_shaper::char_iterator>{};
        // Make room for implicit line-separators.
        char_its.reserve(text.size() + lines.size());
        for (auto const& line : lines) {
            // Add all the characters of a line.
            for (auto it = line.first; it != line.last; ++it) {
                char_its.push_back(it);
            }
            if (not is_Zp_or_Zl(line.last_category)) {
                // No explicit paragraph-separator or line-separator, at a virtual one.
                char_its.push_back(text.end());
            }
        }

        auto const [char_its_last, paragraph_directions] = unicode_bidi(
            char_its.begin(),
            char_its.end(),
            [&](text_shaper::char_const_iterator it) {
                if (it != text.end()) {
                    return it->grapheme.starter();
                } else {
                    return unicode_LS;
                }
            },
            [&](text_shaper::char_iterator it, char32_t code_point) {
                hi_axiom(it != text.end());
                it->replace_glyph(code_point);
            },
            [&](text_shaper::char_iterator it, unicode_bidi_class direction) {
                if (it != text.end()) {
                    it->direction = direction;
                }
            },
            bidi_context);

        // The unicode bidi algorithm may have deleted a few characters.
        char_its.erase(char_its_last, char_its.cend());

        // Add the paragraph direction for each line.
        auto par_it = paragraph_directions.cbegin();
        for (auto& line : lines) {
            hi_axiom(par_it != paragraph_directions.cend());
            line.paragraph_direction = *par_it;
            if (line.last_category == unicode_general_category::Zp) {
                par_it++;
            }
        }
        hi_assert(par_it <= paragraph_directions.cend());

        // Add the character indices for each line in display order.
        auto line_it = lines.begin();
        line_it->columns.clear();
        auto column_nr = 0_uz;
        for (auto const char_it : char_its) {
            if (char_it == text.end()) {
                // Ignore the virtual line separators.
                continue;
            } else if (char_it >= line_it->last) {
                // Skip to the next line.
                hi_axiom(line_it->columns.size() <= narrow_cast<size_t>(std::distance(line_it->first, line_it->last)));
                ++line_it;
                line_it->columns.clear();
                column_nr = 0_uz;
            }
            hi_axiom(line_it != lines.end());
            hi_axiom(char_it >= line_it->first);
            hi_axiom(char_it < line_it->last);
            line_it->columns.push_back(char_it);

            // Assign line_nr and column_nr, for quick back referencing.
            char_it->line_nr = line_it->line_nr;
            char_it->column_nr = column_nr++;
        }

        // All of the characters in the text must be positioned.
        for (auto& c : text) {
            hi_axiom(c.line_nr != std::numeric_limits<size_t>::max() and c.column_nr != std::numeric_limits<size_t>::max());
        }
    }

    [[nodiscard]] static float advance_op(float width) noexcept
    {
        return std::abs(width);
    }

    [[nodiscard]] static float whitespace_op(float width) noexcept
    {
        return width < 0.0f;
    }

    [[nodiscard]] static generator<std::pair<std::vector<size_t>, float>> get_widths(
        unicode_line_break_vector const& opportunities,
        std::vector<float> const& widths,
        unit::pixel_density pixel_density) noexcept
    {
        struct entry_type {
            size_t min_height;
            size_t max_height;
            float min_width;
            float max_width;
        };

        auto stack = std::vector<entry_type>{};

        auto const a4_one_column = (au::milli(au::meters)(172.0f) * pixel_density.ppi).in(unit::pixels);
        auto const a4_two_column = (au::milli(au::meters)(88.0f) * pixel_density.ppi).in(unit::pixels);

        // Max-width first.
        auto [max_width, max_lines] = detail::unicode_LB_maximum_width(opportunities, widths, advance_op, whitespace_op);
        auto height = max_lines.size();
        co_yield {std::move(max_lines), max_width};

        if (max_width >= a4_two_column) {
            // If this is wide text, then only try a few sizes.
            if (max_width > a4_one_column) {
                auto [width, lines] = detail::unicode_LB_width(opportunities, widths, a4_one_column, advance_op, whitespace_op);
                if (std::exchange(height, lines.size()) > lines.size()) {
                    co_yield {std::move(lines), width};
                }
            }

            auto [width, lines] = detail::unicode_LB_width(opportunities, widths, a4_two_column, advance_op, whitespace_op);
            if (std::exchange(height, lines.size()) > lines.size()) {
                co_yield {std::move(lines), width};
            }

        } else {
            // With small text we try every size that changes the number of lines.
            auto [min_width, min_lines] = detail::unicode_LB_minimum_width(opportunities, widths, advance_op, whitespace_op);
            if (min_lines.size() >= height) {
                // There are no multiple sizes.
                co_return;
            }

            stack.emplace_back(min_lines.size(), height, min_width, max_width);
            co_yield {std::move(min_lines), min_width};

            do {
                auto const entry = stack.back();
                stack.pop_back();

                if (entry.max_height > entry.max_height + 1 and entry.max_width >= entry.min_width + 2.0f) {
                    // There lines between the current two sizes; split in two.
                    auto const half_width = (entry.min_width + entry.max_width) * 0.5f;

                    auto [split_width, split_lines] = detail::unicode_LB_width(opportunities, widths, half_width, advance_op, whitespace_op);
                    auto const split_height = split_lines.size();

                    if (split_height == entry.min_height) {
                        // We didn't find a proper split, need to try the upper half. Use `half_width` to split right down the
                        // middle.
                        stack.emplace_back(split_height, entry.max_height, half_width, entry.max_width);

                    } else if (split_height == entry.max_height) {
                        // We didn't find a proper split, need to try the lower half. Use `half_width` to split right down the
                        // middle.
                        stack.emplace_back(entry.min_height, split_height, entry.min_width, half_width);

                    } else {
                        // Split through the middle, use the split_width for faster searching.
                        co_yield {std::move(split_lines), split_width};
                        stack.emplace_back(entry.min_height, split_height, entry.min_width, split_width);
                        stack.emplace_back(split_height, entry.max_height, split_width, entry.max_width);
                    }
                }
            } while (not stack.empty());
        }
    }

    /** Create lines from the characters in the text shaper.
     *
     * @param rectangle The rectangle to position the glyphs in.
     * @param baseline The position of the recommended base-line.
     * @param sub_pixel_size The size of a sub-pixel in device-independent-pixels.
     * @param line_spacing The scaling of the spacing between lines.
     * @param paragraph_spacing The scaling of the spacing between paragraphs.
     */
    [[nodiscard]] line_vector make_lines(aarectangle rectangle, float baseline, extent2 sub_pixel_size) noexcept
    {
        auto const line_sizes = unicode_fold_lines(_line_break_opportunities, _line_break_widths, rectangle.width(), advance_op, whitespace_op);

        auto r = text_shaper::line_vector{};
        r.reserve(line_sizes.size());

        auto char_it = _text.begin();
        auto width_it = _line_break_widths.begin();
        auto line_nr = 0_uz;
        for (auto const line_size : line_sizes) {
            hi_axiom(line_size > 0);
            auto const char_eol = char_it + line_size;
            auto const width_eol = width_it + line_size;

            auto const line_width = detail::unicode_LB_width(width_it, width_eol, advance_op, whitespace_op);
            r.emplace_back(line_nr++, _text.begin(), char_it, char_eol, line_width, _initial_line_metrics);

            char_it = char_eol;
            width_it = width_eol;
        }

        if (r.empty() or is_Zp_or_Zl(r.back().last_category)) {
            r.emplace_back(line_nr++, _text.begin(), _text.end(), _text.end(), 0.0f, _initial_line_metrics);
            r.back().paragraph_direction = _text_direction;
        }

        layout_lines_vertical_spacing(r, sub_pixel_size.height());
        return r;
    }

    /** Position the glyphs.
     *
     * @param rectangle The rectangle to position the glyphs in.
     * @param sub_pixel_size The size of a sub-pixel in device-independent-pixels.
     * @post Glyphs in _text are positioned inside the given rectangle.
     */
    void position_glyphs(aarectangle rectangle, extent2 sub_pixel_size) noexcept
    {
        hi_assert(not _lines.empty());

        // The bidi algorithm will reorder the characters on each line, and mirror the brackets in the text when needed.
        bidi_algorithm(_lines, _text, _bidi_context);
        for (auto& line : _lines) {
            // Position the glyphs on each line. Possibly morph glyphs to handle ligatures and calculate the bounding rectangles.
            line.layout(_alignment, rectangle.left(), rectangle.right(), sub_pixel_size.width());
        }
    }

    /** Resolve the script of each character in text.
     */
    void resolve_script() noexcept
    {
        // Find the first script in the text if no script is found use the text_shaper's default script.
        auto first_script = _script;
        for (auto& c : _text) {
            auto const script = ucd_get_script(c.grapheme.starter());
            if (script != iso_15924::wildcard() or script == iso_15924::uncoded() or script == iso_15924::common() or
                script == iso_15924::inherited()) {
                first_script = script;
                break;
            }
        }

        // Backward pass: fix start of words and open-brackets.
        // After this pass unknown-script is no longer in the text.
        // Close brackets will not be fixed, those will be fixed in the last forward pass.
        auto word_script = iso_15924::common();
        auto previous_script = first_script;
        for (auto i = std::ssize(_text) - 1; i >= 0; --i) {
            auto& c = _text[i];

            if (_word_break_opportunities[i + 1] != unicode_break_opportunity::no) {
                word_script = iso_15924::common();
            }

            c.script = ucd_get_script(c.grapheme.starter());
            if (c.script == iso_15924::uncoded() or c.script == iso_15924::common()) {
                auto const bracket_type = ucd_get_bidi_paired_bracket_type(c.grapheme.starter());
                // clang-format off
                c.script =
                    bracket_type == unicode_bidi_paired_bracket_type::o ? previous_script :
                    bracket_type == unicode_bidi_paired_bracket_type::c ? iso_15924::common() :
                    word_script;
                // clang-format on

            } else if (c.script != iso_15924::inherited()) {
                previous_script = word_script = c.script;
            }
        }

        // Forward pass: fix all common and inherited with previous or first script.
        previous_script = first_script;
        for (auto i = 0_uz; i != _text.size(); ++i) {
            auto& c = _text[i];

            if (c.script == iso_15924::common() or c.script == iso_15924::inherited()) {
                c.script = previous_script;

            } else {
                previous_script = c.script;
            }
        }
    }

    [[nodiscard]] std::pair<text_cursor, text_cursor>
    get_selection_from_break(text_cursor cursor, std::vector<unicode_break_opportunity> const& break_opportunities) const noexcept
    {
        if (_text.empty()) {
            return {{}, {}};
        }

        // In the algorithm below we search before and after the character that the cursor is at.
        // We do not use the before/after differentiation.

        auto const first_index = [&]() {
            auto i = cursor.index();
            while (break_opportunities[i] == unicode_break_opportunity::no) {
                --i;
            }
            return i;
        }();
        auto const last_index = [&]() {
            auto i = cursor.index();
            while (break_opportunities[i + 1] == unicode_break_opportunity::no) {
                ++i;
            }
            return i;
        }();

        return {get_before_cursor(first_index), get_after_cursor(last_index)};
    }

    [[nodiscard]] std::pair<font_metrics_px, unicode_general_category>
    get_line_metrics(text_shaper::char_const_iterator first, text_shaper::char_const_iterator last) const noexcept
    {
        auto metrics = _initial_line_metrics;
        for (auto it = first; it != last; ++it) {
            // Only calculate line metrics based on visible characters.
            // For example a paragraph separator is seldom available in a font.
            if (is_visible(it->general_category)) {
                inplace_max(metrics, it->font_metrics());
            }
        }

        auto const last_category = (first != last) ? (last - 1)->general_category : unicode_general_category::Cn;
        return {metrics, last_category};
    }

    /** Get the height of the text.
     *
     * This is the vertical distance from the cap-height of the top most line, and the base-line of the bottom most line.
     *
     * @param lines A list of number-of-graphemes per line.
     */
    [[nodiscard]] float get_text_height(std::vector<size_t> const& lines) const noexcept
    {
        if (lines.empty()) {
            return 0.0f;
        }

        auto line_it = lines.cbegin();
        auto char_it_first = _text.begin();
        auto char_it_last = char_it_first + *line_it++;

        // Add the cap-height of the first line.
        auto [previous_metrics, previous_category] = get_line_metrics(char_it_first, char_it_last);
        auto total_height = previous_metrics.cap_height;

        for (; line_it != lines.cend(); ++line_it) {
            char_it_first = std::exchange(char_it_last, char_it_last + *line_it);

            // Advance to the base-line of the next line.
            auto [current_metrics, current_category] = get_line_metrics(char_it_first, char_it_last);
            auto const line_height = previous_metrics.descender + std::max(previous_metrics.line_gap, current_metrics.line_gap) +
                current_metrics.ascender;

            auto const spacing = previous_category == unicode_general_category::Zp ? _paragraph_spacing :
                                                                                     _line_spacing;
            total_height = total_height + spacing * line_height;

            previous_metrics = std::move(current_metrics);
            previous_category = std::move(current_category);
        }

        return total_height.in(unit::pixels);
    }
};

} // namespace hi::inline v1
