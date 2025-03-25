// Copyright Take Vos 2023.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

/** @file widgets/menu_button_widget.hpp Defines menu_button_widget.
 * @ingroup widgets
 */

#pragma once

#include "widget.hpp"
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

hi_export_module(hikogui.widgets.menu_button_widget);

hi_export namespace hi {
inline namespace v1 {

/** Add menu-button around a small-button.
 *
 * @ingroup widgets
 */
template<std::derived_from<widget> ButtonWidget>
class menu_button_widget : public widget {
public:
    using super = widget;
    using button_widget_type = ButtonWidget;
    using delegate_type = button_widget_type::delegate_type;

    /** The label to show when the button is in the 'on' state.
     */
    observer<hi::label> label = txt("on");

    /** The label to for the shortcut.
     */
    observer<hi::label> shortcut = hi::label{};

    template<typename... Args>
    [[nodiscard]] static std::shared_ptr<delegate_type> make_default_delegate(Args&&... args)
    {
        return button_widget_type::make_default_delegate(std::forward<Args>(args)...);
    }

    template<std::derived_from<delegate_type> Delegate>
    menu_button_widget(std::shared_ptr<Delegate> delegate) noexcept : super()
    {
        _button_widget = std::make_unique<button_widget_type>(std::move(delegate));
        _button_widget->focus_group = keyboard_focus_group::menu;
        _button_widget->set_parent(this);

        _label_widget = std::make_unique<label_widget>(label);
        _label_widget->set_parent(this);

        _shortcut_widget = std::make_unique<label_widget>(shortcut);
        _shortcut_widget->set_parent(this);

        _button_widget_cbt = _button_widget->subscribe([this] {
            this->set_checked(_button_widget->checked());
            this->notifier();
        });

        _button_widget_cbt();

        style.set_name("menu-button");
    }

    /** Construct a widget with a label.
     *
     * @param parent The parent widget that owns this toggle widget.
     * @param args The arguments to the default button delegate of the embedded
     *             widget followed by arguments to `attributes_type`
     */
    template<typename... Args>
    menu_button_widget(Args&&... args) : menu_button_widget(make_default_delegate(std::forward<Args>(args)...))
    {
    }

    /// @privatesection
    [[nodiscard]] box_constraints update_constraints() noexcept override
    {
        _grid.clear();
        _grid.add_cell(0, 0, grid_cell_type::button);
        _grid.add_cell(1, 0, grid_cell_type::label, true);
        _grid.add_cell(2, 0, grid_cell_type::shortcut);

        for (auto& cell : _grid) {
            if (cell.value == grid_cell_type::button) {
                auto constraints = _button_widget->update_constraints();
                inplace_max(constraints.minimum.width(), theme().size() * 2.0f);
                inplace_max(constraints.preferred.width(), theme().size() * 2.0f);
                inplace_max(constraints.maximum.width(), theme().size() * 2.0f);
                cell.set_constraints(constraints);

            } else if (cell.value == grid_cell_type::label) {
                cell.set_constraints(_label_widget->update_constraints());

            } else if (cell.value == grid_cell_type::shortcut) {
                auto constraints = _shortcut_widget->update_constraints();
                inplace_max(constraints.minimum.width(), theme().size() * 3.0f);
                inplace_max(constraints.preferred.width(), theme().size() * 3.0f);
                inplace_max(constraints.maximum.width(), theme().size() * 3.0f);
                cell.set_constraints(constraints);

            } else {
                hi_no_default();
            }
        }

        auto constraints = _grid.constraints(os_settings::left_to_right(), style.vertical_alignment);
        constraints.minimum += style.padding_px.size();
        constraints.preferred += style.padding_px.size();
        constraints.maximum += style.padding_px.size();
        constraints.margins = {};
        return constraints;
    }

    void set_layout(widget_layout const& context) noexcept override
    {
        super::set_layout(context);

        auto shape = context.shape;
        shape.rectangle -= style.padding_px;
        _grid.set_layout(shape);

        for (auto const& cell : _grid) {
            if (cell.value == grid_cell_type::button) {
                _button_widget->set_layout(context.transform(cell.shape, transform_command::level));

            } else if (cell.value == grid_cell_type::label) {
                _label_widget->set_layout(context.transform(cell.shape));

            } else if (cell.value == grid_cell_type::shortcut) {
                _shortcut_widget->set_layout(context.transform(cell.shape));

            } else {
                hi_no_default();
            }
        }
    }

    void draw(draw_context const& context) const noexcept override
    {
        if (overlaps(context, layout())) {
            context.draw_box(
                layout(),
                layout().rectangle(),
                style.background_color,
                style.border_color,
                style.border_width_px,
                border_side::inside);
        }
        
        return super::draw(context);
    }

    [[nodiscard]] generator<widget_intf&> children(bool include_invisible) const noexcept override
    {
        co_yield *_button_widget;
        co_yield *_label_widget;
        co_yield *_shortcut_widget;
    }

    [[nodiscard]] hitbox hitbox_test(point2 position) const noexcept override
    {
        hi_axiom(loop::main().on_thread());

        if (enabled() and layout().contains(position)) {
            // Accept the hitbox of the menu_button_widget on behalf of the button_widget.
            return {_button_widget->id(), layout().elevation, hitbox_type::button};
        } else {
            return {};
        }
    }
    /// @endprivatesection
protected:
    enum class grid_cell_type { button, label, shortcut };

    grid_layout<grid_cell_type> _grid;

    std::unique_ptr<button_widget_type> _button_widget;

    std::unique_ptr<label_widget> _label_widget;
    std::unique_ptr<label_widget> _shortcut_widget;

    callback<void()> _button_widget_cbt;
};

} // namespace v1
} // namespace hi::v1
