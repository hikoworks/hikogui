// Copyright Take Vos 2022.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

/** @file widgets/scroll_aperture_widget.hpp Defines scroll_aperture_widget.
 * @ingroup widgets
 */

#pragma once

#include "widget.hpp"
#include "../macros.hpp"
#include <coroutine>

hi_export_module(hikogui.widgets.scroll_aperture_widget);

hi_export namespace hi {
inline namespace v1 {

/** A scroll aperture widget.
 *
 * A widget that is used as a child of the `scroll_widget` which
 * displays a partial rectangle (the aperture) of the content.
 *
 * @ingroup widgets
 */
class scroll_aperture_widget : public widget {
public:
    using super = widget;

    observer<float> content_width;
    observer<float> content_height;
    observer<float> aperture_width;
    observer<float> aperture_height;
    observer<float> offset_x;
    observer<float> offset_y;

    scroll_aperture_widget() noexcept : super()
    {
        hi_axiom(loop::main().on_thread());

        _content_width_cbt = content_width.subscribe([&](auto...) {
            ++global_counter<"scroll_aperture_widget:content_width:relayout">;
            request_relayout();
        });
        _content_height_cbt = content_height.subscribe([&](auto...) {
            ++global_counter<"scroll_aperture_widget:content_height:relayout">;
            request_relayout();
        });
        _aperture_width_cbt = aperture_width.subscribe([&](auto...) {
            ++global_counter<"scroll_aperture_widget:aperture_width:relayout">;
            request_relayout();
        });
        _aperture_height_cbt = aperture_height.subscribe([&](auto...) {
            ++global_counter<"scroll_aperture_widget:aperture_height:relayout">;
            request_relayout();
        });
        _offset_x_cbt = offset_x.subscribe([&](auto...) {
            ++global_counter<"scroll_aperture_widget:offset_x:relayout">;
            request_relayout();
        });
        _offset_y_cbt = offset_y.subscribe([&](auto...) {
            ++global_counter<"scroll_aperture_widget:offset_y:relayout">;
            request_relayout();
        });

        style.set_name("scroll-aperture");
    }

    void set_widget(std::unique_ptr<widget> new_child) noexcept
    {
        if (new_child) {
            new_child->set_parent(this);
        }
        auto old_child = std::exchange(_content, std::move(new_child));
        if (old_child) {
            old_child->set_parent(nullptr);
        }
    }

    template<typename Widget, typename... Args>
    Widget& emplace(Args&&... args)
    {
        hi_axiom(loop::main().on_thread());
        hi_axiom(_content == nullptr);

        auto tmp = std::make_unique<Widget>(std::forward<Args>(args)...);
        auto& ref = *tmp;
        set_widget(std::move(tmp));
        return ref;
    }

    [[nodiscard]] bool x_axis_scrolls() const noexcept
    {
        return *content_width > *aperture_width;
    }

    [[nodiscard]] bool y_axis_scrolls() const noexcept
    {
        return *content_height > *aperture_height;
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

        // The margins of the content are internalized as padding inside the
        // aperture. So that the scroll bars can be placed directly on the edge
        // of the aperture.
        _padding = max(_content_constraints.margins, style.padding_px);

        // The minimum size of ther aperture is smaller than the content,
        // otherwise the content would not be scrolled.
        return {
            style.size_px,
            max(style.size_px, _content_constraints.preferred + _padding),
            max(style.size_px, _content_constraints.maximum + _padding),
            style.margins_px,
            embed(_content_constraints.baseline, unit::pixels(_padding.bottom()), unit::pixels(_padding.top()))};
    }

    void set_layout(widget_layout const& context) noexcept override
    {
        super::set_layout(context);

        auto const aperture_size = context.size() - _padding;
        aperture_width = aperture_size.width();
        aperture_height = aperture_size.height();

        // Start scrolling with the preferred size as minimum, so
        // that widgets in the content don't get unnecessarily squeezed.
        content_width = std::min(*aperture_width, _content_constraints.preferred.width());
        content_height = std::min(*aperture_height, _content_constraints.preferred.height());
        
        // Make sure the offsets are limited to the scrollable area.
        auto const offset_x_max = std::max(*content_width - *aperture_width, 0.0f);
        auto const offset_y_max = std::max(*content_height - *aperture_height, 0.0f);
        offset_x = std::clamp(*offset_x, 0.0f, offset_x_max);
        offset_y = std::clamp(*offset_y, 0.0f, offset_y_max);

        // The position of the content rectangle relative to the scroll view.
        // The size is further adjusted if the either the horizontal or vertical scroll bar is invisible.
        _content_shape = box_shape{
            aarectangle{
                -*offset_x + _padding.left(),
                -*offset_y + _padding.bottom(),
                *content_width,
                *content_height},
            lift(context.baseline(), unit::pixels(_padding.bottom()), unit::pixels(_padding.top()))};

        // The content needs to be at a higher elevation, so that hitbox check
        // will work correctly for handling scrolling with mouse wheel.
        _content->set_layout(context.transform(_content_shape, transform_command::level, context.rectangle()));
    }

    [[nodiscard]] hitbox hitbox_test(point2 position) const noexcept override
    {
        hi_axiom(loop::main().on_thread());

        auto r = _content->hitbox_test_from_parent(position);

        if (layout().contains(position)) {
            r = std::max(r, hitbox{id(), layout().elevation});
        }
        return r;
    }

    bool handle_event(gui_event const& event) noexcept override
    {
        hi_axiom(loop::main().on_thread());

        if (event == gui_event_type::mouse_wheel) {
            auto const new_offset_x =
                *offset_x + std::round((unit::dips(event.mouse().wheel_delta.x()) * style.pixel_density).in(unit::pixels));
            auto const new_offset_y =
                *offset_y + std::round((unit::dips(event.mouse().wheel_delta.y()) * style.pixel_density).in(unit::pixels));
            auto const max_offset_x = std::max(0.0f, *content_width - *aperture_width);
            auto const max_offset_y = std::max(0.0f, *content_height - *aperture_height);

            offset_x = std::clamp(new_offset_x, 0.0f, max_offset_x);
            offset_y = std::clamp(new_offset_y, 0.0f, max_offset_y);
            ++global_counter<"scroll_aperture_widget:mouse_wheel:relayout">;
            request_relayout();
            return true;
        } else {
            return super::handle_event(event);
        }
    }

    void scroll_to_show(hi::aarectangle to_show) noexcept override
    {
        if (layout()) {
            auto safe_rectangle = intersect(layout().rectangle(), layout().clipping_rectangle);
            auto delta_x = 0.0f;
            auto delta_y = 0.0f;

            if (safe_rectangle.width() > _padding.width() and
                safe_rectangle.height() > _padding.height()) {
                // This will look visually better, if the selected widget is moved with some margin from
                // the edge of the scroll widget. The margins of the content do not have anything to do
                // with the margins that are needed here.
                safe_rectangle = safe_rectangle - _padding;

                if (to_show.right() > safe_rectangle.right()) {
                    delta_x = to_show.right() - safe_rectangle.right();
                } else if (to_show.left() < safe_rectangle.left()) {
                    delta_x = to_show.left() - safe_rectangle.left();
                }

                if (to_show.top() > safe_rectangle.top()) {
                    delta_y = to_show.top() - safe_rectangle.top();
                } else if (to_show.bottom() < safe_rectangle.bottom()) {
                    delta_y = to_show.bottom() - safe_rectangle.bottom();
                }

                // Scroll the widget
                offset_x = std::round(offset_x + delta_x);
                offset_y = std::round(offset_y + delta_y);
            }

            // There may be recursive scroll view, and they all need to move until the rectangle is visible.
            if (auto* p = parent()) {
                p->scroll_to_show(layout().to_parent * translate2(delta_x, delta_y) * to_show);
            }

        } else {
            return super::scroll_to_show(to_show);
        }
    }
    /// @endprivatesection
private:
    box_constraints _content_constraints;
    box_shape _content_shape;
    std::unique_ptr<widget> _content;
    margins _padding;

    callback<void(float)> _content_width_cbt;
    callback<void(float)> _content_height_cbt;
    callback<void(float)> _aperture_width_cbt;
    callback<void(float)> _aperture_height_cbt;
    callback<void(float)> _offset_x_cbt;
    callback<void(float)> _offset_y_cbt;
};

} // namespace v1
} // namespace hi::v1
