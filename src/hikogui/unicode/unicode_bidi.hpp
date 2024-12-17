// Copyright Take Vos 2024.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "ucd_bidi_classes.hpp"
#include "ucd_bidi_mirroring_glyphs.hpp"
#include "ucd_bidi_paired_bracket_types.hpp"
#include "ucd_decompositions.hpp"
#include "../algorithm/algorithm.hpp"
#include "../container/container.hpp"
#include "../macros.hpp"
#include <gsl/gsl>
#include <span>
#include <vector>
#include <tuple>
#include <algorithm>
#include <generator>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <compare>

hi_export_module(hikogui.unicode : unicode_bidi);

hi_export namespace hi::inline v1 {
class unicode_bidi {
public:
    constexpr unicode_bidi() noexcept = default;
    constexpr unicode_bidi(unicode_bidi const&) noexcept = default;
    constexpr unicode_bidi(unicode_bidi&&) noexcept = default;
    constexpr unicode_bidi& operator=(unicode_bidi const&) noexcept = default;
    constexpr unicode_bidi& operator=(unicode_bidi&&) noexcept = default;

    /** Retrieve the ordering of text.
     *
     * @return A list in display-order of the indexes in logical order.
     */
    [[nodiscard]] constexpr std::span<size_t> ordering() noexcept
    {
        return _display_order;
    }

    /** Apply the first phase of the bidi-algorithm, before line wrapping.
     *
     * @param code_points The code-points to apply the algorithm on.
     * @param mode The default paragraph direction. L, R or B.
     */
    constexpr void set_text(std::span<char32_t const> code_points, unicode_bidi_class mode)
    {
        assert(mode == unicode_bidi_class::L or mode == unicode_bidi_class::R or mode == unicode_bidi_class::B);

        clear_and_resize(_infos, code_points.size(), info_type{});
        for (auto i = size_t{0}; i != code_points.size(); ++i) {
            _infos[i].original_class = ucd_get_bidi_class(code_points[i]);
            _infos[i].direction = _infos[i].original_class;
            _infos[i].level = 0;
            _infos[i].paired_bracket_type = ucd_get_bidi_paired_bracket_type(code_points[i]);
        }

        auto paragraph_sizes = P1_split();
        P1(code_points, paragraph_sizes, mode);
    }

    /** Apply the second phase of the bidi-algorithm.
     *
     * @param line_sizes The length of each line, after line wrapping.
     */
    constexpr void set_lines(std::span<size_t const> line_sizes)
    {
        clear_and_resize(_display_order, _infos.size(), size_t{});
        for (auto i = size_t{0}; i != _infos.size(); ++i) {
            _display_order[i] = i;
        }

        auto i = size_t{0};
        auto paragraph_i = size_t{0};
        for (auto line_size : line_sizes) {
            auto const l_first = i;
            auto const l_last = i + line_size;
            assert(l_first <= l_last);

            assert(paragraph_i < _paragraph_embedding_levels.size());
            L1(l_first, l_last, _paragraph_embedding_levels[paragraph_i]);
            L2(l_first, l_last);

            if (l_first != l_last and _infos[l_last - 1].direction == unicode_bidi_class::B) {
                ++paragraph_i;
            }
            i = l_last;
        }
    }

    /** Retrieve the paragraph embedding levels.
     *
     * @return The paragraph embedding levels.
     */
    [[nodiscard]] constexpr std::span<int8_t> paragraph_embedding_levels() noexcept
    {
        return _paragraph_embedding_levels;
    }

    /** Retrieve the embedding levels.
     *
     * @note This function is slow and should only be used for unit-tests.
     * @return The embedding levels.
     */
    [[nodiscard]] constexpr std::vector<int8_t> embedding_levels()
    {
        auto r = std::vector<int8_t>(_infos.size(), 0);
        for (auto i = size_t{0}; i != _infos.size(); ++i) {
            r[i] = _infos[i].level;
        }
        return r;
    }

    /** Set the unicode bidi class of each character.
     *
     * @note This function does a incomplete version of the bidi-algorithm and
     *       should only be used for unit-tests.
     * @param classes The unicode bidi class of each character.
     */
    constexpr void set_classes(std::span<unicode_bidi_class const> classes, unicode_bidi_class mode)
    {
        clear_and_resize(_infos, classes.size(), info_type{});
        for (auto i = size_t{0}; i != classes.size(); ++i) {
            _infos[i].original_class = classes[i];
            _infos[i].direction = classes[i];
            _infos[i].level = 0;
            _infos[i].paired_bracket_type = unicode_bidi_paired_bracket_type::n;
        }

        auto code_points = std::vector<char32_t>(_infos.size(), U'\0');

        auto paragraph_sizes = P1_split();
        P1(code_points, paragraph_sizes, mode);
    }

private:
    constexpr static auto max_depth = 125;

    /** Information about a character for the Unicode Bidirectional Algorithm.
     */
    struct info_type {
        /** The current embedding level of the character.
         */
        int8_t level = 0;

        /** Copy of the level before X10 is executed.
         */
        int8_t level_before_X10 = 0;

        /** The direction of the character.
         */
        unicode_bidi_class direction = unicode_bidi_class::BN;

        /** The original character class of the character.
         */
        unicode_bidi_class original_class = unicode_bidi_class::BN;

        /** The paired bracket type of the character.
         */
        unicode_bidi_paired_bracket_type paired_bracket_type = unicode_bidi_paired_bracket_type::n;

        [[nodiscard]] constexpr bool is_removed_by_X9() const noexcept
        {
            using enum unicode_bidi_class;

            auto const oc = original_class;
            if (level < 0) {
                assert(direction == BN);
                assert(oc == BN or oc == LRE or oc == RLE or oc == LRO or oc == RLO or oc == PDF);
            } else {
                assert(direction != BN);
                assert(oc != BN and oc != LRE and oc != RLE and oc != LRO and oc != RLO and oc != PDF);
            }

            return level < 0;
        }
    };

    struct run_type {
        size_t first;
        size_t last;
        int8_t embedding_level;
        unicode_bidi_class first_class;
        unicode_bidi_class last_class;

        [[nodiscard]] constexpr bool has_isolate_initiator() const noexcept
        {
            using enum unicode_bidi_class;
            return last_class == LRI or last_class == RLI or last_class == FSI;
        }

        [[nodiscard]] constexpr bool has_PDI() const noexcept
        {
            using enum unicode_bidi_class;
            return first_class == PDI;
        }
    };

    class run_sequence_type : public sequence<run_type> {
    public:
        using super = sequence<run_type>;
        using super::super;

        int8_t embedding_level;
        unicode_bidi_class sos;
        unicode_bidi_class eos;

        [[nodiscard]] constexpr unicode_bidi_class embedding_direction() const noexcept
        {
            return embedding_level % 2 == 0 ? unicode_bidi_class::L : unicode_bidi_class::R;
        }

        [[nodiscard]] constexpr bool has_isolate_initiator() const noexcept
        {
            return back().has_isolate_initiator();
        }
    };

    struct bracket_pair_type {
        run_sequence_type::index_iterator open;
        run_sequence_type::index_iterator close;

        [[nodiscard]] constexpr friend auto operator<=>(bracket_pair_type const& lhs, bracket_pair_type const& rhs) noexcept
        {
            return lhs.open <=> rhs.open;
        }
    };

    struct X1_context_type {
        struct stack_entry_type {
            int8_t embedding_level;
            unicode_bidi_class override_status;
            bool isolate_status;
        };

        using stack_type = hi::stack<stack_entry_type, max_depth + 2>;

        stack_type status_stack = {};
        size_t overflow_isolate_count = 0;
        size_t overflow_embedding_count = 0;
        size_t valid_isolate_count = 0;

        [[nodiscard]] constexpr size_t size() const noexcept
        {
            return status_stack.size();
        }

        [[nodiscard]] constexpr stack_entry_type const& current() const noexcept
        {
            assert(not status_stack.empty());
            return status_stack.back();
        }

        constexpr void push(int8_t level, unicode_bidi_class override_status, bool isolate_status) noexcept
        {
            using enum unicode_bidi_class;

            assert(level <= max_depth);
            assert(not status_stack.full());
            assert(override_status == ON or override_status == L or override_status == R);
            status_stack.emplace_back(level, override_status, isolate_status);
        }

        constexpr void pop()
        {
            assert(not status_stack.empty());
            status_stack.pop_back();
        }

        [[nodiscard]] constexpr int8_t next_even() const noexcept
        {
            auto const level = current().embedding_level;
            return (level % 2 == 0) ? level + 2 : level + 1;
        }

        [[nodiscard]] constexpr int8_t next_odd() const noexcept
        {
            auto const level = current().embedding_level;
            return (level % 2 == 1) ? level + 2 : level + 1;
        }
    };

    /** Information about a character for the Unicode Bidirectional Algorithm.
     */
    std::vector<info_type> _infos;

    /** The display order of the characters.
     */
    std::vector<size_t> _display_order;

    /** The embedding levels of each paragraph.
     */
    std::vector<int8_t> _paragraph_embedding_levels;

    /** Determine the paragraph direction of a paragraph.
     *
     * @tparam rule_X5c If true, the function will end at the matching PDI.
     *                  Used from inside X5c when recursing.
     * @param first The first character of the paragraph.
     * @param last The last character of the paragraph.
     * @param mode The direction mode of the paragraph. Either L, R, or B (LTR-auto).
     */
    template<bool rule_X5c>
    [[nodiscard]] constexpr unicode_bidi_class P2(size_t first, size_t last, unicode_bidi_class mode) noexcept
    {
        using enum unicode_bidi_class;

        assert(first <= last);

        if (mode == L or mode == R) {
            return mode;
        }

        auto isolate_level = size_t{0};
        for (auto i = first; i != last; ++i) {
            auto const& info = _infos[i];

            switch (info.original_class) {
            case L:
            case AL:
            case R:
                if (isolate_level == 0) {
                    return info.original_class;
                }
                break;
            case LRI:
            case RLI:
            case FSI:
                ++isolate_level;
                break;
            case PDI:
                if (isolate_level > 0) {
                    --isolate_level;

                } else if (rule_X5c) {
                    // End at the matching PDI, when recursing for rule X5c.
                    return mode;
                }
                break;
            default:;
            }
        }
        return mode;
    }

    /** Determine the paragraph embedding level of a paragraph.
     *
     * @param paragraph_bidi_class The paragraph direction.
     * @return The paragraph's embedding level.
     */
    [[nodiscard]] constexpr static int8_t P3(unicode_bidi_class paragraph_bidi_class) noexcept
    {
        return static_cast<int8_t>(
            paragraph_bidi_class == unicode_bidi_class::AL or paragraph_bidi_class == unicode_bidi_class::R);
    }

    constexpr static void X2(info_type const& info, X1_context_type& context) noexcept
    {
        assert(info.original_class == unicode_bidi_class::RLE);

        auto const next_level = context.next_odd();
        if (next_level <= max_depth and context.overflow_isolate_count == 0 and context.overflow_embedding_count == 0) {
            context.push(next_level, unicode_bidi_class::ON, false);

        } else if (context.overflow_isolate_count == 0) {
            ++context.overflow_embedding_count;
        }
    }

    constexpr static void X3(info_type const& info, X1_context_type& context) noexcept
    {
        assert(info.original_class == unicode_bidi_class::LRE);

        auto const next_level = context.next_even();
        if (next_level <= max_depth and context.overflow_isolate_count == 0 and context.overflow_embedding_count == 0) {
            context.push(next_level, unicode_bidi_class::ON, false);

        } else if (context.overflow_isolate_count == 0) {
            ++context.overflow_embedding_count;
        }
    }

    constexpr static void X4(info_type const& info, X1_context_type& context) noexcept
    {
        assert(info.original_class == unicode_bidi_class::RLO);

        auto const next_level = context.next_odd();
        if (next_level <= max_depth and context.overflow_isolate_count == 0 and context.overflow_embedding_count == 0) {
            context.push(next_level, unicode_bidi_class::R, false);

        } else if (context.overflow_isolate_count == 0) {
            ++context.overflow_embedding_count;
        }
    }

    constexpr static void X5(info_type const& info, X1_context_type& context) noexcept
    {
        assert(info.original_class == unicode_bidi_class::LRO);

        auto const next_level = context.next_even();
        if (next_level <= max_depth and context.overflow_isolate_count == 0 and context.overflow_embedding_count == 0) {
            context.push(next_level, unicode_bidi_class::L, false);

        } else if (context.overflow_isolate_count == 0) {
            ++context.overflow_embedding_count;
        }
    }

    constexpr static void X5a(info_type& info, X1_context_type& context) noexcept
    {
        assert(info.original_class == unicode_bidi_class::RLI or info.original_class == unicode_bidi_class::FSI);

        auto const& current = context.current();
        info.level = current.embedding_level;

        if (current.override_status != unicode_bidi_class::ON) {
            info.direction = current.override_status;
        }

        auto const next_level = context.next_odd();
        if (next_level <= max_depth and context.overflow_isolate_count == 0 and context.overflow_embedding_count == 0) {
            ++context.valid_isolate_count;
            context.push(next_level, unicode_bidi_class::ON, true);
        } else {
            ++context.overflow_isolate_count;
        }
    }

    constexpr static void X5b(info_type& info, X1_context_type& context) noexcept
    {
        assert(info.original_class == unicode_bidi_class::LRI or info.original_class == unicode_bidi_class::FSI);

        auto const& current = context.current();
        info.level = current.embedding_level;

        if (current.override_status != unicode_bidi_class::ON) {
            info.direction = current.override_status;
        }

        auto const next_level = context.next_even();
        if (next_level <= max_depth and context.overflow_isolate_count == 0 and context.overflow_embedding_count == 0) {
            ++context.valid_isolate_count;
            context.push(next_level, unicode_bidi_class::ON, true);
        } else {
            ++context.overflow_isolate_count;
        }
    }

    constexpr void X5c(info_type& info, size_t i, size_t last, X1_context_type& context) noexcept
    {
        assert(info.original_class == unicode_bidi_class::FSI);

        auto const sub_paragraph_bidi_class = P2<true>(i + 1, last, unicode_bidi_class::B);
        auto const sub_paragraph_embedding_level = P3(sub_paragraph_bidi_class);
        if (sub_paragraph_embedding_level == 1) {
            X5a(info, context);
        } else {
            X5b(info, context);
        }
    }

    constexpr static void X6(info_type& info, X1_context_type& context) noexcept
    {
        using enum unicode_bidi_class;

        auto const oc = info.original_class;
        assert(
            oc != RLE and oc != LRE and oc != RLO and oc != LRO and oc != RLI and oc != LRI and oc != FSI and oc != PDI and
            oc != PDF and oc != B and oc != BN);

        info.level = context.current().embedding_level;
        if (context.current().override_status != unicode_bidi_class::ON) {
            info.direction = context.current().override_status;
        }
    }

    constexpr static void X6a(info_type& info, X1_context_type& context) noexcept
    {
        assert(info.original_class == unicode_bidi_class::PDI);

        if (context.overflow_isolate_count != 0) {
            --context.overflow_isolate_count;

        } else if (context.valid_isolate_count == 0) {
            // Mismatched PDI, do nothing.
            ;

        } else {
            context.overflow_embedding_count = 0;
            while (context.current().isolate_status == false) {
                context.pop();
            }
            context.pop();
            --context.valid_isolate_count;
        }

        auto const& current = context.current();

        info.level = current.embedding_level;
        if (current.override_status != unicode_bidi_class::ON) {
            info.direction = current.override_status;
        }
    }

    constexpr static void X7(info_type& info, X1_context_type& context) noexcept
    {
        assert(info.original_class == unicode_bidi_class::PDF);

        if (context.overflow_isolate_count != 0) {
            // PDF is in scope of isolate, wait until the isolate is terminated.
            ;

        } else if (context.overflow_embedding_count != 0) {
            --context.overflow_embedding_count;

        } else if (context.current().isolate_status == false and context.size() >= 2) {
            context.pop();

        } else {
            // PDF does not match embedding character.
        }
    }

    constexpr static void X8(info_type& info, int8_t paragraph_embedding_level) noexcept
    {
        assert(info.original_class == unicode_bidi_class::B);

        info.level = paragraph_embedding_level;
    }

    constexpr static void X9(info_type& info) noexcept
    {
        using enum unicode_bidi_class;

        auto const oc = info.original_class;
        assert(oc == RLE or oc == LRE or oc == RLO or oc == LRO or oc == PDF or oc == BN);

        info.level = -1;
        info.direction = unicode_bidi_class::BN;
    }

    /** Determine explicit embedding levels and directions.
     *
     * @param first index to the first character in the paragraph.
     * @param last index beyond the last character in the paragraph.
     * @param paragraph_embedding_level The embedding level of the paragraph.
     */
    constexpr void X1(size_t first, size_t last, int8_t paragraph_embedding_level) noexcept
    {
        using enum unicode_bidi_class;

        auto context = X1_context_type{};
        context.push(paragraph_embedding_level, unicode_bidi_class::ON, false);

        for (auto i = first; i != last; ++i) {
            auto& info = _infos[i];

            switch (info.original_class) {
            case RLE:
                X2(info, context);
                X9(info);
                break;
            case LRE:
                X3(info, context);
                X9(info);
                break;
            case RLO:
                X4(info, context);
                X9(info);
                break;
            case LRO:
                X5(info, context);
                X9(info);
                break;
            case RLI:
                X5a(info, context);
                break;
            case LRI:
                X5b(info, context);
                break;
            case FSI:
                X5c(info, i, last, context);
                break;
            case PDI:
                X6a(info, context);
                break;
            case PDF:
                X7(info, context);
                X9(info);
                break;
            case B:
                X8(info, paragraph_embedding_level);
                break;
            case BN:
                X9(info);
                break;
            default:
                X6(info, context);
            }

            // Make copy of the level before X10 is executed.
            info.level_before_X10 = info.level;
        }
    }

    /** A run of characters with the same embedding level.
     */
    std::vector<run_type> _BD7_runs;
    /** Determine level-runs.
     *
     * @param first index to the first character in the paragraph.
     * @param last index beyond the last character in the paragraph.
     * @return A vector of run-lengths of embedding-levels.
     */
    [[nodiscard]] constexpr std::vector<run_type> const& BD7(size_t first, size_t last) noexcept
    {
        using enum unicode_bidi_class;

        _BD7_runs.clear();

        if (first == last) {
            return _BD7_runs;
        }

        auto run = run_type{first, first + 1, _infos[first].level_before_X10, _infos[first].direction, _infos[first].direction};
        for (auto i = first + 1; i != last; ++i) {
            auto const& info = _infos[i];

            if (info.is_removed_by_X9()) {
                continue;
            }

            // Get the information of the first character not removed by X9.
            // A character removed by X9 is assigned an embedding level of -1.
            if (run.embedding_level < 0) {
                run.embedding_level = info.level_before_X10;
                run.first_class = info.direction;
            }

            if (run.embedding_level != info.level_before_X10) {
                run.last = i;
                if (run.embedding_level >= 0) {
                    _BD7_runs.push_back(std::move(run));
                }
                run = run_type{i, i + 1, info.level_before_X10, info.direction, info.direction};
            }

            // Get the direction of the last character not removed by X9.
            run.last_class = info.direction;
        }
        run.last = _infos.size();
        if (run.embedding_level >= 0) {
            _BD7_runs.push_back(std::move(run));
        }

        return _BD7_runs;
    }

    constexpr void BD13_sos_eos(run_sequence_type& sequence, int8_t paragraph_embedding_level)
    {
        using enum unicode_bidi_class;

        // Calculate the sos and eos for the isolated run sequence.
        auto const before_embedding_level = [&] {
            for (auto i = sequence.front().first; i != 0; --i) {
                assert(i - 1 < _infos.size());
                if (not _infos[i - 1].is_removed_by_X9()) {
                    return _infos[i - 1].level_before_X10;
                }
            }
            return paragraph_embedding_level;
        }();

        auto const after_embedding_level = [&] {
            if (sequence.has_isolate_initiator()) {
                // "and if there is none or the last character of the sequence
                // is an isolate initiator (lacking a matching PDI), with the
                // paragraph embedding level"
                return paragraph_embedding_level;
            }

            assert(sequence.back().last <= _infos.size());
            for (auto i = sequence.back().last; i != _infos.size(); ++i) {
                // X9. Retaining BNs and Explicit Formatting Characters.
                if (not _infos[i].is_removed_by_X9()) {
                    return _infos[i].level_before_X10;
                }
            }

            // "and if there is none or the last character of the sequence is an
            // isolate initiator (lacking a matching PDI), with the paragraph
            // embedding level"
            return paragraph_embedding_level;
        }();

        sequence.sos = std::max(sequence.embedding_level, before_embedding_level) % 2 == 0 ? L : R;
        sequence.eos = std::max(sequence.embedding_level, after_embedding_level) % 2 == 0 ? L : R;
    }

    run_sequence_type _BD13_run_sequence;
    /** Determine isolated run sequences.
     *
     * @pre _runs are initialized by `BD7()`.
     * @param paragraph_embedding_level The embedding level of the paragraph.
     * @param Function A callable object that will be called for each isolated run sequence.
     * @return A vector of isolated run sequences.
     */
    template<std::invocable<run_sequence_type const&> Function>
    constexpr void BD13(int8_t paragraph_embedding_level, std::vector<run_type> runs, Function const& function) noexcept
    {
        std::reverse(runs.begin(), runs.end());
        while (not runs.empty()) {
            _BD13_run_sequence.clear();
            _BD13_run_sequence.push_back(runs.back());
            runs.pop_back();

            while (_BD13_run_sequence.has_isolate_initiator() and not runs.empty()) {
                // Search for matching PDI in the run_levels. This should have the same embedding level.
                auto isolation_level = 1;
                for (auto it = std::rbegin(runs); it != std::rend(runs); ++it) {
                    if (it->has_PDI() and --isolation_level == 0) {
                        _BD13_run_sequence.push_back(*it);
                        runs.erase(std::next(it).base());
                        break;
                    }
                    if (it->has_isolate_initiator()) {
                        ++isolation_level;
                    }
                }

                if (isolation_level != 0) {
                    // No PDI that matches the isolate initiator of this isolated run sequence.
                    break;
                }
            }

            assert(not _BD13_run_sequence.empty());
            _BD13_run_sequence.embedding_level = _BD13_run_sequence.back().embedding_level;

#if not defined(NDEBUG)
            for (auto const& run : _BD13_run_sequence) {
                assert(run.embedding_level == _BD13_run_sequence.embedding_level);
            }
#endif

            BD13_sos_eos(_BD13_run_sequence, paragraph_embedding_level);
            function(_BD13_run_sequence);
        }
    }

    constexpr void set_direction(
        sequence<run_type>::index_iterator first,
        sequence<run_type>::index_iterator last,
        unicode_bidi_class direction) noexcept
    {
        for (auto it = first; it != last; ++it) {
            auto const& i = *it;
            auto& info = _infos[i];

            if (info.is_removed_by_X9()) {
                continue;
            }

            info.direction = direction;
        }
    }

    constexpr void set_direction(size_t first, size_t last, unicode_bidi_class direction) noexcept
    {
        for (auto i = first; i != last; ++i) {
            auto& info = _infos[i];

            if (info.is_removed_by_X9()) {
                continue;
            }

            info.direction = direction;
        }
    }

    constexpr void set_level(size_t first, size_t last, int8_t level) noexcept
    {
        for (auto i = first; i != last; ++i) {
            auto& info = _infos[i];

            if (info.is_removed_by_X9()) {
                continue;
            }

            info.level = level;
        }
    }

    constexpr void W1(run_sequence_type const& sequence) noexcept
    {
        using enum unicode_bidi_class;

        auto previous_bidi_class = sequence.sos;
        for (auto it = sequence.index_begin(); it != sequence.index_end(); ++it) {
            auto const i = *it;

            assert(i < _infos.size());
            auto& info = _infos[i];

            if (info.is_removed_by_X9()) {
                continue;
            }

            if (info.direction == NSM) {
                switch (previous_bidi_class) {
                case LRI:
                case RLI:
                case FSI:
                case PDI:
                    info.direction = ON;
                    break;
                default:
                    info.direction = previous_bidi_class;
                    break;
                }
            }

            previous_bidi_class = info.direction;
        }
    }

    constexpr void W2(run_sequence_type const& sequence) noexcept
    {
        using enum unicode_bidi_class;

        auto last_strong_direction = sequence.sos;
        for (auto it = sequence.index_begin(); it != sequence.index_end(); ++it) {
            auto const i = *it;

            assert(i < _infos.size());
            auto& info = _infos[i];

            switch (info.direction) {
            case R:
            case L:
            case AL:
                last_strong_direction = info.direction;
                break;
            case EN:
                if (last_strong_direction == AL) {
                    info.direction = AN;
                }
                break;
            default:;
            }
        }
    }

    constexpr void W3(run_sequence_type const& sequence) noexcept
    {
        using enum unicode_bidi_class;

        for (auto it = sequence.index_begin(); it != sequence.index_end(); ++it) {
            auto const i = *it;

            assert(i < _infos.size());
            auto& info = _infos[i];
            if (info.direction == AL) {
                info.direction = R;
            }
        }
    }

    constexpr void W4(run_sequence_type const& sequence) noexcept
    {
        using enum unicode_bidi_class;

        info_type* second_ptr = nullptr;
        auto first = ON;
        auto second = ON;
        for (auto it = sequence.index_begin(); it != sequence.index_end(); ++it) {
            auto const i = *it;

            assert(i < _infos.size());
            auto& info = _infos[i];

            auto const third = info.direction;

            if (info.is_removed_by_X9()) {
                continue;
            }

            if (first == EN and (second == ES or second == CS) and third == EN) {
                assert(second_ptr != nullptr);
                second_ptr->direction = EN;

            } else if (first == AN and second == CS and third == AN) {
                assert(second_ptr != nullptr);
                second_ptr->direction = AN;
            }

            first = second;
            second = third;
            second_ptr = &info;
        }
    }

    constexpr void W5(run_sequence_type const& sequence) noexcept
    {
        using enum unicode_bidi_class;

        enum class state_type { idle, found_ET, found_EN };

        auto const null = sequence.index_end();

        auto state = state_type::idle;
        auto start = null;
        for (auto it = sequence.index_begin(); it != sequence.index_end(); ++it) {
            auto const i = *it;

            assert(i < _infos.size());
            auto& info = _infos[i];

            if (info.is_removed_by_X9()) {
                continue;
            }

            switch (state) {
            case state_type::idle:
                if (info.direction == ET) {
                    state = state_type::found_ET;
                    start = it;
                } else if (info.direction == EN) {
                    state = state_type::found_EN;
                    start = it;
                }
                break;

            case state_type::found_ET:
                if (info.direction == EN) {
                    state = state_type::found_EN;
                } else if (info.direction == ET) {
                    state = state_type::found_ET;
                } else {
                    state = state_type::idle;
                    start = null;
                }
                break;

            case state_type::found_EN:
                if (info.direction == EN or info.direction == ET) {
                    state = state_type::found_EN;
                } else {
                    assert(start != null);
                    set_direction(start, it, EN);
                    state = state_type::idle;
                    start = null;
                }
                break;

            default:
                std::unreachable();
            }
        }

        if (state == state_type::found_EN) {
            assert(start != null);
            set_direction(start, sequence.index_end(), EN);
        }
    }

    constexpr void W6(run_sequence_type const& sequence) noexcept
    {
        using enum unicode_bidi_class;

        for (auto it = sequence.index_begin(); it != sequence.index_end(); ++it) {
            auto const i = *it;

            assert(i < _infos.size());
            auto& info = _infos[i];

            if (info.direction == ES or info.direction == CS or info.direction == ET) {
                info.direction = ON;
            }
        }
    }

    constexpr void W7(run_sequence_type const& sequence) noexcept
    {
        using enum unicode_bidi_class;

        auto last_strong_direction = sequence.sos;
        for (auto it = sequence.index_begin(); it != sequence.index_end(); ++it) {
            auto const i = *it;

            assert(i < _infos.size());
            auto& info = _infos[i];

            if (info.direction == R or info.direction == L) {
                last_strong_direction = info.direction;

            } else if (info.direction == EN and last_strong_direction == L) {
                info.direction = L;
            }
        }
    }

    std::vector<bracket_pair_type> _BD16_bracket_pairs = {};
    /** Determine bracket pairs.
     *
     * @param code_points The code points of the full text.
     */
    [[nodiscard]] constexpr std::vector<bracket_pair_type> const&
    BD16(std::span<char32_t const> code_points, run_sequence_type const& sequence)
    {
        struct bracket_start {
            run_sequence_type::index_iterator it;
            char32_t mirrored_bracket;
        };

        using enum unicode_bidi_class;

        assert(_infos.size() == code_points.size());

        _BD16_bracket_pairs.clear();

        auto stack = hi::stack<bracket_start, 63>{};
        for (auto it = sequence.index_begin(); it != sequence.index_end(); ++it) {
            auto const i = *it;

            assert(i < _infos.size());
            auto& info = _infos[i];
            assert(i < code_points.size());
            auto const code_point = code_points[i];

            if (info.direction == ON) {
                switch (info.paired_bracket_type) {
                case unicode_bidi_paired_bracket_type::o:
                    if (stack.full()) {
                        // Stop processing
                        std::sort(_BD16_bracket_pairs.begin(), _BD16_bracket_pairs.end());
                        return _BD16_bracket_pairs;

                    } else {
                        // If there is a canonical equivalent of the opening bracket, find it's mirrored glyph
                        // to compare with the closing bracket.
                        auto mirrored_glyph = ucd_get_bidi_mirroring_glyph(code_point);
                        if (auto const canonical_equivalent = ucd_get_decomposition(code_point).canonical_equivalent()) {
                            assert(
                                ucd_get_bidi_paired_bracket_type(*canonical_equivalent) == unicode_bidi_paired_bracket_type::o);

                            mirrored_glyph = ucd_get_bidi_mirroring_glyph(*canonical_equivalent);
                        }

                        stack.emplace_back(it, mirrored_glyph);
                    }
                    break;

                case unicode_bidi_paired_bracket_type::c:
                    {
                        auto const canonical_equivalent = ucd_get_decomposition(code_point).canonical_equivalent();
                        auto jt = stack.end();
                        while (jt != stack.begin()) {
                            --jt;

                            if (jt->mirrored_bracket == code_point or
                                (canonical_equivalent and jt->mirrored_bracket == *canonical_equivalent)) {
                                _BD16_bracket_pairs.emplace_back(jt->it, it);
                                stack.pop_back(jt);
                                break;
                            }
                        }
                    }
                    break;

                default:;
                }
            }
        }

        std::sort(_BD16_bracket_pairs.begin(), _BD16_bracket_pairs.end());
        return _BD16_bracket_pairs;
    }

    [[nodiscard]] constexpr static unicode_bidi_class N0_strong(unicode_bidi_class direction)
    {
        using enum unicode_bidi_class;

        switch (direction) {
        case L:
            return L;
        case R:
        case EN:
        case AN:
            return R;
        default:
            return ON;
        }
    }

    [[nodiscard]] constexpr unicode_bidi_class N0_preceding_strong_type(
        run_sequence_type::index_iterator const& begin,
        run_sequence_type::index_iterator const& open_bracket,
        unicode_bidi_class sos)
    {
        using enum unicode_bidi_class;

        auto it = open_bracket;
        while (it != begin) {
            auto const i = *--it;
            assert(i < _infos.size());
            if (auto const direction = N0_strong(_infos[i].direction); direction != ON) {
                return direction;
            }
        }

        return sos;
    }

    [[nodiscard]] constexpr unicode_bidi_class
    N0_enclosed_strong_type(bracket_pair_type const& pair, unicode_bidi_class embedding_direction)
    {
        using enum unicode_bidi_class;

        auto opposite_direction = ON;
        for (auto it = pair.open + 1; it != pair.close; ++it) {
            auto const i = *it;

            assert(i < _infos.size());
            auto const direction = N0_strong(_infos[i].direction);
            if (direction == ON) {
                continue;
            }
            if (direction == embedding_direction) {
                return direction;
            }
            opposite_direction = direction;
        }

        return opposite_direction;
    }

    constexpr void N0_marks(
        run_sequence_type::index_iterator const& first,
        run_sequence_type::index_iterator const& last,
        unicode_bidi_class bracket_direction)
    {
        for (auto it = first + 1; it != last; ++it) {
            auto const i = *it;

            assert(i < _infos.size());
            auto& info = _infos[i];

            if (info.is_removed_by_X9()) {
                continue;
            }

            if (info.original_class == unicode_bidi_class::NSM) {
                info.direction = bracket_direction;
            } else {
                break;
            }
        }
    }

    constexpr void N0(run_sequence_type const& sequence, std::span<char32_t const> code_points)
    {
        using enum unicode_bidi_class;

        for (auto& bracket_pair : BD16(code_points, sequence)) {
            auto direction = N0_enclosed_strong_type(bracket_pair, sequence.embedding_direction());
            if (direction == ON) {
                continue;
            }

            if (direction != sequence.embedding_direction()) {
                direction = N0_preceding_strong_type(sequence.index_begin(), bracket_pair.open, sequence.sos);

                if (direction == sequence.embedding_direction() or direction == ON) {
                    direction = sequence.embedding_direction();
                }
            }

            _infos[*bracket_pair.open].direction = direction;
            _infos[*bracket_pair.close].direction = direction;

            N0_marks(bracket_pair.open, bracket_pair.close, direction);
            N0_marks(bracket_pair.close, sequence.index_end(), direction);
        }
    }

    constexpr void N1(run_sequence_type const& sequence)
    {
        using enum unicode_bidi_class;

        auto const null = sequence.index_end();

        auto start = null;
        auto direction_before_NI = sequence.sos == EN or sequence.sos == AN ? R : sequence.sos;
        for (auto it = sequence.index_begin(); it != sequence.index_end(); ++it) {
            auto const i = *it;

            assert(i < _infos.size());
            auto& info = _infos[i];

            if (info.is_removed_by_X9()) {
                continue;
            }

            if (is_NI(info.direction)) {
                if (start == null) {
                    start = it;
                }

            } else {
                auto const direction_after_NI = info.direction == EN or info.direction == AN ? R : info.direction;
                if (start != null and direction_before_NI == direction_after_NI) {
                    set_direction(start, it, direction_before_NI);
                }

                start = null;
                direction_before_NI = direction_after_NI;
            }
        }

        auto const direction_after_NI = sequence.eos == EN or sequence.eos == AN ? R : sequence.eos;
        if (start != null and direction_before_NI == direction_after_NI) {
            set_direction(start, sequence.index_end(), direction_before_NI);
        }
    }

    constexpr void N2(run_sequence_type const& sequence)
    {
        for (auto it = sequence.index_begin(); it != sequence.index_end(); ++it) {
            auto const i = *it;

            assert(i < _infos.size());
            auto& info = _infos[i];

            if (is_NI(info.direction)) {
                info.direction = sequence.embedding_direction();
            }
        }
    }

    constexpr void I1_I2(run_sequence_type const& sequence)
    {
        using enum unicode_bidi_class;

        for (auto it = sequence.index_begin(); it != sequence.index_end(); ++it) {
            auto const i = *it;

            assert(i < _infos.size());
            auto& info = _infos[i];

            if ((info.level % 2) == 0) {
                // I1
                if (info.direction == R) {
                    info.level += 1;
                } else if (info.direction == AN or info.direction == EN) {
                    info.level += 2;
                }
            } else {
                // I2
                if (info.direction == L or info.direction == AN or info.direction == EN) {
                    info.level += 1;
                }
            }
        }
    }

    /** Determine implicit embedding levels.
     *
     * @param code_points The code points of the characters of the text.
     * @param first index to the first character in the paragraph.
     * @param last index beyond the last character in the paragraph.
     * @param paragraph_embedding_level The embedding level of the paragraph.
     */
    constexpr void X10(std::span<char32_t const> code_points, size_t first, size_t last, int8_t paragraph_embedding_level)
    {
        using enum unicode_bidi_class;

        assert(_infos.size() == code_points.size());

        BD13(paragraph_embedding_level, BD7(first, last), [this, code_points](run_sequence_type const& sequence) {
            W1(sequence);
            W2(sequence);
            W3(sequence);
            W4(sequence);
            W5(sequence);
            W6(sequence);
            W7(sequence);
            N0(sequence, code_points);
            N1(sequence);
            N2(sequence);
            I1_I2(sequence);
        });
    }

    /** Reset the whitespace characters at the end of the line, segment and
     * paragraph.
     *
     * @param first index to the first character in the line.
     * @param last index beyond the last character in the line.
     * @param paragraph_embedding_level The embedding level of the paragraph, to
     *        which this line belongs.
     */
    constexpr void L1(size_t first, size_t last, int8_t paragraph_embedding_level) noexcept
    {
        using enum unicode_bidi_class;

        auto const null = last;
        auto start = null;

        for (auto i = first; i != last; ++i) {
            auto& info = _infos[i];

            if (info.is_removed_by_X9()) {
                continue;
            }

            if (info.original_class == unicode_bidi_class::WS or is_isolate_formatter(info.original_class)) {
                if (start == null) {
                    start = i;
                }

            } else if (info.original_class == unicode_bidi_class::S or info.original_class == unicode_bidi_class::B) {
                if (start != null) {
                    set_level(start, i, paragraph_embedding_level);
                }
                _infos[i].level = paragraph_embedding_level;
                start = null;

            } else {
                start = null;
            }
        }

        // Handle sequence of WS, FSI, LRI, RLI, PDI at the end of a line.
        if (start != null) {
            set_level(start, _infos.size(), paragraph_embedding_level);
        }
    }

    /** Reverse runs of right-to-left characters.
     *
     * @param first index to the first character in the line.
     * @param last index beyond the last character in the line.
     */
    constexpr void L2(size_t first, size_t last) noexcept
    {
        assert(first < last);

        auto highest = int8_t{0};
        auto lowest_odd = int8_t{127};

        for (auto i = first; i != last; ++i) {
            auto const& info = _infos[i];
            highest = std::max(highest, info.level);
            if (info.level % 2 == 1) {
                lowest_odd = std::min(lowest_odd, info.level);
            }
        }

        if (highest < lowest_odd) {
            return;
        }

        auto const null = last;
        for (int8_t level = highest; level >= lowest_odd; --level) {
            auto start = null;
            for (auto i = first; i != last; ++i) {
                auto const& info = _infos[i];

                if (info.is_removed_by_X9()) {
                    continue;
                }

                if (start == null) {
                    if (info.level >= level) {
                        start = i;
                    }

                } else if (info.level < level) {
                    std::reverse(_display_order.begin() + start, _display_order.begin() + i);
                    start = null;
                }
            }

            // Reverse the last sequence if it was not closed.
            if (start != null) {
                std::reverse(_display_order.begin() + start, _display_order.end());
            }
        }
    }

    // unicode_bidi_L3() is not implemented due to the algorithm working on only
    // the base code-point of each grapheme.

    constexpr void L4(std::span<char32_t> code_points) noexcept
    {
        using enum unicode_bidi_class;

        assert(_infos.size() == code_points.size());

        for (auto i = size_t{0}; i != _infos.size(); ++i) {
            auto& info = _infos[i];
            auto& code_point = code_points[i];

            if (info.paired_bracket_type != unicode_bidi_paired_bracket_type::n and info.direction == R) {
                code_point = ucd_get_bidi_mirroring_glyph(code_point);
            }
        }
    }

    std::vector<size_t> _P1_paragraph_sizes;
    /** Split the text into paragraphs.
     *
     * @return The number of graphemes per paragraph.
     */
    [[nodiscard]] constexpr std::vector<size_t> const& P1_split() noexcept
    {
        _P1_paragraph_sizes.clear();

        auto start = size_t{0};
        for (auto i = size_t{0}; i != _infos.size(); ++i) {
            if (_infos[i].direction == unicode_bidi_class::B) {
                _P1_paragraph_sizes.push_back(i + 1 - start);
                start = i + 1;
            }
        }

        if (start != _infos.size()) {
            _P1_paragraph_sizes.push_back(_infos.size() - start);
        }

        return _P1_paragraph_sizes;
    }

    /** Apply the bidirectional algorithm at paragraph level, before line breaking.
     *
     * @post _paragraph_embedding_levels contains the embedding levels of each paragraph.
     * @param code_points The code points of the characters in the paragraph.
     * @param paragraph_sizes The number of characters in each paragraph.
     * @param mode The mode for determining the paragraph direction. L: The
     *             paragraph direction is left-to-right. R: The paragraph direction
     *             is right-to-left. B: The paragraph direction is determined by the
     *             first strong character in the paragraph.
     * @return The paragraph embedding levels, one for each paragraph in the text.
     */
    constexpr void
    P1(std::span<char32_t const> code_points, std::vector<size_t> const& paragraph_sizes, unicode_bidi_class mode) noexcept
    {
        assert(mode == unicode_bidi_class::L or mode == unicode_bidi_class::R or mode == unicode_bidi_class::B);
        assert(_infos.size() == code_points.size());

        _paragraph_embedding_levels.clear();
        _paragraph_embedding_levels.reserve(paragraph_sizes.size());

        auto i = size_t{0};
        for (auto paragraph_size : paragraph_sizes) {
            auto const p_first = i;
            auto const p_last = i + paragraph_size;
            i += paragraph_size;

            assert(p_first <= p_last);
            assert(p_last <= _infos.size());

            auto const paragraph_direction = P2<false>(p_first, p_last, mode);
            auto const paragraph_embedding_level = P3(paragraph_direction);
            _paragraph_embedding_levels.push_back(paragraph_embedding_level);

            X1(p_first, p_last, paragraph_embedding_level);
            X10(code_points, p_first, p_last, paragraph_embedding_level);
        }
    }
};
}
