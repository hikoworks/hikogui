// Copyright Take Vos 2024.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "ucd_bidi_classes.hpp"
#include "ucd_bidi_mirroring_glyphs.hpp"
#include "ucd_bidi_paired_bracket_types.hpp"
#include "ucd_decompositions.hpp"
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
constexpr auto unicode_bidi_max_depth = 125;

/** Information about a character for the Unicode Bidirectional Algorithm.
 */
struct unicode_bidi_char_info {
    /** The current embedding level of the character.
     */
    int8_t level;

    /** The direction of the character.
     */
    unicode_bidi_class direction;

    /** The original character class of the character.
     */
    unicode_bidi_class original_class;

    /** The paired bracket type of the character.
     */
    unicode_bidi_paired_bracket_type paired_bracket_type;

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

/** Determine the paragraph direction of a paragraph.
 *
 * @tparam rule_X5c If true, the function will end at the matching PDI.
 *                  Used from inside X5c when recursing.
 * @param work_pad
 * @param directions The bidi classes of the characters in the paragraph.
 * @param mode The direction mode of the paragraph. Either L, R, or B (LTR-auto).
 */
template<bool rule_X5c>
[[nodiscard]] constexpr unicode_bidi_class
unicode_bidi_P2(std::span<unicode_bidi_char_info const> work_pad, unicode_bidi_class mode) noexcept
{
    using enum unicode_bidi_class;

    if (mode == L or mode == R) {
        return mode;
    }

    auto isolate_level = size_t{0};
    for (auto const& info : work_pad) {
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
[[nodiscard]] constexpr int8_t unicode_bidi_P3(unicode_bidi_class paragraph_bidi_class) noexcept
{
    return static_cast<int8_t>(paragraph_bidi_class == unicode_bidi_class::AL or paragraph_bidi_class == unicode_bidi_class::R);
}

struct unicode_bidi_X1_context {
    struct stack_entry_type {
        int8_t embedding_level;
        unicode_bidi_class override_status;
        bool isolate_status;
    };

    using stack_type = hi::stack<stack_entry_type, unicode_bidi_max_depth + 2>;

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

        assert(level <= unicode_bidi_max_depth);
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

constexpr void unicode_bidi_X2(unicode_bidi_char_info const& info, unicode_bidi_X1_context& context) noexcept
{
    assert(info.original_class == unicode_bidi_class::RLE);

    auto const next_level = context.next_odd();
    if (next_level <= unicode_bidi_max_depth and context.overflow_isolate_count == 0 and context.overflow_embedding_count == 0) {
        context.push(next_level, unicode_bidi_class::ON, false);

    } else if (context.overflow_isolate_count == 0) {
        ++context.overflow_embedding_count;
    }
}

constexpr void unicode_bidi_X3(unicode_bidi_char_info const& info, unicode_bidi_X1_context& context) noexcept
{
    assert(info.original_class == unicode_bidi_class::LRE);

    auto const next_level = context.next_even();
    if (next_level <= unicode_bidi_max_depth and context.overflow_isolate_count == 0 and context.overflow_embedding_count == 0) {
        context.push(next_level, unicode_bidi_class::ON, false);

    } else if (context.overflow_isolate_count == 0) {
        ++context.overflow_embedding_count;
    }
}

constexpr void unicode_bidi_X4(unicode_bidi_char_info const& info, unicode_bidi_X1_context& context) noexcept
{
    assert(info.original_class == unicode_bidi_class::RLO);

    auto const next_level = context.next_odd();
    if (next_level <= unicode_bidi_max_depth and context.overflow_isolate_count == 0 and context.overflow_embedding_count == 0) {
        context.push(next_level, unicode_bidi_class::R, false);

    } else if (context.overflow_isolate_count == 0) {
        ++context.overflow_embedding_count;
    }
}

constexpr void unicode_bidi_X5(unicode_bidi_char_info const& info, unicode_bidi_X1_context& context) noexcept
{
    assert(info.original_class == unicode_bidi_class::LRO);

    auto const next_level = context.next_even();
    if (next_level <= unicode_bidi_max_depth and context.overflow_isolate_count == 0 and context.overflow_embedding_count == 0) {
        context.push(next_level, unicode_bidi_class::L, false);

    } else if (context.overflow_isolate_count == 0) {
        ++context.overflow_embedding_count;
    }
}

constexpr void unicode_bidi_X5a(unicode_bidi_char_info& info, unicode_bidi_X1_context& context) noexcept
{
    assert(info.original_class == unicode_bidi_class::RLI or info.original_class == unicode_bidi_class::FSI);

    auto const& current = context.current();
    info.level = current.embedding_level;

    if (current.override_status != unicode_bidi_class::ON) {
        info.direction = current.override_status;
    }

    auto const next_level = context.next_odd();
    if (next_level <= unicode_bidi_max_depth and context.overflow_isolate_count == 0 and context.overflow_embedding_count == 0) {
        ++context.valid_isolate_count;
        context.push(next_level, unicode_bidi_class::ON, true);
    } else {
        ++context.overflow_isolate_count;
    }
}

constexpr void unicode_bidi_X5b(unicode_bidi_char_info& info, unicode_bidi_X1_context& context) noexcept
{
    assert(info.original_class == unicode_bidi_class::LRI or info.original_class == unicode_bidi_class::FSI);

    auto const& current = context.current();
    info.level = current.embedding_level;

    if (current.override_status != unicode_bidi_class::ON) {
        info.direction = current.override_status;
    }

    auto const next_level = context.next_even();
    if (next_level <= unicode_bidi_max_depth and context.overflow_isolate_count == 0 and context.overflow_embedding_count == 0) {
        ++context.valid_isolate_count;
        context.push(next_level, unicode_bidi_class::ON, true);
    } else {
        ++context.overflow_isolate_count;
    }
}

constexpr void unicode_bidi_X5c(
    unicode_bidi_char_info& info,
    std::span<unicode_bidi_char_info> work_pad,
    size_t i,
    unicode_bidi_X1_context& context) noexcept
{
    assert(info.original_class == unicode_bidi_class::FSI);

    auto const sub_paragraph_bidi_class = unicode_bidi_P2<true>(work_pad.subspan(i + 1), unicode_bidi_class::B);
    auto const sub_paragraph_embedding_level = unicode_bidi_P3(sub_paragraph_bidi_class);
    if (sub_paragraph_embedding_level == 1) {
        unicode_bidi_X5a(info, context);
    } else {
        unicode_bidi_X5b(info, context);
    }
}

constexpr void unicode_bidi_X6(unicode_bidi_char_info& info, unicode_bidi_X1_context& context) noexcept
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

constexpr void unicode_bidi_X6a(unicode_bidi_char_info& info, unicode_bidi_X1_context& context) noexcept
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

constexpr void unicode_bidi_X7(unicode_bidi_char_info& info, unicode_bidi_X1_context& context) noexcept
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

constexpr void unicode_bidi_X8(unicode_bidi_char_info& info, int8_t paragraph_embedding_level) noexcept
{
    assert(info.original_class == unicode_bidi_class::B);

    info.level = paragraph_embedding_level;
}

constexpr void unicode_bidi_X9(unicode_bidi_char_info& info) noexcept
{
    using enum unicode_bidi_class;

    auto const oc = info.original_class;
    assert(oc == RLE or oc == LRE or oc == RLO or oc == LRO or oc == PDF or oc == BN);

    info.level = -1;
    info.direction = unicode_bidi_class::BN;
}

/** Determine explicit embedding levels and directions.
 *
 * @param directions The bidi classes of the characters in the paragraph.
 * @param embedding_levels The embedding levels of the characters in the paragraph.
 * @param paragraph_embedding_level The embedding level of the paragraph.
 */
constexpr void unicode_bidi_X1(std::span<unicode_bidi_char_info> work_pad, int8_t paragraph_embedding_level) noexcept
{
    using enum unicode_bidi_class;

    auto context = unicode_bidi_X1_context{};
    context.push(paragraph_embedding_level, unicode_bidi_class::ON, false);

    for (auto i = size_t{0}; i != work_pad.size(); ++i) {
        auto& info = work_pad[i];

        switch (info.original_class) {
        case RLE:
            unicode_bidi_X2(info, context);
            unicode_bidi_X9(info);
            break;
        case LRE:
            unicode_bidi_X3(info, context);
            unicode_bidi_X9(info);
            break;
        case RLO:
            unicode_bidi_X4(info, context);
            unicode_bidi_X9(info);
            break;
        case LRO:
            unicode_bidi_X5(info, context);
            unicode_bidi_X9(info);
            break;
        case RLI:
            unicode_bidi_X5a(info, context);
            break;
        case LRI:
            unicode_bidi_X5b(info, context);
            break;
        case FSI:
            unicode_bidi_X5c(info, work_pad, i, context);
            break;
        case PDI:
            unicode_bidi_X6a(info, context);
            break;
        case PDF:
            unicode_bidi_X7(info, context);
            unicode_bidi_X9(info);
            break;
        case B:
            unicode_bidi_X8(info, paragraph_embedding_level);
            break;
        case BN:
            unicode_bidi_X9(info);
            break;
        default:
            unicode_bidi_X6(info, context);
        }
    }
}

struct unicode_bidi_run {
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

/** Determine level-runs.
 *
 * @param embedding_levels The embedding levels of the characters in the paragraph.
 * @return A vector of run-lengths of embedding-levels.
 */
[[nodiscard]] constexpr std::vector<unicode_bidi_run> unicode_bidi_BD7(std::span<unicode_bidi_char_info const> work_pad) noexcept
{
    using enum unicode_bidi_class;

    std::vector<unicode_bidi_run> r;

    if (work_pad.empty()) {
        return r;
    }

    auto run = unicode_bidi_run{0, 1, work_pad[0].level, work_pad[0].direction, work_pad[0].direction};
    for (auto i = size_t{1}; i != work_pad.size(); ++i) {
        auto const& info = work_pad[i];

        if (info.is_removed_by_X9()) {
            continue;
        }

        // Get the information of the first character not removed by X9.
        // A character removed by X9 is assigned an embedding level of -1.
        if (run.embedding_level < 0) {
            run.embedding_level = info.level;
            run.first_class = info.direction;
        }

        if (run.embedding_level != info.level) {
            run.last = i;
            if (run.embedding_level >= 0) {
                r.push_back(std::move(run));
            }
            run = unicode_bidi_run{i, i + 1, info.level, info.direction, info.direction};
        }

        // Get the direction of the last character not removed by X9.
        run.last_class = info.direction;
    }
    run.last = work_pad.size();
    if (run.embedding_level >= 0) {
        r.push_back(std::move(run));
    }

    return r;
}

class unicode_bidi_isolate_run_sequence : public sequence<unicode_bidi_run> {
public:
    using super = sequence<unicode_bidi_run>;
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

[[nodiscard]] constexpr std::pair<unicode_bidi_class, unicode_bidi_class> unicode_bidi_BD13_sos_eos(
    std::span<unicode_bidi_char_info const> work_pad,
    unicode_bidi_isolate_run_sequence& sequence,
    int8_t paragraph_embedding_level)
{
    using enum unicode_bidi_class;

    // Calculate the sos and eos for the isolated run sequence.
    auto const before_embedding_level = [&] {
        for (auto i = sequence.front().first; i != 0; --i) {
            assert(i - 1 < work_pad.size());
            if (not work_pad[i - 1].is_removed_by_X9()) {
                return work_pad[i - 1].level;
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

        assert(sequence.back().last <= work_pad.size());
        for (auto i = sequence.back().last; i != work_pad.size(); ++i) {
            // X9. Retaining BNs and Explicit Formatting Characters.
            if (not work_pad[i].is_removed_by_X9()) {
                return work_pad[i].level;
            }
        }

        // "and if there is none or the last character of the sequence is an
        // isolate initiator (lacking a matching PDI), with the paragraph
        // embedding level"
        return paragraph_embedding_level;
    }();

    auto const sos = std::max(sequence.embedding_level, before_embedding_level) % 2 == 0 ? L : R;
    auto const eos = std::max(sequence.embedding_level, after_embedding_level) % 2 == 0 ? L : R;
    return {sos, eos};
}

/** Determine isolated run sequences.
 *
 * @param directions The bidi classes of the characters in the paragraph.
 * @param runs The run-lengths of embedding-levels.
 * @return A vector of isolated run sequences.
 */
[[nodiscard]] constexpr std::vector<unicode_bidi_isolate_run_sequence> unicode_bidi_BD13(
    std::span<unicode_bidi_char_info const> work_pad,
    std::vector<unicode_bidi_run> runs,
    int8_t paragraph_embedding_level) noexcept
{
    std::vector<unicode_bidi_isolate_run_sequence> r;
    std::reverse(runs.begin(), runs.end());
    while (not runs.empty()) {
        auto s = unicode_bidi_isolate_run_sequence({runs.back()});
        runs.pop_back();

        while (s.has_isolate_initiator() and not runs.empty()) {
            // Search for matching PDI in the run_levels. This should have the same embedding level.
            auto isolation_level = 1;
            for (auto it = std::rbegin(runs); it != std::rend(runs); ++it) {
                if (it->has_PDI() and --isolation_level == 0) {
                    s.push_back(*it);
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

        assert(not s.empty());
        s.embedding_level = s.back().embedding_level;

#if not defined(NDEBUG)
        for (auto const& run : s) {
            assert(run.embedding_level == s.embedding_level);
        }
#endif

        std::tie(s.sos, s.eos) = unicode_bidi_BD13_sos_eos(work_pad, s, paragraph_embedding_level);
        r.push_back(std::move(s));
    }

    return r;
}

constexpr void unicode_bidi_set_direction(
    std::span<unicode_bidi_char_info> work_pad,
    sequence<unicode_bidi_run>::index_iterator first,
    sequence<unicode_bidi_run>::index_iterator last,
    unicode_bidi_class direction) noexcept
{
    for (auto it = first; it != last; ++it) {
        auto const& i = *it;
        auto& info = work_pad[i];

        if (info.is_removed_by_X9()) {
            continue;
        }

        info.direction = direction;
    }
}

constexpr void unicode_bidi_set_direction(
    std::span<unicode_bidi_char_info> work_pad,
    size_t first,
    size_t last,
    unicode_bidi_class direction) noexcept
{
    for (auto i = first; i != last; ++i) {
        auto& info = work_pad[i];

        if (info.is_removed_by_X9()) {
            continue;
        }

        info.direction = direction;
    }
}

constexpr void
unicode_bidi_set_level(std::span<unicode_bidi_char_info> work_pad, size_t first, size_t last, int8_t level) noexcept
{
    for (auto i = first; i != last; ++i) {
        auto& info = work_pad[i];

        if (info.is_removed_by_X9()) {
            continue;
        }

        info.level = level;
    }
}

constexpr void
unicode_bidi_W1(unicode_bidi_isolate_run_sequence const& sequence, std::span<unicode_bidi_char_info> work_pad) noexcept
{
    using enum unicode_bidi_class;

    auto previous_bidi_class = sequence.sos;
    for (auto it = sequence.index_begin(); it != sequence.index_end(); ++it) {
        auto const i = *it;

        assert(i < work_pad.size());
        auto& info = work_pad[i];

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

constexpr void
unicode_bidi_W2(unicode_bidi_isolate_run_sequence const& sequence, std::span<unicode_bidi_char_info> work_pad) noexcept
{
    using enum unicode_bidi_class;

    auto last_strong_direction = sequence.sos;
    for (auto it = sequence.index_begin(); it != sequence.index_end(); ++it) {
        auto const i = *it;

        assert(i < work_pad.size());
        auto& info = work_pad[i];

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

constexpr void
unicode_bidi_W3(unicode_bidi_isolate_run_sequence const& sequence, std::span<unicode_bidi_char_info> work_pad) noexcept
{
    using enum unicode_bidi_class;

    for (auto it = sequence.index_begin(); it != sequence.index_end(); ++it) {
        auto const i = *it;

        assert(i < work_pad.size());
        auto& info = work_pad[i];
        if (info.direction == AL) {
            info.direction = R;
        }
    }
}

constexpr void
unicode_bidi_W4(unicode_bidi_isolate_run_sequence const& sequence, std::span<unicode_bidi_char_info> work_pad) noexcept
{
    using enum unicode_bidi_class;

    unicode_bidi_char_info* second_ptr = nullptr;
    auto first = ON;
    auto second = ON;
    for (auto it = sequence.index_begin(); it != sequence.index_end(); ++it) {
        auto const i = *it;

        assert(i < work_pad.size());
        auto& info = work_pad[i];

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

constexpr void
unicode_bidi_W5(unicode_bidi_isolate_run_sequence const& sequence, std::span<unicode_bidi_char_info> work_pad) noexcept
{
    using enum unicode_bidi_class;

    enum class state_type { idle, found_ET, found_EN };

    auto const null = sequence.index_end();

    auto state = state_type::idle;
    auto start = null;
    for (auto it = sequence.index_begin(); it != sequence.index_end(); ++it) {
        auto const i = *it;

        assert(i < work_pad.size());
        auto& info = work_pad[i];

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
                unicode_bidi_set_direction(work_pad, start, it, EN);
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
        unicode_bidi_set_direction(work_pad, start, sequence.index_end(), EN);
    }
}

constexpr void
unicode_bidi_W6(unicode_bidi_isolate_run_sequence const& sequence, std::span<unicode_bidi_char_info> work_pad) noexcept
{
    using enum unicode_bidi_class;

    for (auto it = sequence.index_begin(); it != sequence.index_end(); ++it) {
        auto const i = *it;

        assert(i < work_pad.size());
        auto& info = work_pad[i];

        if (info.direction == ES or info.direction == CS or info.direction == ET) {
            info.direction = ON;
        }
    }
}

constexpr void
unicode_bidi_W7(unicode_bidi_isolate_run_sequence const& sequence, std::span<unicode_bidi_char_info> work_pad) noexcept
{
    using enum unicode_bidi_class;

    auto last_strong_direction = sequence.sos;
    for (auto it = sequence.index_begin(); it != sequence.index_end(); ++it) {
        auto const i = *it;

        assert(i < work_pad.size());
        auto& info = work_pad[i];

        if (info.direction == R or info.direction == L) {
            last_strong_direction = info.direction;

        } else if (info.direction == EN and last_strong_direction == L) {
            info.direction = L;
        }
    }
}

struct unicode_bidi_bracket_pair {
    unicode_bidi_isolate_run_sequence::index_iterator open;
    unicode_bidi_isolate_run_sequence::index_iterator close;

    [[nodiscard]] constexpr friend auto
    operator<=>(unicode_bidi_bracket_pair const& lhs, unicode_bidi_bracket_pair const& rhs) noexcept
    {
        return lhs.open <=> rhs.open;
    }
};

[[nodiscard]]
constexpr std::vector<unicode_bidi_bracket_pair> unicode_bidi_BD16(
    unicode_bidi_isolate_run_sequence const& sequence,
    std::span<unicode_bidi_char_info const> work_pad,
    std::span<char32_t const> code_points)
{
    struct bracket_start {
        unicode_bidi_isolate_run_sequence::index_iterator it;
        char32_t mirrored_bracket;
    };

    using enum unicode_bidi_class;

    assert(work_pad.size() == code_points.size());

    auto pairs = std::vector<unicode_bidi_bracket_pair>{};
    auto stack = hi::stack<bracket_start, 63>{};

    for (auto it = sequence.index_begin(); it != sequence.index_end(); ++it) {
        auto const i = *it;

        assert(i < work_pad.size());
        auto& info = work_pad[i];
        assert(i < code_points.size());
        auto const code_point = code_points[i];

        if (info.direction == ON) {
            switch (info.paired_bracket_type) {
            case unicode_bidi_paired_bracket_type::o:
                if (stack.full()) {
                    // Stop processing
                    std::sort(pairs.begin(), pairs.end());
                    return pairs;

                } else {
                    // If there is a canonical equivalent of the opening bracket, find it's mirrored glyph
                    // to compare with the closing bracket.
                    auto mirrored_glyph = ucd_get_bidi_mirroring_glyph(code_point);
                    if (auto const canonical_equivalent = ucd_get_decomposition(code_point).canonical_equivalent()) {
                        assert(ucd_get_bidi_paired_bracket_type(*canonical_equivalent) == unicode_bidi_paired_bracket_type::o);

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
                            pairs.emplace_back(jt->it, it);
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

    std::sort(pairs.begin(), pairs.end());
    return pairs;
}

[[nodiscard]] constexpr unicode_bidi_class unicode_bidi_N0_strong(unicode_bidi_class direction)
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

[[nodiscard]] constexpr unicode_bidi_class unicode_bidi_N0_preceding_strong_type(
    std::span<unicode_bidi_char_info const> work_pad,
    unicode_bidi_isolate_run_sequence::index_iterator const& begin,
    unicode_bidi_isolate_run_sequence::index_iterator const& open_bracket,
    unicode_bidi_class sos)
{
    using enum unicode_bidi_class;

    auto it = open_bracket;
    while (it != begin) {
        auto const i = *--it;
        assert(i < work_pad.size());
        if (auto const direction = unicode_bidi_N0_strong(work_pad[i].direction); direction != ON) {
            return direction;
        }
    }

    return sos;
}

[[nodiscard]] constexpr unicode_bidi_class unicode_bidi_N0_enclosed_strong_type(
    unicode_bidi_bracket_pair const& pair,
    std::span<unicode_bidi_char_info const> work_pad,
    unicode_bidi_class embedding_direction)
{
    using enum unicode_bidi_class;

    auto opposite_direction = ON;
    for (auto it = pair.open + 1; it != pair.close; ++it) {
        auto const i = *it;

        assert(i < work_pad.size());
        auto const direction = unicode_bidi_N0_strong(work_pad[i].direction);
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

constexpr void unicode_bidi_N0_marks(
    unicode_bidi_isolate_run_sequence::index_iterator const& first,
    unicode_bidi_isolate_run_sequence::index_iterator const& last,
    std::span<unicode_bidi_char_info> work_pad,
    unicode_bidi_class bracket_direction)
{
    for (auto it = first + 1; it != last; ++it) {
        auto const i = *it;

        assert(i < work_pad.size());
        auto& info = work_pad[i];

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

constexpr void unicode_bidi_N0(
    unicode_bidi_isolate_run_sequence const& sequence,
    std::span<unicode_bidi_char_info> work_pad,
    std::span<char32_t const> code_points)
{
    using enum unicode_bidi_class;

    auto const bracket_pairs = unicode_bidi_BD16(sequence, work_pad, code_points);

    for (auto& pair : bracket_pairs) {
        auto pair_direction = unicode_bidi_N0_enclosed_strong_type(pair, work_pad, sequence.embedding_direction());
        if (pair_direction == ON) {
            continue;
        }

        if (pair_direction != sequence.embedding_direction()) {
            pair_direction = unicode_bidi_N0_preceding_strong_type(work_pad, sequence.index_begin(), pair.open, sequence.sos);

            if (pair_direction == sequence.embedding_direction() or pair_direction == ON) {
                pair_direction = sequence.embedding_direction();
            }
        }

        work_pad[*pair.open].direction = pair_direction;
        work_pad[*pair.close].direction = pair_direction;

        unicode_bidi_N0_marks(pair.open, pair.close, work_pad, pair_direction);
        unicode_bidi_N0_marks(pair.close, sequence.index_end(), work_pad, pair_direction);
    }
}

constexpr void unicode_bidi_N1(unicode_bidi_isolate_run_sequence const& sequence, std::span<unicode_bidi_char_info> work_pad)
{
    using enum unicode_bidi_class;

    auto const null = sequence.index_end();

    auto start = null;
    auto direction_before_NI = sequence.sos == EN or sequence.sos == AN ? R : sequence.sos;
    for (auto it = sequence.index_begin(); it != sequence.index_end(); ++it) {
        auto const i = *it;

        assert(i < work_pad.size());
        auto& info = work_pad[i];

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
                unicode_bidi_set_direction(work_pad, start, it, direction_before_NI);
            }

            start = null;
            direction_before_NI = direction_after_NI;
        }
    }

    auto const direction_after_NI = sequence.eos == EN or sequence.eos == AN ? R : sequence.eos;
    if (start != null and direction_before_NI == direction_after_NI) {
        unicode_bidi_set_direction(work_pad, start, sequence.index_end(), direction_before_NI);
    }
}

constexpr void unicode_bidi_N2(unicode_bidi_isolate_run_sequence const& sequence, std::span<unicode_bidi_char_info> work_pad)
{
    for (auto it = sequence.index_begin(); it != sequence.index_end(); ++it) {
        auto const i = *it;

        assert(i < work_pad.size());
        auto& info = work_pad[i];

        if (is_NI(info.direction)) {
            info.direction = sequence.embedding_direction();
        }
    }
}

constexpr void unicode_bidi_I1_I2(unicode_bidi_isolate_run_sequence const& sequence, std::span<unicode_bidi_char_info> work_pad)
{
    using enum unicode_bidi_class;

    for (auto it = sequence.index_begin(); it != sequence.index_end(); ++it) {
        auto const i = *it;

        assert(i < work_pad.size());
        auto& info = work_pad[i];

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

constexpr void unicode_bidi_X10(
    std::span<unicode_bidi_char_info> work_pad,
    std::span<char32_t const> code_points,
    int8_t paragraph_embedding_level)
{
    using enum unicode_bidi_class;

    assert(work_pad.size() == code_points.size());

    auto const isolated_run_sequence_set = unicode_bidi_BD13(work_pad, unicode_bidi_BD7(work_pad), paragraph_embedding_level);

    for (auto i = size_t{0}; i != isolated_run_sequence_set.size(); ++i) {
        auto const& isolated_run_sequence = isolated_run_sequence_set[i];

        unicode_bidi_W1(isolated_run_sequence, work_pad);
        unicode_bidi_W2(isolated_run_sequence, work_pad);
        unicode_bidi_W3(isolated_run_sequence, work_pad);
        unicode_bidi_W4(isolated_run_sequence, work_pad);
        unicode_bidi_W5(isolated_run_sequence, work_pad);
        unicode_bidi_W6(isolated_run_sequence, work_pad);
        unicode_bidi_W7(isolated_run_sequence, work_pad);
        unicode_bidi_N0(isolated_run_sequence, work_pad, code_points);
        unicode_bidi_N1(isolated_run_sequence, work_pad);
        unicode_bidi_N2(isolated_run_sequence, work_pad);
        unicode_bidi_I1_I2(isolated_run_sequence, work_pad);
    }
}

/** Reset the whitespace characters at the end of the line, segment and paragraph.
 *
 * @param original_directions The original bidi classes of the characters in the line.
 */
constexpr void unicode_bidi_L1(std::span<unicode_bidi_char_info> work_pad, int8_t paragraph_embedding_level) noexcept
{
    using enum unicode_bidi_class;

    auto const null = work_pad.size();
    auto start = null;

    for (auto i = size_t{0}; i != work_pad.size(); ++i) {
        auto& info = work_pad[i];

        if (info.is_removed_by_X9()) {
            continue;
        }

        if (info.original_class == unicode_bidi_class::WS or is_isolate_formatter(info.original_class)) {
            if (start == null) {
                start = i;
            }

        } else if (info.original_class == unicode_bidi_class::S or info.original_class == unicode_bidi_class::B) {
            if (start != null) {
                unicode_bidi_set_level(work_pad, start, i, paragraph_embedding_level);
            }
            work_pad[i].level = paragraph_embedding_level;
            start = null;

        } else {
            start = null;
        }
    }

    // Handle sequence of WS, FSI, LRI, RLI, PDI at the end of a line.
    if (start != null) {
        unicode_bidi_set_level(work_pad, start, work_pad.size(), paragraph_embedding_level);
    }
}

[[nodiscard]] constexpr std::vector<size_t> unicode_bidi_L2(std::span<unicode_bidi_char_info> work_pad) noexcept
{
    auto highest = int8_t{0};
    auto lowest_odd = int8_t{127};
    for (auto const info : work_pad) {
        highest = std::max(highest, info.level);
        if (info.level % 2 == 1) {
            lowest_odd = std::min(lowest_odd, info.level);
        }
    }

    auto r = std::vector<size_t>{};
    r.reserve(work_pad.size());
    for (auto i = size_t{0}; i != work_pad.size(); ++i) {
        r.push_back(i);
    }

    if (highest < lowest_odd) {
        return r;
    }

    auto const null = work_pad.size();

    for (int8_t level = highest; level >= lowest_odd; --level) {
        auto start = null;
        for (auto i = size_t{0}; i != work_pad.size(); ++i) {
            auto const& info = work_pad[i];

            if (info.is_removed_by_X9()) {
                continue;
            }

            if (start == null) {
                if (info.level >= level) {
                    start = i;
                }

            } else if (info.level < level) {
                std::reverse(r.begin() + start, r.begin() + i);
                start = null;
            }
        }

        // Reverse the last sequence if it was not closed.
        if (start != null) {
            std::reverse(r.begin() + start, r.end());
        }
    }

    return r;
}

// unicode_bidi_L3() is not implemented due to the algorithm working on only
// the base code-point of each grapheme.

constexpr void unicode_bidi_L4(std::span<unicode_bidi_char_info> work_pad, std::span<char32_t> code_points) noexcept
{
    using enum unicode_bidi_class;

    assert(work_pad.size() == code_points.size());

    for (auto i = size_t{0}; i != work_pad.size(); ++i) {
        auto& info = work_pad[i];
        auto& code_point = code_points[i];

        if (info.paired_bracket_type != unicode_bidi_paired_bracket_type::n and info.direction == R) {
            code_point = ucd_get_bidi_mirroring_glyph(code_point);
        }
    }
}

[[nodiscard]] constexpr std::vector<size_t> unicode_bidi_P1_split(std::span<unicode_bidi_char_info const> work_pad) noexcept
{
    auto r = std::vector<size_t>{};

    auto start = size_t{0};
    for (auto i = size_t{0}; i != work_pad.size(); ++i) {
        if (work_pad[i].direction == unicode_bidi_class::B) {
            r.emplace_back(i + 1 - start);
            start = i + 1;
        }
    }

    if (start != work_pad.size()) {
        r.emplace_back(work_pad.size() - start);
    }

    return r;
}

/** Apply the bidirectional algorithm at paragraph level, before line breaking.\
 *
 * @param directions The bidi classes of the characters in the paragraph.
 * @param embedding_levels The embedding levels of the characters in the
 *                         paragraph.
 * @param brackets The paired bracket types of the characters in the paragraph.
 * @param code_points The code points of the characters in the paragraph.
 * @param mode The mode for determining the paragraph direction. L: The
 *             paragraph direction is left-to-right. R: The paragraph direction
 *             is right-to-left. B: The paragraph direction is determined by the
 *             first strong character in the paragraph.
 * @return The paragraph embedding levels, one for each paragraph in the text.
 */
constexpr std::vector<int8_t> unicode_bidi_P1(
    std::span<unicode_bidi_char_info> work_pad,
    std::span<char32_t const> code_points,
    unicode_bidi_class mode) noexcept
{
    assert(mode == unicode_bidi_class::L or mode == unicode_bidi_class::R or mode == unicode_bidi_class::B);
    assert(work_pad.size() == code_points.size());

    auto paragraph_sizes = unicode_bidi_P1_split(work_pad);
    auto r = std::vector<int8_t>{};
    r.reserve(paragraph_sizes.size());
    auto i = size_t{0};
    for (auto paragraph_size : paragraph_sizes) {
        auto const p_first = i;
        auto const p_last = i + paragraph_size;
        i += paragraph_size;

        assert(p_first < p_last);
        assert(p_last <= work_pad.size());

        auto const p_work_pad = work_pad.subspan(p_first, p_last - p_first);
        auto const p_code_points = code_points.subspan(p_first, p_last - p_first);

        auto const paragraph_direction = unicode_bidi_P2<false>(p_work_pad, mode);
        auto const paragraph_embedding_level = unicode_bidi_P3(paragraph_direction);
        r.push_back(paragraph_embedding_level);

        unicode_bidi_X1(p_work_pad, paragraph_embedding_level);
        unicode_bidi_X10(p_work_pad, p_code_points, paragraph_embedding_level);
    }

    return r;
}

struct unicode_bidi_on_paragraphs_result {
    std::vector<unicode_bidi_char_info> work_pad;
    std::vector<int8_t> paragraph_embedding_levels;
};

[[nodiscard]] constexpr unicode_bidi_on_paragraphs_result
unicode_bidi_on_paragraphs(std::span<char32_t const> code_points, unicode_bidi_class mode)
{
    assert(mode == unicode_bidi_class::L or mode == unicode_bidi_class::R or mode == unicode_bidi_class::B);

    auto work_pad = std::vector<unicode_bidi_char_info>(code_points.size(), unicode_bidi_char_info{});

    for (auto i = size_t{0}; i != code_points.size(); ++i) {
        work_pad[i].original_class = ucd_get_bidi_class(code_points[i]);
        work_pad[i].direction = work_pad[i].original_class;
        work_pad[i].level = 0;
        work_pad[i].paired_bracket_type = ucd_get_bidi_paired_bracket_type(code_points[i]);
    }

    auto paragraph_embedding_levels = unicode_bidi_P1(work_pad, code_points, mode);

    return {std::move(work_pad), std::move(paragraph_embedding_levels)};
}
}