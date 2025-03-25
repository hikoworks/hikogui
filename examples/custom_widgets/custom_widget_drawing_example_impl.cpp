// Copyright Take Vos 2021-2022.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

#include "hikogui/hikogui.hpp"
#include "hikogui/crt.hpp"
#include <numbers>

enum class drawing_type {
    box,
    lines,
    circle,
    glyph,
    image,
};

auto drawing_list = std::vector<std::pair<drawing_type, hi::label>>{
    {drawing_type::box, hi::txt("Box")},
    {drawing_type::lines, hi::txt("Lines")},
    {drawing_type::circle, hi::txt("Circle")},
    {drawing_type::glyph, hi::txt("Glyph")},
    {drawing_type::image, hi::txt("Image")},
};

enum class shape_type { square, rectangle, convex, concave, glyph_aspect_ratio, image_aspect_ratio };

auto shape_list = std::vector<std::pair<shape_type, hi::label>>{
    {shape_type::square, hi::txt("Square")},
    {shape_type::rectangle, hi::txt("Rectangle")},
    {shape_type::convex, hi::txt("Convex")},
    {shape_type::concave, hi::txt("Concave")},
    {shape_type::glyph_aspect_ratio, hi::txt("Glyph Aspect Ratio")},
    {shape_type::image_aspect_ratio, hi::txt("Image Aspect Ratio")},
};

enum class gradient_type { solid, horizontal, vertical, corners };

auto gradient_list = std::vector<std::pair<gradient_type, hi::label>>{
    {gradient_type::solid, hi::txt("Solid")},
    {gradient_type::horizontal, hi::txt("Horizontal")},
    {gradient_type::vertical, hi::txt("Vertical")},
    {gradient_type::corners, hi::txt("Corners")},
};

auto border_width_list = std::vector<std::pair<float, hi::label>>{
    {0.0f, hi::txt("no border")},
    {1.0f, hi::txt("1 px")},
    {2.0f, hi::txt("2 px")},
    {4.0f, hi::txt("4 px")},
    {8.0f, hi::txt("8 px")},
};

// Every widget must inherit from hi::widget.
class drawing_widget : public hi::widget {
public:
    using super = hi::widget;
    
    constexpr static auto blue = hi::color(0.05f, 0.05f, 0.50f);
    constexpr static auto red = hi::color(0.50f, 0.05f, 0.05f);
    constexpr static auto cyan = hi::color(0.05f, 0.50f, 0.50f);
    constexpr static auto white = hi::color(0.50f, 0.50f, 0.50f);
    constexpr static auto redish = hi::color(0.70f, 0.30f, 0.00f);
    constexpr static auto greenish = hi::color(0.00f, 0.30f, 0.70f);
    constexpr static auto blueish = hi::color(0.00f, 0.70f, 0.30f);
    constexpr static auto redish2 = hi::color(0.70f, 0.00f, 0.30f);

    hi::observer<drawing_type> drawing = drawing_type::box;
    hi::observer<shape_type> shape = shape_type::square;
    hi::observer<gradient_type> gradient = gradient_type::solid;
    hi::observer<bool> rotating = false;
    hi::observer<bool> clip = false;
    hi::observer<hi::border_side> border_side = hi::border_side::on;
    hi::observer<float> border_width = 0.0f;
    hi::observer<bool> rounded = false;

    // Every constructor of a widget starts with a `window` and `parent` argument.
    // In most cases these are automatically filled in when calling a container widget's `emplace()` function.
    drawing_widget() noexcept : widget(), _image(hi::URL("resource:mars3.png"))
    {
        // clang-format off
        _drawing_cbt = this->drawing.subscribe([&](auto...){ request_redraw(); });
        _shape_cbt = this->shape.subscribe([&](auto...){ request_redraw(); });
        _gradient_cbt = this->gradient.subscribe([&](auto...){ request_redraw(); });
        _rotating_cbt = this->rotating.subscribe([&](auto...){ request_redraw(); });
        _clip_cbt = this->clip.subscribe([&](auto...){ request_redraw(); });
        _border_side_cbt = this->border_side.subscribe([&](auto...){ request_redraw(); });
        _border_width_cbt = this->border_width.subscribe([&](auto...){ request_redraw(); });
        _rounded_cbt = this->rounded.subscribe([&](auto...){ request_redraw(); });
        // clang-format on
    }

    // The set_constraints() function is called when the window is first initialized,
    // or when a widget wants to change its constraints.
    [[nodiscard]] hi::box_constraints update_constraints() noexcept override
    {
        this->_glyph = find_glyph(hi::elusive_icon::Briefcase);

        if (_image_was_modified.exchange(false)) {
            if (not(_image_backing = hi::gfx_pipeline_image::paged_image{surface(), _image})) {
                // Could not get an image, retry.
                _image_was_modified = true;
                ++hi::global_counter<"drawing_widget:no-backing-image:constrain">;
                send_to_window({hi::gui_event_type::window_reconstrain});
            }
        }

        // Certain expensive calculations, such as loading of images and shaping of text
        // can be done in this function.

        // The constrains below have different minimum, preferred and maximum sizes.
        // When the window is initially created it will try to size itself so that
        // the contained widgets are at their preferred size. Having a different minimum
        // and/or maximum size will allow the window to be resizable.
        return {{100, 100}, {150, 150}, {400, 400}, hi::alignment{}, theme().margin()};
    }

    // The `set_layout()` function is called when the window has resized, or when
    // a widget wants to change the internal layout.
    //
    // NOTE: The size of the layout may be larger than the maximum constraints of this widget.
    void set_layout(hi::widget_layout const& context) noexcept override
    {
        super::set_layout(context);

        // Make a size scaled to the layout.
        auto const max_size = hi::narrow_cast<hi::extent2>(layout().size()) * 0.9f;
        auto const max_rectangle = hi::aarectangle{hi::point2{max_size.width() * -0.5f, max_size.height() * -0.5f}, max_size};

        // Here we can do some semi-expensive calculations which must be done when resizing the widget.
        // In this case we make two rectangles which are used in the `draw()` function.
        auto const glyph_size = _glyph.front_glyph_metrics().bounding_rectangle.size();
        auto const glyph_scale = hi::scale2::uniform(glyph_size, max_size);
        auto const new_glyph_size = glyph_scale * glyph_size;
        _glyph_rectangle = align(max_rectangle, new_glyph_size, hi::alignment::middle_center());

        auto const image_size = hi::extent2{static_cast<float>(_image.width()), static_cast<float>(_image.height())};
        auto const image_scale = hi::scale2::uniform(image_size, max_size);
        auto const new_image_size = image_scale * image_size;
        _image_rectangle = align(max_rectangle, new_image_size, hi::alignment::middle_center());
    }

    [[nodiscard]] hi::quad_color fill_color() const noexcept
    {
        switch (*gradient) {
        case gradient_type::solid:
            return hi::quad_color(blue);
        case gradient_type::horizontal:
            return hi::quad_color{blue, red, blue, red};
        case gradient_type::vertical:
            return hi::quad_color{blue, blue, red, red};
        case gradient_type::corners:
            return hi::quad_color{red, blue, cyan, white};
        default:
            hi_no_default();
        }
    }

    [[nodiscard]] hi::quad_color line_color() const noexcept
    {
        if (*border_width == 0.0f) {
            // Due to inaccuracies in the shaders, a thin border may present itself inside
            // the anti-aliased edges; so make it the same color as the fill. This is the
            // same thing that happens when you call draw_box with only a fill color.
            return fill_color();
        } else {
            switch (*gradient) {
            case gradient_type::solid:
                return hi::quad_color(redish);
            case gradient_type::horizontal:
                return hi::quad_color{redish, greenish, redish, greenish};
            case gradient_type::vertical:
                return hi::quad_color{redish, redish, greenish, greenish};
            case gradient_type::corners:
                return hi::quad_color{redish, greenish, blueish, redish2};
            default:
                hi_no_default();
            }
        }
    }

    [[nodiscard]] hi::quad shape_quad() const noexcept
    {
        switch (*shape) {
        case shape_type::square:
            return hi::quad{
                hi::point3{-40.0f, -40.0f}, hi::point3{40.0f, -40.0f}, hi::point3{-40.0f, 40.0f}, hi::point3{40.0f, 40.0f}};
        case shape_type::rectangle:
            return hi::quad{
                hi::point3{-50.0f, -40.0f}, hi::point3{50.0f, -40.0f}, hi::point3{-50.0f, 40.0f}, hi::point3{50.0f, 40.0f}};
        case shape_type::convex:
            return hi::quad{
                hi::point3{-50.0f, -10.0f}, hi::point3{50.0f, -40.0f}, hi::point3{-50.0f, 40.0f}, hi::point3{50.0f, 50.0f}};
        case shape_type::concave:
            return hi::quad{
                hi::point3{20.0f, 20.0f}, hi::point3{50.0f, -40.0f}, hi::point3{-50.0f, 40.0f}, hi::point3{50.0f, 50.0f}};
        case shape_type::glyph_aspect_ratio:
            return _glyph_rectangle;
        case shape_type::image_aspect_ratio:
            return _image_rectangle;
        default:
            hi_no_default();
        }
    }

    [[nodiscard]] hi::rotate3 rotation(hi::draw_context const& context) const noexcept
    {
        float angle = 0.0f;
        if (*rotating) {
            request_redraw();
            constexpr auto interval_in_ns = 10'000'000'000;
            auto const repeating_interval = context.display_time_point.time_since_epoch().count() % interval_in_ns;
            angle = (float(repeating_interval) / float(interval_in_ns)) * 2.0f * std::numbers::pi_v<float>;
        }
        return hi::rotate3(angle, hi::vector3{0.0f, 0.0f, 1.0f});
    }

    [[nodiscard]] hi::corner_radii corners() const noexcept
    {
        if (*rounded) {
            return hi::corner_radii{20.0f, 10.0f, 5.0f, 0.0f};
        } else {
            return {};
        }
    }

    [[nodiscard]] hi::line_end_cap end_cap() const noexcept
    {
        return *rounded ? hi::line_end_cap::round : hi::line_end_cap::flat;
    }

    // The `draw()` function is called when all or part of the window requires redrawing.
    // This may happen when showing the window for the first time, when the operating-system
    // requests a (partial) redraw, or when a widget requests a redraw of itself.
    void draw(hi::draw_context const& context) const noexcept override
    {
        using namespace std::chrono_literals;

        auto const clipping_rectangle =
            *clip ? hi::aarectangle{0, 0, layout().width(), layout().height() / 2} : layout().rectangle();

        auto const translation =
            hi::translate3(std::floor(layout().width() * 0.5f), std::floor(hi::narrow_cast<float>(layout().height())) * 0.5f, 0.0f);
        auto const transform = translation * rotation(context);

        auto const circle_radius = hypot(shape_quad().bottom()) * 0.5f;
        auto const circle = hi::circle{hi::point3{0.0f, 0.0f, 0.0f}, circle_radius};

        // We only need to draw the widget when it is visible and when the visible area of
        // the widget overlaps with the scissor-rectangle (partial redraw) of the drawing context.
        if (mode() > hi::widget_mode::invisible and overlaps(context, layout())) {
            switch (*drawing) {
            case drawing_type::box:
                context.draw_box(
                    layout(),
                    transform * shape_quad(),
                    clipping_rectangle,
                    fill_color(),
                    line_color(),
                    *border_width,
                    *border_side,
                    corners());
                break;

            case drawing_type::lines:
                {
                    // There is a concave corner at left-bottom, so I want this to be the second point the lines pass through.
                    auto const quad = shape_quad();
                    auto const line1 = hi::line_segment{get<0>(quad), get<1>(quad)};
                    auto const line2 = hi::line_segment{get<0>(quad), get<2>(quad)};
                    auto const line3 = hi::line_segment{get<3>(quad), get<2>(quad)};
                    auto const width = std::max(0.5f, *border_width);
                    context.draw_line(layout(), transform * line1, clipping_rectangle, width, fill_color(), end_cap(), end_cap());
                    context.draw_line(layout(), transform * line2, clipping_rectangle, width, fill_color(), end_cap(), end_cap());
                    context.draw_line(layout(), transform * line3, clipping_rectangle, width, fill_color(), end_cap(), end_cap());
                }
                break;

            case drawing_type::circle:
                context.draw_circle(
                    layout(), translation * circle, clipping_rectangle, fill_color(), line_color(), *border_width, *border_side);
                break;

            case drawing_type::glyph:
                // A full rectangle is visible.
                context.draw_glyph(layout(), transform * shape_quad(), _glyph, clipping_rectangle, fill_color());
                break;

            case drawing_type::image:
                if (not context.draw_image(layout(), transform * shape_quad(), _image_backing, clipping_rectangle)) {
                    // Image was not yet uploaded to the texture atlas, redraw until it does.
                    request_redraw();
                }
                break;

            default:
                hi_no_default();
            }
        }

        return super::draw(context);
    }

private:
    hi::font_glyph_ids _glyph;
    hi::aarectangle _glyph_rectangle;
    std::atomic<bool> _image_was_modified = true;
    hi::png _image;
    hi::aarectangle _image_rectangle;
    hi::gfx_pipeline_image::paged_image _image_backing;

    // The callback objects should be declared as the last member variables,
    // so that they will get destroyed first, to delay the destruction of the
    // widget when a callback is in-flight.
    hi::callback<void(drawing_type)> _drawing_cbt;
    hi::callback<void(shape_type)> _shape_cbt;
    hi::callback<void(gradient_type)> _gradient_cbt;
    hi::callback<void(bool)> _rotating_cbt;
    hi::callback<void(bool)> _clip_cbt;
    hi::callback<void(hi::border_side)> _border_side_cbt;
    hi::callback<void(float)> _border_width_cbt;
    hi::callback<void(bool)> _rounded_cbt;
};

int hi_main(int argc, char* argv[])
{
    hi::set_application_name("Custom widget drawing example");
    hi::set_application_vendor("HikoGUI");
    hi::set_application_version({1, 0, 0});

    hi::observer<drawing_type> drawing = drawing_type::box;
    hi::observer<shape_type> shape = shape_type::square;
    hi::observer<bool> rotating = false;
    hi::observer<bool> clip = false;
    hi::observer<gradient_type> gradient = gradient_type::solid;
    hi::observer<hi::border_side> border_side = hi::border_side::on;
    hi::observer<float> border_width = 0.0f;
    hi::observer<bool> rounded = false;

    // Startup renderdoc for debugging
    hi::start_render_doc();

    auto widget = std::make_unique<hi::window_widget>(hi::txt("Drawing Custom Widget"));

    auto& custom_widget = widget->content().emplace<drawing_widget>("A1:D1");
    custom_widget.drawing = drawing;
    custom_widget.shape = shape;
    custom_widget.rotating = rotating;
    custom_widget.clip = clip;
    custom_widget.gradient = gradient;
    custom_widget.border_side = border_side;
    custom_widget.border_width = border_width;
    custom_widget.rounded = rounded;

    widget->content().emplace<hi::label_widget>("A2", hi::txt("Drawing type:"));
    widget->content().emplace<hi::selection_widget>("B2:D2", drawing, drawing_list);

    widget->content().emplace<hi::label_widget>("A3", hi::txt("Shape:"));
    widget->content().emplace<hi::selection_widget>("B3:D3", shape, shape_list);

    widget->content().emplace<hi::label_widget>("A4", hi::txt("Gradient:"));
    widget->content().emplace<hi::selection_widget>("B4:D4", gradient, gradient_list);

    widget->content().emplace<hi::label_widget>("A5", hi::txt("Border side:"));
    widget->content().emplace<hi::radio_with_label_widget>("B5", border_side, hi::border_side::on, hi::txt("on"));
    widget->content().emplace<hi::radio_with_label_widget>("C5", border_side, hi::border_side::inside, hi::txt("inside"));
    widget->content().emplace<hi::radio_with_label_widget>("D5", border_side, hi::border_side::outside, hi::txt("outside"));

    widget->content().emplace<hi::label_widget>("A6", hi::txt("Border width:"));
    widget->content().emplace<hi::selection_widget>("B6:D6", border_width, border_width_list);

    widget->content().emplace<hi::label_widget>("A7", hi::txt("Rotate:"));
    widget->content().emplace<hi::toggle_with_label_widget>("B7:D7", rotating);

    widget->content().emplace<hi::label_widget>("A8", hi::txt("Clip:"));
    widget->content().emplace<hi::toggle_with_label_widget>("B8:D8", clip);

    widget->content().emplace<hi::label_widget>("A9", hi::txt("Rounded:"));
    widget->content().emplace<hi::toggle_with_label_widget>("B9:D9", rounded);

    auto window = std::make_unique<hi::gui_window>(std::move(widget));

    auto close_cbt = window->closing.subscribe(
        [&] {
            window = {};
        },
        hi::callback_flags::main);
    return hi::loop::main().resume();
}
