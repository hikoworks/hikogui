// Copyright Take Vos 2021-2022.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

/** @file widgets/radio_widget.hpp Defines radio_widget.
 * @ingroup widgets
 */

#pragma once

#include "widget.hpp"
#include "utility.hpp"
#include "with_label_widget.hpp"
#include "menu_button_widget.hpp"
#include "radio_delegate.hpp"
#include "../telemetry/telemetry.hpp"
#include "../macros.hpp"

hi_export_module(hikogui.widgets.radio_widget);

hi_export namespace hi {
inline namespace v1 {

/** A radio widget is used in a set to select one of the options.
 *
 * A radio is a button with two different states with different visual
 * representation:
 *  - **on**: A pip is shown inside the circle.
 *  - **off**: An empty circle is shown.
 *
 * The user can activate the radio button by clicking on it, or using the
 * keyboard activate (space bar or enter) when the radio button is focused.
 * Activating the radio button will set it to the 'on' state. Using another
 * radio button in a set may turn the current radio button to the 'off' state.
 *
 * @image html radio_widget.gif
 *
 * Style:
 *
 *  - `width`: The width of the widget.
 *  - `height`: The height of the widget, and the diameter of the radio button.
 *  - `margin-left`: The margin to the left of the radio button.
 *  - `margin-bottom`: The margin below the radio button.
 *  - `margin-right`: The margin to the right of the radio button.
 *  - `margin-top`: The margin above the radio button.
 *  - `border-width`: The width of the border of the radio button.
 *  - `border-color`: The color of the border of the radio button.
 *  - `background-color`: The color of the background of the radio button.
 *  - `accent-color`: The color of the pip when the radio button is in the 'on'
 *                    state.
 *  - `horizontal-alignment`: The horizontal alignment of the radio button.
 *  - `vertical-alignment`: The vertical alignment of the radio button.
 *
 * The alignment is used to place the radio button inside the layout rectangle,
 * which may be larger than the style's width and height. Horizontally the radio
 * button is aligned to the left, center, or right of the layout rectangle.
 * Vertically the radio button's alignment is a little bit more complex:
 *
 *  - **top**:    The middle of the radio button is aligned to the middle of
 *                text when the text is aligned to top. The middle of the text
 *                is determined from the `font-size` and computed `cap-height`.
 *                This may mean that the radio button will be drawn into its
 *                margins.
 *  - **middle**: The middle of the radio button is aligned to the middle of the
 *                layout rectangle.
 *  - **bottom**: The middle of the radio button is aligned to the middle of
 *                text when the text is aligned to bottom. The middle of the
 *                text is determined from the `font-size` and computed
 *                `cap-height`. This may mean that the radio button will be
 *                drawn into its margins.
 *
 * Since a radio button is a circle it is drawn slightly larger than the
 * given diameter to make it look visual the same size as a square.
 *
 * @snippet widgets/radio_example_impl.cpp Create a radio button
 */
class radio_widget : public widget {
public:
    using super = widget;
    using delegate_type = radio_delegate;

    /** The delegate that controls the button widget.
     */
    std::shared_ptr<delegate_type> delegate;

    keyboard_focus_group focus_group = keyboard_focus_group::normal;

    template<typename... Args>
    [[nodiscard]] static std::shared_ptr<delegate_type> make_default_delegate(Args&&... args) noexcept
    {
        return make_shared_ctad<default_radio_delegate>(std::forward<Args>(args)...);
    }

    ~radio_widget()
    {
        this->delegate->deinit(this);
    }

    /** Construct a radio widget.
     *
     * @param parent The parent widget that owns this radio widget.
     * @param delegate The delegate to use to manage the state of the radio button.
     */
    template<std::derived_from<delegate_type> Delegate>
    radio_widget(std::shared_ptr<Delegate> delegate) noexcept : super(), delegate(std::move(delegate))
    {
        hi_axiom_not_null(this->delegate);

        this->delegate->init(this);
        _delegate_cbt = this->delegate->subscribe(this, [this] {
            set_checked(this->delegate->state(this) != widget_value::off);
            this->notifier();
        });
        _delegate_cbt();

        style.set_name("radio");
    }

    /** Construct a radio widget with a default button delegate.
     *
     * @param parent The parent widget that owns this toggle widget.
     * @param args The arguments to the `default_radio_delegate`
     *                followed by arguments to `attributes_type`
     */
    template<typename... Args>
    radio_widget(Args&&... args) : radio_widget(make_default_delegate(std::forward<Args>(args)...))
    {
    }

    /// @privatesection
    [[nodiscard]] layout::constraints get_constraints() const noexcept override
    {
        return style.make_constraints_for_fixed_size();
    }

    void set_layout(layout::shape const& shape) noexcept override
    {
        _shape = shape;

        auto const midline = shape.get_midline(style.x_height_px);

        auto const horizontal_alignment = os_settings::alignment(style.horizontal_alignment);
        _button_rectangle = align_to_middle(shape.rectangle(), style.size_px, horizontal_alignment, midline);

        auto const button_diameter = style.size_px.height();
        auto const button_radius = std::round(button_diameter * 0.5f);
        auto const button_size = extent2{button_diameter, button_diameter};

        _button_circle = circle{_button_rectangle};

        _pip_circle = align(_button_rectangle, circle{button_radius - style.border_width_px * 3.0f}, alignment::middle_center());
    }

    void draw(draw_context const& context) const noexcept override
    {
        if (overlaps(context, _shape.rectangle)) {
            if (focus_group != keyboard_focus_group::menu) {
                context.draw_circle(
                    _shape.translate_z() * (_button_circle * 1.02f),
                    _shape.clipping_rectangle,
                    style.background_color,
                    style.border_color,
                    style.border_width_px,
                    border_side::inside);
            }

            switch (_animated_value.update(delegate->state(this) != widget_value::off ? 1.0f : 0.0f, context.display_time_point)) {
            case animator_state::idle:
                break;
            case animator_state::running:
                request_redraw();
                break;
            case animator_state::end:
                notifier();
                break;
            default:
                hi_no_default();
            }

            // draw pip
            auto const float_value = _animated_value.current_value();
            if (float_value > 0.0) {
                context.draw_circle(
                    _shape.translate_z(0.1f) * (_pip_circle * (1.02f * float_value)),
                    _shape.clipping_rectangle,
                    style.accent_color);
            }
        }

        return super::draw(context);
    }

    [[nodiscard]] hitbox hitbox_test(point2 position) const noexcept override
    {
        hi_axiom(loop::main().on_thread());

        if (enabled() and _shape.contains(position)) {
            return {id(), _shape.elevation, hitbox_type::button};
        } else {
            return {};
        }
    }

    [[nodiscard]] bool accepts_keyboard_focus(keyboard_focus_group group) const noexcept override
    {
        hi_axiom(loop::main().on_thread());
        return enabled() and to_bool(group & this->focus_group);
    }

    bool handle_event(gui_event const& event) noexcept override
    {
        hi_axiom(loop::main().on_thread());

        switch (event.type()) {
        case gui_event_type::gui_activate:
            if (enabled()) {
                delegate->activate(this);
                request_redraw();
                return true;
            }
            break;

        default:;
        }

        return super::handle_event(event);
    }
    /// @endprivatesection

private:
    constexpr static std::chrono::nanoseconds _animation_duration = std::chrono::milliseconds(150);

    layout::shape _shape;

    aarectangle _button_rectangle;

    circle _button_circle;

    mutable animator<float> _animated_value = _animation_duration;
    circle _pip_circle;

    callback<void()> _delegate_cbt;
};

using radio_with_label_widget = with_label_widget<radio_widget>;
using radio_menu_button_widget = menu_button_widget<radio_widget>;

} // namespace v1
} // namespace hi::v1
