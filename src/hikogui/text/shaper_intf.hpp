

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
// clang-format off
enum class shaper_state {
    none            = 0b0000'0000, ///< Nothing has been calculated.
    word_breaks     = 0b0000'0001, ///< Word-breaks have been calculated.
    sentence_breaks = 0b0000'0010, ///< Sentence-breaks have been calculated.
    line_breaks     = 0b0000'0100, ///< Line-breaks have been calculated.
    glyphs          = 0b0000'1000, ///< Graphemes are converted to glyphs.
    bidi_1          = 0b0001'0000, ///< 1st pass of the bidirectional algorithm.
    bidi_2          = 0b0010'0000, ///< 2nd pass of the bidirectional algorithm.
    substitutions   = 0b0100'0000, ///< Glyph substitutions have been done.
    positions       = 0b1000'0000, ///< Glyphs have been positioned.
};
// clang-format on

[[nodiscard]] constexpr shaper_state operator|(shaper_state const& lhs, shaper_state const& rhs) noexcept
{
    return static_cast<shaper_state>(std::to_underlying(lhs) | std::to_underlying(rhs));
}

[[nodiscard]] constexpr shaper_state operator&(shaper_state const& lhs, shaper_state const& rhs) noexcept
{
    return static_cast<shaper_state>(std::to_underlying(lhs) & std::to_underlying(rhs));
}

[[nodiscard]] constexpr shaper_state operator^(shaper_state const& lhs, shaper_state const& rhs) noexcept
{
    return static_cast<shaper_state>(std::to_underlying(lhs) ^ std::to_underlying(rhs));
}

[[nodiscard]] constexpr shaper_state operator~(shaper_state const& lhs) noexcept
{
    return static_cast<shaper_state>(~std::to_underlying(lhs));
}

[[nodiscard]] constexpr shaper_state& operator|=(shaper_state& lhs, shaper_state const& rhs) noexcept
{
    return lhs = lhs | rhs;
}

[[nodiscard]] constexpr shaper_state& operator&=(shaper_state& lhs, shaper_state const& rhs) noexcept
{
    return lhs = lhs & rhs;
}

[[nodiscard]] constexpr shaper_state& operator^=(shaper_state& lhs, shaper_state const& rhs) noexcept
{
    return lhs = lhs ^ rhs;
}

[[nodiscard]] constexpr bool to_bool(shaper_state const& rhs) noexcept
{
    return static_cast<bool>(std::to_underlying(rhs));
}

class shaper {
public:
    constexpr shaper() noexcept = default;
    shaper(shaper const&) = default;
    shaper(shaper&&) noexcept = default;
    shaper& operator=(shaper const&) = default;
    shaper& operator=(shaper&&) noexcept = default;
    ~shaper() = default;

    /** Set the style of the text.
     *
     * @param style The text style set for the text.
     */
    void set_style(text_style_set const& style) noexcept
    {
        _style = style;
        _state &= shaper_state::word_breaks | shaper_state::sentence_breaks | shaper_state::line_breaks | shaper_state::bidi_1;
    }

    /** Set the base font size of the text.
     *
     * @param font_size The base font-size.
     */
    void set_font_size(unit::pixels_per_em_f font_size) noexcept
    {
        _font_size = font_size;
        _state &= shaper_state::word_breaks | shaper_state::sentence_breaks | shaper_state::line_breaks | shaper_state::glyphs |
            shaper_state::bidi_1;
    }

    /** Set the base color of the text.
     *
     * @param color The base color.
     */
    void set_color(hi::color color) noexcept
    {
        _color = color;
        // _state remains the same.
    }

    /** Set the alignment of the text.
     *
     * @param alignment The horizontal and vertical alignment of the text.
     */
    void set_alignment(hi::alignment alignment) noexcept
    {
        _alignment = alignment;
        _state &= shaper_state::word_breaks | shaper_state::sentence_breaks | shaper_state::line_breaks | shaper_state::bidi_1 |
            shaper_state::bidi_2 | shaper_state::substitutions;
    }

    /** Set the text to format.
     *
     * @note sets state: <= idle
     * @param text The text to shape.
     */
    void set_text(gstring_view const& text) noexcept
    {
        _text = text;
        _state = shaper_state::none;
    }

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
     * @param width The width of the text, or infinity if the text is not wrapped.
     */
    void set_layout(point2 origin, float width)
    {
        _origin = origin;
        _width = width;
        _state &= shaper_state::word_breaks | shaper_state::sentence_breaks | shaper_state::line_breaks | shaper_state::bidi_1;
    }

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
     * @param maximum_width The maximum width the text is allowed to be, or
     *                      infinity if the text is not wrapped.
     */
    [[nodiscard]] extent2 get_size(float maximum_width = std::numeric_limits<float>::infinity());

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
    [[nodiscard]] line_segment get_caret(text_cursor cursor);

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
    shaper_state _state = shaper_state::none;

    text_style_set _style;
    unit::pixels_per_em_f _font_size = unit::pixels_per_em(0.0f);
    hi::color _color;
    hi::alignment _alignment;
    gstring _text;
    point2 _origin;
    float _width = 0.0f;

    std::vector<unicode_break_opportunity> _word_breaks;
    std::vector<unicode_break_opportunity> _sentence_breaks;
    std::vector<unicode_break_opportunity> _line_breaks;

    /** Progress the state to the given state.
     *
     * This function will progress the state to the given state. If the state
     * is already at the given state, the function will do nothing.
     *
     * @param state The state to progress to.
     */
    void progress_to(shaper_state state);

    void execute_word_breaks();
    void execute_sentence_breaks();
    void execute_line_breaks();
    void execute_glyphs();
    void execute_bidi_1();
    void execute_bidi_2();
    void execute_substitutions();
    void execute_positions();
};
}
