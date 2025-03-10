// Copyright Take Vos 2022.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

/** @file text/phrasing.hpp The phrasing type.
 * @ingroup text
 */

module;
#include "../macros.hpp"

#include <cstdint>
#include <optional>
#include <bit>
#include <string_view>
#include <format>

export module hikogui_unicode_phrasing;
import hikogui_utility;


export namespace hi::inline v1 {

/** Phrasing.
 * @ingroup unicode
 *
 * This is the phrasing of a piece of text. The phrasing determines
 * the style of text on a semantic level. Similar to HTML phrasing tags.
 *
 * The underlying value must be between 0 through 63;
 * so that the phrasing_mask can be 64-bits.
 */
enum class phrasing : uint8_t {
    /** Regular, normal text.
     */
    regular = 0,

    /** Emphesised text; spoken as if the text is special importance, significant or promonent.
     * Often formatted in italic.
     */
    emphesis = 1,

    /** Strong text; spoken louder, as if the text is not to be missed.
     * Often formatted in bold.
     */
    strong = 2,

    /** Text is a piece of programming-code; a variable name, a function name.
     * Often formatted in a constant-width font, with a greater weight and in a different color and possible
     * background block, than the surrounding text.
     */
    code = 3,

    /** An abbreviation.
     * Sometimes formatted with a double underline and hovering will show the expansion of the abbreviation.
     */
    abbreviation = 4,

    /** The text is quoted from somewhere.
     * Often formatted using a more italic / cursive font, with a lower weight.
     */
    quote = 5,

    /** Used in help text to show which key or button to press.
     * Often formatted with a background that looks raised up like a button.
     * With the text in inverted color.
     */
    keyboard = 6,

    /** The text is marked or highlighted as if being marked by a highlight pen.
     * Often formatted with a yellow background.
     */
    highlight = 7,

    /** Text formatted as math.
     * Often formatted using a special math font.
     */
    math = 8,

    /** Used in help text to show an example.
     * Often formatted using a non-proportional font with a low resultion bitmap-like style.
     */
    example = 9, ///< Used for displaying console output.

    /** Unarticulated.
     * Often formatted using an underlying line.
     */
    unarticulated = 10,

    /** Format a heading
     * Often in bold, larger font and on a line by itself.
     */
    title = 11,

    /** Format a "good" message
     * Often in bright green.
     */
    success = 12,

    /** Format a warning message
     * Often in bright yellow.
     */
    warning = 13,

    /** Format a "bad" message
     * Often in bright red.
     */
    error = 14,
};

// clang-format off
constexpr auto phrasing_metadata = enum_metadata{
    phrasing::regular, "regular",
    phrasing::emphesis, "emphesis",
    phrasing::strong, "strong",
    phrasing::code, "code",
    phrasing::abbreviation, "abbreviation",
    phrasing::quote, "quote",
    phrasing::keyboard, "keyboard",
    phrasing::highlight, "highlight",
    phrasing::math, "math",
    phrasing::example, "example",
    phrasing::unarticulated, "unarticulated",
    phrasing::title, "title",
    phrasing::success, "success",
    phrasing::warning, "warning",
    phrasing::error, "error",
};
// clang-format on

// clang-format off
[[nodiscard]] constexpr std::optional<phrasing> to_phrasing(char c)
{
    switch (c) {
    case 'r': return phrasing::regular;
    case 'e': return phrasing::emphesis;
    case 's': return phrasing::strong;
    case 'c': return phrasing::code;
    case 'a': return phrasing::abbreviation;
    case 'q': return phrasing::quote;
    case 'k': return phrasing::keyboard;
    case 'h': return phrasing::highlight;
    case 'm': return phrasing::math;
    case 'x': return phrasing::example;
    case 'u': return phrasing::unarticulated;
    case 't': return phrasing::title;
    case 'S': return phrasing::success;
    case 'W': return phrasing::warning;
    case 'E': return phrasing::error;
    default:
        return std::nullopt;
    }
}
// clang-format on

enum class phrasing_mask : uint16_t {
    regular = 1 << std::to_underlying(phrasing::regular),
    emphesis = 1 << std::to_underlying(phrasing::emphesis),
    strong = 1 << std::to_underlying(phrasing::strong),
    code = 1 << std::to_underlying(phrasing::code),
    abbreviation = 1 << std::to_underlying(phrasing::abbreviation),
    quote = 1 << std::to_underlying(phrasing::quote),
    keyboard = 1 << std::to_underlying(phrasing::keyboard),
    highlight = 1 << std::to_underlying(phrasing::highlight),
    math = 1 << std::to_underlying(phrasing::math),
    example = 1 << std::to_underlying(phrasing::example),
    unarticulated = 1 << std::to_underlying(phrasing::unarticulated),
    title = 1 << std::to_underlying(phrasing::title),
    success = 1 << std::to_underlying(phrasing::success),
    warning = 1 << std::to_underlying(phrasing::warning),
    error = 1 << std::to_underlying(phrasing::error),

    all = regular | emphesis | strong | code | abbreviation | quote | keyboard | highlight | math | example | unarticulated |
        title | success | warning | error
};

static_assert(
    std::bit_width(phrasing_metadata.size() - 1) <= sizeof(std::underlying_type_t<phrasing_mask>) * CHAR_BIT,
    "All phrasings must fit the phrasing_mask.");

[[nodiscard]] constexpr phrasing_mask operator&(phrasing_mask const& lhs, phrasing_mask const& rhs) noexcept
{
    return static_cast<phrasing_mask>(std::to_underlying(lhs) & std::to_underlying(rhs));
}

[[nodiscard]] constexpr phrasing_mask operator|(phrasing_mask const& lhs, phrasing_mask const& rhs) noexcept
{
    return static_cast<phrasing_mask>(std::to_underlying(lhs) | std::to_underlying(rhs));
}

[[nodiscard]] constexpr phrasing_mask to_phrasing_mask(phrasing const& rhs) noexcept
{
    hi_axiom(std::to_underlying(rhs) < sizeof(phrasing_mask) * CHAR_BIT);
    return static_cast<phrasing_mask>(1 << std::to_underlying(rhs));
}

[[nodiscard]] constexpr phrasing_mask to_phrasing_mask(std::string const& str)
{
    auto r = phrasing_mask{};

    for (hilet c : str) {
        if (c == '*') {
            r = phrasing_mask::all;
        } else if (hilet p = to_phrasing(c)) {
            r = r | to_phrasing_mask(*p);
        } else {
            throw parse_error(std::format("Unknown character '{}' in text-phrasing-mask", c));
        }
    }

    return r;
}

[[nodiscard]] constexpr bool all(phrasing_mask const& rhs) noexcept
{
    return (rhs & phrasing_mask::all) == phrasing_mask::all;
}

[[nodiscard]] constexpr bool to_bool(phrasing_mask const& rhs) noexcept
{
    return to_bool(std::to_underlying(rhs));
}

/** Check if the text-phrasing is included in the text-phrasing-mask.
 *
 * @param lhs The text-phrasing-mask, i.e. the pattern.
 * @param rhs The text-phrasing.
 * @return True when the text-phrasing is part of the text-phrasing-mask.
 */
[[nodiscard]] constexpr bool matches(phrasing_mask const& lhs, phrasing const& rhs) noexcept
{
    return to_bool(lhs & to_phrasing_mask(rhs));
}

} // namespace hi::inline v1
