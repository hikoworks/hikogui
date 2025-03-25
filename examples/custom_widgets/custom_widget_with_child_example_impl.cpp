// Copyright Take Vos 2021-2022.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

#include "hikogui/hikogui.hpp"
#include "hikogui/crt.hpp"

// Every widget must inherit from hi::widget.
class widget_with_child : public hi::widget {
public:
    using super = hi::widget;

    // Every constructor of a widget starts with a `window` and `parent` argument.
    // In most cases these are automatically filled in when calling a container widget's `emplace()` function.
    template<typename Label>
    widget_with_child(Label&& label) noexcept : widget()
    {
        // Our child widget is a `label_widget` which requires a label to be passed as an third argument.
        // We use a templated argument to forward the label into the `label_widget`.
        _label_widget = std::make_unique<hi::label_widget>(std::forward<Label>(label), hi::alignment::middle_center());
        _label_widget->set_parent(this);
    }

    // The set_constraints() function is called when the window is first initialized,
    // or when a widget wants to change its constraints.
    [[nodiscard]] hi::box_constraints update_constraints() noexcept override
    {
        // We need to recursively set the constraints of any child widget here as well
        _label_constraints = _label_widget->update_constraints();

        // We add the ability to resize the widget beyond the size of the label.
        auto r = hi::box_constraints{};
        r.minimum.width() = _label_constraints.minimum.width();
        r.preferred.width() = _label_constraints.preferred.width() + theme().margin<float>();
        r.maximum.width() = _label_constraints.maximum.width() + 100.0f;
        r.minimum.height() = _label_constraints.minimum.height();
        r.preferred.height() = _label_constraints.preferred.height() + theme().margin<float>();
        r.maximum.height() = _label_constraints.maximum.height() + 50.0f;
        r.margins = theme().margin();
        r.alignment = _label_constraints.alignment;
        return r;
    }

    // The `set_layout()` function is called when the window has resized, or when
    // a widget wants to change the internal layout.
    //
    // NOTE: The size of the layout may be larger than the maximum constraints of this widget.
    void set_layout(hi::widget_layout const& context) noexcept override
    {
        super::set_layout(context);

        // The layout of the child widget are also calculated here, which only needs to be done
        // when the layout of the current widget changes.
        auto const label_rectangle = align(context.rectangle(), _label_constraints.preferred, hi::alignment::middle_center());
        _label_shape = hi::box_shape{_label_constraints, label_rectangle, theme().baseline_adjustment()};

        // The layout of any child widget must always be set, even if the layout didn't actually change.
        // This is because child widgets may need to re-layout for other reasons.
        _label_widget->set_layout(context.transform(_label_shape));
    }

    // The `draw()` function is called when all or part of the window requires redrawing.
    // This may happen when showing the window for the first time, when the operating-system
    // requests a (partial) redraw, or when a widget requests a redraw of itself.
    void draw(hi::draw_context const& context) const noexcept override
    {
        if (mode() > hi::widget_mode::invisible) {
            // We only need to draw the widget when it is visible and when the visible area of
            // the widget overlaps with the scissor-rectangle (partial redraw) of the drawing context.
            if (overlaps(context, layout())) {
                // There may be stylistic reasons to draw into the margin, for example
                // round objects need to be drawn slightly larger than square objects.
                // The standard clipping rectangle is 2 pixels larger than the layout().rectangle().
                // In this example we draw the border outside the rectangle.
                context.draw_box(
                    layout(),
                    layout().rectangle(),
                    background_color(),
                    foreground_color(),
                    theme().border_width(),
                    hi::border_side::outside,
                    theme().rounding_radius());
            }

            // Child widget only need to be drawn when the parent is visible, but the child may have
            // a visible area outside of the parent's visible area, therefor it should do its own
            // overlap check.
            _label_widget->draw(context);
        }

        return super::draw(context);
    }

protected:
    // This function MUST be overridden when a widget has children.
    //
    // The order of the children returned is used for determining the next widget for
    // keyboard navigation.
    //
    // The allocator argument should not be used by the function, it is used by the caller
    // to allocate the co-routine's frame on the stack.
    [[nodiscard]] hi::generator<widget_intf&> children(bool include_invisible) noexcept override
    {
        // This function, written as a co-routine, yields a references to each of its children.
        co_yield *_label_widget;
    }

private:
    // Child widgets are owned by their parent.
    std::unique_ptr<hi::label_widget> _label_widget;
    hi::box_constraints _label_constraints;
    hi::box_shape _label_shape;
};

int hi_main(int argc, char* argv[])
{
    hi::set_application_name("Custom widget with child example");
    hi::set_application_vendor("HikoGUI");
    hi::set_application_version({1, 0, 0});

    auto widget = std::make_unique<hi::window_widget>(hi::txt("Widget with child"));
    widget->content().emplace<widget_with_child>("A1", hi::txt("Widget with child"));

    auto window = std::make_unique<hi::gui_window>(std::move(widget));

    auto close_cbt = window->closing.subscribe(
        [&] {
            window = {};
        },
        hi::callback_flags::main);

    return hi::loop::main().resume();
}
