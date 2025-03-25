// Copyright Take Vos 2023.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "../concurrency/concurrency.hpp"
#include "../theme/theme.hpp"
#include "../macros.hpp"
#include <cstdint>
#include <utility>
#include <concepts>
#include <compare>

hi_export_module(hikogui.GUI : widget_state);

hi_export namespace hi { inline namespace v1 {


enum class widget_value {
    off = 0,
    on = 1,
    other = 2,
};

enum class widget_phase {
    /** The widget is disabled and shown with a muted color.
     */
    disabled = 0,

    /** The widget is enabled.
     */
    enabled = 1,

    /** The mouse is hovering over the widget.
     */
    hover = 2,

    /** The widget is being activated by a click or keyboard action.
     */
    active = 3,
};

}}
