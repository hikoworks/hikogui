// Copyright Take Vos 2022.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "baseline_priority.hpp"
#include "../geometry/geometry.hpp"
#include "../utility/utility.hpp"
#include "../macros.hpp"
#include <limits>
#include <optional>

hi_export_module(hikogui.layout.shape);

hi_export namespace hi::inline v1::layout {

struct shape {
    /** Rectangle of the widget.
     * 
     * The rectangle is in window coordinates.
     */
    aarectangle rectangle;

    /** Clipping rectangle.
     * 
     * The widget is only allowed to draw inside this rectangle.
     * The clipping rectangle is in window coordinates.
     */
    aarectangle clip_rectangle;

    /** The size of the window.
     * 
     * This is used when creating overlay widgets, to determine the size and
     * location of the overlay.
     */
    extent2 window_size;

    /** The elevation of the widget above the window.
     * 
     * The elevation is used to determine the order of drawing widgets.
     */
    float elevation = 0.0f;

    /** The baseline of the widget.
     * 
     * The baseline is used to align text in the widget.
     * The baseline is the y-coordinate on the window.
     */
    float baseline;

    /** The priority of the baseline.
     * This value is used for children to inherit the baseline of the parent
     */
    hi::layout::baseline_priority baseline_priority = baseline_priority::none;
};

} // namespace v1
