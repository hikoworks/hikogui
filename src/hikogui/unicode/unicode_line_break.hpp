// Copyright Take Vos 2022.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

/** @file unicode/unicode_line_break.hpp
 */

#pragma once

#include "unicode_break_opportunity.hpp"
#include "ucd_general_categories.hpp"
#include "ucd_grapheme_cluster_breaks.hpp"
#include "ucd_line_break_classes.hpp"
#include "ucd_east_asian_widths.hpp"
#include "../algorithm/algorithm.hpp"
#include "../utility/utility.hpp"
#include "../macros.hpp"
#include <cstdint>
#include <vector>
#include <algorithm>
#include <numeric>
#include <ranges>
#include <string_view>

hi_export_module(hikogui.unicode.unicode_line_break);

hi_export namespace hi::inline v1 {

class unicode_line_break {
public:
    constexpr unicode_line_break() noexcept = default;
    constexpr unicode_line_break(unicode_line_break const&) noexcept = default;
    constexpr unicode_line_break(unicode_line_break&&) noexcept = default;
    constexpr unicode_line_break& operator=(unicode_line_break const&) noexcept = default;
    constexpr unicode_line_break& operator=(unicode_line_break&&) noexcept = default;

    [[nodiscard]] constexpr std::span<unicode_break_opportunity const> opportunities() const noexcept
    {
        return _opportunities;
    }

    constexpr void set_text(std::span<char32_t const> base_code_points)
    {
        auto const text_size = base_code_points.size();

        clear_and_resize(_opportunities, text_size + 1, unicode_break_opportunity::unassigned);
        clear_and_resize(_infos, text_size, info_type{});

        LB1(base_code_points);
        LB2_3();
        LB4_8a();
        LB9();
        LB10();
        LB11_31();
    }

    /** Fold the text to a maximum width.
     *
     * @param advances The advances of the code points.
     * @param maximum_width The maximum width of a line.
     * @param [out] r The number of code points in each line.
     * @return The maximum width of the text after folding.
     */
    template<typename T, std::ranges::random_access_range R>
    [[nodiscard]] constexpr T fold(R const& advances, T maximum_width, std::vector<size_t>& r) const
        requires (std::convertible_to<std::ranges::range_value_t<R>, T>)
    {
        r.clear();

        auto width = T{};
        auto i = size_t{0};
        while (i != advances.size()) {
            auto const [next_i, line_width] = fit_line(advances, i, maximum_width);
            assert(next_i > i);

            r.push_back(next_i - i);

            i = next_i;
            width = std::max(width, line_width);
        }

        return width;
    }

private:
    struct info_type {
        unicode_line_break_class original_class = unicode_line_break_class::XX;
        unicode_line_break_class current_class = unicode_line_break_class::XX;
        bool is_extended_pictographic = false;
        bool is_Cn = false;
        unicode_east_asian_width east_asian_width = unicode_east_asian_width::A;

        constexpr info_type() noexcept = default;
        constexpr info_type(info_type const&) noexcept = default;
        constexpr info_type(info_type&&) noexcept = default;
        constexpr info_type& operator=(info_type const&) noexcept = default;
        constexpr info_type& operator=(info_type&&) noexcept = default;

        constexpr explicit info_type(
            unicode_line_break_class break_class,
            bool is_Cn,
            bool is_extended_pictographic,
            unicode_east_asian_width east_asian_width) noexcept :
            original_class(break_class),
            current_class(break_class),
            is_Cn(is_Cn),
            is_extended_pictographic(is_extended_pictographic),
            east_asian_width(east_asian_width)
        {
        }

        constexpr explicit operator unicode_line_break_class() const noexcept
        {
            return current_class;
        }

        constexpr info_type& operator|=(unicode_line_break_class rhs) noexcept
        {
            current_class = rhs;
            return *this;
        }

        [[nodiscard]] constexpr bool operator==(unicode_line_break_class rhs) const noexcept
        {
            return current_class == rhs;
        }

        [[nodiscard]] constexpr bool operator==(unicode_east_asian_width rhs) const noexcept
        {
            return east_asian_width == rhs;
        }

        [[nodiscard]] constexpr bool is_white_space() const noexcept
        {
            using enum unicode_line_break_class;
            auto const c = current_class;
            return c == SP or c == ZW or c == BK or c == CR or c == LF or c == NL;
        }
    };

    std::vector<info_type> _infos;
    unicode_line_break_vector _opportunities;

    constexpr void LB1(std::span<char32_t const> code_points) noexcept
    {
        auto i = size_t{0};
        for (auto code_point : code_points) {
            auto const east_asian_width = ucd_get_east_asian_width(code_point);
            auto const break_class = ucd_get_line_break_class(code_point);
            auto const general_category = ucd_get_general_category(code_point);
            auto const grapheme_cluster_break = ucd_get_grapheme_cluster_break(code_point);

            auto const resolved_break_class = [&]() {
                switch (break_class) {
                    using enum unicode_line_break_class;
                case AI:
                case SG:
                case XX:
                    return AL;
                case CJ:
                    return NS;
                case SA:
                    return is_Mn_or_Mc(general_category) ? CM : AL;
                default:
                    return break_class;
                }
            }();

            _infos[i++] = info_type{
                resolved_break_class,
                general_category == unicode_general_category::Cn,
                grapheme_cluster_break == unicode_grapheme_cluster_break::Extended_Pictographic,
                east_asian_width};
        }
    }

    constexpr void LB2_3() noexcept
    {
        assert(not _opportunities.empty());
        // LB2
        _opportunities.front() = unicode_break_opportunity::no;
        // LB3
        _opportunities.back() = unicode_break_opportunity::mandatory;
    }

    template<typename MatchFunc>
    constexpr void LB_walk(MatchFunc match_func) noexcept
    {
        using enum unicode_line_break_class;

        if (_infos.empty()) {
            return;
        }

        auto cur = _infos.begin();
        auto const last = _infos.end() - 1;
        auto const last2 = _infos.end();
        auto opportunity = _opportunities.begin() + 1;

        auto cur_sp_class = XX;
        auto cur_nu_class = XX;
        auto prev_class = XX;
        auto num_ri = 0_uz;
        while (cur != last) {
            auto const next = cur + 1;
            auto const cur_class = unicode_line_break_class{*cur};
            auto const next2_class = cur + 2 == last2 ? XX : unicode_line_break_class{*(cur + 2)};

            // Keep track of classes followed by zero or more SP.
            if (cur_class != SP) {
                cur_sp_class = cur_class;
            }

            // Keep track of a "NU (NU|SY|IS)*" and "NU (NU|SY|IS)* (CL|CP)?".
            if (cur_nu_class == CL) {
                // Only a single CL|CP class may be at the end, then the number is closed.
                cur_nu_class = XX;
            } else if (cur_nu_class == NU) {
                if (cur_class == CL or cur_class == CP) {
                    cur_nu_class = CL;
                } else if (cur_class != NU and cur_class != SY and cur_class != IS) {
                    cur_nu_class = XX;
                }
            } else if (cur_class == NU) {
                cur_nu_class = NU;
            }

            // Keep track of consecutive RI, but only count the actual RIs.
            if (cur->original_class == RI) {
                ++num_ri;
            } else if (*cur != RI) {
                num_ri = 0;
            }

            if (*opportunity == unicode_break_opportunity::unassigned) {
                *opportunity = match_func(prev_class, cur, next, next2_class, cur_sp_class, cur_nu_class, num_ri);
            }

            prev_class = cur_class;
            cur = next;
            ++opportunity;
        }
    }

    constexpr void LB4_8a() noexcept
    {
        LB_walk([](auto const prev,
                   auto const cur,
                   auto const next,
                   auto const next2,
                   auto const cur_sp,
                   auto const cur_nu,
                   auto const num_ri) {
            using enum unicode_break_opportunity;
            using enum unicode_line_break_class;
            if (*cur == BK) {
                return mandatory; // LB4: 4.0
            } else if (*cur == CR and *next == LF) {
                return no; // LB5: 5.01
            } else if (*cur == CR or *cur == LF or *cur == NL) {
                return mandatory; // LB5: 5.02, 5.03, 5.04
            } else if (*next == BK or *next == CR or *next == LF or *next == NL) {
                return no; // LB6: 6.0
            } else if (*next == SP or *next == ZW) {
                return no; // LB7: 7.01, 7.02
            } else if (cur_sp == ZW) {
                return yes; // LB8: 8.0
            } else if (*cur == ZWJ) {
                return no; // LB8a: 8.1
            } else {
                return unassigned;
            }
        });
    }

    constexpr void LB9() noexcept
    {
        using enum unicode_line_break_class;
        using enum unicode_break_opportunity;

        if (_infos.empty()) {
            return;
        }

        auto cur = _infos.begin();
        auto const last = _infos.end() - 1;
        auto opportunity = _opportunities.begin() + 1;

        auto X = XX;
        while (cur != last) {
            auto const next = cur + 1;

            if ((*cur == CM or *cur == ZWJ) and X != XX) {
                // Treat all CM/ZWJ as X (if there is an X).
                *cur |= X;
            } else {
                // Reset X on non-CM/ZWJ.
                X = XX;
            }

            if ((*cur != BK and *cur != CR and *cur != LF and *cur != NL and *cur != SP and *cur != ZW) and
                (*next == CM or *next == ZWJ)) {
                // [^BK CR LF NL SP ZW] x [CM ZWJ]*
                *opportunity = no;

                if (X == XX) {
                    // The first character of [^BK CR LF NL SP ZW] x [CM ZWJ]* => X
                    X = static_cast<unicode_line_break_class>(*cur);
                }
            }

            cur = next;
            ++opportunity;
        }
    }

    constexpr void LB10() noexcept
    {
        using enum unicode_line_break_class;

        for (auto& x : _infos) {
            if (x == CM or x == ZWJ) {
                x |= AL;
            }
        }
    }

    constexpr void LB11_31() noexcept
    {
        LB_walk([&](auto const prev,
                    auto const cur,
                    auto const next,
                    auto const next2,
                    auto const cur_sp,
                    auto const cur_nu,
                    auto const num_ri) {
            using enum unicode_break_opportunity;
            using enum unicode_line_break_class;
            using enum unicode_east_asian_width;

            if (*cur == WJ or *next == WJ) {
                return no; // LB11: 11.01, 11.02
            } else if (*cur == GL) {
                return no; // LB12: 12.0
            } else if (*cur != SP and *cur != BA and *cur != HY and *next == GL) {
                return no; // LB12a: 12.1
            } else if (*next == CL or *next == CP or *next == EX or *next == IS or *next == SY) {
                return no; // LB13: 13.0
            } else if (cur_sp == OP) {
                return no; // LB14: 14.0
            } else if (cur_sp == QU and *next == OP) {
                return no; // LB15: 15.0
            } else if ((cur_sp == CL or cur_sp == CP) and *next == NS) {
                return no; // LB16: 16.0
            } else if (cur_sp == B2 and *next == B2) {
                return no; // LB17: 17.0
            } else if (*cur == SP) {
                return yes; // LB18: 18.0
            } else if (*cur == QU or *next == QU) {
                return no; // LB19: 19.01, 19.02
            } else if (*cur == CB or *next == CB) {
                return yes; // LB20: 20.01, 20.02
            } else if (*cur == BB or *next == BA or *next == HY or *next == NS) {
                return no; // LB21: 21.01, 21.02, 21.03, 21.04
            } else if (prev == HL and (*cur == HY or *cur == BA)) {
                return no; // LB21a: 21.1
            } else if (*cur == SY and *next == HL) {
                return no; // LB21b: 21.2
            } else if (*next == IN) {
                return no; // LB22: 22.0
            } else if ((*cur == AL or *cur == HL) and *next == NU) {
                return no; // LB23: 23.02
            } else if (*cur == NU and (*next == AL or *next == HL)) {
                return no; // LB23: 23.03
            } else if (*cur == PR and (*next == ID or *next == EB or *next == EM)) {
                return no; // LB23a: 23.12
            } else if ((*cur == ID or *cur == EB or *cur == EM) and *next == PO) {
                return no; // LB23a: 23.13
            } else if ((*cur == PR or *cur == PO) and (*next == AL or *next == HL)) {
                return no; // LB24: 24.02
            } else if ((*cur == AL or *cur == HL) and (*next == PR or *next == PO)) {
                return no; // LB24: 24.03
            } else if (
                (*cur == PR or *cur == PO) and ((*next == OP and next2 == NU) or (*next == HY and next2 == NU) or *next == NU)) {
                return no; // LB25: 25.01
            } else if ((*cur == OP or *cur == HY) and *next == NU) {
                return no; // LB25: 25.02
            } else if (*cur == NU and (*next == NU or *next == SY or *next == IS)) {
                return no; // LB25: 25.03
            } else if (cur_nu == NU and (*next == NU or *next == SY or *next == IS or *next == CL or *next == CP)) {
                return no; // LB25: 25.04
            } else if ((cur_nu == NU or cur_nu == CL) and (*next == PO or *next == PR)) {
                return no; // LB25: 25.05
            } else if (*cur == JL and (*next == JL or *next == JV or *next == H2 or *next == H3)) {
                return no; // LB26: 26.01
            } else if ((*cur == JV or *cur == H2) and (*next == JV or *next == JT)) {
                return no; // LB26: 26.02
            } else if ((*cur == JT or *cur == H3) and *next == JT) {
                return no; // LB26: 26.03
            } else if ((*cur == JL or *cur == JV or *cur == JT or *cur == H2 or *cur == H3) and *next == PO) {
                return no; // LB27: 27.01
            } else if (*cur == PR and (*next == JL or *next == JV or *next == JT or *next == H2 or *next == H3)) {
                return no; // LB27: 27.02
            } else if ((*cur == AL or *cur == HL) and (*next == AL or *next == HL)) {
                return no; // LB28: 28.0
            } else if (*cur == IS and (*next == AL or *next == HL)) {
                return no; // LB29: 29.0
            } else if ((*cur == AL or *cur == HL or *cur == NU) and (*next == OP and *next != F and *next != W and *next != H)) {
                return no; // LB30: 30.01
            } else if ((*cur == CP and *cur != F and *cur != W and *cur != H) and (*next == AL or *next == HL or *next == NU)) {
                return no; // LB30: 30.02
            } else if (*cur == RI and *next == RI and (num_ri % 2) == 1) {
                return no; // LB30a: 30.11, 30.12, 30.13
            } else if (*cur == EB and *next == EM) {
                return no; // LB30b: 30.21
            } else if (cur->is_extended_pictographic and cur->is_Cn and *next == EM) {
                return no; // LB30b: 30.22
            } else {
                return yes; // LB31: 999.0
            }
        });
    }

    /** Calculate the width of a line, excluding trailing whitespace.
     *
     * @tparam T The type of the width.
     * @param advances Width of each character in the text.
     * @param first Index to the first character of the line
     * @param last Index one past the last character of the line.
     * @return The width of the line.
     */
    template<typename T>
    [[nodiscard]] constexpr T width(std::span<T const> advances, size_t first, size_t last) noexcept
    {
        auto const first_info = _infos.begin() + first;
        auto const last_info = _infos.begin() + last;

        // Find the position of the last visible character.
        auto const last_visible_it = rfind_if_not(first_info, last_info, [](auto const& x) { return x.is_white_space(); });
        if (last_visible_it == last_info) {
            // None of the characters on this line are visible.
            return T{};
        }

        // Calculate the width of the line, excluding the trailing whitespace.
        auto const first_advance = advances.begin() + first;
        auto const last_advance = advances.begin() + std::distance(first_info, last_visible_it) + 1;
        return std::accumulate(first_advance, last_advance, T{}, [&](auto acc, auto x) {
            return acc + x;
        });
    }

    /** Get the width of the entire text.
     *
     * @tparam T The type of the width.
     * @param advances Width of each character in the text.
     * @param line_lengths Number of characters on each line.
     * @return The width of the text.
     */
    template<typename T>
    [[nodiscard]] constexpr T width(std::span<T const> advances, std::span<size_t const> line_lengths) noexcept
    {
        auto max_width = T{};
        auto i = size_t{0};
        for (auto length : line_lengths) {
            inplace_max(max_width, width(advances, i, i + length));
            i += length;
        }
        return max_width;
    }

    /** Get the length of each line when broken with mandatory breaks.
     *
     * @return A list of line lengths.
     */
    constexpr void mandatory_lines(std::vector<size_t>& r)
    {
        r.clear();

        auto length = 0_uz;
        for (auto it = _opportunities.begin() + 1; it != _opportunities.end(); ++it) {
            ++length;
            if (*it == unicode_break_opportunity::mandatory) {
                r.push_back(length);
                length = 0_uz;
            }
        }
    }

    /** Create a line which will fit to the maximum width.
     *
     * @tparam T The type of the width.
     * @param advances Width of each character in the text.
     * @param first Index to the first character of the line.
     * @param maximum_width The maximum width of the line.
     * @return Index beyond the last character of the line. And the width of the line.
     */
    template<typename T, std::ranges::random_access_range R>
    [[nodiscard]] constexpr std::pair<size_t, T> fit_line(R const& advances, size_t first, T maximum_width) const noexcept
        requires (std::convertible_to<std::ranges::range_value_t<R>, T>)
    {
        using enum unicode_break_opportunity;

        assert(_opportunities.size() == advances.size() + 1);
        assert(_opportunities.back() == mandatory);

        auto width = T{};
        auto end_of_line = first;
        auto width_of_line = T{};
        auto found_visible = false;
        for (auto i = first;; ++i) {
            width += advances[i];
            found_visible |= not _infos[i].is_white_space();

            if (width > maximum_width and found_visible) {
                if (i == first) {
                    // Not even a single character fits, return a single character.
                    return {first + 1, advances[first]};

                } else if (end_of_line == first) {
                    // There is not a single word that fits in the maximum
                    // width. Return up to the previous character. trailing
                    // non-breaking whitespace will be included in this word
                    return {i, width - advances[i]};

                } else {
                    // Return the previous word.
                    return {end_of_line, width_of_line};
                }
            }

            // Check if there is a break opportunity after this character.
            if (_opportunities[i + 1] == mandatory) {
                // Found an end-of-line. If this character overflows the maximum width, it will be a white-space character.
                return {i + 1, width};

            } else if (_opportunities[i + 1] == yes) {
                // Found a word, or a chunk of whitespace (possibly overflowing the maximum width).
                end_of_line = i + 1;
                width_of_line = width;
                found_visible = false;
            }
        }

        // We should never reach this point, as there is always a mandatory break at the end of the text.
        std::unreachable();
    }
};

} // namespace hi::inline v1
