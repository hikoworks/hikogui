// Copyright Take Vos 2021.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

/** @file widgets/overlay_widget.hpp Defines overlay_widget.
 * @ingroup widgets
 */

#pragma once

#include "widget.hpp"
#include "../macros.hpp"
#include <coroutine>

hi_export_module(hikogui.widgets.overlay_widget);

hi_export namespace hi {
inline namespace v1 {

/** A GUI widget which may exist anywhere on a window overlaid above any other widget.
 * @ingroup widgets
 *
 * The overlay widget allows a content widget to be shown on top of other
 * widgets in the window. It may be used for pop-up widgets, dialog boxes and
 * sheets.
 *
 * The size of the overlay widget is based on the `widget::minimum_size()`,
 * `widget::preferred_size()` and `widget::maximum_size()`. Unlike other
 * container widgets the clipping rectangle is made tightly around the container
 * widget so that no drawing will happen outside of the overlay. The overlay
 * itself will draw outside the clipping rectangle, for drawing a border and
 * potentially a shadow.
 *
 * As an overlay widget still confined to a window, like other widgets, when
 * setting its layout parameters, it is recommended to use
 * `widget::make_overlay_rectangle()` to make a rectangle that will fit inside
 * the window.
 *
 * It is recommended that the content of an overlay widget is a scroll widget
 * so that when the overlay widget is drawn smaller than the requested rectangle
 * the content will behave correctly.
 */
class overlay_widget : public widget {
public:
    using super = widget;

    ~overlay_widget() {}

    /** Constructs an empty overlay widget.
     *
     * @param parent The parent widget.
     */
    overlay_widget() noexcept : super()
    {
        style.set_name("overlay");
    }

    void set_widget(std::unique_ptr<widget> new_widget) noexcept
    {
        if (new_widget) {
            new_widget->set_parent(this);
        }
        auto old_widget = std::exchange(_content, std::move(new_widget));
        if (old_widget) {
            old_widget->set_parent(nullptr);
        }

        ++global_counter<"overlay_widget:set_widget:constrain">;
        request_reconstrain();
    }

    /** Add a content widget directly to this overlay widget.
     *
     * This widget is added as the content widget.
     *
     * @pre No content widgets have been added before.
     * @tparam Widget The type of the widget to be constructed.
     * @param args The arguments passed to the constructor of the widget.
     * @return A reference to the widget that was created.
     */
    template<typename Widget, typename... Args>
    Widget& emplace(Args&&... args) noexcept
    {
        hi_axiom(loop::main().on_thread());
        hi_assert(_content == nullptr);

        auto tmp = std::make_unique<Widget>(std::forward<Args>(args)...);
        auto& ref = *tmp;
        set_widget(std::move(tmp));
        return ref;
    }

    /// @privatesection
    [[nodiscard]] generator<widget_intf&> children(bool include_invisible) const noexcept override
    {
        if (_content) {
            co_yield *_content;
        }
    }

    [[nodiscard]] box_constraints update_constraints() noexcept override
    {
        _content_constraints = _content->update_constraints();
        return _content_constraints;
    }

    void set_layout(widget_layout const& context) noexcept override
    {
        // The clipping rectangle of the overlay matches the rectangle exactly, with a border around it.
        auto context_ = context;
        context_.clipping_rectangle = context_.rectangle() + style.border_width_px;

        super::set_layout(context_);

        auto const content_rectangle = context.rectangle();
        _content_shape = box_shape{content_rectangle, baseline{}};

        // The content should not draw in the border of the overlay, so give a tight clipping rectangle.
        _content->set_layout(layout().transform(_content_shape, context.rectangle()));
    }

    void draw(draw_context const& context) const noexcept override
    {
        if (overlaps(context, layout())) {
            context.draw_box(
                layout(),
                layout().rectangle(),
                background_color(),
                foreground_color(),
                theme().border_width(),
                border_side::outside);
        }

        return super::draw(context);
    }

    [[nodiscard]] color background_color() const noexcept override
    {
        return theme().fill_color(layout().layer + 1);
    }

    [[nodiscard]] color foreground_color() const noexcept override
    {
        return theme().border_color();
    }

    void scroll_to_show(hi::aarectangle rectangle) noexcept override
    {
        // An overlay is in an absolute position on the window,
        // so do not forward the scroll_to_show message to its parent.
    }

    [[nodiscard]] hitbox hitbox_test(point2 position) const noexcept override
    {
        hi_axiom(loop::main().on_thread());
        return _content->hitbox_test_from_parent(position);
    }

    bool handle_event(gui_event const& event) noexcept override
    {
        // Short-cut event handling, no-events should be passed below the overlay.
        return true;
    }
    /// @endprivatesection
private:
    std::unique_ptr<widget> _content;
    box_constraints _content_constraints;
    box_shape _content_shape;
};

} // namespace v1
} // namespace hi::v1
