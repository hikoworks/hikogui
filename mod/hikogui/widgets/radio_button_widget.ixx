// Copyright Take Vos 2021-2022.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

/** @file widgets/radio_button_widget.hpp Defines radio_button_widget.
 * @ingroup widgets
 */

module;
#include "../macros.hpp"


export module hikogui_widgets_radio_button_widget;
import hikogui_telemetry;
import hikogui_widgets_button_delegate;
import hikogui_widgets_menu_button_widget;
import hikogui_widgets_widget;
import hikogui_widgets_with_label_widget;

export namespace hi { inline namespace v1 {

template<typename Context>
concept radio_button_widget_attribute =
    forward_of<Context, observer<hi::alignment>> or forward_of<Context, keyboard_focus_group>;

/** A GUI widget that permits the user to make a binary choice.
 * @ingroup widgets
 *
 * A radio_button is a button with three different states with different visual
 * representation:
 *  - **on**: A check-mark is shown inside the box, and the `radio_button_widget::on_label` is shown.
 *  - **off**: An empty box is shown, and the `radio_button_widget::off_label` is shown.
 *  - **other**: A dash is shown inside the box, and the `radio_button_widget::other_label` is shown.
 *
 * @image html radio_button_widget.gif
 *
 * Each time a user activates the radio_button-button it toggles between the 'on' and 'off' states.
 * If the radio_button is in the 'other' state an activation will switch it to
 * the 'off' state.
 *
 * A radio_button cannot itself switch state to 'other', this state may be
 * caused by external factors. The canonical example is a tree structure
 * of radio_buttons; when child radio_buttons have different values from each other
 * the parent radio_button state is set to 'other'.
 *
 * In the following example we create a radio_button widget on the window
 * which observes `value`. When the value is 1 the radio_button is 'on',
 * when the value is 2 the radio_button is 'off'.
 *
 * @snippet widgets/radio_button_example_impl.cpp Create a radio_button
 */
class radio_button_widget final : public widget {
public:
    using super = widget;
    using delegate_type = button_delegate;

    struct attributes_type {
        observer<alignment> alignment = alignment::top_left();
        keyboard_focus_group focus_group = keyboard_focus_group::normal;

        attributes_type(attributes_type const &) noexcept = default;
        attributes_type(attributes_type &&) noexcept = default;
        attributes_type &operator=(attributes_type const &) noexcept = default;
        attributes_type &operator=(attributes_type &&) noexcept = default;

        template<radio_button_widget_attribute... Attributes>
        explicit attributes_type(Attributes &&...attributes) noexcept
        {
            set_attributes(std::forward<Attributes>(attributes)...);
        }

        void set_attributes() noexcept
        {
        }

        template<radio_button_widget_attribute First, radio_button_widget_attribute... Rest>
        void set_attributes(First&& first, Rest&&...rest) noexcept
        {
            if constexpr (forward_of<First, observer<hi::alignment>>) {
                alignment = std::forward<First>(first);

            } else if constexpr (forward_of<First, keyboard_focus_group>) {
                focus_group = std::forward<First>(first);

            } else {
                hi_static_no_default();
            }

            set_attributes(std::forward<Rest>(rest)...);
        }
    };

    attributes_type attributes;

    /** The delegate that controls the button widget.
     */
    not_null<std::shared_ptr<delegate_type>> delegate;

    template<typename... Args>
    [[nodiscard]] static not_null<std::shared_ptr<button_delegate>> make_default_delegate(Args &&...args)
        requires requires { make_shared_ctad_not_null<default_radio_button_delegate>(std::forward<Args>(args)...); }
    {
        return make_shared_ctad_not_null<default_radio_button_delegate>(std::forward<Args>(args)...);
    }

    ~radio_button_widget()
    {
        this->delegate->deinit(*this);
    }

    /** Construct a radio_button widget.
     *
     * @param parent The parent widget that owns this radio_button widget.
     * @param delegate The delegate to use to manage the state of the radio_button button.
     */
    radio_button_widget(
        not_null<widget *> parent,
        attributes_type attributes,
        not_null<std::shared_ptr<delegate_type>> delegate) noexcept :
        super(parent), attributes(std::move(attributes)), delegate(std::move(delegate))
    {
        _delegate_cbt = this->delegate->subscribe([&]{
            this->request_redraw();
        });

        this->delegate->init(*this);
    }

    /** Construct a radio_button widget with a default button delegate.
     *
     * @see default_button_delegate
     * @param parent The parent widget that owns this radio_button widget.
     * @param value The value or `observer` value which represents the state of the radio_button.
     * @param on_value The on-value. This value is used to determine which value yields an 'on' state.
     */
    template<
        incompatible_with<attributes_type> Value,
        forward_of<observer<observer_decay_t<Value>>> OnValue,
        radio_button_widget_attribute... Attributes>
    radio_button_widget(
        not_null<widget *> parent,
        Value&& value,
        OnValue&& on_value,
        Attributes &&...attributes) noexcept
        requires requires
    {
        make_default_delegate(std::forward<Value>(value), std::forward<OnValue>(on_value));
        attributes_type{std::forward<Attributes>(attributes)...};
    } :
        radio_button_widget(
            parent,
            attributes_type{std::forward<Attributes>(attributes)...},
            make_default_delegate(std::forward<Value>(value), std::forward<OnValue>(on_value)))
    {
    }

    /** Get the current state of the button.
     * @return The state of the button: on / off / other.
     */
    [[nodiscard]] button_state state() const noexcept
    {
        hi_axiom(loop::main().on_thread());
        return delegate->state(*this);
    }

    /// @privatesection
    [[nodiscard]] box_constraints update_constraints() noexcept override
    {
        _button_size = {theme().size(), theme().size()};
        return box_constraints{_button_size, _button_size, _button_size, *attributes.alignment, theme().margin(), {}};
    }

    void set_layout(widget_layout const& context) noexcept override
    {
        if (compare_store(_layout, context)) {
            _button_rectangle = align(context.rectangle(), _button_size, os_settings::alignment(*attributes.alignment));

            _button_circle = circle{_button_rectangle};

            _pip_circle = align(_button_rectangle, circle{theme().size() * 0.5f - 3.0f}, alignment::middle_center());
        }
        super::set_layout(context);
    }

    void draw(draw_context const& context) noexcept override
    {
        if (*mode > widget_mode::invisible and overlaps(context, layout())) {
            if (attributes.focus_group != keyboard_focus_group::menu) {
                context.draw_circle(
                    layout(), _button_circle * 1.02f, background_color(), focus_color(), theme().border_width(), border_side::inside);
            }

            switch (_animated_value.update(state() == button_state::on ? 1.0f : 0.0f, context.display_time_point)) {
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
            auto float_value = _animated_value.current_value();
            if (float_value > 0.0) {
                context.draw_circle(layout(), _pip_circle * 1.02f * float_value, accent_color());
            }
        }
    }

    [[nodiscard]] color background_color() const noexcept override
    {
        hi_axiom(loop::main().on_thread());
        if (_pressed) {
            return theme().color(semantic_color::fill, _layout.layer + 2);
        } else {
            return super::background_color();
        }
    }

    [[nodiscard]] hitbox hitbox_test(point2 position) const noexcept final
    {
        hi_axiom(loop::main().on_thread());

        if (*mode >= widget_mode::partial and layout().contains(position)) {
            return {id, _layout.elevation, hitbox_type::button};
        } else {
            return {};
        }
    }

    [[nodiscard]] bool accepts_keyboard_focus(keyboard_focus_group group) const noexcept override
    {
        hi_axiom(loop::main().on_thread());
        return *mode >= widget_mode::partial and to_bool(group & attributes.focus_group);
    }

    bool handle_event(gui_event const& event) noexcept override
    {
        hi_axiom(loop::main().on_thread());

        switch (event.type()) {
        case gui_event_type::gui_activate:
            if (*mode >= widget_mode::partial) {
                delegate->activate(*this);
                request_redraw();
                return true;
            }
            break;

        case gui_event_type::mouse_down:
            if (*mode >= widget_mode::partial and event.mouse().cause.left_button) {
                _pressed = true;
                request_redraw();
                return true;
            }
            break;

        case gui_event_type::mouse_up:
            if (*mode >= widget_mode::partial and event.mouse().cause.left_button) {
                _pressed = false;

                // with_label_widget or other widgets may have accepted the hitbox
                // for this widget. Which means the widget_id in the mouse-event
                // may match up with the radio_button.
                if (event.mouse().hitbox.widget_id == id) {
                    // By staying we can give focus to the parent widget.
                    handle_event(gui_event_type::gui_activate_stay);
                }
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

    extent2 _button_size;
    aarectangle _button_rectangle;

    circle _button_circle;

    animator<float> _animated_value = _animation_duration;
    circle _pip_circle;

    bool _pressed = false;

    callback<void()> _delegate_cbt;
};

using radio_button_with_label_widget = with_label_widget<radio_button_widget>;
using radio_menu_button_widget = menu_button_widget<radio_button_widget>;

}} // namespace hi::v1
