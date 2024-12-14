// Copyright Take Vos 2022.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

/** @file unicode/unicode_word_break.hpp
 */

#pragma once

#include "unicode_break_opportunity.hpp"
#include "ucd_general_categories.hpp"
#include "ucd_grapheme_cluster_breaks.hpp"
#include "ucd_word_break_properties.hpp"
#include "grapheme.hpp"
#include "gstring.hpp"
#include "../utility/utility.hpp"
#include "../macros.hpp"
#include <algorithm>
#include <vector>
#include <iterator>

hi_export_module(hikogui.unicode.unicode_word_break);

hi_export namespace hi::inline v1 {

/** Unicode word-break algorithm.
 * 
 * See https://www.unicode.org/reports/tr29/ for the specification.
 * 
 * We are using a class so that it will be easy to reuse the allocations of
 * the internal state of this algorithm across multiple calls.
 */
class unicode_word_break {
public:
    constexpr unicode_word_break() noexcept = default;
    constexpr unicode_word_break(unicode_word_break const&) noexcept = default;
    constexpr unicode_word_break(unicode_word_break&&) noexcept = default;
    constexpr unicode_word_break& operator=(unicode_word_break const&) noexcept = default;
    constexpr unicode_word_break& operator=(unicode_word_break&&) noexcept = default;

    /** Get the break opportunities for the text.
     * 
     * @note See `set_text()` on how to set the text.
     * @return A list of break opportunities.
     */
    [[nodiscard]] constexpr std::span<unicode_break_opportunity const> opportunities() const noexcept
    {
        return _opportunities;
    }

    /** Set the text to calculate the break opportunities for.
     * 
     * @param base_code_points The base code points of the text.
     */
    constexpr void set_text(std::span<char32_t const> base_code_points)
    {
        clear_and_resize(_opportunities, base_code_points.size() + 1, unicode_break_opportunity::unassigned);
        clear_and_resize(_infos, base_code_points.size(), info_type{});

        for (auto i = size_t{0}; i != base_code_points.size(); ++i) {
            auto const code_point = base_code_points[i];
            auto const word_break_property = ucd_get_word_break_property(code_point);
            auto const grapheme_cluster_break = ucd_get_grapheme_cluster_break(code_point);
            _infos[i] =
                info_type{word_break_property, grapheme_cluster_break == unicode_grapheme_cluster_break::Extended_Pictographic};
        }

        WB1_WB3d();
        WB4();
        WB5_WB999();
    }

private:
    class info_type {
    public:
        constexpr info_type() noexcept = default;
        constexpr info_type(info_type const&) noexcept = default;
        constexpr info_type(info_type&&) noexcept = default;
        constexpr info_type& operator=(info_type const&) noexcept = default;
        constexpr info_type& operator=(info_type&&) noexcept = default;

        constexpr info_type(unicode_word_break_property const& word_break_property, bool pictographic) noexcept :
            _property(std::to_underlying(word_break_property)), _pictographic(pictographic), _skip(false)
        {
        }

        constexpr info_type& make_skip() noexcept
        {
            _skip = true;
            return *this;
        }

        [[nodiscard]] constexpr bool is_skip() const noexcept
        {
            return _skip;
        }

        [[nodiscard]] constexpr bool is_pictographic() const noexcept
        {
            return _pictographic;
        }

        [[nodiscard]] constexpr friend bool operator==(info_type const& lhs, unicode_word_break_property const& rhs) noexcept
        {
            return lhs._property == std::to_underlying(rhs);
        }

        [[nodiscard]] constexpr friend bool operator==(info_type const&, info_type const&) noexcept = default;

        [[nodiscard]] constexpr friend bool is_AHLetter(info_type const& rhs) noexcept
        {
            return rhs == unicode_word_break_property::ALetter or rhs == unicode_word_break_property::Hebrew_Letter;
        }

        [[nodiscard]] constexpr friend bool is_MidNumLetQ(info_type const& rhs) noexcept
        {
            return rhs == unicode_word_break_property::MidNumLet or rhs == unicode_word_break_property::Single_Quote;
        }

    private:
        uint8_t _pictographic : 1 = 0;
        uint8_t _skip : 1 = 0;
        uint8_t _property : 6 = 0;
    };

    std::vector<unicode_break_opportunity> _opportunities;
    std::vector<info_type> _infos;

    constexpr void WB1_WB3d() noexcept
    {
        using enum unicode_break_opportunity;
        using enum unicode_word_break_property;

        assert(_opportunities.size() == _infos.size() + 1);

        _opportunities.front() = yes; // WB1
        _opportunities.back() = yes; // WB2

        for (auto i = 1_uz; i < _infos.size(); ++i) {
            auto const prev = _infos[i - 1];
            auto const next = _infos[i];

            _opportunities[i] = [&]() {
                if (prev == CR and next == LF) {
                    return no; // WB3
                } else if (prev == Newline or prev == CR or prev == LF) {
                    return yes; // WB3a
                } else if (next == Newline or next == CR or next == LF) {
                    return yes; // WB3b
                } else if (prev == ZWJ and next.is_pictographic()) {
                    return no; // WB3c
                } else if (prev == WSegSpace and next == WSegSpace) {
                    return no; // WB3d
                } else {
                    return unassigned;
                }
            }();
        }
    }

    constexpr void WB4() noexcept
    {
        using enum unicode_break_opportunity;
        using enum unicode_word_break_property;

        assert(_opportunities.size() == _infos.size() + 1);

        for (auto i = 1_uz; i < _infos.size(); ++i) {
            auto const prev = _infos[i - 1];
            auto& next = _infos[i];

            if ((prev != Newline and prev != CR and prev != LF) and (next == Extend or next == Format or next == ZWJ)) {
                if (_opportunities[i] == unassigned) {
                    _opportunities[i] = no;
                }
                next.make_skip();
            }
        }
    }

    constexpr void WB5_WB999() noexcept
    {
        using enum unicode_break_opportunity;
        using enum unicode_word_break_property;

        assert(_opportunities.size() == _infos.size() + 1);

        for (auto i = 0_uz; i != _infos.size(); ++i) {
            if (_opportunities[i] != unassigned) {
                continue;
            }

            auto const& next = _infos[i];

            // WB4: (Extend | Format | ZWJ)* is assigned to no-break.
            assert(not next.is_skip());

            auto prev_i = narrow_cast<ptrdiff_t>(i) - 1;
            auto prev = info_type{};
            for (; prev_i >= 0; --prev_i) {
                if (not _infos[prev_i].is_skip()) {
                    prev = _infos[prev_i];
                    break;
                }
            }

            auto prev_prev_i = prev_i - 1;
            auto prev_prev = info_type{};
            for (; prev_prev_i >= 0; --prev_prev_i) {
                if (not _infos[prev_prev_i].is_skip()) {
                    prev_prev = _infos[prev_prev_i];
                    break;
                }
            }

            auto next_next_i = i + 1;
            auto next_next = info_type{};
            for (; next_next_i != _infos.size(); ++next_next_i) {
                if (not _infos[next_next_i].is_skip()) {
                    next_next = _infos[next_next_i];
                    break;
                }
            }

            auto RI_i = prev_i - 1;
            auto RI_is_pair = true;
            if (prev == Regional_Indicator and next == Regional_Indicator) {
                // Track back before prev, and count consecutive RI.
                for (; RI_i >= 0; --RI_i) {
                    if (_infos[RI_i].is_skip()) {
                        continue;
                    } else if (_infos[RI_i] != Regional_Indicator) {
                        break;
                    }
                    RI_is_pair = not RI_is_pair;
                }
            }

            _opportunities[i] = [&] {
                if (is_AHLetter(prev) and is_AHLetter(next)) {
                    return no; // WB5
                } else if (is_AHLetter(prev) and (next == MidLetter or is_MidNumLetQ(next)) and is_AHLetter(next_next)) {
                    return no; // WB6
                } else if (is_AHLetter(prev_prev) and (prev == MidLetter or is_MidNumLetQ(prev)) and is_AHLetter(next)) {
                    return no; // WB7
                } else if (prev == Hebrew_Letter and next == Single_Quote) {
                    return no; // WB7a
                } else if (prev == Hebrew_Letter and next == Double_Quote and next_next == Hebrew_Letter) {
                    return no; // WB7b
                } else if (prev_prev == Hebrew_Letter and prev == Double_Quote and next == Hebrew_Letter) {
                    return no; // WB7c
                } else if (prev == Numeric and next == Numeric) {
                    return no; // WB8
                } else if (is_AHLetter(prev) and next == Numeric) {
                    return no; // WB9
                } else if (prev == Numeric and is_AHLetter(next)) {
                    return no; // WB10
                } else if (prev_prev == Numeric and (prev == MidNum or is_MidNumLetQ(prev)) and next == Numeric) {
                    return no; // WB11
                } else if (prev == Numeric and (next == MidNum or is_MidNumLetQ(next)) and next_next == Numeric) {
                    return no; // WB12
                } else if (prev == Katakana and next == Katakana) {
                    return no; // WB13
                } else if (
                    (is_AHLetter(prev) or prev == Numeric or prev == Katakana or prev == ExtendNumLet) and next == ExtendNumLet) {
                    return no; // WB13a
                } else if (prev == ExtendNumLet and (is_AHLetter(next) or next == Numeric or next == Katakana)) {
                    return no; // WB13b
                } else if (prev == Regional_Indicator and next == Regional_Indicator and RI_is_pair) {
                    return no; // WB15 WB16
                } else {
                    return yes; // WB999
                }
            }();
        }
    }
};

} // namespace hi::inline v1
