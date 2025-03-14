// Copyright Take Vos 2022.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "../utility/utility.hpp"
#include "../i18n/i18n.hpp"
#include "../telemetry/telemetry.hpp"
#include "../concurrency/concurrency.hpp"
#include "../char_maps/char_maps.hpp"
#include "grapheme_attributes.hpp"
#include "unicode_normalization.hpp"
#include "ucd_general_categories.hpp"
#include "ucd_canonical_combining_classes.hpp"
#include "ucd_scripts.hpp"
#include "phrasing.hpp"
#include "../macros.hpp"
#include <cstdint>
#include <string>
#include <string_view>
#include <cstddef>
#include <memory>
#include <vector>
#include <algorithm>
#include <bit>
#include <array>
#include <atomic>
#include <unordered_map>
#include <mutex>
#include <chrono>
#include <format>

hi_export_module(hikogui.unicode : grapheme);

hi_export namespace hi::inline v1 {
namespace detail {

class long_grapheme_table {
public:
    long_grapheme_table() = default;
    long_grapheme_table(long_grapheme_table const&) = delete;
    long_grapheme_table(long_grapheme_table&&) = delete;
    long_grapheme_table& operator=(long_grapheme_table const&) = delete;
    long_grapheme_table& operator=(long_grapheme_table&&) = delete;

    /** Get the grapheme from the table.
     *
     * @param start The start position of the grapheme in the table.
     *              If the start value came from another thread it is
     *              important that this was transferred properly, as this
     *              the only way that this function becomes thread safe.
     * @return The code-points of the grapheme.
     */
    [[nodiscard]] std::u32string get_grapheme(uint32_t start) const noexcept
    {
        // If `start` came from another thread it will have been transferred
        // to this thread pr
        auto src = std::addressof(_table[start]);
        auto const length = *src >> 21;

        auto r = std::u32string{};
        r.resize_and_overwrite(length, [&](char32_t *dst, size_t count) {
            std::copy_n(src, count, dst);
            *dst &= 0x1f'ffff;
            return count;
        });
        return r;
    }

    /** Get the size of the grapheme.
     *
     * @param start The start position of the grapheme in the table.
     *              If the start value came from another thread it is
     *              important that this was transferred properly, as this
     *              the only way that this function becomes thread safe.
     * @return The number of code-points of the grapheme.
     */
    [[nodiscard]] size_t get_grapheme_size(uint32_t start) const noexcept
    {
        return _table[start] >> 21;
    }

    /** Get the starter (first) code-point of a grapheme.
     *
     * @param start The start position of the grapheme in the table.
     *              If the start value came from another thread it is
     *              important that this was transferred properly, as this
     *              the only way that this function becomes thread safe.
     * @return The starter (first) code-point of the a grapheme.
     */
    [[nodiscard]] char32_t get_grapheme_starter(uint32_t start) const noexcept
    {
        return char32_t{_table[start] & 0x1f'ffff};
    }

    /** Find or insert a grapheme in the table.
     *
     * @param code_points The code-points forming a grapheme. The grapheme must
     *                 be NFC normalized. The grapheme must be no more than
     *                 31 code-points (stream-safe).
     * @return Index where the grapheme is in the table. -1 if the table is full.
     */
    template<typename CodePoints>
    [[nodiscard]] int32_t add_grapheme(CodePoints&& code_points) noexcept
    {
        static_assert(std::is_same_v<typename std::remove_cvref_t<CodePoints>::value_type, char32_t>);

        hi_axiom(code_points.size() >= 2);
        hi_axiom(unicode_is_NFC_grapheme(code_points.cbegin(), code_points.cend()));

        auto const lock = std::scoped_lock(_mutex);

        // See if this grapheme already exists and return its index.
        if (auto const it = _indices.find(code_points); it != _indices.end()) {
            return it->second;
        }

        // Check if there is enough room in the table to add the code-points.
        if (_head + code_points.size() >= _table.size()) {
            return -1;
        }

        auto const insert_index = _head;
        _head += narrow_cast<uint32_t>(code_points.size());

        // Copy the grapheme into the table, and set the size on the first entry.
        std::copy(code_points.cbegin(), code_points.cend(), _table.begin() + insert_index);
        _table[insert_index] |= char_cast<char32_t>(code_points.size() << 21);

        // Add the grapheme to the quickly searchable index table.
        _indices.emplace(std::forward<CodePoints>(code_points), insert_index);

        return insert_index;
    }

private:
    mutable unfair_mutex _mutex = {};
    uint32_t _head = {};

    /** Table of code-points for graphemes.
     *
     * - [7:0] number of code-point of the grapheme (only on the first code-point).
     * - [28:8] code-point.
     */
    std::array<char32_t, 0x0f'0000> _table = {};

    std::unordered_map<std::u32string, uint32_t> _indices = {};
};

inline long_grapheme_table long_graphemes = {};

} // namespace detail

struct composed_t {};

/** A grapheme-cluster, what a user thinks a character is.
 *
 * A grapheme should not include typographical ligatures such as 'fi' as
 * the font should handle creating ligatures.
 *
 * If a grapheme is initialized with more than 1 code-points a long_grapheme
 * is allocated. This grapheme is never deleted from memory.
 *
 * This class is trivial and constant-destructible so that it can be used
 * as a character class in `std::basic_string` and used as a non-type template parameter.
 */
struct grapheme {
    using value_type = uint64_t;

    /** The grapheme's value.
     *
     * This class will hold:
     *  - [20: 0] U+0000 to U+10ffff single code-point,
     *            0x110000 to 0x1fffff index into long_grapheme table.
     *  - [35:21] ISO-639 language-code: 0 is wildcard.
     *  - [45:36] ISO-15924 script-code: 0 is wildcard.
     *  - [55:46] ISO-3166 region-code: 0 is wildcard.
     *  - [61:56] phrasing
     *  - [62:62] reserved = 0
     *  - [63:63] If bit is set this is a end-of-file.
     */
    value_type _value;

    constexpr grapheme() noexcept = default;
    constexpr grapheme(grapheme const&) noexcept = default;
    constexpr grapheme(grapheme&&) noexcept = default;
    constexpr grapheme& operator=(grapheme const&) noexcept = default;
    constexpr grapheme& operator=(grapheme&&) noexcept = default;

    constexpr grapheme(std::in_place_t, value_type value) : _value(value) {}

    constexpr value_type& intrinsic() noexcept
    {
        return _value;
    }

    constexpr value_type const& intrinsic() const noexcept
    {
        return _value;
    }

    /** Encode a single code-point.
     */
    constexpr grapheme(char32_t code_point) noexcept : _value(char_cast<value_type>(code_point))
    {
        hi_axiom(code_point <= 0x10'ffff);
        hi_axiom(ucd_get_canonical_combining_class(code_point) == 0, "Single code-point must be a starter");
        set_script();
    }

    constexpr grapheme(char ascii_char) noexcept : _value(char_cast<value_type>(ascii_char))
    {
        hi_axiom(ascii_char >= 0 and ascii_char <= 0x7f);
        // All ASCII characters are starters.
        set_script();
    }

    /** Encode a single code-point.
     */
    constexpr grapheme& operator=(char32_t code_point) noexcept
    {
        hi_axiom(code_point <= 0x10'ffff);
        hi_axiom(ucd_get_canonical_combining_class(code_point) == 0, "Single code-point must be a starter");

        _value = char_cast<value_type>(code_point);
        set_script();
        return *this;
    }

    /** Encode a single code-point.
     */
    constexpr grapheme& operator=(char ascii_char) noexcept
    {
        hi_axiom(ascii_char >= 0 and ascii_char <= 0x7f);
        // All ASCII characters are starters.

        _value = char_cast<value_type>(ascii_char);
        set_script();
        return *this;
    }

    /** Encode a grapheme from a list of code-points.
     *
     * @param code_points The NFC/NKFC normalized and composed code-point of this grapheme.
     */
    template<typename CodePoints>
    constexpr grapheme(composed_t, CodePoints&& code_points) noexcept
    {
        static_assert(std::is_same_v<typename std::remove_cvref_t<CodePoints>::value_type, char32_t>);

        hi_axiom(not code_points.empty());
        if (code_points.size() == 1) {
            auto const code_point = code_points.front();
            hi_axiom(code_point <= 0x10'ffff);
            hi_axiom(ucd_get_canonical_combining_class(code_point) == 0);
            _value = char_cast<value_type>(code_point);

        } else {
            auto const index = detail::long_graphemes.add_grapheme(std::forward<CodePoints>(code_points));
            if (index >= 0) {
                _value = narrow_cast<value_type>(index + 0x11'0000);

            } else {
                [[unlikely]] hi_log_error_once(
                    "grapheme::error::too-many", "Too many long graphemes encoded, replacing with U+fffd");
                _value = char_cast<value_type>(U'\ufffd');
            }
        }
        set_script();
    }

    /** Encode a grapheme from a list of code-points.
     *
     * @param code_points The non-normalized list of code-points.
     */
    constexpr explicit grapheme(std::u32string_view code_points) noexcept :
        grapheme(composed_t{}, unicode_normalize(code_points, unicode_normalize_config::NFC()))
    {
    }

    /** Get the codepoint/index part of the grapheme.
     */
    [[nodiscard]] constexpr uint32_t index() const noexcept
    {
        return _value & 0x1f'ffff;
    }

    [[nodiscard]] constexpr iso_639 language() const noexcept
    {
        return iso_639{std::in_place_t{}, narrow_cast<uint16_t>((_value >> 21) & 0x7fff)};
    }

    constexpr void set_language(iso_639 rhs) noexcept
    {
        hi_axiom(rhs.intrinsic() <= 0x7fff);

        constexpr auto mask = ~(value_type{0x7fff} << 21);
        _value &= mask;
        _value |= wide_cast<value_type>(rhs.intrinsic()) << 21;
    }

    /** Get the script of the starter code-point.
     *
     * @return The script for the starter code-point retrieved from the Unicode
     *         Datastarter.
     */
    [[nodiscard]] constexpr iso_15924 starter_script() const noexcept
    {
        return ucd_get_script(starter());
    }

    /** Get the script of the starter code-point.
     *
     * @param default_script The
     * @return The script for the starter code-point retrieved from the Unicode
     *         Datastarter; or the @a default_script if the starter-script is
     *         common or inherited.
     */
    [[nodiscard]] constexpr iso_15924 starter_script(iso_15924 default_script) const noexcept
    {
        auto const starter_script_ = starter_script();
        if (starter_script_ == iso_15924::common() and starter_script_ == iso_15924::inherited()) {
            return default_script;
        } else {
            return starter_script_;
        }
    }

    [[nodiscard]] constexpr iso_15924 script() const noexcept
    {
        return iso_15924{std::in_place_t{}, narrow_cast<uint16_t>((_value >> 36) & 0x3ff)};
    }

    /** Set the script of the grapheme.
     *
     * @param rhs The new script for the grapheme, if the starter-script is
     *            common or inherited.
     */
    constexpr void set_script(iso_15924 rhs) noexcept
    {
        auto const new_script = starter_script(rhs);
        hi_axiom(new_script.intrinsic() < 1000);

        constexpr auto mask = ~(value_type{0x3ff} << 36);
        _value &= mask;
        _value |= wide_cast<value_type>(new_script.intrinsic()) << 36;
    }

    /** Get the script of the grapheme to the starter script.
     */
    constexpr void set_script() noexcept
    {
        set_script(starter_script());
    }

    [[nodiscard]] constexpr iso_3166 region() const noexcept
    {
        return iso_3166{std::in_place_t{}, narrow_cast<uint16_t>((_value >> 46) & 0x3ff)};
    }

    constexpr void set_region(iso_3166 rhs) noexcept
    {
        hi_axiom(rhs.intrinsic() < 1000);

        constexpr auto mask = ~(value_type{0x3ff} << 46);
        _value &= mask;
        _value |= wide_cast<value_type>(rhs.intrinsic()) << 46;
    }

    [[nodiscard]] constexpr hi::language_tag language_tag() const noexcept
    {
        auto tmp = _value;
        tmp >>= 21;
        auto const language_ = iso_639{std::in_place_t{}, narrow_cast<uint16_t>(tmp & 0x7fff)};
        tmp >>= 15;
        auto const script_ = iso_15924{std::in_place_t{}, narrow_cast<uint16_t>(tmp & 0x3ff)};
        tmp >>= 10;
        auto const region_ = iso_3166{std::in_place_t{}, narrow_cast<uint16_t>(tmp & 0x3ff)};
        return hi::language_tag{language_, script_, region_};
    }

    constexpr void set_language_tag(hi::language_tag rhs) noexcept
    {
        hi_axiom(rhs.region.intrinsic() < 1000);
        hi_axiom(rhs.language.intrinsic() <= 0x7fff);

        auto const new_script = starter_script(rhs.script);
        hi_axiom(new_script.intrinsic() < 1000);

        auto tmp = wide_cast<value_type>(rhs.region.intrinsic());
        tmp <<= 10;
        tmp |= new_script.intrinsic();
        tmp <<= 15;
        tmp |= rhs.language.intrinsic();
        tmp <<= 21;

        constexpr auto mask = ~(uint64_t{0x7'ffff'ffff} << 21);
        _value &= mask;
        _value |= tmp;
    }

    [[nodiscard]] constexpr hi::phrasing phrasing() const noexcept
    {
        return static_cast<hi::phrasing>((_value >> 56) & 0x3f);
    }

    constexpr void set_phrasing(hi::phrasing rhs) noexcept
    {
        hi_axiom(std::to_underlying(rhs) <= 0x3f);

        constexpr auto mask = ~(value_type{0x3f} << 56);
        _value &= mask;
        _value |= static_cast<value_type>(rhs) << 56;
    }

    [[nodiscard]] constexpr grapheme_attributes attributes() const noexcept
    {
        auto r = grapheme_attributes{};

        auto tmp = _value;
        tmp >>= 21;
        r.language = iso_639{std::in_place_t{}, narrow_cast<uint16_t>(tmp & 0x7fff)};
        tmp >>= 15;
        r.script = iso_15924{std::in_place_t{}, narrow_cast<uint16_t>(tmp & 0x3ff)};
        tmp >>= 10;
        r.region = iso_3166{std::in_place_t{}, narrow_cast<uint16_t>(tmp & 0x3ff)};
        tmp >>= 10;
        r.phrasing = static_cast<hi::phrasing>(tmp & 0x3f);
        return r;
    }

    /** Return the number of code-points encoded in the grapheme.
     */
    [[nodiscard]] constexpr std::size_t size() const noexcept
    {
        if (auto i = index(); i <= 0x10'ffff) {
            return 1_uz;
        } else {
            return detail::long_graphemes.get_grapheme_size(i - 0x11'0000);
        }
    }

    [[nodiscard]] constexpr char32_t starter() const noexcept
    {
        if (auto i = index(); i <= 0x10'ffff) {
            return char_cast<char32_t>(i);
        } else {
            return detail::long_graphemes.get_grapheme_starter(i - 0x11'0000);
        }
    }

    [[nodiscard]] constexpr bool is_ascii() const noexcept
    {
        return index() <= 127;
    }

    /** Get a list of code-point normalized to NFC.
     */
    [[nodiscard]] constexpr std::u32string composed() const noexcept
    {
        if (auto const i = index(); i <= 0x10'ffff) {
            return std::u32string{char_cast<char32_t>(i)};
        } else {
            return detail::long_graphemes.get_grapheme(i - 0x11'0000);
        }
    }

    /** Get a list of code-point normalized to NFD.
     */
    [[nodiscard]] constexpr std::u32string
    decomposed(unicode_normalize_config config = unicode_normalize_config::NFD()) const noexcept
    {
        return unicode_decompose(composed(), config);
    }

    /** Compare equivalence of two graphemes.
     *
     * This comparision ignores language-tag and phrasing of a grapheme.
     */
    [[nodiscard]] friend constexpr bool operator==(grapheme const& lhs, grapheme const& rhs) noexcept
    {
        return lhs.index() == rhs.index();
    }

    [[nodiscard]] friend constexpr bool operator==(grapheme const& lhs, char32_t const& rhs) noexcept
    {
        hi_axiom(char_cast<value_type>(rhs) <= 0x10'ffff);
        return lhs.index() == char_cast<value_type>(rhs);
    }

    [[nodiscard]] friend constexpr bool operator==(grapheme const& lhs, char const& rhs) noexcept
    {
        hi_axiom(char_cast<value_type>(rhs) <= 0x7f);
        return lhs.index() == char_cast<value_type>(rhs);
    }

    /** Compare two graphemes lexicographically.
     */
    [[nodiscard]] friend constexpr std::strong_ordering operator<=>(grapheme const& lhs, grapheme const& rhs) noexcept
    {
        return lhs.decomposed() <=> rhs.decomposed();
    }

    [[nodiscard]] friend constexpr std::strong_ordering operator<=>(grapheme const& lhs, char32_t const& rhs) noexcept
    {
        return lhs <=> grapheme{rhs};
    }

    [[nodiscard]] friend constexpr std::strong_ordering operator<=>(grapheme const& lhs, char const& rhs) noexcept
    {
        return lhs <=> grapheme{rhs};
    }

    [[nodiscard]] friend constexpr std::string to_string(grapheme const& rhs) noexcept
    {
        return hi::to_string(rhs.composed());
    }

    [[nodiscard]] friend constexpr std::wstring to_wstring(grapheme const& rhs) noexcept
    {
        return hi::to_wstring(rhs.composed());
    }

    [[nodiscard]] friend constexpr std::u32string to_u32string(grapheme const& rhs) noexcept
    {
        return rhs.composed();
    }
};

} // namespace hi::inline v1

template<>
struct std::hash<hi::grapheme> {
    [[nodiscard]] std::size_t operator()(hi::grapheme const& rhs) const noexcept
    {
        return std::hash<hi::grapheme::value_type>{}(rhs._value);
    }
};
