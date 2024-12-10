

#pragma once

#include "text_style_set.hpp"
#include "text_style.hpp"
#include "text_cursor.hpp"
#include "../geometry/geometry.hpp"
#include "../units/units.hpp"
#include "../unicode/unicode.hpp"
#include "../font/font.hpp"
#include "../macros.hpp"
#include <vector>

hi_export_module(hikogui.text : shaper_intf);

hi_export namespace hi::inline v1 {

class shaper {
public:
    enum class state_type {
        /** No calculations are done.
         */
        idle,

        /** The bidi algorithm has been executed.
         */
        bidi,

        /** The metrics of the text are calculated.
         */
        metrics,

        /** The text has been laid out.
         */
        layout
    };

    constexpr shaper() noexcept = default;
    shaper(shaper const&) = default;
    shaper(shaper&&) noexcept = default;
    shaper& operator=(shaper const&) = default;
    shaper& operator=(shaper&&) noexcept = default;
    ~shaper() = default;

    /** Get the current state of the shaper.
     *
     * @return The current state of the shaper.
     */
    [[nodiscard]] state_type state() const noexcept
    {
        return _state;
    }

    /** Set the style of the text.
     *
     * @note sets state: <= bidi
     * @param text_style The text style set for the text.
     */
    void set_style(text_style_set const& text_style) noexcept
    {
        text_style_set = text_style;
        retreat_to(state_type::bidi);
    }


    /** Set the base font size of the text.
     *
     * @note sets state: <= bidi
     * @param font_size The base font-size.
     */
    void set_font_size(unit::pixels_per_em font_size);

    /** Set the base color of the text.
     *
     * @note Does not change the state.
     * @param color The base color.
     */
    void set_color(hi::color color)

    /** Set the alignment of the text.
     *
     * @note sets state: <= metrics
     * @param alignment The horizontal and vertical alignment of the text.
     */
    void set_alignment(hi::alignment alignment);

    /** Set the text to format.
     *
     * @note sets state: <= idle
     * @param text The text to shape.
     */
    void set_text(gstring const& text);

    /** Set the layout of the text.
     *
     * This function will set the origin of the text and the width of the text.
     * The width is the maximum width the text is allowed to be. The text will
     * be wrapped to fit the width.
     * 
     * The origin is the position of the baseline and left-side of the text. The
     * text will be placed to the right of the origin. Depending on the alignment
     * the text will be placed above, below or centered around the origin. 
     * 
     * @note sets state: <= layout
     * @param origin The origin of the text.
     * @param width The width of the text.
     */
    void set_layout(point2 origin, float width);

    /** Get the bounds of the text.
     * 
     * This function will return the bounds of the text. The bounds are the
     * smallest rectangle that covers the text. The bounds are from the left
     * side of the left-most grapheme to the right side of the right-most
     * grapheme. The bottom is placed at the descender of the bottom line of text
     * and the top is placed at the ascender of the top line of text.
     * 
     * @note sets state: >= layout
     * @return The bounds of the text.
     */
    [[nodiscard]] aarectangle get_bounds();

    /** Get the size of the text.
     *
     * This function will return the size of the text, with an optional maximum
     * width. If the maximum width is set, the text will be wrapped to fit the
     * width.
     *
     * The returned width is the width of the text, and the height is the height
     * of the text. The height is from cap-height of the first line of text to
     * the baseline of the last line of text. This means the ascender and
     * descender may go beyond the height of the text, it is recommended to use
     * margins to prevent clipping of the text.
     *
     * @note sets state: >= metrics
     * @param maximum_width The maximum width the text is allowed to be. or
     *                      std::nullopt to use the natural width.
     */
    [[nodiscard]] extent2 get_size(std::optional<float> maximum_width = std::nullopt);

    /** Return the baseline of the text.
     *
     * The baseline is a function that takes the height of the text and returns
     * the baseline of the text. The baseline is the position where the text is
     * placed on the screen.
     *
     * This function uses the alignment set by set_alignment() to determine the
     * baseline. The baseline uses the cap-height of the first and middle line
     * of text to determine the baseline.
     *
     * The middle line is always based on the natural layout of the text, i.e.
     * the text is not wrapped. Even when the text is wrapped, the baseline is
     * calculated based on the natural layout.
     *
     * @note sets state: >= metrics
     * @return The baseline function.
     */
    [[nodiscard]] hi::baseline baseline();

    /** Get rectangles for a piece of text.
     *
     * This function will return the minimum number of rectangles that covers
     * a piece of text logically between the two given cursor positions.
     *
     * The rectangle's left side is placed at the origin of the grapheme. The
     * bottom is placed at the descender and the top at the ascender. The right
     * is placed at the advance right of the origin of the grapheme.
     * 
     * If there is extra whitespace between graphemes due to justification, the
     * rectangle will be extended to cover the whitespace.
     *
     * @note sets state: >= layout
     * @param first The start of the selection.
     * @param last The end of the selection.
     * @return The rectangles surrounding the text inside the selection.
     */
    [[nodiscard]] std::vector<aarectangle> get_rectangles(text_cursor first, text_cursor last);

    /** Get the caret at the cursor position.
     *
     * The line's bottom is placed at the descender and the top at the ascender.
     * The line will cross either the origin of the grapheme or the advance
     * right of the grapheme. The line is optionally diagonal based on the
     * italic angle of the font.
     *
     * @note sets state: >= layout
     * @param cursor The cursor position.
     * @return A line representing the caret. The line is optionally diagonal
     *         based on the italic angle of the font.
     */
    [[nodiscard]] line get_caret(text_cursor cursor);

    /** Get the text-cursor position from a mouse-cursor position.
     * 
     * @note sets state: >= layout
     * @param position The position of the mouse-cursor.
     * @return The text-cursor position.
     */
    [[nodiscard]] text_cursor get_cursor(point2 position);

    /** Get the cursor left of the current cursor.
     *
     * The cursor returned always skips over at exactly one grapheme.
     *  - Skips over LTR grapheme, the cursor is placed in front of the
     *    grapheme.
     *  - Skips over RTL grapheme, the cursor is placed behind the grapheme.
     *
     * @note sets state: >= bidi
     * @param cursor The current cursor position.
     * @return The cursor position left of the current cursor.
     */
    [[nodiscard]] text_cursor go_left(text_cursor cursor);

    /** Get the cursor right of the current cursor.
     *
     * The cursor returned always skips over at exactly one grapheme.
     *  - Skips over LTR grapheme, the cursor is placed behind the
     *    grapheme.
     *  - Skips over RTL grapheme, the cursor is placed in front of the grapheme.
     *
     * @note sets state: >= bidi
     * @param cursor The current cursor position.
     * @return The cursor position right of the current cursor.
     */
    [[nodiscard]] text_cursor go_right(text_cursor cursor);

    /** Get the cursor a word left of the current cursors.
     * 
     * The cursor returned always skips over at exactly one word.
     * - Skips over LTR word, the cursor is placed in front of the word.
     * - Skips over RTL word, the cursor is placed behind the word.
     * 
     * @note sets state: >= bidi
     * @param cursor The current cursor position.
     * @return The cursor position one word left of the current cursor.
     */
    [[nodiscard]] text_cursor go_word_left(text_cursor cursor);

    /** Get the cursor a word right of the current cursors.
     * 
     * The cursor returned always skips over at exactly one word.
     * - Skips over LTR word, the cursor is placed behind the word.
     * - Skips over RTL word, the cursor is placed in front of the word.
     * 
     * @note sets state: >= bidi
     * @param cursor The current cursor position.
     * @return The cursor position one word right of the current cursor.
     */
    [[nodiscard]] text_cursor go_word_right(text_cursor cursor);

    /** Get the cursor a sentence left of the current cursors.
     * 
     * The cursor returned always skips over at exactly one sentence.
     * - Skips over LTR sentence, the cursor is placed in front of the sentence.
     * - Skips over RTL sentence, the cursor is placed behind the sentence.
     * 
     * @note sets state: >= bidi
     * @param cursor The current cursor position.
     * @return The cursor position one sentence left of the current cursor.
     */
    [[nodiscard]] text_cursor go_sentence_left(text_cursor cursor);

    /** Get the cursor a sentence right of the current cursors.
     * 
     * The cursor returned always skips over at exactly one sentence.
     * - Skips over LTR sentence, the cursor is placed behind the sentence.
     * - Skips over RTL sentence, the cursor is placed in front of the sentence.
     * 
     * @note sets state: >= bidi
     * @param cursor The current cursor position.
     * @return The cursor position one sentence right of the current cursor.
     */
    [[nodiscard]] text_cursor go_sentence_right(text_cursor cursor);

    /** Get the cursor a paragraph left of the current cursors.
     * 
     * The cursor returned always skips over at exactly one paragraph.
     * - Skips over LTR paragraph, the cursor is placed in front of the paragraph.
     * - Skips over RTL paragraph, the cursor is placed behind the paragraph.
     * 
     * @note sets state: >= bidi
     * @param cursor The current cursor position.
     * @return The cursor position one paragraph left of the current cursor.
     */
    [[nodiscard]] text_cursor go_paragraph_left(text_cursor cursor);

    /** Get the cursor a paragraph right of the current cursors.
     * 
     * The cursor returned always skips over at exactly one paragraph.
     * - Skips over LTR paragraph, the cursor is placed behind the paragraph.
     * - Skips over RTL paragraph, the cursor is placed in front of the paragraph.
     * 
     * @note sets state: >= bidi
     * @param cursor The current cursor position.
     * @return The cursor position one paragraph right of the current cursor.
     */
    [[nodiscard]] text_cursor go_paragraph_right(text_cursor cursor);

    /** Get the cursor above the current cursor.
     *
     * @note sets state: >= layout
     * @param cursor The current cursor position.
     * @param start_cursor The cursor at the start of vertical movement.
     * @return The cursor position above the current cursor.
     */
    [[nodiscard]] text_cursor go_up(text_cursor cursor, text_cursor start_cursor);

    /** Get the cursor below the current cursor.
     *
     * @note sets state: >= layout
     * @param cursor The current cursor position.
     * @param start_cursor The cursor at the start of vertical movement.
     * @return The cursor position below the current cursor.
     */
    [[nodiscard]] text_cursor go_down(text_cursor cursor, text_cursor start_cursor);

private:
    state_type _state = state_type::idle;
    text_style_set _style;
    unit::pixels_per_em _font_size = unit::pixels(0.0f);
    hi::color _color;
    hi::alignment _alignment;
    gstring _text;
    point2 _origin;
    float _width = 0.0f;

    /** Progress the state to the given state.
     * 
     * This function will progress the state to the given state. If the state
     * is already at the given state, the function will do nothing.
     * 
     * To progress the state, the function will execute different parts of the
     * shaping algorithm. The algorithm is executed in the following order:
     * - bidi: The bidirectional algorithm is executed.
     * - metrics: Information such as glyph/font metrics are collected.
     *            Enough to calculate the size of the text.
     * - layout: The text is wrapped to fit the width, and the glyphs are
     *           substituted with the correct glyphs and positioned correctly.
     * 
     * @param state The state to progress to.
     */
    void progress_to(state_type state);

    /** Retreat the state to the given state.
     * 
     * This function will retreat the state to the given state. If the state is
     * already at the given state, the function will do nothing.
     * 
     * @param state The state to retreat to.
     */
    void retreat_to(state_type state);
};


}

