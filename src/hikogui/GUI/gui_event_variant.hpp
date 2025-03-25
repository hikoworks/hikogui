// Copyright Take Vos 2022.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

/** @file GUI/gui_event_variant.hpp Definition of a GUI event variant.
 * @ingroup GUI
 */

#pragma once

#include "gui_event_type.hpp"
#include "../macros.hpp"

hi_export_module(hikogui.GUI : gui_event_variant);

hi_export namespace hi { inline namespace v1 {

/** A granular gui event type.
 * @ingroup GUI
 */
enum class gui_event_variant {
    /** The gui_event does not have associated data.
     */
    other,

    /** The gui_event has mouse data.
     */
    mouse,

    /** The gui_event has keyboard data.
     */
    keyboard,

    /** The gui_event has keyboard target data.
     */
    keyboard_target,

    /** The gui_event has grapheme data.
     */
    grapheme,

    /** The gui_event has clipboard data.
     */
    clipboard_data,
};

/** Convert a gui event type, to an gui event variant.
 * @ingroup GUI
 */
[[nodiscard]] constexpr gui_event_variant to_gui_event_variant(gui_event_type type) noexcept
{
    switch (type) {
    case gui_event_type::mouse_move:
    case gui_event_type::mouse_drag:
    case gui_event_type::mouse_down:
    case gui_event_type::mouse_up:
    case gui_event_type::mouse_wheel:
    case gui_event_type::mouse_enter:
    case gui_event_type::mouse_exit:
    case gui_event_type::mouse_exit_window:
        return gui_event_variant::mouse;

    case gui_event_type::keyboard_down:
    case gui_event_type::keyboard_up:
    case gui_event_type::keyboard_enter:
    case gui_event_type::keyboard_exit:
        return gui_event_variant::keyboard;

    case gui_event_type::window_set_keyboard_target:
        return gui_event_variant::keyboard_target;

    case gui_event_type::window_set_clipboard:
    case gui_event_type::text_edit_paste:
        return gui_event_variant::clipboard_data;

    case gui_event_type::keyboard_grapheme:
    case gui_event_type::keyboard_partial_grapheme:
        return gui_event_variant::grapheme;

    default:
        return gui_event_variant::other;
    }
}

}} // namespace hi::v1
