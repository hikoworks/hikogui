// Copyright Take Vos 2020-2021.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

/** @file widgets/window_controls_macos_widget.hpp Defines window_controls_macos_widget.
 * @ingroup widgets
 */

#pragma once

#include "widget.hpp"
#include "../font/font.hpp"
#include "../macros.hpp"
#include <memory>
#include <string>
#include <array>

hi_export_module(hikogui.widgets.window_controls_macos_widget);

hi_export namespace hi {
inline namespace v1 {

/** Window control button widget.
 * This widget will display a set of buttons to control the
 * window. Most common these buttons are the minimize, maximize/restore and close buttons.
 * On MacOS these are red, green and yellow which gives them the traffic lights name.
 *
 * @ingroup widgets
 */
class window_controls_macos_widget : public widget {
public:
    using super = widget;

    window_controls_macos_widget() noexcept : super()
    {
        style.set_name("window-controls");
    }

    /// @privatesection
    [[nodiscard]] box_constraints update_constraints() noexcept override
    {
        auto const size = extent2{DIAMETER * 3.0f + 2.0f * MARGIN + 2.0f * SPACING, DIAMETER + 2.0f * MARGIN};
        return {size, size, size};
    }

    void set_layout(widget_layout const& context) noexcept override
    {
        super::set_layout(context);

        auto extent = context.size();
        if (extent.height() > floor_cast<int>(theme().large_size() * 1.2f)) {
            extent = extent2{extent.width(), theme().large_size()};
        }

        closeRectangle = aarectangle{point2(MARGIN, extent.height() / 2.0f - RADIUS), extent2{DIAMETER, DIAMETER}};

        minimizeRectangle =
            aarectangle{point2(MARGIN + DIAMETER + SPACING, extent.height() / 2.0f - RADIUS), extent2{DIAMETER, DIAMETER}};

        maximizeRectangle = aarectangle{
            point2(MARGIN + DIAMETER + SPACING + DIAMETER + SPACING, extent.height() / 2.0f - RADIUS),
            extent2{DIAMETER, DIAMETER}};

        closeWindowGlyph = find_glyph(hikogui_icon::CloseWindow);
        minimizeWindowGlyph = find_glyph(hikogui_icon::MinimizeWindow);
        maximizeWindowGlyph = find_glyph(hikogui_icon::MaximizeWindowMacOS);
        restoreWindowGlyph = find_glyph(hikogui_icon::RestoreWindowMacOS);
        auto const glyph_size = 5.0f;

        auto const closeWindowGlyphBB = closeWindowGlyph.front_glyph_metrics().bounding_rectangle * glyph_size;
        auto const minimizeWindowGlyphBB = minimizeWindowGlyph.front_glyph_metrics().bounding_rectangle * glyph_size;
        auto const maximizeWindowGlyphBB = maximizeWindowGlyph.front_glyph_metrics().bounding_rectangle * glyph_size;
        auto const restoreWindowGlyphBB = restoreWindowGlyph.front_glyph_metrics().bounding_rectangle * glyph_size;

        closeWindowGlyphRectangle = align(closeRectangle, closeWindowGlyphBB, alignment::middle_center());
        minimizeWindowGlyphRectangle = align(minimizeRectangle, minimizeWindowGlyphBB, alignment::middle_center());
        maximizeWindowGlyphRectangle = align(maximizeRectangle, maximizeWindowGlyphBB, alignment::middle_center());
        restoreWindowGlyphRectangle = align(maximizeRectangle, restoreWindowGlyphBB, alignment::middle_center());
    }

    void draw(draw_context const& context) const noexcept override
    {
        if (overlaps(context, layout())) {
            auto const close_circle_color = not front() ? color(0.246f, 0.246f, 0.246f) :
                pressedClose                                                    ? color(1.0f, 0.242f, 0.212f) :
                                                                                  color(1.0f, 0.1f, 0.082f);
            context.draw_box(layout(), closeRectangle, close_circle_color, corner_radii{RADIUS});

            auto const minimize_circle_color = not front() ? color(0.246f, 0.246f, 0.246f) :
                pressedMinimize                                                    ? color(1.0f, 0.847f, 0.093f) :
                                                                                     color(0.784f, 0.521f, 0.021f);
            context.draw_box(layout(), minimizeRectangle, minimize_circle_color, corner_radii{RADIUS});

            auto const maximize_circle_color = not front() ? color(0.246f, 0.246f, 0.246f) :
                pressedMaximize                                                    ? color(0.223f, 0.863f, 0.1f) :
                                                                                     color(0.082f, 0.533f, 0.024f);

            context.draw_box(layout(), maximizeRectangle, maximize_circle_color, corner_radii{RADIUS});

            if (phase() == widget_phase::hover) {
                context.draw_glyph(
                    layout(), translate_z(0.1f) * closeWindowGlyphRectangle, closeWindowGlyph, color{0.319f, 0.0f, 0.0f});
                context.draw_glyph(
                    layout(), translate_z(0.1f) * minimizeWindowGlyphRectangle, minimizeWindowGlyph, color{0.212f, 0.1f, 0.0f});

                if (layout().window_size_state == gui_window_size::maximized) {
                    context.draw_glyph(
                        layout(), translate_z(0.1f) * restoreWindowGlyphRectangle, restoreWindowGlyph, color{0.0f, 0.133f, 0.0f});
                } else {
                    context.draw_glyph(
                        layout(),
                        translate_z(0.1f) * maximizeWindowGlyphRectangle,
                        maximizeWindowGlyph,
                        color{0.0f, 0.133f, 0.0f});
                }
            }
        }

        return super::draw(context);
    }

    bool handle_event(gui_event const& event) noexcept override
    {
        switch (event.type()) {
        case gui_event_type::mouse_move:
        case gui_event_type::mouse_drag:
            {
                // Check the hover states of each button.
                auto state_has_changed = false;
                state_has_changed |= compare_store(hoverClose, closeRectangle.contains(event.mouse().position));
                state_has_changed |= compare_store(hoverMinimize, minimizeRectangle.contains(event.mouse().position));
                state_has_changed |= compare_store(hoverMaximize, maximizeRectangle.contains(event.mouse().position));
                if (state_has_changed) {
                    request_redraw();
                }
            }
            break;

        case gui_event_type::mouse_exit:
            hoverClose = false;
            hoverMinimize = false;
            hoverMaximize = false;
            request_redraw();
            return super::handle_event(event);

        case gui_event_type::mouse_down:
            if (event.mouse().cause.left_button) {
                if (closeRectangle.contains(event.mouse().position)) {
                    pressedClose = true;

                } else if (minimizeRectangle.contains(event.mouse().position)) {
                    pressedMinimize = true;

                } else if (maximizeRectangle.contains(event.mouse().position)) {
                    pressedMaximize = true;
                }
                request_redraw();
                return true;
            }
            break;

        case gui_event_type::mouse_up:
            if (event.mouse().cause.left_button) {
                pressedClose = false;
                pressedMinimize = false;
                pressedMaximize = false;
                request_redraw();

                if (closeRectangle.contains(event.mouse().position)) {
                    return send_to_window({gui_event_type::window_close});

                } else if (minimizeRectangle.contains(event.mouse().position)) {
                    return send_to_window({gui_event_type::window_minimize});

                } else if (maximizeRectangle.contains(event.mouse().position)) {
                    switch (layout().window_size_state) {
                    case gui_window_size::normal:
                        return send_to_window({gui_event_type::window_maximize});

                    case gui_window_size::maximized:
                        return send_to_window({gui_event_type::window_normalize});

                    default:
                        hi_no_default();
                    }
                }
                return true;
            }
            break;

        default:;
        }
        return super::handle_event(event);
    }

    [[nodiscard]] hitbox hitbox_test(point2 position) const noexcept override
    {
        hi_axiom(loop::main().on_thread());

        if (enabled() and layout().contains(position) and
            (closeRectangle.contains(position) or minimizeRectangle.contains(position) or maximizeRectangle.contains(position))) {
            return hitbox{id(), layout().elevation, hitbox_type::button};
        } else {
            return {};
        }
    }
    /// @endprivatesection
private:
    constexpr static int GLYPH_SIZE = 5;
    constexpr static int RADIUS = 5;
    constexpr static int DIAMETER = RADIUS * 2;
    constexpr static int MARGIN = 10;
    constexpr static int SPACING = 8;

    aarectangle closeRectangle;
    aarectangle minimizeRectangle;
    aarectangle maximizeRectangle;

    font_glyph_ids closeWindowGlyph;
    font_glyph_ids minimizeWindowGlyph;
    font_glyph_ids maximizeWindowGlyph;
    font_glyph_ids restoreWindowGlyph;

    aarectangle closeWindowGlyphRectangle;
    aarectangle minimizeWindowGlyphRectangle;
    aarectangle maximizeWindowGlyphRectangle;
    aarectangle restoreWindowGlyphRectangle;

    bool hoverClose = false;
    bool hoverMinimize = false;
    bool hoverMaximize = false;

    bool pressedClose = false;
    bool pressedMinimize = false;
    bool pressedMaximize = false;
};

} // namespace v1
} // namespace hi::v1
