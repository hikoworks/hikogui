// Copyright Take Vos 2022.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "../utility/utility.hpp"
#include "../macros.hpp"
#include <cctype>
#include <compare>
#include <string_view>
#include <string>
#include <format>

hi_export_module(hikogui.i18n.iso_639);

hi_export namespace hi::inline v1 {

/** ISO-639 language code.
 *
 * A 2 or 3 lower case language code selected from the following iso standards:
 *  1. ISO 639-1 (2002)
 *  2. ISO 639-2 (1998)
 *  3. ISO 639-3 (2007)
 *  4. ISO 639-5 (2008)
 *
 * This class compresses this 2 or 3 character language code inside 15 bits.
 */
hi_export class iso_639 {
public:
    /** Set the letter at a specific position.
     *
     * @tparam I index
     * @param rhs The language code to change.
     * @param c The character to set. a-z, A-Z, 0-5 or nul.
     */
    template<std::size_t I>
    constexpr friend bool set(iso_639& rhs, char c) noexcept
    {
        if (not (c == 0 or (c >= 'a' and c <= 'z') or (c >= 'A' and c <= 'Z') or (c >= '1' and c <= '5'))) {
            // Must be letters or the digits between '1' and '5', or nul.
            return false;
        }

        // clang-format off
        uint16_t const x =
            (c >= 'a' and c <= 'z') ? c - 'a' + 1 :
            (c >= 'A' and c <= 'Z') ? c - 'A' + 1 :
            (c >= '1' and c <= '5') ? c - '1' + 27 :
            0;
        // clang-format on

        hi_axiom(x <= 0x1f);
        constexpr auto shift = I * 5;
        rhs._v &= ~(0x1f << shift);
        rhs._v |= x << shift;
        return true;
    }

    /** Get the letter at a specific position.
     *
     * @tparam I index
     * @param rhs The language code read from.
     * @return The character at index, a-z, 0-5 or nul.
     */
    template<std::size_t I>
    [[nodiscard]] constexpr friend char get(iso_639 const& rhs) noexcept
    {
        constexpr auto shift = I * 5;
        auto const x = (rhs._v >> shift) & 0x1f;
        if (x == 0) {
            return 0;
        } else if (x <= 26) {
            return 'a' + narrow_cast<char>(x - 1);
        } else {
            return '1' + narrow_cast<char>(x - 27);
        }
    }

    constexpr iso_639(iso_639 const&) noexcept = default;
    constexpr iso_639(iso_639&&) noexcept = default;
    constexpr iso_639& operator=(iso_639 const&) noexcept = default;
    constexpr iso_639& operator=(iso_639&&) noexcept = default;

    /** Construct empty language.
     */
    constexpr iso_639() noexcept : _v(0) {}

    /** Construct a language from the 2 or 3 letter code.
     */
    constexpr iso_639(std::string_view str) : _v(0)
    {
        if (str.size() != 2 and str.size() != 3) {
            throw parse_error("ISO-639 incorrect length.");
        }

        if (not set<0>(*this, str[0])) {
            throw parse_error("Must be letters or the digits between '1' and '5', or nul.");
        }

        if (not set<1>(*this, str[1])) {
            throw parse_error("Must be letters or the digits between '1' and '5', or nul.");
        }

        if (str.size() == 3) {
            if (not set<2>(*this, str[2]))  {
                throw parse_error("Must be letters or the digits between '1' and '5', or nul.");
            }
        }
    }

    constexpr iso_639(std::in_place_t, uint16_t v) noexcept : _v(v) {}

    [[nodiscard]] constexpr uint16_t const& intrinsic() const noexcept
    {
        return _v;
    }

    [[nodiscard]] constexpr uint16_t& intrinsic() noexcept
    {
        return _v;
    }

    /** Get the number of character.
     *
     * @return 2 or 3 for a code, or 0 if empty.
     */
    [[nodiscard]] constexpr std::size_t size() const noexcept
    {
        auto const tmp = _v & 0x7fff;
        // clang-format off
        return
            tmp == 0 ? 0 :
            tmp <= 0x1f ? 1 :
            tmp <= 0x3ff ? 2 :
            3;
        // clang-format on
    }

    /** Check if the language is empty.
     */
    [[nodiscard]] constexpr bool empty() const noexcept
    {
        return _v == 0;
    }

    /** Check if the language is used.
     */
    constexpr explicit operator bool() const noexcept
    {
        return not empty();
    }

    /** Get the hash value for this language code.
     */
    [[nodiscard]] size_t hash() const noexcept
    {
        return std::hash<uint16_t>{}(_v);
    }

    /** Get the 2 or 3 letter ISO-639 code.
     */
    [[nodiscard]] constexpr std::string code() const noexcept
    {
        auto r = std::string{};
        if (size() >= 2) {
            r += get<0>(*this);
            r += get<1>(*this);
        }
        if (size() == 3) {
            r += get<2>(*this);
        }
        return r;
    }

    [[nodiscard]] constexpr uint32_t open_type_code() const noexcept
    {
        uint32_t r = 0;
        if (size() == 2) {
            r |= get<0>(*this) << 24;
            r |= get<1>(*this) << 16;
            r |= 0x2020;
        } else if (size() == 3) {
            r |= get<0>(*this) << 24;
            r |= get<1>(*this) << 16;
            r |= get<2>(*this) << 8;
            r |= 0x20;
        } else {
            r = 0x20202020;
        }
        return r;
    }

    [[nodiscard]] constexpr friend std::string to_string(iso_639 const &rhs) noexcept
    {
        return rhs.code();
    }

    /** Compare two language codes.
     */
    [[nodiscard]] constexpr friend bool operator==(iso_639 const& lhs, iso_639 const& rhs) noexcept = default;

    /** Compare two language codes.
     */
    [[nodiscard]] constexpr friend auto operator<=>(iso_639 const& lhs, iso_639 const& rhs) noexcept = default;

    /** Check if rhs matches with lhs.
     *
     * @param lhs The language or wild-card.
     * @param rhs The language.
     * @return True when lhs is a wild-card or when lhs and rhs are equal.
     */
    [[nodiscard]] constexpr friend bool matches(iso_639 const& lhs, iso_639 const& rhs) noexcept
    {
        return lhs.empty() or lhs == rhs;
    }

private:
    /**
     * Encoded as follows:
     * - [15:15] reserved '0'
     * - [14:10] optional third letter
     * - [9:5] second letter
     * - [4:0] first letter
     *
     * The alphabet for the 5 bit letters are: <nul>abcdefghijklmnopqrstuvwxyz12345.
     */
    uint16_t _v;
};

} // namespace hi::inline v1

hi_export template<>
struct std::hash<hi::iso_639> {
    [[nodiscard]] size_t operator()(hi::iso_639 const& rhs) const noexcept
    {
        return rhs.hash();
    }
};
