

#pragma once

#include "shaper_intf.hpp"
#include "shaper_utils.hpp"

hi_export_module(hikogui.text : shaper_impl);

hi_export namespace hi::inline v1 {

inline void shaper::execute_base_code_points()
{
    clear_and_resize(_base_code_points, _text.size(), U'\u0000');
    for (auto i = size_t{0}; i != _text.size(); ++i) {
        _base_code_points[i] = _text[i].starter();
    }

    _state.base_code_points = true;
}

inline void shaper::execute_line_breaks()
{
    assert(_state.base_code_points);
    _line_breaks.set_text(_base_code_points);
    _state.line_breaks = true;
}

inline void shaper::execute_word_breaks()
{
    assert(_state.base_code_points);
    _word_breaks.set_text(_base_code_points);
    _state.word_breaks = true;
}

inline void shaper::execute_sentence_breaks()
{
    assert(_state.base_code_points);
    _sentence_breaks.set_text(_base_code_points);
    _state.sentence_breaks = true;
}

inline void shaper::progress_to(state_type new_state)
{
    new_state.line_breaks |= new_state.bidi_2;
    new_state.base_code_points |= new_state.line_breaks or new_state.word_breaks or new_state.sentence_breaks;

    if ((_state & new_state) == new_state) {
        return;
    }

    if (new_state.base_code_points and not _state.base_code_points) {
        execute_base_code_points();
    }
    
    if (new_state.line_breaks and not _state.line_breaks) {
        execute_line_breaks();
    }

    if (new_state.word_breaks and not _state.word_breaks) {
        execute_word_breaks();
    }

    if (new_state.sentence_breaks and not _state.sentence_breaks) {
        execute_sentence_breaks();
    }

    if (new_state.bidi_1 and not _state.bidi_1) {
        execute_bidi_1();
    }

    if (new_state.glyphs and not _state.glyphs) {
        execute_glyphs();
    }


    if (new_state.bidi_2 and not _state.bidi_2) {
        execute_bidi_2();
    }

    if (new_state.substitutions and not _state.substitutions) {
        execute_substitutions();
    }

    if (new_state.positions and not _state.positions) {
        execute_positions();
    }

    assert((_state & new_state) == new_state);
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