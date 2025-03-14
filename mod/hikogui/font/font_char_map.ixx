// Copyright Take Vos 2023.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

/** @file font/font_char_map.hpp Defined font_char_map type.
 * @ingroup font
 */

module;
#include "../macros.hpp"

#include <bitset>
#include <cstdint>
#include <vector>
#include <tuple>
#include <algorithm>
#include <string>

export module hikogui_font_font_char_map;
import hikogui_algorithm;
import hikogui_font_glyph_id;
import hikogui_utility;

export namespace hi { inline namespace v1 {

/** Character map of a font.
 *
 * This type serves the check if a code-point is supported by a font
 * (even when the font is unloaded), and to retrieve the glyph mapped to
 * the code-point.
 *
 * @ingroup font
 */
export class font_char_map {
public:
    constexpr font_char_map() noexcept = default;
    constexpr font_char_map(font_char_map const&) noexcept = default;
    constexpr font_char_map(font_char_map&&) noexcept = default;
    constexpr font_char_map& operator=(font_char_map const&) noexcept = default;
    constexpr font_char_map& operator=(font_char_map&&) noexcept = default;

    [[nodiscard]] constexpr bool empty() const noexcept
    {
        return _map.empty();
    }

    /** Reserve space for a set of ranges to be added.
     * @param n The number of ranges to be added.
     */
    constexpr void reserve(size_t n)
    {
        return _map.reserve(n);
    }

    /** Get the number of code-points supported by the char-map.
     */
    constexpr size_t count() const noexcept
    {
        return _count;
    }

    /** Update a code-point mask.
     *
     * @param mask The mask to be updated.
     * @return Number of code-point that where added and where not in the mask before.
     */
    constexpr size_t update_mask(std::bitset<0x11'0000>& mask) const noexcept
    {
        auto r = 0_uz;
        for (hilet& entry : _map) {
            // Make sure this loop is inclusive.
            for (auto cp = entry.start_code_point(); cp <= entry.end_code_point; ++cp) {
                if (not mask.test(cp)) {
                    ++r;
                    mask.set(cp);
                }
            }
        }
        return r;
    }

    /** Add a range of code points.
     *
     * @param start_code_point The starting code-point of the range.
     * @param end_code_point The ending code-point of the range (inclusive).
     * @param start_glyph The starting glyph of the range.
     */
    constexpr void add(char32_t start_code_point, char32_t end_code_point, uint16_t start_glyph) noexcept
    {
#ifndef NDEBUG
        _prepared = false;
#endif
        hi_axiom(start_code_point <= end_code_point);
        auto todo = wide_cast<size_t>(end_code_point - start_code_point + 1);
        _count += todo;
        hi_axiom(start_glyph + todo < 0xffff, "Only glyph-ids 0 through 0xfffe are valid");

        while (todo != 0) {
            hilet doing = std::min(todo, entry_type::max_count);
            hi_axiom(doing != 0);

            _map.emplace_back(start_code_point, char_cast<char32_t>(start_code_point + doing - 1), start_glyph);

            todo -= doing;
            start_code_point += narrow_cast<char32_t>(doing);
            start_glyph += narrow_cast<uint16_t>(doing);
        }
    }

    /** Prepare the map for searching.
     *
     * @pre All ranged of code-point must have been added with `add()`.
     * @post The character map can be searched using `find()`.
     */
    void prepare() noexcept
    {
        if (_map.empty()) {
            return;
        }

        // Sort the entries in reverse order so that the lower_bound search becomes upper_bound.
        std::sort(_map.begin(), _map.end(), [](hilet& a, hilet& b) {
            return a.end_code_point < b.end_code_point;
        });

        auto it = _map.begin();
        auto prev_it = it++;
        while (it != _map.end()) {
            hi_axiom(prev_it->end_code_point < it->start_code_point());

            if (mergeable(*prev_it, *it)) {
                hilet merged_count = std::min(prev_it->count() + it->count(), entry_type::max_count);
                hilet move_count = merged_count - prev_it->count();
                hi_axiom(move_count <= entry_type::max_count);

                prev_it->end_code_point += narrow_cast<char32_t>(move_count);
                prev_it->set_count(prev_it->count() + move_count);

                if (move_count == it->count()) {
                    it = _map.erase(it);
                    // Don't change prev or it; since we just deleted instead of advanced.
                    continue;

                } else {
                    hi_axiom(move_count < it->count());
                    it->start_glyph += narrow_cast<uint16_t>(move_count);
                    it->set_count(it->count() - move_count);
                }
            }

            prev_it = it++;
        }

        _map.shrink_to_fit();

#ifndef NDEBUG
        _prepared = true;
#endif
    }

    /** Find a glyph for a code_point.
     *
     * @param code_point The code-point to find in the character map.
     * @return The corrosponding glyph found representing the code-point, or an empty glyph if not found.
     */
    [[nodiscard]] glyph_id find(char32_t code_point) const noexcept
    {
#ifndef NDEBUG
        hi_assert(_prepared);
#endif

        if (hilet item_ptr = fast_lower_bound(std::span{_map}, char_cast<uint32_t>(code_point))) {
            return item_ptr->get(code_point);
        }
        return {};
    }

private:
    struct entry_type {
        constexpr static size_t max_count = 0x1'0000;

        char32_t end_code_point;
        uint16_t start_glyph;
        uint16_t _count;

        constexpr entry_type(char32_t start_code_point, char32_t end_code_point, uint16_t start_glyph) noexcept :
            end_code_point(end_code_point),
            start_glyph(start_glyph),
            _count(narrow_cast<uint16_t>(end_code_point - start_code_point))
        {
            hi_axiom(start_code_point <= end_code_point);
        }

        [[nodiscard]] constexpr size_t count() const noexcept
        {
            return wide_cast<size_t>(_count) + 1;
        }

        constexpr void set_count(size_t new_count) noexcept
        {
            hi_axiom(new_count > 0);
            hi_axiom(new_count <= max_count);
            _count = narrow_cast<uint16_t>(new_count - 1);
        }

        [[nodiscard]] constexpr char32_t start_code_point() const noexcept
        {
            return end_code_point - _count;
        }

        [[nodiscard]] constexpr uint16_t end_glyph() const noexcept
        {
            return narrow_cast<uint16_t>(start_glyph + _count);
        }

        [[nodiscard]] constexpr friend bool mergeable(entry_type const& lhs, entry_type const& rhs) noexcept
        {
            return lhs.end_code_point + 1 == rhs.start_code_point() and lhs.end_glyph() + 1 == rhs.start_glyph;
        }

        [[nodiscard]] constexpr glyph_id get(char32_t code_point) const noexcept
        {
            auto diff = wide_cast<ptrdiff_t>(code_point) - wide_cast<ptrdiff_t>(end_code_point);
            if (diff > 0) {
                return {};
            }
            diff += _count;
            if (diff < 0) {
                return {};
            }
            diff += start_glyph;
            return glyph_id{truncate<uint16_t>(diff)};
        }
    };

    std::vector<entry_type> _map = {};

    /** Total number of code-points added.
     */
    size_t _count = 0;

#ifndef NDEBUG
    bool _prepared = false;
#endif
};

}} // namespace hi::v1
