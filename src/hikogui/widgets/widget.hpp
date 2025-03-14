// Copyright Take Vos 2019-2022.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

/** @file widgets/widget.hpp Defines widget.
 * @ingroup widgets
 */

#pragma once

#include "../layout/layout.hpp"
#include "../geometry/geometry.hpp"
#include "../observer/observer.hpp"
#include "../time/time.hpp"
#include "../settings/settings.hpp"
#include "../numeric/numeric.hpp"
#include "../theme/theme.hpp"
#include "../GUI/GUI.hpp"
#include "../macros.hpp"
#include <memory>
#include <vector>
#include <string>
#include <ranges>

hi_export_module(hikogui.widgets.widget);

hi_export namespace hi { inline namespace v1 {

/** An interactive graphical object as part of the user-interface.
 *
 * Rendering is done in three distinct phases:
 *  1. Updating Constraints: `widget::update_constraints()`
 *  2. Updating Layout: `widget::set_layout()`
 *  3. Drawing: `widget::draw()`
 *
 * @ingroup widgets
 */
class widget : public widget_intf {
public:
    /** The minimum size this widget is allowed to be.
     */
    observer<extent2> minimum = extent2{};

    /** The maximum size this widget is allowed to be.
     */
    observer<extent2> maximum = extent2::large();

    /** Constructor for creating sub views.
     */
    explicit widget() noexcept : widget_intf()
    {
    }

    virtual ~widget() {}
    widget(const widget&) = delete;
    widget& operator=(const widget&) = delete;
    widget(widget&&) = delete;
    widget& operator=(widget&&) = delete;

    /** Check if the widget will accept keyboard focus.
     *
     */
    [[nodiscard]] bool accepts_keyboard_focus(keyboard_focus_group group) const noexcept override
    {
        hi_axiom(loop::main().on_thread());
        return false;
    }

    /** Handle command.
     * If a widget does not fully handle a command it should pass the
     * command to the super class' `handle_event()`.
     */
    bool handle_event(gui_event const& event) noexcept override
    {
        hi_axiom(loop::main().on_thread());

        switch (event.type()) {
        case gui_event_type::keyboard_enter:
            set_focus(true);
            this->scroll_to_show();
            return true;

        case gui_event_type::keyboard_exit:
            set_focus(false);
            return true;

        case gui_event_type::mouse_enter:
            set_hover(true);
            return true;

        case gui_event_type::mouse_exit:
            set_hover(false);
            return true;

        case gui_event_type::mouse_down:
            if (enabled() and event.mouse().cause.left_button) {
                set_active(true);
                handle_event(gui_event_type::gui_activate_stay);
            }
            return true;

        case gui_event_type::mouse_up:
            if (enabled() and event.mouse().cause.left_button) {
                set_active(false);
            }
            return true;

        case gui_event_type::window_activate:
            set_front(true);
            // All widgets need the active value set.
            return false;

        case gui_event_type::window_deactivate:
            set_front(false);
            // All widgets need the active value unset.
            return false;

        case gui_event_type::gui_activate_stay:
            send_to_window(gui_event_type::gui_activate);
            if (accepts_keyboard_focus(keyboard_focus_group::menu)) {
                // By going forward and backward we select the current parent,
                // the widget that opened the menu-stack.
                send_to_window(gui_event_type::gui_widget_next);
                send_to_window(gui_event_type::gui_widget_prev);
            }
            return true;

        case gui_event_type::gui_activate_next:
            send_to_window(gui_event_type::gui_activate);
            return send_to_window(gui_event_type::gui_widget_next);

        case gui_event_type::gui_widget_next:
            send_to_window(
                gui_event::window_set_keyboard_target(id(), keyboard_focus_group::normal, keyboard_focus_direction::forward));
            return true;

        case gui_event_type::gui_widget_prev:
            send_to_window(
                gui_event::window_set_keyboard_target(id(), keyboard_focus_group::normal, keyboard_focus_direction::backward));
            return true;

        case gui_event_type::gui_menu_next:
            if (enabled() and accepts_keyboard_focus(keyboard_focus_group::menu)) {
                send_to_window(
                    gui_event::window_set_keyboard_target(id(), keyboard_focus_group::menu, keyboard_focus_direction::forward));
                return true;
            }
            break;

        case gui_event_type::gui_menu_prev:
            if (enabled() and accepts_keyboard_focus(keyboard_focus_group::menu)) {
                send_to_window(
                    gui_event::window_set_keyboard_target(id(), keyboard_focus_group::menu, keyboard_focus_direction::backward));
                return true;
            }
            break;

        case gui_event_type::gui_toolbar_next:
            if (enabled() and accepts_keyboard_focus(keyboard_focus_group::toolbar)) {
                send_to_window(
                    gui_event::window_set_keyboard_target(id(), keyboard_focus_group::toolbar, keyboard_focus_direction::forward));
                return true;
            }
            break;

        case gui_event_type::gui_toolbar_prev:
            if (enabled() and accepts_keyboard_focus(keyboard_focus_group::toolbar)) {
                send_to_window(
                    gui_event::window_set_keyboard_target(id(), keyboard_focus_group::toolbar, keyboard_focus_direction::backward));
                return true;
            }
            break;

        default:;
        }

        return false;
    }

    bool handle_event_recursive(
        gui_event const& event,
        std::vector<widget_id> const& reject_list = std::vector<widget_id>{}) noexcept override
    {
        hi_axiom(loop::main().on_thread());

        auto handled = false;

        for (auto& child : visible_children()) {
            handled |= child.handle_event_recursive(event, reject_list);
        }

        if (!std::ranges::any_of(reject_list, [&](auto const& x) {
                return x == id();
            })) {
            handled |= handle_event(event);
        }

        return handled;
    }

    [[nodiscard]] virtual widget_id find_next_widget(
        widget_id current_keyboard_widget,
        keyboard_focus_group group,
        keyboard_focus_direction direction) const noexcept override
    {
        hi_axiom(loop::main().on_thread());

        auto found = false;

        if (current_keyboard_widget == 0 and accepts_keyboard_focus(group)) {
            // If there was no current_keyboard_widget, then return this if it accepts focus.
            return id();

        } else if (current_keyboard_widget == id()) {
            found = true;
        }

        auto children_ = std::vector<widget_intf const *>{};
        for (auto& child : visible_children()) {
            children_.push_back(std::addressof(child));
        }

        if (direction == keyboard_focus_direction::backward) {
            std::reverse(begin(children_), end(children_));
        }

        for (auto *child : children_) {
            hi_axiom_not_null(child);

            if (found) {
                // Find the first focus accepting widget.
                if (auto tmp = child->find_next_widget({}, group, direction); tmp != 0) {
                    return tmp;
                }

            } else {
                auto tmp = child->find_next_widget(current_keyboard_widget, group, direction);
                if (tmp == current_keyboard_widget) {
                    // The current widget was found, but no next widget available in the child.
                    // Try the first widget that does accept keyboard focus.
                    found = true;

                } else if (tmp != 0) {
                    // Return the next widget that was found in the child-widget.
                    return tmp;
                }
            }
        }

        if (found) {
            // Either:
            // 1. current_keyboard_widget was nullptr; this widget, nor its child widgets accept focus.
            // 2. current_keyboard_widget was this; none of the child widgets accept focus.
            // 3. current_keyboard_widget is a child; none of the following widgets accept focus.
            return current_keyboard_widget;
        }

        return {};
    }

    using widget_intf::scroll_to_show;
    void scroll_to_show(hi::aarectangle rectangle) noexcept override
    {
        hi_axiom(loop::main().on_thread());

        if (auto p = parent()) {
            p->scroll_to_show(layout().to_parent * rectangle);
        }
    }

    [[nodiscard]] hi::theme const& theme() const noexcept
    {
        if (auto w = window()) {
            return w->theme;
        } else {
            std::unreachable();
        }
    }

    [[nodiscard]] gfx_surface const *surface() const noexcept
    {
        if (auto w = window()) {
            return w->surface.get();
        } else {
            return nullptr;
        }
    }

    [[nodiscard]] virtual color background_color() const noexcept
    {
        if (enabled()) {
            if (hover()) {
                return theme().fill_color(layout().layer + 1);
            } else {
                return theme().fill_color(layout().layer);
            }
        } else {
            return theme().fill_color(layout().layer - 1);
        }
    }

    [[nodiscard]] virtual color foreground_color() const noexcept
    {
        if (enabled()) {
            return theme().border_color();
        } else {
            return theme().disabled_color();
        }
    }

    [[nodiscard]] virtual color focus_color() const noexcept
    {
        if (enabled()) {
            if (focus()) {
                return theme().accent_color();
            } else {
                return theme().border_color();
            }
        } else {
            return theme().disabled_color();
        }
    }

    [[nodiscard]] virtual color accent_color() const noexcept
    {
        if (enabled()) {
            return theme().accent_color();
        } else {
            return theme().disabled_color();
        }
    }

protected:
    /** Make an overlay rectangle.
     *
     * This function tries to create a rectangle for an overlay-widget that
     * will fit on the window. It will try and keep the rectangle in the given
     * position and of the given size, but may return something smaller and shifted.
     *
     * @param requested_rectangle A rectangle in the local coordinate system.
     * @return A rectangle that fits the window's constraints in the local coordinate system.
     */
    [[nodiscard]] aarectangle make_overlay_rectangle(aarectangle requested_rectangle) const noexcept
    {
        hi_axiom(loop::main().on_thread());

        // Move the request_rectangle to window coordinates.
        auto const requested_window_rectangle = translate2{layout().clipping_rectangle_on_window()} * requested_rectangle;
        auto const window_bounds = aarectangle{layout().window_size} - theme().margin<float>();
        auto const response_window_rectangle = fit(window_bounds, requested_window_rectangle);
        return layout().from_window * response_window_rectangle;
    }
};

}} // namespace hi::v1
