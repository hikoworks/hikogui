

#pragma once

#include "shaper_intf.hpp"
#include "shaper_utils.hpp"

hi_export_module(hikogui.text : shaper_impl);

hi_export namespace hi::inline v1 {

[[nodiscard]] inline void shaper::progress_to(shaper_state new_state)
{
    if ((_state & new_state) == new_state) {
        return;
    }

    if (to_bool(new_state & shaper_state::word_breaks) and not to_bool(_state & shaper_state::word_breaks)) {
        execute_word_breaks();
        _state |= shaper_state::word_breaks;
    }

    if (to_bool(new_state & shaper_state::sentence_breaks) and not to_bool(_state & shaper_state::sentence_breaks)) {
        execute_sentence_breaks();
        _state |= shaper_state::sentence_breaks;
    }

    if (to_bool(new_state & shaper_state::line_breaks) and not to_bool(_state & shaper_state::line_breaks)) {
        execute_line_breaks();
        _state |= shaper_state::line_breaks;
    }

    if (to_bool(new_state & shaper_state::bidi_1) and not to_bool(_state & shaper_state::bidi_1)) {
        execute_bidi_1();
        _state |= shaper_state::bidi_1;
    }

    if (to_bool(new_state & shaper_state::glyphs) and not to_bool(_state & shaper_state::glyphs)) {
        execute_glyphs();
        _state |= shaper_state::glyphs;
    }

    if (to_bool(new_state & shaper_state::bidi_2) and not to_bool(_state & shaper_state::bidi_2)) {
        assert(to_bool(_state & shaper_state::bidi_1));
        assert(to_bool(_state & shaper_state::glyphs));
        execute_bidi_2();
        _state |= shaper_state::bidi_2;
    }

    if (to_bool(new_state & shaper_state::substitutions) and not to_bool(_state & shaper_state::substitutions)) {
        assert(to_bool(_state & shaper_state::glyphs));
        assert(to_bool(_state & shaper_state::bidi_2));
        execute_substitutions();
        _state |= shaper_state::substitutions;
    }

    if (to_bool(new_state & shaper_state::positions) and not to_bool(_state & shaper_state::positions)) {
        assert(to_bool(_state & shaper_state::bidi_2));
        assert(to_bool(_state & shaper_state::substitutions));
        execute_positions();
        _state |= shaper_state::positions;
    }
}

inline void shaper::execute_word_breaks()
{
    _word_breaks = unicode_word_break(_text);
}

[[nodiscard]] inline aarectangle shaper::get_bounds()
{
    hi_not_implemented();
    return {};
}

[[nodiscard]] inline extent2 shaper::get_size(float maximum_width)
{
    hi_not_implemented();
    return {};
}

[[nodiscard]] inline baseline shaper::baseline()
{
    hi_not_implemented();
    return {};
}

[[nodiscard]] inline line_segment shaper::get_caret(text_cursor cursor)
{
    hi_not_implemented();
    return {point2{}, point2{}};
}

[[nodiscard]] inline std::vector<aarectangle> shaper::get_rectangles(text_cursor first, text_cursor last)
{
    hi_not_implemented();
    return {};
}

[[nodiscard]] inline text_cursor shaper::get_cursor(point2 position)
{
    hi_not_implemented();
    return {};
}

[[nodiscard]] inline text_cursor shaper::go_left(text_cursor cursor)
{
    hi_not_implemented();
    return cursor;
}

[[nodiscard]] inline text_cursor shaper::go_right(text_cursor cursor)
{
    hi_not_implemented();
    return cursor;
}

[[nodiscard]] inline text_cursor shaper::go_word_left(text_cursor cursor)
{
    hi_not_implemented();
    return cursor;
}

[[nodiscard]] inline text_cursor shaper::go_word_right(text_cursor cursor)
{
    hi_not_implemented();
    return cursor;
}

[[nodiscard]] inline text_cursor shaper::go_sentence_left(text_cursor cursor)
{
    hi_not_implemented();
    return cursor;
}

[[nodiscard]] inline text_cursor shaper::go_sentence_right(text_cursor cursor)
{
    hi_not_implemented();
    return cursor;
}

[[nodiscard]] inline text_cursor shaper::go_paragraph_left(text_cursor cursor)
{
    hi_not_implemented();
    return cursor;
}

[[nodiscard]] inline text_cursor shaper::go_paragraph_right(text_cursor cursor)
{
    hi_not_implemented();
    return cursor;
}

[[nodiscard]] inline text_cursor shaper::go_up(text_cursor cursor, text_cursor start_cursor)
{
    hi_not_implemented();
    return cursor;
}

[[nodiscard]] inline text_cursor shaper::go_down(text_cursor cursor, text_cursor start_cursor)
{
    hi_not_implemented();
    return cursor;
}


} // namespace hi::inline v1