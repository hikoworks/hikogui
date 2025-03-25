// Copyright Take Vos 2024.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "../macros.hpp"
#include <utility>

hi_export_module(hikogui.theme : style_pseudo_class);

hi_export namespace hi {
inline namespace v1 {

// clang-format off
/** The different dynamic states of a widget from the point of view of styles.
 */
enum class style_pseudo_class {
    /** The widget is disabled, often shown in gray.
     * 
     * @note Must be one of: disabled, enabled, hover, active.
     */
    disabled  = 0b0'0'0'00,

    /** The widget is enabled, the normal idle state.
     * 
     * @note Must be one of: disabled, enabled, hover, active.
     */
    enabled   = 0b0'0'0'01,

    /** The mouse hovers over the widget.
     * 
     * @note Must be one of: disabled, enabled, hover, active.
     */
    hover     = 0b0'0'0'10,

    /** The widget was clicked by the mouse or otherwise activated.
     * 
     * @note Must be one of: disabled, enabled, hover, active.
     */
    active    = 0b0'0'0'11,

    /** The widget has keyboard focus.
     */
    focus     = 0b0'0'1'00,

    /** A widget like a radio button or checkbox is checked.
     */
    checked   = 0b0'1'0'00,

    /** The window is the front (the active) window.
     */
    front     = 0b1'0'0'00,

    /** The mask to use extract the values: disabled, enabled, hover and active.
     */
    phase_mask = 0b0'0'0'11,
};
// clang-format on

constexpr auto style_pseudo_class_size = size_t{32};

[[nodiscard]] constexpr style_pseudo_class operator~(style_pseudo_class const& rhs) noexcept
{
    return static_cast<style_pseudo_class>(~std::to_underlying(rhs));
}

[[nodiscard]] constexpr style_pseudo_class operator|(style_pseudo_class const& lhs, style_pseudo_class const& rhs) noexcept
{
    return static_cast<style_pseudo_class>(std::to_underlying(lhs) | std::to_underlying(rhs));
}

[[nodiscard]] constexpr style_pseudo_class operator&(style_pseudo_class const& lhs, style_pseudo_class const& rhs) noexcept
{
    return static_cast<style_pseudo_class>(std::to_underlying(lhs) & std::to_underlying(rhs));
}

constexpr style_pseudo_class& operator|=(style_pseudo_class& lhs, style_pseudo_class const& rhs) noexcept
{
    return lhs = lhs | rhs;
}

constexpr style_pseudo_class& operator&=(style_pseudo_class& lhs, style_pseudo_class const& rhs) noexcept
{
    return lhs = lhs & rhs;
}

} // namespace v1
}
