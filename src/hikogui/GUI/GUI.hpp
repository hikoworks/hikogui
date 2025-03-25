
#pragma once

#include "gui_event.hpp" // export
#include "gui_event_type.hpp" // export
#include "gui_event_variant.hpp" // export
#include "gui_window_size.hpp" // export
#include "gui_window_win32.hpp" // export
#include "hitbox.hpp" // export
#include "keyboard_bindings.hpp" // export
#include "keyboard_focus_direction.hpp" // export
#include "keyboard_focus_group.hpp" // export
#include "keyboard_key.hpp" // export
#include "keyboard_modifiers.hpp" // export
#include "keyboard_state.hpp" // export
#include "keyboard_virtual_key_intf.hpp" // export
#include "keyboard_virtual_key_win32_impl.hpp" // export
#include "mouse_buttons.hpp" // export
#include "mouse_cursor.hpp" // export
#include "theme.hpp" // export
#include "theme_book.hpp" // export
#include "widget_id.hpp" // export
#include "widget_intf.hpp" // export
#include "widget_layout.hpp" // export
#include "widget_state.hpp" // export

hi_export_module(hikogui.GUI);

hi_export namespace hi { inline namespace v1 {
/**
\defgroup GUI Graphical User Interface
\brief Operating System interface to windows, keyboard and mouse handling
*/
}}

