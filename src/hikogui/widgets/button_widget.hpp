// Copyright Take Vos 2021-2022.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

/** @file widgets/button_widget.hpp Defines button_widget.
 * @ingroup widgets
 */

#pragma once

#include "widget.hpp"
#include "button_delegate.hpp"
#include "../macros.hpp"

hi_export_module(hikogui.widgets.button_widget);

hi_export namespace hi {
inline namespace v1 {

/** A button widget.
 * @ingroup widgets
 */
class button_widget : public widget {
public:
    using super = widget;
    using delegate_type = button_delegate;

    /** The delegate that controls the button widget.
     */
    std::shared_ptr<delegate_type> delegate;

    keyboard_focus_group focus_group = keyboard_focus_group::normal;

    /** The label shown inside the button.
     */
    observer<label> label = txt("my button");

    template<typename... Args>
    [[nodiscard]] static std::shared_ptr<delegate_type> make_default_delegate(Args&&... args)
    {
        return make_shared_ctad<default_button_delegate>(std::forward<Args>(args)...);
    }

    ~button_widget()
    {
        this->delegate->deinit(this);
    }

    button_widget(std::shared_ptr<delegate_type> delegate) noexcept : super(), delegate(std::move(delegate))
    {
        assert(this->delegate != nullptr);

        _label_widget = std::make_unique<label_widget>(label);
        _label_widget->set_parent(this);

        this->delegate->init(this);
        _delegate_cbt = this->delegate->subscribe(this, [this] {
            set_checked(this->delegate->state(this) != widget_value::off);
            this->notifier();
        });
        _delegate_cbt();

        style.set_name("button");
    }

    /** Construct a checkbox widget with a default button delegate.
     *
     * @param args The arguments to the `default_button_delegate`
     */
    template<typename... Args>
    button_widget(Args&&... args) : button_widget(make_default_delegate(std::forward<Args>(args)...))
    {
    }

    /// @privatesection
    [[nodiscard]] layout::constraints get_constraints() const noexcept override
    {
        _shaper.set_margin(style.margins_px);
        _shaper.set_padding(style.padding_px);
        _shaper.set_base_line_priority(style.base_line_priority);

        return _shaper.get_constraints([&] {
            return _label_widget->update_constraints();
        });
    }

    void set_layout(layout::shape const& shape) noexcept override
    {
        _shaper.set_layout(shape, [&](layout::shape const& shape) {
            _label_widget->set_layout(shape);
        });
    }

    void draw(draw_context const& context) const noexcept override
    {
        if (overlaps(context, _shaper.rectangle())) {
            context.draw_box(
                _shaper.rectangle(),
                _shaper.clip_rectangle(),
                style.background_color,
                style.border_color,
                style.border_width_px,
                border_side::inside,
                style.border_radius_px);
        }

        return super::draw(context);
    }

    [[nodiscard]] generator<widget_intf&> children(bool include_invisible) const noexcept override
    {
        co_yield *_label_widget;
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

    [[nodiscard]] hitbox hitbox_test(point2 position) const noexcept override
    {
        assert(loop::main().on_thread());

        if (enabled() and _shaper.rectangle().contains(position)) {
            return {id(), _shaper.elevation(), hitbox_type::button};
        } else {
            return {};
        }
    }

    [[nodiscard]] bool accepts_keyboard_focus(keyboard_focus_group group) const noexcept override
    {
        assert(loop::main().on_thread());

        return enabled() and to_bool(group & focus_group);
    }
    /// @endprivatesection
private:
    mutable layout::embed _shaper;
    box_constraints _label_constraints;
    std::unique_ptr<label_widget> _label_widget;

    callback<void()> _delegate_cbt;
};

} // namespace v1
} // namespace hi::v1
