// Copyright Take Vos 2019-2022.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

/** @file widgets/widget.hpp Defines widget.
 * @ingroup widgets
 */

module;
#include "../macros.hpp"

#include <memory>
#include <vector>
#include <string>
#include <ranges>

export module hikogui_widgets_widget;
import hikogui_GUI;
import hikogui_coroutine;
import hikogui_geometry;
import hikogui_layout;
import hikogui_numeric;
import hikogui_observer;
import hikogui_settings;
import hikogui_time;
import hikogui_widgets_widget_mode;

export namespace hi { inline namespace v1 {

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
    /** The widget mode.
     * The current visibility and interactivity of a widget.
     */
    observer<widget_mode> mode = widget_mode::enabled;

    /** Mouse cursor is hovering over the widget.
     */
    observer<bool> hover = false;

    /** The widget has keyboard focus.
     */
    observer<bool> focus = false;

    /** The minimum size this widget is allowed to be.
     */
    observer<extent2> minimum = extent2{};

    /** The maximum size this widget is allowed to be.
     */
    observer<extent2> maximum = extent2::large();

    /*! Constructor for creating sub views.
     */
    explicit widget(widget_intf const * parent) noexcept : widget_intf(parent)
    {
        hi_axiom(loop::main().on_thread());

        _mode_cbt = mode.subscribe([&](auto...) {
            ++global_counter<"widget:mode:constrain">;
            process_event({gui_event_type::window_reconstrain});
        });

        _focus_cbt = focus.subscribe([&](auto...) {
            ++global_counter<"widget:focus:redraw">;
            request_redraw();
        });

        _hover_cbt = hover.subscribe([&](auto...) {
            ++global_counter<"widget:hover:redraw">;
            request_redraw();
        });
    }

    virtual ~widget() {}
    widget(const widget&) = delete;
    widget& operator=(const widget&) = delete;
    widget(widget&&) = delete;
    widget& operator=(widget&&) = delete;

    using widget_intf::children;
    [[nodiscard]] generator<widget_intf&> children(bool include_invisible) noexcept override
    {
        co_return;
    }

    /** Find the widget that is under the mouse cursor.
     * This function will recursively test with visual child widgets, when
     * widgets overlap on the screen the hitbox object with the highest elevation is returned.
     *
     * @param position The coordinate of the mouse local to the widget.
     * @return A hit_box object with the cursor-type and a reference to the widget.
     */
    [[nodiscard]] hitbox hitbox_test(point2 position) const noexcept override
    {
        return {};
    }

    /** Call hitbox_test from a parent widget.
     *
     * This function will transform the position from parent coordinates to local coordinates.
     *
     * @param position The coordinate of the mouse local to the parent widget.
     */
    [[nodiscard]] virtual hitbox hitbox_test_from_parent(point2 position) const noexcept
    {
        return hitbox_test(_layout.from_parent * position);
    }

    /** Call hitbox_test from a parent widget.
     *
     * This function will transform the position from parent coordinates to local coordinates.
     *
     * @param position The coordinate of the mouse local to the parent widget.
     * @param sibling_hitbox The hitbox of a sibling to combine with the hitbox of this widget.
     */
    [[nodiscard]] virtual hitbox hitbox_test_from_parent(point2 position, hitbox sibling_hitbox) const noexcept
    {
        return std::max(sibling_hitbox, hitbox_test(_layout.from_parent * position));
    }

    /** Check if the widget will accept keyboard focus.
     *
     */
    [[nodiscard]] bool accepts_keyboard_focus(keyboard_focus_group group) const noexcept override
    {
        hi_axiom(loop::main().on_thread());
        return false;
    }

    [[nodiscard]] box_constraints update_constraints() noexcept override
    {
        _layout = {};
        return {*minimum, *minimum, *maximum};
    }

    void set_layout(widget_layout const& context) noexcept override
    {
        _layout = context;
    }

    widget_layout const& layout() const noexcept override
    {
        return _layout;
    }

    void draw(draw_context const& context) noexcept override {}

    /** Send a event to the window.
     */
    bool process_event(gui_event const& event) const noexcept override
    {
        if (parent != nullptr) {
            return parent->process_event(event);
        } else {
            return true;
        }
    }

    /** Request the widget to be redrawn on the next frame.
     */
    void request_redraw() const noexcept override
    {
        process_event({gui_event_type::window_redraw, layout().clipping_rectangle_on_window()});
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
            focus = true;
            this->scroll_to_show();
            ++global_counter<"widget:keyboard_enter:redraw">;
            request_redraw();
            return true;

        case gui_event_type::keyboard_exit:
            focus = false;
            ++global_counter<"widget:keyboard_exit:redraw">;
            request_redraw();
            return true;

        case gui_event_type::mouse_enter:
            hover = true;
            ++global_counter<"widget:mouse_enter:redraw">;
            request_redraw();
            return true;

        case gui_event_type::mouse_exit:
            hover = false;
            ++global_counter<"widget:mouse_exit:redraw">;
            request_redraw();
            return true;

        case gui_event_type::gui_activate_stay:
            process_event(gui_event_type::gui_activate);
            if (accepts_keyboard_focus(keyboard_focus_group::menu)) {
                // By going forward and backward we select the current parent,
                // the widget that opened the menu-stack.
                process_event(gui_event_type::gui_widget_next);
                process_event(gui_event_type::gui_widget_prev);
            }
            return true;

        case gui_event_type::gui_activate_next:
            process_event(gui_event_type::gui_activate);
            return process_event(gui_event_type::gui_widget_next);

        case gui_event_type::gui_widget_next:
            process_event(
                gui_event::window_set_keyboard_target(id, keyboard_focus_group::normal, keyboard_focus_direction::forward));
            return true;

        case gui_event_type::gui_widget_prev:
            process_event(
                gui_event::window_set_keyboard_target(id, keyboard_focus_group::normal, keyboard_focus_direction::backward));
            return true;

        case gui_event_type::gui_menu_next:
            if (*mode >= widget_mode::partial and accepts_keyboard_focus(keyboard_focus_group::menu)) {
                process_event(
                    gui_event::window_set_keyboard_target(id, keyboard_focus_group::menu, keyboard_focus_direction::forward));
                return true;
            }
            break;

        case gui_event_type::gui_menu_prev:
            if (*mode >= widget_mode::partial and accepts_keyboard_focus(keyboard_focus_group::menu)) {
                process_event(
                    gui_event::window_set_keyboard_target(id, keyboard_focus_group::menu, keyboard_focus_direction::backward));
                return true;
            }
            break;

        case gui_event_type::gui_toolbar_next:
            if (*mode >= widget_mode::partial and accepts_keyboard_focus(keyboard_focus_group::toolbar)) {
                process_event(
                    gui_event::window_set_keyboard_target(id, keyboard_focus_group::toolbar, keyboard_focus_direction::forward));
                return true;
            }
            break;

        case gui_event_type::gui_toolbar_prev:
            if (*mode >= widget_mode::partial and accepts_keyboard_focus(keyboard_focus_group::toolbar)) {
                process_event(
                    gui_event::window_set_keyboard_target(id, keyboard_focus_group::toolbar, keyboard_focus_direction::backward));
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

        for (auto& child : this->children(false)) {
            handled |= child.handle_event_recursive(event, reject_list);
        }

        if (!std::ranges::any_of(reject_list, [&](hilet& x) {
                return x == id;
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
            return id;

        } else if (current_keyboard_widget == id) {
            found = true;
        }

        auto children_ = std::vector<widget_intf const *>{};
        for (auto& child : children(false)) {
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

        if (parent) {
            parent->scroll_to_show(_layout.to_parent * rectangle);
        }
    }

    void set_window(gui_window *window) noexcept override
    {
        if (parent) {
            return parent->set_window(window);
        } else {
            return;
        }
    }

    [[nodiscard]] gui_window *window() const noexcept override
    {
        if (parent) {
            return parent->window();
        } else {
            return nullptr;
        }
    }

    [[nodiscard]] hi::theme const& theme() const noexcept
    {
        hilet w = window();
        hi_assert_not_null(w);
        return w->theme;
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
        if (*mode >= widget_mode::partial) {
            if (*hover) {
                return theme().color(semantic_color::fill, _layout.layer + 1);
            } else {
                return theme().color(semantic_color::fill, _layout.layer);
            }
        } else {
            return theme().color(semantic_color::fill, _layout.layer - 1);
        }
    }

    [[nodiscard]] virtual color foreground_color() const noexcept
    {
        if (*mode >= widget_mode::partial) {
            if (*hover) {
                return theme().color(semantic_color::border, _layout.layer + 1);
            } else {
                return theme().color(semantic_color::border, _layout.layer);
            }
        } else {
            return theme().color(semantic_color::border, _layout.layer - 1);
        }
    }

    [[nodiscard]] virtual color focus_color() const noexcept
    {
        if (*mode >= widget_mode::partial) {
            if (*focus) {
                return theme().color(semantic_color::accent);
            } else if (*hover) {
                return theme().color(semantic_color::border, _layout.layer + 1);
            } else {
                return theme().color(semantic_color::border, _layout.layer);
            }
        } else {
            return theme().color(semantic_color::border, _layout.layer - 1);
        }
    }

    [[nodiscard]] virtual color accent_color() const noexcept
    {
        if (*mode >= widget_mode::partial) {
            return theme().color(semantic_color::accent);
        } else {
            return theme().color(semantic_color::border, _layout.layer - 1);
        }
    }

    [[nodiscard]] virtual color label_color() const noexcept
    {
        if (*mode >= widget_mode::partial) {
            return theme().text_style(semantic_text_style::label)->color;
        } else {
            return theme().color(semantic_color::border, _layout.layer - 1);
        }
    }

protected:
    widget_layout _layout;

    callback<void(widget_mode)> _mode_cbt;
    callback<void(bool)> _focus_cbt;
    callback<void(bool)> _hover_cbt;

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
        hilet requested_window_rectangle = translate2{layout().clipping_rectangle_on_window()} * requested_rectangle;
        hilet window_bounds = aarectangle{layout().window_size} - theme().margin<float>();
        hilet response_window_rectangle = fit(window_bounds, requested_window_rectangle);
        return layout().from_window * response_window_rectangle;
    }
};

}} // namespace hi::v1
