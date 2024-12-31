// Copyright Take Vos 2021-2022.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

/** @file widgets/checkbox_widget.hpp Defines checkbox_widget.
 * @ingroup widgets
 */

#pragma once

#include "widget.hpp"
#include "with_label_widget.hpp"
#include "menu_button_widget.hpp"
#include "toggle_delegate.hpp"
#include "../telemetry/telemetry.hpp"
#include "../macros.hpp"

hi_export_module(hikogui.widgets.checkbox_widget);

hi_export namespace hi {
inline namespace v1 {

/** A checkbox widget is used to select a true/false option.
 *
 * A checkbox is a button with three different states with different visual
 * representation:
 *  - **on**: A checkmark is shown inside the square.
 *  - **off**: An empty square is shown.
 *  - **other**: A minus sign is shown inside the square.
 *
 * The user can activate the checkbox button by clicking on it, or using the
 * keyboard activate (space bar or enter) when the checkbox button is focused.
 * Activating the checkbox button will toggle it between the 'on' and 'off'
 * states, or to the 'off' state when the checkbox was in the 'other' state.
 *
 * @image html checkbox_widget.gif
 *
 * Style:
 *
 *  - `width`: The width of the widget.
 *  - `height`: The height of the widget.
 *  - `margin-left`: The margin to the left of the checkbox.
 *  - `margin-bottom`: The margin below the checkbox.
 *  - `margin-right`: The margin to the right of the checkbox.
 *  - `margin-top`: The margin above the checkbox.
 *  - `border-width`: The width of the border of the checkbox.
 *  - `border-color`: The color of the border of the checkbox.
 *  - `border-bottom-left-radius`: The radius of the bottom left corner of the
 *                                 checkbox.
 *  - `border-bottom-right-radius`: The radius of the bottom right corner of the
 *                                  checkbox.
 *  - `border-top-left-radius`: The radius of the top left corner of the
 *                              checkbox.
 *  - `border-top-right-radius`: The radius of the top right corner of the
 *                               checkbox.
 *  - `background-color`: The color of the background of the checkbox.
 *  - `accent-color`: The color of the checkmark or minus sign when the checkbox
 *                    is in the 'on' or 'other' state.
 *  - `horizontal-alignment`: The horizontal alignment of the checkbox.
 *  - `vertical-alignment`: The vertical alignment of the checkbox.
 *
 * The alignment is used to place the checkbox inside the layout rectangle,
 * which may be larger than the style's width and height. Horizontally the
 * checkbox is aligned to the left, center, or right of the layout rectangle.
 * Vertically the checkbox's alignment is a little bit more complex:
 *
 *  - **top**:    The middle of the checkbox is aligned to the middle of
 *                text when the text is aligned to top. The middle of the text
 *                is determined from the `font-size` and computed `cap-height`.
 *                This may mean that the checkbox will be drawn into its
 *                margins.
 *  - **middle**: The middle of the checkbox is aligned to the middle of the
 *                layout rectangle.
 *  - **bottom**: The middle of the checkbox is aligned to the middle of
 *                text when the text is aligned to bottom. The middle of the
 *                text is determined from the `font-size` and computed
 *                `cap-height`. This may mean that the checkbox will be
 *                drawn into its margins.
 *
 * @snippet widgets/checkbox_example_impl.cpp Create a checkbox button
 */
class checkbox_widget : public widget {
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

    ~checkbox_widget()
    {
        this->delegate->deinit(this);
    }

    /** Construct a checkbox widget.
     *
     * @param parent The parent widget that owns this checkbox widget.
     * @param delegate The delegate to use to manage the state of the checkbox button.
     */
    template<std::derived_from<delegate_type> Delegate>
    checkbox_widget(std::shared_ptr<Delegate> delegate) noexcept : super(), delegate(std::move(delegate))
    {
        hi_axiom_not_null(this->delegate);

        this->delegate->init(this);
        _delegate_cbt = this->delegate->subscribe(this, [this] {
            set_checked(this->delegate->state(this) != widget_value::off);
            this->notifier();
        });
        _delegate_cbt();

        style.set_name("checkbox");
    }

    /** Construct a checkbox widget with a default button delegate.
     *
     * @param args The arguments to the `default_toggle_delegate`
     */
    template<typename... Args>
    checkbox_widget(Args&&... args) : checkbox_widget(make_default_delegate(std::forward<Args>(args)...))
    {
    }

    /// @privatesection
    void set_layout(widget_layout const& context) noexcept override
    {
        super::set_layout(context);

        auto const middle = context.get_middle(style.cap_height);
        auto const extended_rectangle = context.rectangle() + style.vertical_margins_px;
        _button_rectangle =
            align_to_middle(extended_rectangle, style.size_px, os_settings::alignment(style.horizontal_alignment), middle.in(unit::pixels));

        _check_glyph = find_glyph(elusive_icon::Ok);
        auto const check_glyph_bb = _check_glyph.front_glyph_metrics().bounding_rectangle * style.font_size_px;
        _check_glyph_rectangle = align(_button_rectangle, check_glyph_bb, alignment::middle_center());

        _minus_glyph = find_glyph(elusive_icon::Minus);
        auto const minus_glyph_bb = _minus_glyph.front_glyph_metrics().bounding_rectangle * style.font_size_px;
        _minus_glyph_rectangle = align(_button_rectangle, minus_glyph_bb, alignment::middle_center());
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
                style.border_radius_px);

            switch (delegate->state(this)) {
            case widget_value::on:
                context.draw_glyph(layout(), translate_z(0.1f) * _check_glyph_rectangle, _check_glyph, style.accent_color);
                break;
            case widget_value::off:
                // Do nothing, the checkbox is off.
                break;
            default:
                // Indeterminate
                context.draw_glyph(layout(), translate_z(0.1f) * _minus_glyph_rectangle, _minus_glyph, style.accent_color);
            }
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
    aarectangle _button_rectangle;
    font_glyph_ids _check_glyph;
    aarectangle _check_glyph_rectangle;
    font_glyph_ids _minus_glyph;
    aarectangle _minus_glyph_rectangle;

    callback<void()> _delegate_cbt;
};

using checkbox_with_label_widget = with_label_widget<checkbox_widget>;
using checkbox_menu_button_widget = menu_button_widget<checkbox_widget>;

} // namespace v1
} // namespace hi::v1
