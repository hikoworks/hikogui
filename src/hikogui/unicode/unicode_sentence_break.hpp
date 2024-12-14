// Copyright Take Vos 2022.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

/** @file unicode/unicode_sentence_break.hpp
 */

#pragma once

#include "ucd_sentence_break_properties.hpp"
#include "unicode_break_opportunity.hpp"
#include "grapheme.hpp"
#include "../utility/utility.hpp"
#include "../macros.hpp"
#include <tuple>
#include <vector>
#include <iterator>
#include <algorithm>

hi_export_module(hikogui.unicode.unicode_sentence_break);

hi_export namespace hi::inline v1 {
class unicode_sentence_break {
public:
    constexpr unicode_sentence_break() noexcept = default;
    constexpr unicode_sentence_break(unicode_sentence_break const&) noexcept = default;
    constexpr unicode_sentence_break(unicode_sentence_break&&) noexcept = default;
    constexpr unicode_sentence_break& operator=(unicode_sentence_break const&) noexcept = default;
    constexpr unicode_sentence_break& operator=(unicode_sentence_break&&) noexcept = default;

    [[nodiscard]] constexpr std::span<unicode_break_opportunity const> opportunities() const noexcept
    {
        return _opportunities;
    }

    constexpr void set_text(std::span<char32_t const> base_code_points)
    {
        clear_and_resize(_opportunities, base_code_points.size() + 1, unicode_break_opportunity::unassigned);
        clear_and_resize(_infos, base_code_points.size(), info_type{});

        for (auto i = size_t{0}; i != base_code_points.size(); ++i) {
            auto const code_point = base_code_points[i];
            _infos[i] = info_type{ucd_get_sentence_break_property(code_point)};
        }

        SB1_SB4();
        SB5();
        SB6_SB998();
    }

private:
    class info_type {
    public:
        constexpr info_type() noexcept = default;
        constexpr info_type(info_type const&) noexcept = default;
        constexpr info_type(info_type&&) noexcept = default;
        constexpr info_type& operator=(info_type const&) noexcept = default;
        constexpr info_type& operator=(info_type&&) noexcept = default;

        constexpr info_type(unicode_sentence_break_property const& sentence_break_property) noexcept :
            _property(std::to_underlying(sentence_break_property))
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

        [[nodiscard]] constexpr friend bool operator==(info_type const& lhs, unicode_sentence_break_property const& rhs) noexcept
        {
            return lhs._property == std::to_underlying(rhs);
        }

        [[nodiscard]] constexpr friend bool operator==(info_type const&, info_type const&) noexcept = default;

        [[nodiscard]] constexpr friend bool is_ParaSep(info_type const& rhs) noexcept
        {
            return rhs == unicode_sentence_break_property::Sep or rhs == unicode_sentence_break_property::CR or
                rhs == unicode_sentence_break_property::LF;
        }

        [[nodiscard]] constexpr friend bool is_SATerm(info_type const& rhs) noexcept
        {
            return rhs == unicode_sentence_break_property::STerm or rhs == unicode_sentence_break_property::ATerm;
        }

    private:
        uint8_t _skip : 1 = 0;
        uint8_t _property : 6 = 0;
    };

    std::vector<unicode_break_opportunity> _opportunities;
    std::vector<info_type> _infos;

    constexpr void SB1_SB4() noexcept
    {
        using enum unicode_break_opportunity;
        using enum unicode_sentence_break_property;

        assert(_opportunities.size() == _infos.size() + 1);

        _opportunities.front() = yes; // SB1
        _opportunities.back() = yes; // SB2

        for (auto i = 1_uz; i < _infos.size(); ++i) {
            auto const prev = _infos[i - 1];
            auto const next = _infos[i];

            _opportunities[i] = [&]() {
                if (prev == CR and next == LF) {
                    return no; // SB3
                } else if (is_ParaSep(prev)) {
                    return yes; // SB4
                } else {
                    return unassigned;
                }
            }();
        }
    }

    constexpr void SB5() noexcept
    {
        using enum unicode_break_opportunity;
        using enum unicode_sentence_break_property;

        assert(_opportunities.size() == _infos.size() + 1);

        for (auto i = 1_uz; i < _infos.size(); ++i) {
            auto const prev = _infos[i - 1];
            auto& next = _infos[i];

            if ((not is_ParaSep(prev) and prev != CR and prev != LF) and (next == Extend or next == Format)) {
                if (_opportunities[i] == unassigned) {
                    _opportunities[i] = no;
                }
                next.make_skip();
            }
        }
    }

    constexpr void SB6_SB998() noexcept
    {
        using enum unicode_break_opportunity;
        using enum unicode_sentence_break_property;

        assert(_opportunities.size() == _infos.size() + 1);

        for (auto i = 0_z; i < _infos.size(); ++i) {
            auto const& next = _infos[i];
            if (_opportunities[i] != unassigned) {
                continue;
            }

            assert(not next.is_skip());

            std::ptrdiff_t k;

            auto const prev = [&] {
                for (k = i - 1; k >= 0; --k) {
                    if (not _infos[k].is_skip()) {
                        return _infos[k];
                    }
                }
                return info_type{};
            }();

            auto const prev_prev = [&] {
                for (--k; k >= 0; --k) {
                    if (not _infos[k].is_skip()) {
                        return _infos[k];
                    }
                }
                return info_type{};
            }();

            // close_sp
            // 0 - no suffix
            // 1 - ends in ParSep
            // 2 - includes SP
            // 4 - includes Close
            auto const [prefix, close_sp_par_found] = [&]() {
                using enum unicode_break_opportunity;

                auto found = 0;
                auto state = ' ';
                for (auto j = i - 1; j >= 0; --j) {
                    if (not _infos[j].is_skip()) {
                        switch (state) {
                        case ' ':
                            if (is_ParaSep(_infos[j])) {
                                found |= 1;
                                state = 'p';
                            } else if (_infos[j] == Sp) {
                                found |= 2;
                                state = 's';
                            } else if (_infos[j] == Close) {
                                found |= 4;
                                state = 'c';
                            } else {
                                return std::make_pair(_infos[j], found);
                            }
                            break;
                        case 'p': // We can only be in the state 'p' once.
                        case 's':
                            if (_infos[j] == Sp) {
                                found |= 2;
                                state = 's';
                            } else if (_infos[j] == Close) {
                                found |= 4;
                                state = 'c';
                            } else {
                                return std::make_pair(_infos[j], found);
                            }
                            break;
                        case 'c':
                            if (_infos[j] == Close) {
                                found |= 4;
                                state = 'c';
                            } else {
                                return std::make_pair(_infos[j], found);
                            }
                            break;
                        }
                    }
                }
                return std::pair{info_type{}, 0};
            }();
            auto const optional_close = (close_sp_par_found & 3) == 0;
            auto const optional_close_sp = (close_sp_par_found & 1) == 0;
            auto const optional_close_sp_par = true;

            auto const end_in_lower = [&] {
                for (auto j = i; j < _infos.size(); ++j) {
                    if (not _infos[j].is_skip()) {
                        if (_infos[j] == Lower) {
                            return true;
                        } else if (_infos[j] == OLetter or _infos[j] == Upper or is_ParaSep(_infos[j]) or is_SATerm(_infos[j])) {
                            return false;
                        }
                    }
                }
                return false;
            }();

            _opportunities[i] = [&]() {
                if (prev == ATerm and next == Numeric) {
                    return no; // SB6
                } else if ((prev_prev == Upper or prev_prev == Lower) and prev == ATerm and next == Upper) {
                    return no; // SB7
                } else if (prefix == ATerm and optional_close_sp and end_in_lower) {
                    return no; // SB8
                } else if (is_SATerm(prefix) and optional_close_sp and (next == SContinue or is_SATerm(next))) {
                    return no; // SB8a
                } else if (is_SATerm(prefix) and optional_close and (next == Close or next == Sp or is_ParaSep(next))) {
                    return no; // SB9
                } else if (is_SATerm(prefix) and optional_close_sp and (next == Sp or is_ParaSep(next))) {
                    return no; // SB10
                } else if (is_SATerm(prefix) and optional_close_sp_par) {
                    return yes; // SB11
                } else {
                    return no; // SB998
                }
            }();
        }
    }
};

}
