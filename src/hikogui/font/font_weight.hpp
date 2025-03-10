// Copyright Take Vos 2021-2022.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "../utility/utility.hpp"
#include "../macros.hpp"
#include <string>
#include <string_view>
#include <unordered_map>
#include <ostream>
#include <array>
#include <coroutine>
#include <format>

hi_export_module(hikogui.font.font_weight);

hi_export namespace hi::inline v1 {

enum class font_weight {
    thin, ///< 100: Thin / Hairline
    extra_light, ///< 200: Ultra-light / Extra-light
    light, ///< 300: Light
    regular, ///< 400: Normal / Regular
    medium, ///< 500: Medium
    semi_bold, ///< 600: Semi-bold
    bold, ///< 700: Bold
    extra_bold, ///< 800: Extra-bold / Ultra-bold
    black, ///< 900: Heavy / Black
    extra_black, ///< 950: Extra-black / Ultra-black
};

constexpr font_weight& operator++(font_weight& rhs) noexcept
{
    hi_axiom(rhs < font_weight::extra_black);
    return rhs = static_cast<font_weight>(std::to_underlying(rhs) + 1);
}

constexpr font_weight& operator--(font_weight& rhs) noexcept
{
    hi_axiom(rhs > font_weight::thin);
    return rhs = static_cast<font_weight>(std::to_underlying(rhs) - 1);
}

// clang-format off
constexpr auto font_weight_metadata = enum_metadata{
    font_weight::thin, "thin",
    font_weight::extra_light, "extra-light",
    font_weight::light, "light",
    font_weight::regular, "regular",
    font_weight::medium, "medium",
    font_weight::semi_bold, "semi-bold",
    font_weight::bold, "bold",
    font_weight::extra_bold, "extra-bold",
    font_weight::black, "black",
    font_weight::extra_black, "extra-black",
};
// clang-format on

/** Convert a font weight value between 50 and 1000 to a font weight.
 */
[[nodiscard]] constexpr font_weight font_weight_from_int(numeric_integral auto rhs)
{
    if (rhs < 50 or rhs > 1000) {
        throw parse_error(std::format("Unknown font-weight {}", rhs));
    }
    return static_cast<font_weight>(((rhs + 50) / 100) - 1);
}

[[nodiscard]] constexpr font_weight font_weight_from_string(std::string_view rhs)
{
    if (auto weight = font_weight_metadata.at_if(rhs)) {
        return *weight;
    } else {
        throw parse_error(std::format("Unknown font-weight {}", rhs));
    }
}

[[nodiscard]] constexpr std::string_view to_string_view(font_weight const& x) noexcept
{
    return font_weight_metadata[x];
}

[[nodiscard]] constexpr std::string to_string(font_weight const& x) noexcept
{
    return std::string{to_string_view(x)};
}

[[nodiscard]] constexpr char to_char(font_weight const& x) noexcept
{
    auto const x_ = static_cast<int>(x);
    hi_axiom(x_ >= 0 && x_ <= 9);
    return char_cast<char>('0' + x_);
}

[[nodiscard]] constexpr int to_int(font_weight const& x) noexcept
{
    auto const x_ = (static_cast<int>(x) + 1) * 100;
    return (x_ == 1000) ? 950 : x_;
}

inline std::ostream& operator<<(std::ostream& lhs, font_weight const& rhs)
{
    return lhs << to_string(rhs);
}

constexpr bool almost_equal(font_weight const& lhs, font_weight const& rhs) noexcept
{
    // Check only if it is bold or not.
    return (lhs > font_weight::medium) == (rhs > font_weight::medium);
}

/** Generate alternatives for the font_weight.
 *
 * @param start The starting font-weight.
 * @return Generated font weights, starting at start, then zig-zag toward thin and extra-black.
 */
[[nodiscard]] inline generator<font_weight> alternatives(font_weight start) noexcept
{
    co_yield start;

    auto min = start;
    auto max = start;
    auto forward = false;
    while (min > font_weight::thin or max < font_weight::extra_black) {
        if ((forward and max == font_weight::extra_black) or (not forward and min == font_weight::thin)) {
            // Change direction to not overflow.
            forward = not forward;
        }

        if (forward) {
            co_yield ++max;
        } else {
            co_yield --min;
        }

        // Zig-zag through each weight.
        forward = not forward;
    }
}

} // namespace hi::inline v1

// XXX #617 MSVC bug does not handle partial specialization in modules.
hi_export template<>
struct std::formatter<hi::font_weight, char> : std::formatter<std::string_view, char> {
    auto format(hi::font_weight const& t, auto& fc) const
    {
        return std::formatter<std::string_view, char>::format(hi::to_string_view(t), fc);
    }
};
