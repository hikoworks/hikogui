// Copyright Take Vos 2021-2022.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

#include "hikogui/hikogui.hpp"
#include "hikogui/crt.hpp"

// Every widget must inherit from hi::widget.
class command_widget : public hi::widget {
public:
    using super = hi::widget;

    // Using an observer allows reading, writing and monitoring of the value outside of the widget.
    hi::observer<bool> value;

    // Every constructor of a widget starts with a `window` and `parent` argument.
    // In most cases these are automatically filled in when calling a container widget's `emplace()` function.
    command_widget() noexcept : hi::widget()
    {
        // To visually show the change in value the widget needs to be redrawn.
        _value_cbt = value.subscribe([&](auto...) {
            request_redraw();
        });
    }

    // The set_constraints() function is called when the window is first initialized,
    // or when a widget wants to change its constraints.
    [[nodiscard]] hi::box_constraints update_constraints() noexcept override
    {
        // Set the minimum, preferred, maximum sizes and the margin around the widget.
        return {{100, 20}, {200, 20}, {300, 50}, style.margins_px};
    }

    // It is common to override the context sensitive colors of the default widget.
    // In this case the background color is 'green' when the value of the widget is true.
    [[nodiscard]] hi::color background_color() const noexcept override
    {
        return *value ? hi::color::green() : theme().fill_color();
    }

    // The `draw()` function is called when all or part of the window requires redrawing.
    // This may happen when showing the window for the first time, when the operating-system
    // requests a (partial) redraw, or when a widget requests a redraw of itself.
    void draw(hi::draw_context const& context) const noexcept override
    {
        // We only need to draw the widget when it is visible and when the visible area of
        // the widget overlaps with the scissor-rectangle (partial redraw) of the drawing context.
        if (mode() > hi::widget_mode::invisible and overlaps(context, layout())) {
            // When drawing this box we use the widget's background_color() and focus_color().
            // These colors are context sensitive; for example focus_color() checks if the widget is enabled,
            // has keyboard focus and the window is active.
            context.draw_box(
                layout(),
                layout().rectangle(),
                background_color(),
                focus_color(),
                theme().border_width(),
                hi::border_side::inside,
                theme().rounding_radius());
        }

        return super::draw(context);
    }

    // Override this function when your widget needs to be controllable by keyboard interaction.
    [[nodiscard]] bool accepts_keyboard_focus(hi::keyboard_focus_group group) const noexcept override
    {
        // This widget will react to "normal" tab/shift-tab keys and mouse clicks to focus the widget.
        return mode() >= hi::widget_mode::partial and to_bool(group & hi::keyboard_focus_group::normal);
    }

    // Override this function when your widget needs to be controllable by mouse interaction.
    [[nodiscard]] hi::hitbox hitbox_test(hi::point2 position) const noexcept override
    {
        // Check if the (mouse) position is within the visual-area of the widget.
        // The hit_rectangle is the layout().rectangle() intersected with the layout().clipping_rectangle.
        if (mode() >= hi::widget_mode::partial and layout().contains(position)) {
            // The `this` argument allows the gui_window to forward mouse events to handle_event(mouse) of this widget.
            // The `position` argument is used to handle widgets that are visually overlapping, widgets with higher elevation
            // get priority. When this widget is enabled it should show a button-cursor, otherwise just the normal arrow.
            return {
                id, layout().elevation, mode() >= hi::widget_mode::partial ? hi::hitbox_type::button : hi::hitbox_type::_default};

        } else {
            return {};
        }
    }

    // Override the handle_event(command) to handle high level commands.
    bool handle_event(hi::gui_event const& event) noexcept override
    {
        switch (event.type()) {
        case hi::gui_event_type::gui_activate:
            if (mode() >= hi::widget_mode::partial) {
                // Handle activate, by default the "spacebar" causes this command.
                value = not *value;
                return true;
            }
            break;

        case hi::gui_event_type::keyboard_grapheme:
            hi_log_error("User typed the letter U+{:x}.", static_cast<uint32_t>(event.grapheme().starter()));
            return true;

        case hi::gui_event_type::mouse_up:
            if (mode() >= hi::widget_mode::partial and event.is_left_button_up(layout().rectangle())) {
                return handle_event(hi::gui_event_type::gui_activate);
            }
            break;

        default:;
        }

        // The default handle_event() will handle hovering and auto-scrolling.
        return widget::handle_event(event);
    }

private:
    hi::callback<void(bool)> _value_cbt;
};

int hi_main(int argc, char *argv[])
{
    hi::set_application_name("Custom widget command example");
    hi::set_application_vendor("HikoGUI");
    hi::set_application_version({1, 0, 0});

    auto widget = std::make_unique<hi::window_widget>(hi::txt("Custom Widget Command"));
    widget->content().emplace<command_widget>("A1");
    widget->content().emplace<command_widget>("A2");

    auto window = std::make_unique<hi::gui_window>(std::move(widget));

    auto close_cbt = window->closing.subscribe(
        [&] {
            window = {};
        },
        hi::callback_flags::main);
    return hi::loop::main().resume();
}
