// Copyright Take Vos 2021-2022.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

/** @file widgets/toolbar_tab_button_widget.hpp Defines toolbar_tab_button_widget.
 * @ingroup widgets
 */

#pragma once

#include "widget.hpp"
#include "radio_delegate.hpp"
#include "../macros.hpp"

hi_export_module(hikogui.widgets.toolbar_tab_button_widget);

hi_export namespace hi {
inline namespace v1 {

/** A graphical control element that allows the user to choose only one of a
 * predefined set of mutually exclusive views of a `tab_widget`.
 *
 * A toolbar tab button generally controls a `tab_widget`, to show one of its
 * child widgets.
 *
 * A toolbar tab button has two different states with different visual
 * representation:
 *  - **on**: The toolbar tab button shows raised among the other tabs.
 *  - **off**: The toolbar tab button is at equal height to other tabs.
 *
 * @image html toolbar_tab_button_widget.gif
 *
 * Each time a user activates the toolbar tab button it switches its state to
 * 'on'.
 *
 * A toolbar tab button cannot itself switch state to 'off', this state may be
 * caused by external factors. The canonical example is another toolbar tab
 * button in a set, which is configured with a different `on_value`.
 *
 * In the following example we create three toolbar tab button widgets on the
 * window which observes the same `value`. Each tab button is configured with a
 * different `on_value`: 0, 1 and 2.
 *
 * @snippet widgets/tab_example_impl.cpp Create three toolbar tab buttons
 *
 * @ingroup widgets
 * @note A toolbar tab button does not directly control a `tab_widget`. Like
 *       `radio_widget` this is accomplished by sharing a delegate or a
 *       observer between the toolbar tab button and the tab widget.
 */
class toolbar_tab_button_widget : public widget {
public:
    using super = widget;
    using delegate_type = radio_delegate;

    /** The label to show when the button is in the 'on' state.
     */
    observer<label> on_label = txt("on");

    /** The label to show when the button is in the 'off' state.
     */
    observer<label> off_label = txt("off");

    template<typename... Args>
    [[nodiscard]] static std::shared_ptr<delegate_type> make_default_delegate(Args&&... args) noexcept
    {
        return make_shared_ctad<default_radio_delegate>(std::forward<Args>(args)...);
    }

    ~toolbar_tab_button_widget()
    {
        _delegate->deinit(this);
    }

    /** Construct a toolbar tab button widget.
     *
     * @param parent The parent widget that owns this radio button widget.
     * @param delegate The delegate to use to manage the state of the tab button widget.
     */
    template<std::derived_from<delegate_type> Delegate>
    toolbar_tab_button_widget(std::shared_ptr<Delegate> delegate) noexcept :
        super(), _delegate(std::move(delegate))
    {
        _on_label_widget = std::make_unique<label_widget>(on_label);
        _on_label_widget->set_parent(this);

        _off_label_widget = std::make_unique<label_widget>(off_label);
        _off_label_widget->set_parent(this);

        hi_axiom_not_null(_delegate);
        _delegate->init(this);
        _delegate_cbt = _delegate->subscribe(this, [this] {
            set_checked(this->_delegate->state(this) != widget_value::off);
        });
        _delegate_cbt();

        style.set_name("toolbar-tab-button");
    }

    /** Construct a toolbar tab button widget with a default radio delegate.
     *
     * @param parent The parent widget that owns this toggle widget.
     * @param args The arguments to the `default_radio_delegate`
     */
    template<typename... Args>
    toolbar_tab_button_widget(Args&&... args) :
        toolbar_tab_button_widget(make_default_delegate(std::forward<Args>(args)...))
    {
    }

    /// @privatesection
    [[nodiscard]] box_constraints update_constraints() noexcept override
    {
        _on_label_constraints = _on_label_widget->update_constraints();
        _off_label_constraints = _off_label_widget->update_constraints();

        _label_constraints = max(_on_label_constraints, _off_label_constraints);
        auto const padding = max(_label_constraints.margins, style.padding_px);

        auto r = _label_constraints + padding;
        r.margins = {};
        r.baseline = embed(_label_constraints.baseline, unit::pixels(padding.bottom()), unit::pixels(padding.top()));
        return r;
    }

    void set_layout(widget_layout const& context) noexcept override
    {
        super::set_layout(context);

        auto const padding = max(_label_constraints.margins, style.padding_px);
        auto const label_rectangle = context.rectangle() + padding;

        _on_label_shape = _off_label_shape =
            box_shape{label_rectangle, lift(context.baseline(), unit::pixels(padding.bottom()), unit::pixels(padding.top()))};

        _on_label_widget->set_layout(context.transform(_on_label_shape));
        _off_label_widget->set_layout(context.transform(_off_label_shape));
    }

    void draw(draw_context const& context) const noexcept override
    {
        if (overlaps(context, layout())) {
            draw_toolbar_tab_button(context);
        }

        return super::draw(context);
    }

    [[nodiscard]] bool accepts_keyboard_focus(keyboard_focus_group group) const noexcept override
    {
        return enabled() and to_bool(group & hi::keyboard_focus_group::toolbar);
    }

    [[nodiscard]] generator<widget_intf&> children(bool include_invisible) const noexcept override
    {
        if (_delegate->state(this) != widget_value::off or include_invisible) {
            co_yield *_on_label_widget;
        }
        if (_delegate->state(this) == widget_value::off or include_invisible) {
            co_yield *_off_label_widget;
        }
    }

    [[nodiscard]] color background_color() const noexcept override
    {
        hi_axiom(loop::main().on_thread());
        if (phase() == widget_phase::active) {
            return theme().fill_color(layout().layer + 2);
        } else {
            return super::background_color();
        }
    }

    [[nodiscard]] hitbox hitbox_test(point2 position) const noexcept override
    {
        hi_axiom(loop::main().on_thread());

        if (enabled() and layout().contains(position)) {
            return {id(), layout().elevation, hitbox_type::button};
        } else {
            return {};
        }
    }

    void activate() noexcept
    {
        _delegate->activate(this);

        notifier();
    }

    bool handle_event(gui_event const& event) noexcept override
    {
        hi_axiom(loop::main().on_thread());

        switch (event.type()) {
        case gui_event_type::gui_activate:
            if (enabled()) {
                activate();
                return true;
            }
            break;

        case gui_event_type::mouse_down:
            if (enabled() and event.mouse().cause.left_button) {
                set_active(true);
                handle_event(gui_event_type::gui_activate);
                return true;
            }
            break;

        case gui_event_type::mouse_up:
            if (enabled() and event.mouse().cause.left_button) {
                set_active(false);
                return true;
            }
            break;

        default:;
        }

        return super::handle_event(event);
    }
    // @endprivatesection
private:
    std::unique_ptr<label_widget> _on_label_widget;
    box_constraints _on_label_constraints;
    box_shape _on_label_shape;

    std::unique_ptr<label_widget> _off_label_widget;
    box_constraints _off_label_constraints;
    box_shape _off_label_shape;

    /** The delegate that controls the button widget.
     */
    std::shared_ptr<delegate_type> _delegate;
    callback<void()> _delegate_cbt;

    box_constraints _label_constraints;

    void draw_toolbar_tab_button(draw_context const& context) const noexcept
    {
        // Draw the outline of the button across the clipping rectangle to clip the
        // bottom of the outline.
        auto const offset = theme().margin<float>() + theme().border_width();
        auto const outline_rectangle = aarectangle{0, -offset, layout().width(), layout().height() + offset};

        // The focus line will be drawn by the parent widget (toolbar_widget) at 0.5.
        auto const button_z = focus() ? translate_z(0.6f) : translate_z(0.0f);

        // clang-format off
        auto button_color = (phase() == widget_phase::hover or checked()) ?
            theme().fill_color(layout().layer - 1) :
            theme().fill_color(layout().layer);
        // clang-format on

        auto const corner_radii =
            hi::corner_radii(0.0f, 0.0f, theme().rounding_radius<float>(), theme().rounding_radius<float>());

        context.draw_box(
            layout(),
            button_z * outline_rectangle,
            button_color,
            focus() ? focus_color() : button_color,
            theme().border_width(),
            border_side::inside,
            corner_radii);
    }
};

} // namespace v1
} // namespace hi::v1
