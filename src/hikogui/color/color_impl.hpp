// Copyright Take Vos 2021-2022.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

/** @file color/color.hpp Defined the color type.
 * @ingroup color
 */

#pragma once

#include "color_intf.hpp"
#include "sRGB.hpp"
#include "../macros.hpp"

hi_export_module(hikogui.color : color_impl);

hi_export namespace hi::inline v1 {

// clang-format off

template<> inline auto named_color<"black"> = detail::named_color_type<"black">{color_from_sRGB<8>(0, 0, 0)};
template<> inline auto named_color<"silver"> = detail::named_color_type<"silver">{color_from_sRGB<8>(192, 192, 192)};
template<> inline auto named_color<"gray"> = detail::named_color_type<"gray">{color_from_sRGB<8>(128, 128, 128)};
template<> inline auto named_color<"white"> = detail::named_color_type<"white">{color_from_sRGB<8>(255, 255, 255)};
template<> inline auto named_color<"maroon"> = detail::named_color_type<"maroon">{color_from_sRGB<8>(128, 0, 0)};
template<> inline auto named_color<"red"> = detail::named_color_type<"red">{color_from_sRGB<8>(255, 0, 0)};
template<> inline auto named_color<"purple"> = detail::named_color_type<"purple">{color_from_sRGB<8>(128, 0, 128)};
template<> inline auto named_color<"fuchsia"> = detail::named_color_type<"fuchsia">{color_from_sRGB<8>(255, 0, 255)};
template<> inline auto named_color<"green"> = detail::named_color_type<"green">{color_from_sRGB<8>(0, 128, 0)};
template<> inline auto named_color<"lime"> = detail::named_color_type<"lime">{color_from_sRGB<8>(0, 255, 0)};
template<> inline auto named_color<"olive"> = detail::named_color_type<"olive">{color_from_sRGB<8>(128, 128, 0)};
template<> inline auto named_color<"yellow"> = detail::named_color_type<"yellow">{color_from_sRGB<8>(255, 255, 0)};
template<> inline auto named_color<"navy"> = detail::named_color_type<"navy">{color_from_sRGB<8>(0, 0, 128)};
template<> inline auto named_color<"blue"> = detail::named_color_type<"blue">{color_from_sRGB<8>(0, 0, 255)};
template<> inline auto named_color<"teal"> = detail::named_color_type<"teal">{color_from_sRGB<8>(0, 128, 128)};
template<> inline auto named_color<"aqua"> = detail::named_color_type<"aqua">{color_from_sRGB<8>(0, 255, 255)};
template<> inline auto named_color<"indigo"> = detail::named_color_type<"indigo">{color_from_sRGB<8>(75, 0, 130)};
template<> inline auto named_color<"orange"> = detail::named_color_type<"orange">{color_from_sRGB<8>(255, 165, 0)};
template<> inline auto named_color<"pink"> = detail::named_color_type<"pink">{color_from_sRGB<8>(255, 192, 203)};
template<> inline auto named_color<"gray0"> = detail::named_color_type<"gray0">{color_from_sRGB<8>(0, 0, 0)};
template<> inline auto named_color<"gray1"> = detail::named_color_type<"gray1">{color_from_sRGB<8>(26, 26, 26)};
template<> inline auto named_color<"gray2"> = detail::named_color_type<"gray2">{color_from_sRGB<8>(51, 51, 51)};
template<> inline auto named_color<"gray3"> = detail::named_color_type<"gray3">{color_from_sRGB<8>(77, 77, 77)};
template<> inline auto named_color<"gray4"> = detail::named_color_type<"gray4">{color_from_sRGB<8>(102, 102, 102)};
template<> inline auto named_color<"gray5"> = detail::named_color_type<"gray5">{color_from_sRGB<8>(127, 127, 127)};
template<> inline auto named_color<"gray6"> = detail::named_color_type<"gray6">{color_from_sRGB<8>(153, 153, 153)};
template<> inline auto named_color<"gray7"> = detail::named_color_type<"gray7">{color_from_sRGB<8>(179, 179, 179)};
template<> inline auto named_color<"gray8"> = detail::named_color_type<"gray8">{color_from_sRGB<8>(204, 204, 204)};
template<> inline auto named_color<"gray9"> = detail::named_color_type<"gray9">{color_from_sRGB<8>(229, 229, 229)};
template<> inline auto named_color<"gray10"> = detail::named_color_type<"gray10">{color_from_sRGB<8>(255, 255, 255)};
template<> inline auto named_color<"transparent"> = detail::named_color_type<"transparent">{color_from_sRGB<8>(0, 0, 0, 0)};

[[nodiscard]] inline color color::black() noexcept { return named_color<"black">; }
[[nodiscard]] inline color color::silver() noexcept { return named_color<"silver">; }
[[nodiscard]] inline color color::gray() noexcept { return named_color<"gray5">; }
[[nodiscard]] inline color color::white() noexcept { return named_color<"white">; }
[[nodiscard]] inline color color::maroon() noexcept { return named_color<"maroon">; }
[[nodiscard]] inline color color::red() noexcept { return named_color<"red">; }
[[nodiscard]] inline color color::purple() noexcept { return named_color<"purple">; }
[[nodiscard]] inline color color::fuchsia() noexcept { return named_color<"fuchsia">; }
[[nodiscard]] inline color color::green() noexcept { return named_color<"green">; }
[[nodiscard]] inline color color::lime() noexcept { return named_color<"lime">; }
[[nodiscard]] inline color color::olive() noexcept { return named_color<"olive">; }
[[nodiscard]] inline color color::yellow() noexcept { return named_color<"yellow">; }
[[nodiscard]] inline color color::navy() noexcept { return named_color<"navy">; }
[[nodiscard]] inline color color::blue() noexcept { return named_color<"blue">; }
[[nodiscard]] inline color color::teal() noexcept { return named_color<"teal">; }
[[nodiscard]] inline color color::aqua() noexcept { return named_color<"aqua">; }
[[nodiscard]] inline color color::indigo() noexcept { return named_color<"indigo">; }
[[nodiscard]] inline color color::orange() noexcept { return named_color<"orange">; }
[[nodiscard]] inline color color::pink() noexcept { return named_color<"pink">; }
[[nodiscard]] inline color color::gray0() noexcept { return named_color<"gray0">; }
[[nodiscard]] inline color color::gray1() noexcept { return named_color<"gray1">; }
[[nodiscard]] inline color color::gray2() noexcept { return named_color<"gray2">; }
[[nodiscard]] inline color color::gray3() noexcept { return named_color<"gray3">; }
[[nodiscard]] inline color color::gray4() noexcept { return named_color<"gray4">; }
[[nodiscard]] inline color color::gray5() noexcept { return named_color<"gray5">; }
[[nodiscard]] inline color color::gray6() noexcept { return named_color<"gray6">; }
[[nodiscard]] inline color color::gray7() noexcept { return named_color<"gray7">; }
[[nodiscard]] inline color color::gray8() noexcept { return named_color<"gray8">; }
[[nodiscard]] inline color color::gray9() noexcept { return named_color<"gray9">; }
[[nodiscard]] inline color color::gray10() noexcept { return named_color<"gray10">; }
[[nodiscard]] inline color color::transparent() noexcept { return named_color<"transparent">; }
// clang-format on

}
