// Copyright Take Vos 2022.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

/** @file GUI/gui_event_type.hpp Definition of GUI event type.
 * @ingroup GUI
 */

#pragma once

#include "../utility/utility.hpp"
#include "../macros.hpp"
#include <string>
#include <string_view>
#include <ostream>

hi_export_module(hikogui.GUI : gui_event_type);

hi_export namespace hi { inline namespace v1 {

/** GUI event type.
 * @ingroup GUI
 */
enum class gui_event_type {
    none,

    // raw-keyboard-events
    keyboard_down,
    keyboard_up,
    keyboard_grapheme,
    keyboard_partial_grapheme,
    keyboard_enter,
    keyboard_exit,

    // raw-mouse-events
    mouse_move,
    mouse_drag,
    mouse_down,
    mouse_up,
    mouse_wheel,
    mouse_enter,
    mouse_exit,
    mouse_exit_window,

    // window-events
    window_minimize, ///< Request the window to minimize.
    window_maximize, ///< Request the window to maximize.
    window_normalize, ///< Request the window to be restored to the original size after a minimize and maximize commands.
    window_close, ///< Request the window to be closed.
    window_open_sysmenu, ///< Open the operating system provided menu for the window.
    window_set_keyboard_target, ///< Change the keyboard target widget for this window.
    window_set_clipboard, ///< Place data on the clipboard.
    window_activate, ///< The window becomes the top-window.
    window_deactivate, ///< The window is not longer the top-window.

    // commands
    text_cursor_left_char,
    text_cursor_right_char,
    text_cursor_down_char,
    text_cursor_up_char,
    text_cursor_left_word,
    text_cursor_right_word,
    text_cursor_begin_line,
    text_cursor_end_line,
    text_cursor_begin_sentence,
    text_cursor_end_sentence,
    text_cursor_begin_document,
    text_cursor_end_document,
    text_select_left_char,
    text_select_right_char,
    text_select_down_char,
    text_select_up_char,
    text_select_word,
    text_select_left_word,
    text_select_right_word,
    text_select_begin_line,
    text_select_end_line,
    text_select_begin_sentence,
    text_select_end_sentence,
    text_select_document,
    text_select_begin_document,
    text_select_end_document,
    text_delete_char_prev,
    text_delete_char_next,
    text_delete_word_prev,
    text_delete_word_next,
    text_swap_chars,
    text_edit_paste,
    text_edit_copy,
    text_edit_cut,
    text_undo,
    text_redo,
    text_insert_line,
    text_insert_line_up,
    text_insert_line_down,
    text_mode_insert,

    // GUI navigation.
    gui_widget_next,
    gui_widget_prev,
    gui_menu_next,
    gui_menu_prev,
    gui_toolbar_open,
    gui_toolbar_next,
    gui_toolbar_prev,
    gui_activate,
    gui_activate_stay,
    gui_activate_next,
    gui_cancel,
};

// clang-format off
constexpr auto gui_event_type_metadata = enum_metadata{
    gui_event_type::none, "none",
    gui_event_type::keyboard_down, "keyboard_down",
    gui_event_type::keyboard_grapheme, "keyboard_grapheme",
    gui_event_type::keyboard_partial_grapheme, "keyboard_partial_grapheme",
    gui_event_type::keyboard_enter, "keyboard_enter",
    gui_event_type::keyboard_exit, "keyboard_exit",
    gui_event_type::mouse_move, "mouse_move",
    gui_event_type::mouse_drag, "mouse_drag",
    gui_event_type::mouse_down, "mouse_down",
    gui_event_type::mouse_up, "mouse_up",
    gui_event_type::mouse_wheel, "mouse_wheel",
    gui_event_type::mouse_enter, "mouse_enter",
    gui_event_type::mouse_exit, "mouse_exit",
    gui_event_type::mouse_exit_window, "mouse_exit_window",
    gui_event_type::window_minimize, "window_minimize",
    gui_event_type::window_maximize, "window_maximize",
    gui_event_type::window_normalize, "window_normalize",
    gui_event_type::window_open_sysmenu, "window_open_sysmenu",
    gui_event_type::window_close, "window_close",
    gui_event_type::window_set_keyboard_target, "window_set_keyboard_target",
    gui_event_type::window_set_clipboard, "window_set_clipboard",
    gui_event_type::window_activate, "window_activate",
    gui_event_type::window_deactivate, "window_deactivate",
    gui_event_type::text_cursor_left_char, "text_cursor_left_char",
    gui_event_type::text_cursor_right_char, "text_cursor_right_char",
    gui_event_type::text_cursor_down_char, "text_cursor_down_char",
    gui_event_type::text_cursor_up_char, "text_cursor_up_char",
    gui_event_type::text_cursor_left_word, "text_cursor_left_word",
    gui_event_type::text_cursor_right_word, "text_cursor_right_word",
    gui_event_type::text_cursor_begin_line, "text_cursor_begin_line",
    gui_event_type::text_cursor_end_line, "text_cursor_end_line",
    gui_event_type::text_cursor_begin_sentence, "text_cursor_begin_sentence",
    gui_event_type::text_cursor_end_sentence, "text_cursor_end_sentence",
    gui_event_type::text_cursor_begin_document, "text_cursor_begin_document",
    gui_event_type::text_cursor_end_document, "text_cursor_end_document",
    gui_event_type::text_select_left_char, "text_select_left_char",
    gui_event_type::text_select_right_char, "text_select_right_char",
    gui_event_type::text_select_down_char, "text_select_down_char",
    gui_event_type::text_select_up_char, "text_select_up_char",
    gui_event_type::text_select_word, "text_select_word",
    gui_event_type::text_select_left_word, "text_select_left_word",
    gui_event_type::text_select_right_word, "text_select_right_word",
    gui_event_type::text_select_begin_line, "text_select_begin_line",
    gui_event_type::text_select_end_line, "text_select_end_line",
    gui_event_type::text_select_begin_sentence, "text_select_begin_sentence",
    gui_event_type::text_select_end_sentence, "text_select_end_sentence",
    gui_event_type::text_select_begin_document, "text_select_begin_document",
    gui_event_type::text_select_end_document, "text_select_end_document",
    gui_event_type::text_select_document, "text_select_document",
    gui_event_type::text_delete_char_prev, "text_delete_char_prev",
    gui_event_type::text_delete_char_next, "text_delete_char_next",
    gui_event_type::text_delete_word_prev, "text_delete_word_prev",
    gui_event_type::text_delete_word_next, "text_delete_word_next",
    gui_event_type::text_insert_line, "text_insert_line",
    gui_event_type::text_insert_line_up, "text_insert_line_up",
    gui_event_type::text_insert_line_down, "text_insert_line_down",
    gui_event_type::text_swap_chars, "text_swap_chars",
    gui_event_type::text_edit_paste, "text_edit_paste",
    gui_event_type::text_edit_copy, "text_edit_copy",
    gui_event_type::text_edit_cut, "text_edit_cut",
    gui_event_type::text_undo, "text_undo",
    gui_event_type::text_redo, "text_redo",
    gui_event_type::text_mode_insert, "text_mode_insert",
    gui_event_type::gui_widget_next, "gui_widget_next",
    gui_event_type::gui_widget_prev, "gui_widget_prev",
    gui_event_type::gui_menu_next, "gui_menu_next",
    gui_event_type::gui_menu_prev, "gui_menu_prev",
    gui_event_type::gui_toolbar_open, "gui_toolbar_open",
    gui_event_type::gui_toolbar_next, "gui_toolbar_next",
    gui_event_type::gui_toolbar_prev, "gui_toolbar_prev",
    gui_event_type::gui_activate, "gui_activate",
    gui_event_type::gui_activate_stay, "gui_activate_stay",
    gui_event_type::gui_activate_next, "gui_activate_next",
    gui_event_type::gui_cancel, "gui_cancel"
};
// clang-format on

/** Convert a GUI event type to a string.
 */
inline std::string_view to_string(gui_event_type rhs) noexcept
{
    return gui_event_type_metadata[rhs];
}

/** Convert a name to a GUI event type.
 */
constexpr gui_event_type to_gui_event_type(std::string_view name) noexcept
{
    return gui_event_type_metadata.at(name, gui_event_type::none);
}

}} // namespace hi::inline v1

