// Copyright Take Vos 2022.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "../utility/utility.hpp"
#include "../macros.hpp"
#include <format>
#include <ostream>
#include <vector>

hi_export_module(hikogui.unicode.unicode_break_opportunity);


hi_export namespace hi::inline v1 {

enum class unicode_break_opportunity : uint8_t {
    no,
    yes,
    mandatory,
    unassigned,
};

class unicode_grapheme_break_vector : public std::vector<unicode_break_opportunity> {
public:
    using super = std::vector<unicode_break_opportunity>;
    using super::super;
};

class unicode_line_break_vector : public std::vector<unicode_break_opportunity> {
public:
    using super = std::vector<unicode_break_opportunity>;
    using super::super;
};

class unicode_word_break_vector : public std::vector<unicode_break_opportunity> {
public:
    using super = std::vector<unicode_break_opportunity>;
    using super::super;
};

class unicode_sentence_break_vector : public std::vector<unicode_break_opportunity> {
public:
    using super = std::vector<unicode_break_opportunity>;
    using super::super;
};

inline std::ostream &operator<<(std::ostream &lhs, unicode_break_opportunity const &rhs) {
    auto const *s = [&] () {
        switch (rhs) {
            using enum unicode_break_opportunity;
        case no: return "X";
        case yes: return ":";
        case mandatory: return "!";
        case unassigned: return "-";
        default: hi_no_default();
        }
    }();
    return lhs << s;
}

}

// XXX #617 MSVC bug does not handle partial specialization in modules.
hi_export template<>
struct std::formatter<hi::unicode_break_opportunity, char> : std::formatter<char const *, char> {
    auto format(hi::unicode_break_opportunity const &t, auto &fc) const
    {
        auto const *s = [&]() {
            switch (t) {
            using enum hi::unicode_break_opportunity;
            case no: return "X";
            case yes: return ":";
            case mandatory: return "!";
            case unassigned: return "-";
            default: hi_no_default();
            }
        }();
        return std::formatter<char const *, char>::format(s, fc);
    }
};
