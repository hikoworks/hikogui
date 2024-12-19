

#pragma once

#include "text_style_set.hpp"
#include "text_style.hpp"
#include "../units/units.hpp"
#include "../unicode/unicode.hpp"
#include "../font/font.hpp"
#include "../macros.hpp"

hi_export_module(hikogui.text : shaper_utils);

hi_export namespace hi::inline v1 {
struct shaper_run_indices {
    size_t first = 0;
    size_t last = 0;
};

/** Get runs of text.
 *
 * A run is a sequence of graphemes that are not separated by a word break and
 * have the same grapheme attributes. Which means that the graphemes of a run
 * can be shaped together. A sequence of whitespace is considered a run.
 *
 * @param text The text to get the runs of.
 * @param word_breaks The word break opportunities between each grapheme.
 * @param [out] r The length of each run.
 */
inline void shaper_make_run_lengths(
    std::span<grapheme const> text, std::span<unicode_break_opportunity const> word_breaks, std::vector<size_t>& r)
{
    r.clear();

    // If the text is empty, then word_breaks is also empty.
    if (text.empty()) {
        return;
    }

    assert(text.size() + 1 == word_breaks.size());
    // The last character is always a word break. Therfore we do not need to
    // bound check the next grapheme access inside the loop.
    assert(word_breaks.back() == unicode_break_opportunity::yes);

    auto run_start = size_t{0};
    for (auto i = size_t{0}; i != text.size(); ++i) {
        // Is this is the last character of the run.
        if (word_breaks[i + 1] == unicode_break_opportunity::yes) {
            r.emplace_back(i + 1 - run_start);
            run_start = i + 1;

        } else if (text[i + 1].attributes() != text[i].attributes()) { // No bound check, see assert above.
            r.emplace_back(i + 1 - run_start);
            run_start = i + 1;
        }
    }
}

/** Get a identifier for each run.
 *
 * This is used by the shaper when advancing glyphs in display order, to easily
 * identify when a new run starts.
 *
 * @param run_lengths The number of graphemes in each run.
 * @param text_size The number of graphemes in the text.
 * @param [out] r An id per grapheme to identify the run the grapheme belongs to.
 */
inline void shaper_make_run_ids(std::span<size_t const> run_lengths, size_t text_size, std::vector<size_t>& r)
{
    clear_and_resize(r, text_size, size_t{0});

    auto id = 0;
    auto r_it = r.begin();
    for (auto length : run_lengths) {
        for (auto i = size_t{0}; i != length; ++i, ++r_it) {
            *r_it = id;
        }
        ++id;
    }

    assert(r_it == r.end());
}

/** Information gathered from a grapheme in preparation for shaping.
 */
struct shaper_grapheme_metrics {
    font_glyph_ids glyphs = {};
    unit::pixels_f advance = unit::pixels(0.0f);
    unit::pixels_f cap_height = unit::pixels(0.0f);
    unit::pixels_f ascender = unit::pixels(0.0f);
    unit::pixels_f descender = unit::pixels(0.0f);
    unit::pixels_f line_gap = unit::pixels(0.0f);
    float line_spacing = 0.0f;
    float paragraph_spacing = 0.0f;

    /** The mirror-glyph replaces the base glyph when grapheme is displayed in RTL.
     */
    glyph_id mirrored_glyph = {};
    unicode_general_category general_category = unicode_general_category::Cn;
    unicode_bidi_paired_bracket_type bracket_type = unicode_bidi_paired_bracket_type::n;
};

/** Get information of each grapheme in preparation for shaping.
 *
 * @param graphemes The graphemes to collect glyphs for.
 * @param run_lengths The length of each run.
 * @param style_set The style of the text.
 * @param [out] r The glyphs (and font) for each grapheme.
 */
inline void shaper_collect_glyphs(
    std::span<grapheme const> graphemes,
    std::vector<size_t> const& run_lengths,
    text_style_set const& style_set,
    unit::pixels_per_em_f font_size,
    std::vector<font_glyph_ids>& glyphs,
    std::vector<shaper_grapheme_metrics>& metrics,
    std::vector<unit::pixels_f>& advances)
{
    // A scratch pad to share the allocation of this vector between calls to
    // find_glyphs() to improve the performance.
    auto find_glyphs_scratch = std::vector<lean_vector<glyph_id>>{};

    clear_and_resize(glyphs, graphemes.size(), font_glyph_ids{});
    clear_and_resize(metrics, graphemes.size(), shaper_grapheme_metrics{});
    clear_and_resize(advances, graphemes.size(), unit::pixels(0.0f));

    auto attributes = grapheme_attributes{};
    text_style style = text_style{};
    auto style_font_size = font_size;
    auto i = size_t{0};
    for (auto run_length : run_lengths) {
        assert(run_length != 0);

        auto const run_of_graphemes = graphemes.subspan(i, run_length);
        auto const run_of_glyphs = std::span{glyphs}.subspan(i, run_length);
        auto const run_of_metrics = std::span{metrics}.subspan(i, run_length);
        auto const run_of_advances = std::span{advances}.subspan(i, run_length);

        if (style.empty() or run_of_graphemes.front().attributes() != attributes) {
            attributes = run_of_graphemes.front().attributes();
            style = style_set[attributes];
            style_font_size = font_size * style.scale();
        }

        find_glyphs(run_of_graphemes, style.font_chain(), run_of_glyphs);

        auto font_id = hi::font_id{};
        hi::font* font = nullptr;
        auto font_metrics = hi::font_metrics_px{};
        for (auto j = size_t{0}; j != run_of_graphemes.size(); ++j) {
            auto const& g = run_of_graphemes[j];
            auto const& glyph_ids = run_of_glyphs[j];
            auto& metrics = run_of_metrics[j];
            auto& advance = run_of_advances[j];

            if (font == nullptr or font_id != glyph_ids.font) {
                font_id = glyph_ids.font;
                font = std::addressof(get_font(font_id));
                font_metrics = style_font_size * font->metrics;
            }

            metrics.glyphs = glyph_ids;
            metrics.advance = style_font_size * font->get_advance(glyph_ids.front());
            metrics.cap_height = font_metrics.cap_height;
            metrics.ascender = font_metrics.ascender;
            metrics.descender = font_metrics.descender;
            metrics.line_gap = font_metrics.line_gap;
            metrics.line_spacing = style.line_spacing();
            metrics.paragraph_spacing = style.paragraph_spacing();
            metrics.general_category = ucd_get_general_category(g.starter());
            metrics.bracket_type = ucd_get_bidi_paired_bracket_type(g.starter());

            // If the grapheme is a bracket, then find the mirrored glyph.
            // Only the base glyph is mirrored, the combining glyphs are not.
            // The mirrored glyph is used when the grapheme is displayed in RTL.
            if (metrics.bracket_type != unicode_bidi_paired_bracket_type::n) {
                auto const mirror_cp = ucd_get_bidi_mirroring_glyph(g.starter());
                metrics.mirrored_glyph = font->find_glyph(mirror_cp);
            }

            advance = metrics.advance;
        }

        i += run_length;
    }
}


[[nodiscard]] inline std::vector<int8_t> shaper_collect_embedding_levels(gstring_view text)
{
    //return unicode_bidi_get_embedding_levels(text.begin(), text.end(), unicode_bidi_class::B, [](auto const& g) {
    //    return g.starter();
    //});
    hi_not_implemented();
    return {};
}

/** Fold lines of a text.
 *
 * @param break_opportunities The line break opportunities in the text.
 * @param grapheme_metrics_range The sizing information of each grapheme.
 * @param maximum_line_width The maximum width of a line.
 */
//[[nodiscard]] inline std::vector<size_t> shaper_fold_lines(
//    unicode_line_break_vector const& break_opportunities,
//    std::vector<shaper_grapheme_metrics> const& grapheme_metrics_range,
//    unit::pixels_f maximum_line_width)
//{
//    return unicode_fold_lines(
//        break_opportunities,
//        grapheme_metrics_range,
//        maximum_line_width,
//        [](auto const& x) {
//            return x.advance;
//        },
//        [](auto const& x) {
//            return is_visible(x.general_category);
//        });
//}

struct shaper_line_metrics {
    unit::pixels_f cap_height = unit::pixels(0.0f);
    unit::pixels_f ascender = unit::pixels(0.0f);
    unit::pixels_f descender = unit::pixels(0.0f);
    unit::pixels_f line_gap = unit::pixels(0.0f);
    unit::pixels_f advance = unit::pixels(0.0f);
    unit::pixels_f width = unit::pixels(0.0f);
    float spacing = 0.0f;
};

[[nodiscard]] inline std::vector<shaper_line_metrics> shaper_collect_line_metrics(
    std::vector<shaper_grapheme_metrics> const& grapheme_metrics_range,
    std::vector<size_t> const& line_lengths)
{
    auto r = std::vector<shaper_line_metrics>{};
    r.reserve(line_lengths.size());

    auto it = grapheme_metrics_range.begin();
    for (auto const line_length : line_lengths) {
        assert(line_length > 0);

        auto const it_end = it + line_length;
        auto const line_ends_paragraph = (it_end - 1)->general_category == unicode_general_category::Zp;

        auto const it_visible = rfind_if(it, it_end, [](auto const& x) {
            return is_visible(x.general_category);
        });
        auto const it_visible_end = it_visible != it_end ? it_visible + 1 : it_end;
        auto const visible_line_span = std::span{it, it_visible_end};

        auto metrics = shaper_line_metrics{};
        for (auto const& g : visible_line_span) {
            metrics.cap_height = std::max(metrics.cap_height, g.cap_height);
            metrics.ascender = std::max(metrics.ascender, g.ascender);
            metrics.descender = std::max(metrics.descender, g.descender);
            metrics.line_gap = std::max(metrics.line_gap, g.line_gap);
            metrics.spacing = std::max(metrics.spacing, line_ends_paragraph ? g.paragraph_spacing : g.line_spacing);
            metrics.width += g.advance;
        }

        r.push_back(std::move(metrics));
        it = it_end;
    }

    assert(not r.empty());
    for (auto i = size_t{0}; i != r.size() - 1; ++i) {
        r[i].advance = (r[i].descender + std::max(r[i].line_gap, r[i + 1].line_gap) + r[i + 1].ascender) * r[i].spacing;
    }
    r.back().advance = unit::pixels(0.0f);

    return r;
}

struct shaper_text_metrics {
    /** The maximum width of the text.
     */
    unit::pixels_f width = unit::pixels(0.0f);

    /** Heigth from top-line's cap-height, to bottom's base-line.
     */
    unit::pixels_f height = unit::pixels(0.0f);

    /** The ascender minus cap-height of top line.
     */
    unit::pixels_f overhang = unit::pixels(0.0f);

    /** The descender of the bottom line.
     */
    unit::pixels_f underhang = unit::pixels(0.0f);

    /** Function to determine the baseline of the text.
     */
    std::function<unit::pixels_f(unit::pixels_f)> baseline_function = [](unit::pixels_f height) {
        return unit::pixels(0.0f);
    };
};

[[nodiscard]] inline shaper_text_metrics
shaper_collect_text_metrics(std::vector<shaper_line_metrics> const& line_metrics, hi::vertical_alignment alignment)
{
    auto r = shaper_text_metrics{};

    if (line_metrics.empty()) {
        return r;
    }

    auto const ascender = line_metrics.front().ascender;
    auto const cap_height = line_metrics.front().ascender;
    auto const descender = line_metrics.back().descender;

    r.height = cap_height;
    for (auto const& m : line_metrics) {
        r.width = std::max(r.width, m.width);
        r.height += m.advance;
    }

    // The overhang and underhang are used to determine the margins around the
    // text.
    if (ascender > cap_height) {
        r.overhang = ascender - cap_height;
    }
    r.underhang = descender;

    // The baseline of the middle line, or the average of the two middle lines.
    // The distance from the bottom of the text to the baseline.
    auto const middle_baseline_from_bottom = r.height - [&] {
        auto const i = line_metrics.size() % 2 == 0 ? (line_metrics.size() / 2 - 1) : line_metrics.size() / 2;

        auto const y =
            std::accumulate(line_metrics.begin(), line_metrics.begin() + i + 1, unit::pixels(0.0f), [](auto sum, auto const& m) {
                return sum + m.advance;
            });

        if (line_metrics.size() % 2 == 0) {
            auto const y2 = y + line_metrics[i].advance;
            return (y + y2) / 2.0f;
        } else {
            return y;
        }
    }();

    // Calculate the offset to the middle baseline from the middle of the text.
    auto const middle_baseline_from_middle = middle_baseline_from_bottom - r.height * 0.5f;

    r.baseline_function = [&]() -> std::function<unit::pixels_f(unit::pixels_f)> {
        switch (alignment) {
        case vertical_alignment::top:
            return [=](unit::pixels_f height) {
                return height - cap_height;
            };

        case vertical_alignment::middle:
            return [=](unit::pixels_f height) {
                return height * 0.5f + middle_baseline_from_middle;
            };

        case vertical_alignment::bottom:
            return [](unit::pixels_f height) {
                return unit::pixels(0.0f);
            };
        }
        std::unreachable();
    }();

    return r;
}

[[nodiscard]] inline std::vector<size_t>
shaper_display_order(std::vector<size_t> const& line_sizes, std::vector<int8_t> const& embedding_levels, gstring_view text)
{
    //return unicode_bidi_to_display_order(line_sizes, embedding_levels.begin(), text.begin(), [](auto const& g) {
    //    return ucd_get_bidi_class(g.starter());
    //})
    hi_not_implemented();
    return {};
}

struct shaper_phase1_result {
    /** A list of break opportunities where lines are allowed to be broken.
     */
    unicode_line_break_vector line_break_opportunities;

    /** A list of break opportunities where the edges of words are.
     *
     * This list is used for selecting, or navigating words.
     *
     * It is also used combined with the grapheme's attributes
     * to make runs that can be shaped as a single entity by the
     * font program.
     */
    unicode_word_break_vector word_break_opportunities;

    /** A list of break opportunities for the edges of sentences.
     *
     * This list is used for selecting, or navigating sentences.
     */
    unicode_sentence_break_vector sentence_break_opportunities;

    /**
     * @brief The length of each run of graphemes that can be handled
     *        as a single entity by the font program.
     */
    std::vector<size_t> run_lengths;

    /** Unique ids for each run, assigned to each grapheme.
     *
     * This is used to identify multiple graphemes that are part
     * of a single run, when processing the graphemes in display order
     * instead of logical order.
     */
    std::vector<size_t> run_ids;

    /** Metrics collected for each grapheme.
     */
    std::vector<shaper_grapheme_metrics> grapheme_metrics;

    /** The unicode-bid embedding level for each grapheme and each paragraph.
     *
     * These are two lists appened together:
     *  - Embedding level for each grapheme, followed by
     *  - Embedding level for each paragraph.
     *
     * The embedding levels can also be used to determine which brackets
     * need to be mirrored (when the embedding level is odd).
     */
    std::vector<int8_t> embedding_levels;

    shaper_text_metrics text_metrics;
};

struct shaper_positioned_glyphs {
    font_glyph_ids glyphs = {};
    std::vector<aarectangle> bounding_boxes;
    float advance = 0.0f;
};

//[[nodiscard]] inline std::vector<shaper_positioned_glyphs> shaper_shape_graphemes(
//    gstring_view text,
//    std::vector<shaper_grapheme_metrics> const& grapheme_metrics,
//    std::vector<size_t> const& display_order,
//    std::vector<size_t> const& run_ids,
//    std::vector<size_t> const& line_lengths,
//    std::vector<int8_t> const& embedding_levels)
//{
//    auto advances = std::vector<unit::pixels_f>{};
//    advances.reserve(display_order.size());
//    auto glyphs = std::vector<font_glyph_ids>{};
//    glyphs.reserve(display_order.size());
//    auto bounding_boxes = std::vector<lean_vector<aarectangle>>{};
//    bounding_boxes.reserve(display_order.size());
//
//    auto display_i = size_t{0};
//    for (auto line_length : line_lengths) {
//        assert(line_length > 0);
//
//        auto line_width = unit::pixels(0.0f);
//        auto line_whitespace_width = unit::pixels(0.0f);
//        auto num_whitespaces = size_t{0};
//
//        auto run_id = std::numeric_limits<size_t>::max();
//        auto run_start = size_t{0};
//        auto run_font_id = hi::font_id{};
//        auto run_glyphs = std::vector<glyph_id>{};
//        auto run_is_visible = false;
//        auto run_language = iso_639{};
//        auto run_script = iso_15924{};
//
//        auto shape_run = [&](size_t column) {
//            if (run_is_visible) {
//                auto const& font = get_font(run_font_id);
//
//                auto [run_glyphs, run_bounding_boxes, run_advances] = font.shape_run(run_glyphs, run_language, run_script);
//                assert(run_glyphs.size() == run_bounding_boxes.size());
//                assert(run_advances.size() == column - run_start);
//
//                glyphs.emplace_back(run_font_id, std::move(run_glyphs));
//                bounding_boxes.push_back(std::move(run_bounding_boxes));
//
//                assert(run_start + 1 <= column);
//                for (auto i = run_start + 1; i != column; ++i) {
//                    glyphs.emplace_back();
//                    bounding_boxes.emplace_back();
//                }
//
//                for (auto advance : run_advances) {
//                    advances.push_back(advance);
//                }
//
//                line_width += std::accumulate(advances.begin(), advances.end(), unit::pixels(0.0f));
//
//            } else {
//                // Whitespace does not need glyphs.
//                for (auto i = run_start; i != column; ++i) {
//                    auto const advance = grapheme_metrics[display_order[display_i + i]].advance;
//                    advances.push_back(advance);
//                    glyphs.push_back({});
//                    bounding_boxes.push_back({});
//
//                    line_whitespace_width += advance;
//                    line_width += advance;
//                }
//                num_whitespaces += column - run_start;
//            }
//        };
//
//        for (auto column = size_t{0}; column != line_length; ++column) {
//            auto const logical_i = display_order[display_i + column];
//            auto const grapheme = text[logical_i];
//            auto const metrics = grapheme_metrics[logical_i];
//
//            assert(run_id != run_ids[logical_i] or run_font_id == metrics.glyphs.font);
//            assert(run_id != run_ids[logical_i] or run_language == grapheme.language());
//            assert(run_id != run_ids[logical_i] or run_script == grapheme.script());
//
//            if (run_id != run_ids[logical_i] and run_id != std::numeric_limits<size_t>::max()) {
//                // The run has changed, shape the previous run.
//                shape_run(column);
//            }
//
//            if (run_id != run_ids[logical_i]) {
//                run_id = run_ids[logical_i];
//                run_start = column;
//                run_glyphs.clear();
//                run_is_visible = false;
//            }
//
//            // Record information about this grapheme for the next run.
//            run_font_id = metrics.glyphs.font;
//            assert(metrics.glyphs.size() != 0);
//
//            if (metrics.bracket_type != unicode_bidi_paired_bracket_type::n and
//                embedding_levels[logical_i] % 2 == 1) {
//                run_glyphs.push_back(metrics.mirrored_glyph);
//            } else {
//                run_glyphs.push_back(metrics.glyphs.front());
//            }
//
//            std::copy(metrics.glyphs.begin() + 1, metrics.glyphs.end(), std::back_inserter(run_glyphs));
//            run_is_visible |= is_visible(metrics.general_category);
//            run_language = grapheme.language();
//            run_script = grapheme.script();
//        }
//        // Shape the last run.
//        shape_run(line_length);
//
//        // XXX exclude trailing white space.
//
//        display_i += line_length;
//    }
//}

///** Precalculate text-shaping before knowing actual width of the text.
// *
// * @param text The text to shape.
// * @param font_size The base font size of the text.
// * @param style The text-style-set containing the styles of the text
// *              for different languages and phrasings.
// * @param maximumum_width The maximum width the text is allowed to be.
// * @param alignment The vertical alignment of the text.
// * @return pre-calculated information used mostly for phase 2.
// *         The text-metrics is used for the constraints of a text-widget.
// */
//[[nodiscard]] inline shaper_phase1_result shaper_phase1(
//    gstring text,
//    unit::pixels_per_em_f font_size,
//    text_style_set style,
//    unit::pixels_f maximum_width,
//    vertical_alignment alignment)
//{
//    auto r = shaper_phase1_result{};
//
//    //r.text = std::move(text);
//    //r.line_break_opportunities = unicode_line_break(r.text);
//    //r.word_break_opportunities = unicode_word_break(r.text);
//    //r.sentence_break_opportunities = unicode_sentence_break(r.text);
//    //r.run_lengths = shaper_make_run_lengths(r.text, r.word_break_opportunities);
//    //r.run_ids = shaper_make_run_ids(r.run_lengths);
//    //r.grapheme_metrics = shaper_collect_grapheme_metrics(r.text, r.run_lengths, font_size, style);
//    //r.embedding_levels = shaper_collect_embedding_levels(r.text);
//
//    // The calculations will be redone in phase 2, using the actual-width of the text.
//    //auto const line_sizes = shaper_fold_lines(r.line_break_opportunities, r.grapheme_metrics, maximum_width);
//    //auto const line_metrics = shaper_collect_line_metrics(r.grapheme_metrics, line_sizes);
//    //r.text_metrics = shaper_collect_text_metrics(line_metrics, alignment);
//    return r;
//}
//
///** Get the natural width and height of the text being shaped.
// *
// * @param context The result of the first phase of shaping.
// * @return The size in pixels of the text.
// */
//[[nodiscard]] extent2 shaper_get_natural_size(shaper_phase1_result const& context)
//{
//}
//
//[[nodiscard]] unit::pixels_f shaper_get_height(shaper_phase1_result const& context, unit::pixels_f width)
//{
//
//}
//
//struct shaper_phase2_result {
//    std::vector<size_t> line_lengths;
//    std::vector<shaper_line_metrics> line_metrics;
//    std::vector<size_t> display_order;
//    shaper_text_metrics text_metrics;
//};
//
///** Calculate the final text-shaping.
// *
// * @param phase1 The pre-calculated information from phase 1.
// * @param actual_width The actual width of the text.
// * @return The final text-metrics and display-order.
// */
//[[nodiscard]] inline shaper_phase2_result shaper_phase2(shaper_phase1_result const& phase1, unit::pixels_f actual_width)
//{
//    auto r = shaper_phase2_result{};
//
//    r.line_lengths = shaper_fold_lines(phase1.line_break_opportunities, phase1.grapheme_metrics, actual_width);
//    r.line_metrics = shaper_collect_line_metrics(phase1.grapheme_metrics, r.line_lengths);
//    r.text_metrics = shaper_collect_text_metrics(r.line_metrics, vertical_alignment::top);
//    r.display_order = shaper_display_order(r.line_lengths, phase1.embedding_levels, phase1.text);
//    return r;
//}
}
