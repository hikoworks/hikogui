// Copyright Take Vos 2022.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "ucd_decompositions.hpp"
#include "ucd_compositions.hpp"
#include "ucd_canonical_combining_classes.hpp"
#include "unicode_description.hpp"
#include "../algorithm/algorithm.hpp"
#include "../utility/utility.hpp"
#include "../macros.hpp"
#include <cstdint>
#include <string>
#include <string_view>
#include <algorithm>

hi_export_module(hikogui.unicode.unicode_normalization);


hi_export namespace hi::inline v1 {

struct unicode_normalize_config {
    /** The types of decompositions, that should be used when decomposing.
     */
    uint64_t decomposition_mask : 17 = 0;

    /** Drop the C0 control characters.
     */
    uint64_t drop_C0 : 1 = 0;

    /** Drop the C1 control characters.
     */
    uint64_t drop_C1 : 1 = 0;

    /** Code-points to be treated as line-separators.
     */
    std::u32string line_separators;

    /** Code-points to be treated as line-separators.
     */
    std::u32string paragraph_separators;

    /** Code-points to be dropped.
     */
    std::u32string drop;

    /** The code-point to output when a line separator was found.
     *
     * If this value is the carriage return CR, then the decomposition algorithm will also emit a line-feed LF code-point.
     */
    char32_t line_separator_character = unicode_LS;

    /** The code-points to output when a paragraph separator was found.
     *
     * If this value is the carriage return CR, then the decomposition algorithm will also emit a line-feed LF code-point.
     */
    char32_t paragraph_separator_character = unicode_PS;

    constexpr unicode_normalize_config& add(unicode_decomposition_type type) noexcept
    {
        decomposition_mask |= 1_uz << std::to_underlying(type);
        return *this;
    }

    [[nodiscard]] constexpr static unicode_normalize_config NFD() noexcept
    {
        auto r = unicode_normalize_config();
        r.add(unicode_decomposition_type::canonical);
        return r;
    }

    [[nodiscard]] constexpr static unicode_normalize_config NFC() noexcept
    {
        return NFD();
    }

    /** Use NFC normalization, convert all line-feed-like characters to PS.
     */
    [[nodiscard]] constexpr static unicode_normalize_config NFC_PS_noctr() noexcept
    {
        auto r = NFC();
        r.drop_C0 = 1;
        r.drop_C1 = 1;
        r.drop = U"\r";
        r.paragraph_separators = U"\n\v\f\u0085\u2028\u2029";
        r.paragraph_separator_character = U'\u2029';
        return r;
    }

    /** Use NFC normalization, convert all line-feed-like characters to CR-LF.
     */
    [[nodiscard]] constexpr static unicode_normalize_config NFC_CRLF_noctr() noexcept
    {
        auto r = NFC();
        r.drop_C0 = 1;
        r.drop_C1 = 1;
        r.drop = U"\r";
        r.paragraph_separators = U"\n\v\f\u0085\u2028\u2029";
        r.paragraph_separator_character = U'\r';
        return r;
    }

    [[nodiscard]] constexpr static unicode_normalize_config NFKD() noexcept
    {
        auto r = unicode_normalize_config::NFD();
        r.add(unicode_decomposition_type::canonical);
        r.add(unicode_decomposition_type::font);
        r.add(unicode_decomposition_type::noBreak);
        r.add(unicode_decomposition_type::initial);
        r.add(unicode_decomposition_type::medial);
        r.add(unicode_decomposition_type::_final);
        r.add(unicode_decomposition_type::isolated);
        r.add(unicode_decomposition_type::circle);
        r.add(unicode_decomposition_type::super);
        r.add(unicode_decomposition_type::sub);
        r.add(unicode_decomposition_type::fraction);
        r.add(unicode_decomposition_type::vertical);
        r.add(unicode_decomposition_type::wide);
        r.add(unicode_decomposition_type::narrow);
        r.add(unicode_decomposition_type::small);
        r.add(unicode_decomposition_type::square);
        r.add(unicode_decomposition_type::compat);
        return r;
    }

    [[nodiscard]] constexpr static unicode_normalize_config NFKC() noexcept
    {
        return NFKD();
    }
};

namespace detail {

constexpr void unicode_decompose(char32_t code_point, unicode_normalize_config config, std::u32string& r) noexcept
{
    for (auto const c : config.line_separators) {
        if (code_point == c) {
            r += config.line_separator_character;
            if (config.line_separator_character == unicode_CR) {
                r += unicode_LF;
            }
            return;
        }
    }

    for (auto const c : config.paragraph_separators) {
        if (code_point == c) {
            r += config.paragraph_separator_character;
            if (config.paragraph_separator_character == unicode_CR) {
                r += unicode_LF;
            }
            return;
        }
    }

    for (auto const c : config.drop) {
        if (code_point == c) {
            return;
        }
    }

    if (config.drop_C0 and ((code_point >= U'\u0000' and code_point <= U'\u001f') or code_point == U'\u007f')) {
        return;
    }

    if (config.drop_C1 and code_point >= U'\u0080' and code_point <= U'\u009f') {
        return;
    }

    auto const decomposition_info = ucd_get_decomposition(code_point);
    if (decomposition_info.should_decompose(config.decomposition_mask)) {
        for (auto const c : decomposition_info.decompose()) {
            unicode_decompose(c, config, r);
        }

    } else {
        auto const ccc = ucd_get_canonical_combining_class(code_point);
        r += code_point | (wide_cast<char32_t>(ccc) << 24);
    }
}

constexpr void unicode_decompose(std::u32string_view text, unicode_normalize_config config, std::u32string& r) noexcept
{
    for (auto const c : text) {
        unicode_decompose(c, config, r);
    }
}

constexpr void unicode_compose(std::u32string& text) noexcept
{
    if (text.size() <= 1) {
        return;
    }

    // This algorithm reads using `i`-index and writes using the `j`-index.
    // When compositing characters, `j` will lag behind.
    auto i = 0_uz;
    auto j = 0_uz;
    while (i != text.size()) {
        auto const code_unit = text[i++];
        auto const code_point = code_unit & 0xff'ffff;
        auto const combining_class = code_unit >> 24;
        auto const first_is_starter = combining_class == 0;

        if (code_unit == 0xffff'ffff) {
            // Snuffed out by compositing in this algorithm.
            // We continue going forward looking for code-points.

        } else if (first_is_starter) {
            // Try composing.
            auto first_code_point = code_point;
            char32_t previous_combining_class = 0;
            for (auto k = i; k != text.size(); ++k) {
                auto const second_code_unit = text[k];
                auto const second_code_point = second_code_unit & 0xff'ffff;
                auto const second_combining_class = second_code_unit >> 24;

                auto const blocking_pair = previous_combining_class != 0 and previous_combining_class >= second_combining_class;
                auto const second_is_starter = second_combining_class == 0;

                auto const composed_code_point = ucd_get_composition(first_code_point, second_code_point);
                if (composed_code_point and not blocking_pair) {
                    // Found a composition.
                    first_code_point = *composed_code_point;
                    // The canonical combined DecompositionOrder is always zero.
                    previous_combining_class = 0;
                    // Snuff out the code-unit.
                    text[k] = 0xffff'ffff;

                } else if (second_is_starter) {
                    // End after failing to compose with the next start-character.
                    break;

                } else {
                    // The start character is not composing with this composingC.
                    previous_combining_class = second_combining_class;
                }
            }
            // Add the new combined character to the text.
            text[j++] = first_code_point;

        } else {
            // Unable to compose this character.
            text[j++] = code_point;
        }
    }

    text.resize(j);
}

constexpr void unicode_reorder(std::u32string& text) noexcept
{
    constexpr auto ccc_less = [](char32_t a, char32_t b) {
        return (a >> 24) < (b >> 24);
    };

    auto const first = text.begin();
    auto const last = text.end();

    if (first == last) {
        return;
    }

    auto cluster_it = first;
    for (auto it = cluster_it + 1; it != last; ++it) {
        if (*it <= 0xff'ffff) {
            std::stable_sort(cluster_it, it, ccc_less);
            cluster_it = it;
        }
    }

    std::stable_sort(cluster_it, last, ccc_less);
}

constexpr void unicode_clean(std::u32string& text) noexcept
{
    // clean up the text by removing the upper bits.
    for (auto& codePoint : text) {
        codePoint &= 0x1f'ffff;
    }
}

} // namespace detail

/** Convert text to a Unicode decomposed normal form.
 *
 * @param text to normalize, in-place.
 * @param normalization_mask Extra features for normalization.
 */
[[nodiscard]] constexpr std::u32string
unicode_decompose(std::u32string_view text, unicode_normalize_config config = unicode_normalize_config::NFD()) noexcept
{
    auto r = std::u32string{};
    detail::unicode_decompose(text, config, r);
    detail::unicode_reorder(r);
    detail::unicode_clean(r);
    return r;
}

/** Convert text to a Unicode composed normal form.
 *
 * @param text to normalize, in-place.
 * @param normalization_mask Extra features for normalization.
 */
[[nodiscard]] constexpr std::u32string
unicode_normalize(std::u32string_view text, unicode_normalize_config config = unicode_normalize_config::NFC()) noexcept
{
    auto r = std::u32string{};
    detail::unicode_decompose(text, config, r);
    detail::unicode_reorder(r);
    detail::unicode_compose(r);
    detail::unicode_clean(r);
    return r;
}

/** Check if the string of code-points is a single grapheme in NFC normal form.
 *
 * @param it An iterator pointing to the first code-point.
 * @param last An iterator pointing beyond the last code-point.
 */
template<std::input_iterator It, std::sentinel_for<It> ItEnd>
[[nodiscard]] constexpr bool unicode_is_NFC_grapheme(It it, ItEnd last) noexcept
{
    if (it == last) {
        // Needs to have at least one code-point.
        return false;
    }

    if (std::distance(it, last) > 31) {
        // A maximum 30 marks is allowed after the starter.
        return false;
    }

    if (ucd_get_canonical_combining_class(*it++) != 0) {
        // The first code-point must be a starter (CCC == 0).
        return false;
    }

    // Check if each consecutive code-point is a mark (CCC != 0).
    // And that the CCC is ordered by numeric value.
    auto max_ccc = uint8_t{1};
    for (; it != last; ++it) {
        auto const ccc = ucd_get_canonical_combining_class(*it);
        if (ccc < max_ccc) {
            return false;
        }
        max_ccc = ccc;

        // XXX Needs check if code-point is allowed in NFC.

    }

    // All tests pass.
    return true;
}

} // namespace hi::inline v1
