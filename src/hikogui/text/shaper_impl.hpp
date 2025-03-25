

#pragma once

#include "shaper_intf.hpp"
#include "shaper_utils.hpp"

hi_export_module(hikogui.text : shaper_impl);

hi_export namespace hi::inline v1 {
inline void shaper::progress_to(state_type new_state)
{
    // Add prerequisites to new_state.
    new_state.bidi_1 |= new_state.bidi_2;
    new_state.fold |= new_state.bidi_2;
    new_state.line_breaks |= new_state.bidi_2 or new_state.fold;
    new_state.word_breaks |= new_state.runs;
    new_state.base_code_points |= new_state.line_breaks or new_state.word_breaks or new_state.sentence_breaks or new_state.bidi_1;

    if ((_state & new_state) == new_state) {
        return;
    }

    if (new_state.base_code_points and not std::exchange(_state.base_code_points, true)) {
        clear_and_resize(_base_code_points, _text.size(), U'\u0000');
        for (auto i = size_t{0}; i != _text.size(); ++i) {
            _base_code_points[i] = _text[i].starter();
        }
    }

    if (new_state.line_breaks and not std::exchange(_state.line_breaks, true)) {
        assert(_state.base_code_points);
        _line_breaks.set_text(_base_code_points);
    }

    if (new_state.word_breaks and not std::exchange(_state.word_breaks, true)) {
        assert(_state.base_code_points);
        _word_breaks.set_text(_base_code_points);
    }

    if (new_state.sentence_breaks and not std::exchange(_state.sentence_breaks, true)) {
        assert(_state.base_code_points);
        _sentence_breaks.set_text(_base_code_points);
    }

    if (new_state.bidi_1 and not std::exchange(_state.bidi_1, true)) {
        assert(_state.base_code_points);
        _bidi.set_text(_base_code_points, _paragraph_direction);
    }

    if (new_state.runs and not std::exchange(_state.runs, true)) {
        assert(_state.word_breaks);
        shaper_make_run_lengths(_text, _word_breaks.opportunities(), _run_lengths);
        shaper_make_run_ids(_run_lengths, _text.size(), _run_ids);
    }

    if (new_state.glyph_metrics and not std::exchange(_state.glyph_metrics, true)) {
        assert(_state.runs);
        shaper_collect_glyph_metrics(_text, _run_lengths, _style, _font_size, _glyphs, _metrics, _advances);
    }

    if (new_state.fold and not std::exchange(_state.fold, true)) {
        assert(_state.line_breaks);
        assert(_state.glyph_metrics);
        _width_after_folding = _line_breaks.fold(_advances, _width, _line_lengths);
    }

    if (new_state.bidi_2 and not std::exchange(_state.bidi_2, true)) {
        assert(_state.bidi_1);
        assert(_state.fold);
        _bidi.set_lines(_line_lengths);
    }

    if (new_state.substitutions and not std::exchange(_state.substitutions, true)) {}

    if (new_state.positions and not std::exchange(_state.positions, true)) {}

    assert((_state & new_state) == new_state);
}

[[nodiscard]] inline aarectangle shaper::get_bounds()
{
    hi_not_implemented();
    return {};
}

[[nodiscard]] inline box_constraints shaper::get_constraints(unit::length_f width, unit::length_f height)
{
    progress_to(state_type::make_glyph_metrics());

    auto r = box_constraints{};

    auto const natural_width =
        _line_breaks.fold(_advances, unit::pixels(std::numeric_limits<float>::max()), _get_constraints_natural_line_lengths);
    shaper_collect_line_metrics(_metrics, _get_constraints_natural_line_lengths, _get_constraints_natural_line_metrics);
    auto const first_cap_height = _get_constraints_natural_line_metrics.front().cap_height;

    assert(not _get_constraints_natural_line_metrics.empty());
    auto const middle_cap_height = _get_constraints_natural_line_metrics[_get_constraints_natural_line_metrics.size() / 2].cap_height;

    return r;
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