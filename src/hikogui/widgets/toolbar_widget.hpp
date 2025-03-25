// Copyright Take Vos 2020-2022.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

/** @file widgets/toolbar_widget.hpp Defines toolbar_widget.
 * @ingroup widgets
 */

#pragma once

#include "widget.hpp"
#include "../layout/layout.hpp"
#include "../geometry/geometry.hpp"
#include "../macros.hpp"
#include <memory>
#include <ranges>
#include <coroutine>

hi_export_module(hikogui.widgets.toolbar_widget);

hi_export namespace hi {
inline namespace v1 {

/** A toolbar widget is located at the top of a window and lays out its children
 * horizontally.
 *
 * The toolbar widget is shown at the top of a window, it is drawn with a
 * different background than the window's content to differentiate from it.
 *
 * Child widgets may be added on both the left side and right side of the
 * toolbar, a small space in the center is added for visual separation.
 *
 * Child widgets are drawn to full height of the toolbar determined by the
 * preferred size of all contained widgets. The width of each widget is
 * determined by their preferred size
 *
 * @ingroup widgets
 */
class toolbar_widget : public widget {
public:
    using super = widget;

    /** Constructs an empty row/column widget.
     *
     * @param parent The parent widget.
     */
    toolbar_widget() noexcept : super()
    {
        hi_axiom(loop::main().on_thread());

        auto spacer = std::make_unique<spacer_widget>();
        spacer->set_parent(this);

        _row.push_back(std::move(spacer));

        style.set_name("toolbar");
    }

    /** Add a widget directly to this toolbar-widget.
     *
     * When `Alignment` is `horizontal_alignment::left` the new widget is added
     * to the right of the previous added widget on the left side of the
     * toolbar.
     *
     * When `Alignment` is `horizontal_alignment::right` the new widget is added
     * to the left of the previous added widget on the right side of the
     * toolbar.
     *
     * @tparam Widget The type of the widget to be constructed.
     * @tparam Alignment Add widgets to the left, or right.
     * @param args The arguments passed to the constructor of the widget.
     * @return A reference to the widget that was created.
     */
    template<typename Widget, horizontal_alignment Alignment = horizontal_alignment::left, typename... Args>
    Widget& emplace(Args&&... args)
    {
        auto widget = std::make_unique<Widget>(std::forward<Args>(args)...);
        auto& ref = *widget;
        insert(Alignment, std::move(widget));
        return ref;
    }

    /// @privatesection
    [[nodiscard]] generator<widget_intf&> children(bool include_invisible) const noexcept override
    {
        for (auto const& child : _row) {
            co_yield *child.value;
        }
    }

    [[nodiscard]] box_constraints update_constraints() noexcept override
    {
        for (auto& cell : _row) {
            cell.set_constraints(cell.value->update_constraints());
        }

        // The margins (off the children) on the outside of the toolbar are ignored.
        auto r = _row.constraints(os_settings::left_to_right(), style.vertical_alignment);
        r.margins = style.margins_px;

        return r;
    }

    void set_layout(widget_layout const& context) noexcept override
    {
        super::set_layout(context);

        _row.set_layout(context.shape);

        for (auto const& child : _row) {
            // Use the shape of the child also as the clipping rectangle for
            // that child, so that drawing outside the child's shape is clipped.
            child.value->set_layout(context.transform(child.shape, transform_command::menu_item, child.shape.rectangle));
        }
    }

    void draw(draw_context const& context) const noexcept override
    {
        if (overlaps(context, layout())) {
            context.draw_box(layout(), layout().rectangle(), style.background_color);

            if (tab_button_has_focus()) {
                // Draw the line at a higher elevation (1.5), so that the tab
                // buttons can draw above or below the focus line depending if
                // that specific button is in focus or not.
                auto const focus_rectangle = aarectangle{0.0f, 0.0f, layout().rectangle().width(), theme().border_width()};
                context.draw_box(layout(), translate3{0.0f, 0.0f, 1.5f} * focus_rectangle, style.accent_color);
            }
        }

        return super::draw(context);
    }

    hitbox hitbox_test(point2 position) const noexcept override
    {
        hi_axiom(loop::main().on_thread());

        // By default the toolbar is used for dragging the window.
        if (enabled()) {
            auto r = layout().contains(position) ? hitbox{id(), layout().elevation, hitbox_type::move_area} : hitbox{};

            for (auto const& child : _row) {
                hi_assert_not_null(child.value);
                r = child.value->hitbox_test_from_parent(position, r);
            }

            return r;
        } else {
            return {};
        }
    }
    
    [[nodiscard]] color focus_color() const noexcept override
    {
        if (enabled()) {
            return theme().accent_color();
        } else {
            return theme().disabled_color();
        }
    }
    /// @endprivatesection
private:
    mutable row_layout<std::unique_ptr<widget>> _row;
    size_t _spacer_index = 0;

    void update_layout_for_child(widget& child, ssize_t index, widget_layout const& context) const noexcept;

    /** Add a widget directly to this widget.
     */
    widget& insert(horizontal_alignment alignment, std::unique_ptr<widget> widget) noexcept
    {
        auto& ref = *widget;
        switch (alignment) {
            using enum horizontal_alignment;
        case left:
            widget->set_parent(this);
            _row.insert(_row.cbegin() + _spacer_index, std::move(widget));
            ++_spacer_index;
            break;
        case right:
            widget->set_parent(this);
            _row.insert(_row.cbegin() + _spacer_index + 1, std::move(widget));
            break;
        default:
            hi_no_default();
        }

        return ref;
    }

    /** Check if a child tab-button has focus.
     *
     * @return If true the toolbar should draw a focus bar.
     */
    bool tab_button_has_focus() const noexcept
    {
        for (auto const& child : visible_children()) {
            if (auto tab_child = dynamic_cast<toolbar_tab_button_widget const*>(std::addressof(child))) {
                if (tab_child->focus() and tab_child->checked()) {
                    return true;
                }
            }
        }

        return false;
    }
};

} // namespace v1
} // namespace hi::v1
