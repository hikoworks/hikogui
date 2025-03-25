// Copyright Take Vos 2021-2022.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

/** @file widgets/abstract_button_widget.hpp Defines abstract_button_widget.
 * @ingroup widgets
 */

#pragma once

#include "widget.hpp"
#include "button_delegate.hpp"
#include "label_widget.hpp"
#include "../algorithm/algorithm.hpp"
#include "../l10n/l10n.hpp"
#include "../observer/observer.hpp"
#include "../macros.hpp"
#include <memory>
#include <string>
#include <array>
#include <optional>
#include <future>
#include <coroutine>

hi_export_module(hikogui.widgets.abstract_button_widget);

hi_export namespace hi { inline namespace v1 {

/** Base class for implementing button widgets.
 *
 * @ingroup widgets
 */
class abstract_button_widget : public widget {
public:
    using super = widget;
    using delegate_type = button_delegate;

    /** The delegate that controls the button widget.
     */
    std::shared_ptr<delegate_type> delegate;

    /** The label to show when the button is in the 'on' state.
     */
    observer<label> on_label = txt("on");

    /** The label to show when the button is in the 'off' state.
     */
    observer<label> off_label = txt("off");

    /** The label to show when the button is in the 'other' state.
     */
    observer<label> other_label = txt("other");

    /** The alignment of the button and on/off/other label.
     */
    observer<alignment> alignment;

    ~abstract_button_widget()
    {
        hi_assert_not_null(delegate);
        delegate->deinit(this);
    }

    abstract_button_widget(std::shared_ptr<delegate_type> delegate) noexcept :
        super(), delegate(std::move(delegate))
    {
        hi_assert_not_null(this->delegate);

        _on_label_widget = std::make_unique<label_widget>(on_label);
        _on_label_widget->set_parent(this);
        _off_label_widget = std::make_unique<label_widget>(off_label);
        _off_label_widget->set_parent(this);
        _other_label_widget = std::make_unique<label_widget>(other_label);
        _other_label_widget->set_parent(this);

        this->delegate->init(this);
        _delegate_cbt = this->delegate->subscribe(this, [this] {
            set_checked(this->delegate->state(this) != widget_value::off);
        });
        _delegate_cbt();
    }

    /// @privatesection
    [[nodiscard]] box_constraints update_constraints() noexcept override
    {
        _on_label_constraints = _on_label_widget->update_constraints();
        _off_label_constraints = _off_label_widget->update_constraints();
        _other_label_constraints = _other_label_widget->update_constraints();
        return max(_on_label_constraints, _off_label_constraints, _other_label_constraints);
    }

    void set_layout(widget_layout const& context) noexcept override
    {
        super::set_layout(context);
        _on_label_widget->set_layout(context.transform(_on_label_shape));
        _off_label_widget->set_layout(context.transform(_off_label_shape));
        _other_label_widget->set_layout(context.transform(_other_label_shape));
    }

    [[nodiscard]] generator<widget_intf&> children(bool include_invisible) const noexcept override
    {
        co_yield *_on_label_widget;
        co_yield *_off_label_widget;
        co_yield *_other_label_widget;
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

    [[nodiscard]] bool accepts_keyboard_focus(keyboard_focus_group group) const noexcept override
    {
        hi_axiom(loop::main().on_thread());
        return enabled() and to_bool(group & hi::keyboard_focus_group::normal);
    }

    void activate() noexcept
    {
        hi_assert_not_null(delegate);
        delegate->activate(this);

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
                return true;
            }
            break;

        case gui_event_type::mouse_up:
            if (enabled() and event.mouse().cause.left_button) {
                set_active(false);

                if (layout().rectangle().contains(event.mouse().position)) {
                    handle_event(gui_event_type::gui_activate);
                }
                return true;
            }
            break;

        default:;
        }

        return super::handle_event(event);
    }
    /// @endprivatesection
protected:
    std::unique_ptr<label_widget> _on_label_widget;
    box_constraints _on_label_constraints;
    box_shape _on_label_shape;

    std::unique_ptr<label_widget> _off_label_widget;
    box_constraints _off_label_constraints;
    box_shape _off_label_shape;

    std::unique_ptr<label_widget> _other_label_widget;
    box_constraints _other_label_constraints;
    box_shape _other_label_shape;

    callback<void()> _delegate_cbt;

    void draw_button(draw_context const& context) const noexcept
    {
        if (delegate->state(this) == widget_value::on) {
            _on_label_widget->draw(context);
        } else if (delegate->state(this) == widget_value::off) {
            _off_label_widget->draw(context);
        } else {
            _other_label_widget->draw(context);
        }
    }
};

}} // namespace hi::v1
