// Copyright Take Vos 2021-2022.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

/** @file widgets/toggle_widget.hpp Defines toggle_widget.
 * @ingroup widgets
 */

#pragma once

#include "widget.hpp"
#include "with_label_widget.hpp"
#include "toggle_delegate.hpp"
#include "../telemetry/telemetry.hpp"
#include "../macros.hpp"
#include <utility>

hi_export_module(hikogui.widgets.toggle_widget);

hi_export namespace hi {
inline namespace v1 {

/** A GUI widget that permits the user to make a binary choice.
 *
 * A toggle is very similar to a `toggle_widget`. The
 * semantic difference between a toggle and a toggle is:
 *  - A toggle is immediately active, turning on and off a feature or service at
 *    the moment you toggle it.
 *  - A toggle determines what happens when another action takes place. Or
 *    only becomes active after pressing the "Apply" or "Save" button on a form.
 *    Or becomes part of a record together with other information to be stored
 *    together in a database of some sort.
 *
 * A toggle is a button with three different states with different visual
 * representation:
 *  - **on**: The switch is thrown to the right and is highlighted, and the
 *    `toggle_widget::on_label` is shown.
 *  - **off**: The switch is thrown to the left and is not highlighted, and the
 *    `toggle_widget::off_label` is shown.
 *  - **other**: The switch is thrown to the left and is not highlighted, and
 *    the `toggle_widget::other_label` is shown.
 *
 * @image html toggle_widget.gif
 *
 * Each time a user activates the toggle-button it toggles between the 'on' and
 * 'off' states. If the toggle is in the 'other' state an activation will switch
 * it to the 'off' state.
 *
 * A toggle cannot itself switch state to 'other', this state may be caused by
 * external factors.
 *
 * In the following example we create a toggle widget on the window which
 * observes `value`. When the value is 1 the toggle is 'on', when the value is 2
 * the toggle is 'off'.
 *
 * @snippet widgets/toggle_example_impl.cpp Create a toggle
 *
 * @ingroup widgets
 */
class toggle_widget : public widget {
public:
    using super = widget;
    using delegate_type = toggle_delegate;

    /** The delegate that controls the button widget.
     */
    std::shared_ptr<delegate_type> delegate;

    keyboard_focus_group focus_group = keyboard_focus_group::normal;

    template<typename... Args>
    [[nodiscard]] static std::shared_ptr<delegate_type> make_default_delegate(Args&&... args)
    {
        return make_shared_ctad<default_toggle_delegate>(std::forward<Args>(args)...);
    }

    ~toggle_widget()
    {
        this->delegate->deinit(this);
    }

    /** Construct a toggle widget.
     *
     * @param parent The parent widget that owns this toggle widget.
     * @param delegate The delegate to use to manage the state of the toggle button.
     */
    template<std::derived_from<delegate_type> Delegate>
    toggle_widget(std::shared_ptr<Delegate> delegate) noexcept : super(), delegate(std::move(delegate))
    {
        hi_axiom_not_null(this->delegate);

        this->delegate->init(this);
        _delegate_cbt = this->delegate->subscribe(this, [this] {
            set_checked(this->delegate->state(this) != widget_value::off);
            this->notifier();
        });
        _delegate_cbt();

        style.set_name("toggle");
    }

    /** Construct a toggle widget with a default button delegate.
     *
     * @param parent The parent widget that owns this toggle widget.
     * @param args The arguments to the `default_toggle_delegate`
     *                followed by arguments to `attributes_type`
     */
    template<typename... Args>
    toggle_widget(Args&&... args) : toggle_widget(make_default_delegate(std::forward<Args>(args)...))
    {
    }

    /// @privatesection
    void set_layout(widget_layout const& context) noexcept override
    {
        super::set_layout(context);

        auto const middle = context.get_middle(style.cap_height);
        auto const extended_rectangle = context.rectangle() + style.vertical_margins_px;
        _button_rectangle = align_to_middle(
            extended_rectangle, style.size_px, os_settings::alignment(style.horizontal_alignment), middle.in(unit::pixels));

        auto const pip_square = aarectangle{get<0>(_button_rectangle), extent2{style.height_px, style.height_px}};
        _pip_circle = align(pip_square, circle{style.height_px * 0.5f - 3.0f}, alignment::middle_center());

        auto const pip_to_button_margin_x2 = style.height_px - _pip_circle.diameter();
        _pip_move_range = style.width_px - _pip_circle.diameter() - pip_to_button_margin_x2;
    }

    void draw(draw_context const& context) const noexcept override
    {
        if (overlaps(context, layout())) {
            context.draw_box(
                layout(),
                _button_rectangle,
                style.background_color,
                style.border_color,
                style.border_width_px,
                border_side::inside,
                corner_radii{style.height_px * 0.5f});

            switch (
                _animated_value.update(delegate->state(this) != widget_value::off ? 1.0f : 0.0f, context.display_time_point)) {
            case animator_state::uninitialized:
                std::unreachable();
            case animator_state::idle:
                break;
            case animator_state::running:
                request_redraw();
                break;
            case animator_state::end:
                notifier();
            }

            auto const pip_offset = translate3{_pip_move_range * _animated_value.current_value(), 0.0f, 0.1f};
            auto const positioned_pip_circle = pip_offset * _pip_circle;
            context.draw_circle(layout(), positioned_pip_circle * 1.02f, style.accent_color);
        }

        return super::draw(context);
    }

    [[nodiscard]] hitbox hitbox_test(point2 position) const noexcept override
    {
        hi_axiom(loop::main().on_thread());

        if (enabled() and _button_rectangle.contains(position)) {
            return {id(), layout().elevation, hitbox_type::button};
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
                ++global_counter<"toggle_widget:handle_event:relayout">;
                request_relayout();
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

    aarectangle _button_rectangle;
    mutable animator<float> _animated_value = _animation_duration;
    circle _pip_circle;
    float _pip_move_range;

    callback<void()> _delegate_cbt;
};

using toggle_with_label_widget = with_label_widget<toggle_widget>;

} // namespace v1
} // namespace hi::v1
