// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "../utility/utility.hpp"
#include "../macros.hpp"
#include <cstdint>
#include <compare>
#include <limits>

hi_export_module(hikogui.font : font_id);

hi_export namespace hi {
inline namespace v1 {

/** An identifier for a font-family that was registered with HikoGUI.
 *
 * The identifier is used as a index into a vector and starts at zero.
 */
class font_id : public tagged_id<font_id, uint32_t, std::numeric_limits<uint32_t>::max()> {
public:
    using super = tagged_id<font_id, uint32_t, std::numeric_limits<uint32_t>::max()>;
    using super::super;
};

} // namespace v1
}

template<>
struct std::hash<hi::font_id> {
    [[nodiscard]] size_t operator()(hi::font_id rhs) const noexcept
    {
        return std::hash<hi::font_id::super>{}(rhs);
    }
};
