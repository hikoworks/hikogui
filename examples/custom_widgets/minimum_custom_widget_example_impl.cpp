// Copyright Take Vos 2021-2022.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

#include "hikogui/hikogui.hpp"
#include "hikogui/crt.hpp"

// Every widget must inherit from hi::widget.
class minimum_widget : public hi::widget {
public:
    using super = hi::widget;

    // Every constructor of a widget starts with a `window` and `parent` argument.
    // In most cases these are automatically filled in when calling a container widget's `emplace()` function.
    minimum_widget() noexcept : widget() {}

    // The set_constraints() function is called when the window is first initialized,
    // or when a widget wants to change its constraints.
    [[nodiscard]] hi::box_constraints update_constraints() noexcept override
    {
        // Certain expensive calculations, such as loading of images and shaping of text
        // can be done in this function.

        // The constrains below have different minimum, preferred and maximum sizes.
        // When the window is initially created it will try to size itself so that
        // the contained widgets are at their preferred size. Having a different minimum
        // and/or maximum size will allow the window to be resizable.
        return {{100, 50}, {200, 100}, {300, 100}, style.margins_px};
    }

    // The `set_layout()` function is called when the window has resized, or when
    // a widget wants to change the internal layout.
    //
    // NOTE: The size of the layout may be larger than the maximum constraints of this widget.
    void set_layout(hi::widget_layout const& context) noexcept override
    {
        super::set_layout(context);

        // Here we can do some semi-expensive calculations which must be done when resizing the widget.
        // In this case we make two rectangles which are used in the `draw()` function.
        _left_rectangle = hi::aarectangle{
            hi::extent2{hi::narrow_cast<float>(context.width()) / 2, hi::narrow_cast<float>(context.height())}};
        _right_rectangle =
            hi::aarectangle{hi::point2{hi::narrow_cast<float>(context.width()) / 2, 0.0}, _left_rectangle.size()};
    }

    // The `draw()` function is called when all or part of the window requires redrawing.
    // This may happen when showing the window for the first time, when the operating-system
    // requests a (partial) redraw, or when a widget requests a redraw of itself.
    void draw(hi::draw_context const& context) const noexcept override
    {
        // We only need to draw the widget when it is visible and when the visible area of
        // the widget overlaps with the scissor-rectangle (partial redraw) of the drawing context.
        if (mode() > hi::widget_mode::invisible and overlaps(context, layout())) {
            // Draw two boxes matching the rectangles calculated during set_layout().
            // The actual RGB colors are taken from the current theme.
            context.draw_box(layout(), _left_rectangle, hi::color::indigo());
            context.draw_box(layout(), _right_rectangle, hi::color::blue());
        }

        return super::draw(context);
    }

private:
    hi::aarectangle _left_rectangle;
    hi::aarectangle _right_rectangle;
};

int hi_main(int argc, char *argv[])
{
    hi::set_application_name("Minimum custom widget example");
    hi::set_application_vendor("HikoGUI");
    hi::set_application_version({1, 0, 0});

    auto widget = std::make_unique<hi::window_widget>(hi::txt("Minimum Custom Widget"));
    widget->content().emplace<minimum_widget>("A1");

    auto window = std::make_unique<hi::gui_window>(std::move(widget));

    auto close_cbt = window->closing.subscribe(
        [&] {
            window = {};
        },
        hi::callback_flags::main);
    return hi::loop::main().resume();
}
