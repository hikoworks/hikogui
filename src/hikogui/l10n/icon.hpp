// Copyright Take Vos 2024.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

/** @file label.hpp Functionality for labels, text and icons.
 */

#pragma once

#include "../utility/utility.hpp"
#include "../unicode/unicode.hpp"
#include "../path/path.hpp"
#include "../font/font.hpp"
#include "../macros.hpp"
#include <string>
#include <type_traits>
#include <memory>
#include <variant>

hi_export_module(hikogui.l10n : icon);


hi_export namespace hi::inline v1 {

/** A variant of icon.
 *
 * May be:
 *  - `std::monostate`
 *  - `hi::elusive_icon`
 *  - `hi::hikogui_icon`
 *  - `hi::font_glyph_ids`
 *  - `hi::pixmap<hi::sfloat_rgba16>`
 */
class icon : public std::variant<std::monostate, elusive_icon, hikogui_icon, font_glyph_ids, bookmark>
{
    using std::variant<std::monostate, elusive_icon, hikogui_icon, font_glyph_ids, bookmark>::variant;

public:
    [[nodiscard]] constexpr bool empty() const noexcept
    {
        return std::holds_alternative<std::monostate>(*this);
    }
};

}