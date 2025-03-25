// Copyright Take Vos 2020-2022.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

/** @file widgets/scroll_bar_widget.hpp Defines scroll_bar_widget.
 * @ingroup widgets
 */

#pragma once

#include "widget.hpp"
#include "../GUI/GUI.hpp"
#include "../geometry/geometry.hpp"
#include "../observer/observer.hpp"
#include "../utility/utility.hpp"
#include "../macros.hpp"
#include <memory>
#include <string>
#include <array>
#include <optional>
#include <future>

hi_export_module(hikogui.widgets.scroll_bar_widget);

hi_export namespace hi {
inline namespace v1 {

/** Scroll bar widget
 * This widget is used in a pair of a vertical and horizontal scrollbar as
 * a child of the `scroll_widget`. The vertical and horizontal scrollbar are
 * displayed next to the `scroll_aperture_widget` and controls what part of
 * the content is displayed in the aperture.
 *
 * @ingroup widgets
 * @tparam Axis which axis (horizontal or vertical) this scroll bar is used for.
 */
template<axis Axis>
class scroll_bar_widget : public widget {
public:
    using super = widget;

    constexpr static hi::axis axis = Axis;

    observer<float> offset;
    observer<float> aperture;
    observer<float> content;

    template<forward_of<observer<float>> Content, forward_of<observer<float>> Aperture, forward_of<observer<float>> Offset>
    scroll_bar_widget(Content&& content, Aperture&& aperture, Offset&& offset) noexcept :
        widget(),
        content(std::forward<Content>(content)),
        aperture(std::forward<Aperture>(aperture)),
        offset(std::forward<Offset>(offset))
    {
        _content_cbt = this->content.subscribe([&](auto...) {
            ++global_counter<"scroll_bar_widget:content:relayout">;
            request_relayout();
        });
        _aperture_cbt = this->aperture.subscribe([&](auto...) {
            ++global_counter<"scroll_bar_widget:aperture:relayout">;
            request_relayout();
        });
        _offset_cbt = this->offset.subscribe([&](auto...) {
            ++global_counter<"scroll_bar_widget:offset:relayout">;
            request_relayout();
        });

        style.set_name("scroll-bar");
    }

    ~scroll_bar_widget() {}

    [[nodiscard]] box_constraints update_constraints() noexcept override
    {
        // The minimum size is twice the length of the slider, which is twice the theme().size()
        if constexpr (axis == axis::vertical) {
            return {
                extent2{theme().icon_size(), theme().size() * 4},
                extent2{theme().icon_size(), theme().size() * 4},
                extent2{theme().icon_size(), large_number_v<int>}};
        } else {
            return {
                extent2{theme().size() * 4, theme().icon_size()},
                extent2{theme().size() * 4, theme().icon_size()},
                extent2{large_number_v<int>, theme().icon_size()}};
        }
    }

    void set_layout(widget_layout const& context) noexcept override
    {
        super::set_layout(context);

        // Calculate the position of the slider.
        auto const slider_offset = std::round(*offset * travel_vs_hidden_content_ratio());
        if constexpr (axis == axis::vertical) {
            _slider_rectangle = aarectangle{0.0f, slider_offset, context.width(), slider_length()};
        } else {
            _slider_rectangle = aarectangle{slider_offset, 0.0f, slider_length(), context.height()};
        }
    }

    [[nodiscard]] bool visible() const noexcept
    {
        return *aperture < *content;
    }

    void draw(draw_context const& context) const noexcept override
    {
        if (overlaps(context, layout())) {
            draw_rails(context);
            draw_slider(context);
        }

        return super::draw(context);
    }

    hitbox hitbox_test(point2 position) const noexcept override
    {
        hi_axiom(loop::main().on_thread());

        if (enabled() and layout().contains(position) and _slider_rectangle.contains(position)) {
            return {id(), layout().elevation, hitbox_type::scroll_bar};
        } else {
            return {};
        }
    }

    bool handle_event(gui_event const& event) noexcept override
    {
        switch (event.type()) {
        case gui_event_type::mouse_down:
            if (event.mouse().cause.left_button) {
                // Record the original scroll-position before the drag starts.
                _offset_before_drag = *offset;
                return true;
            }
            break;

        case gui_event_type::mouse_drag:
            if (event.mouse().cause.left_button) {
                // The distance the slider has to move relative to the slider position at the
                // start of the drag.
                auto const slider_movement = axis == axis::vertical ? event.drag_delta().y() : event.drag_delta().x();
                auto const content_movement = round_cast<int>(slider_movement * hidden_content_vs_travel_ratio());
                auto const new_offset = _offset_before_drag + content_movement;
                offset = clamp_offset(new_offset);
                return true;
            }
            break;

        default:;
        }

        return super::handle_event(event);
    }

    [[nodiscard]] bool accepts_keyboard_focus(keyboard_focus_group group) const noexcept override
    {
        return false;
    }

    [[nodiscard]] color background_color() const noexcept override
    {
        return theme().fill_color(layout().layer);
    }

    [[nodiscard]] color foreground_color() const noexcept override
    {
        if (phase() == widget_phase::hover) {
            return theme().fill_color(layout().layer + 2);
        } else {
            return theme().fill_color(layout().layer + 1);
        }
    }

private:
    aarectangle _slider_rectangle;

    float _offset_before_drag;

    callback<void(float)> _content_cbt;
    callback<void(float)> _aperture_cbt;
    callback<void(float)> _offset_cbt;

    /** Create a new offset value.
     *
     * Clamp the new offset value by the amount of scrollable distance.
     */
    [[nodiscard]] float clamp_offset(float new_offset) const noexcept
    {
        auto const scrollable_distance = std::max(0.0f, *content - *aperture);
        return std::clamp(new_offset, 0.0f, scrollable_distance);
    }

    [[nodiscard]] float rail_length() const noexcept
    {
        hi_axiom(loop::main().on_thread());
        return axis == axis::vertical ? layout().height() : layout().width();
    }

    [[nodiscard]] float slider_length() const noexcept
    {
        hi_axiom(loop::main().on_thread());

        auto const preferred_length = [&] {
            if (*content == 0.0f) {
                return rail_length();
            } else {
                return std::round(*aperture * rail_length() / *content);
            }
        }();

        return std::clamp(preferred_length, theme().size() * 2.0f, rail_length());
    }

    /** The amount of travel that the slider can make.
     */
    [[nodiscard]] float slider_travel_range() const noexcept
    {
        hi_axiom(loop::main().on_thread());
        return rail_length() - slider_length();
    }

    /** The amount of content hidden from view.
     */
    [[nodiscard]] float hidden_content() const noexcept
    {
        hi_axiom(loop::main().on_thread());
        return *content - *aperture;
    }

    /** Get the ratio of the hidden content vs the slider travel range.
     * We can not simply take the ratio of content vs rail length, because
     * there is a minimum slider length.
     */
    [[nodiscard]] float hidden_content_vs_travel_ratio() const noexcept
    {
        hi_axiom(loop::main().on_thread());

        auto const _slider_travel_range = slider_travel_range();
        return _slider_travel_range != 0 ? std::round(hidden_content() / _slider_travel_range) : 0.0f;
    }

    /** Get the ratio of the slider travel range vs hidden content.
     * We can not simply take the ratio of content vs rail length, because
     * there is a minimum slider length.
     */
    [[nodiscard]] float travel_vs_hidden_content_ratio() const noexcept
    {
        hi_axiom(loop::main().on_thread());

        auto const _hidden_content = hidden_content();
        return _hidden_content != 0 ? slider_travel_range() / _hidden_content : 0.0f;
    }

    void draw_rails(draw_context const& context) const noexcept
    {
        auto const corner_radii =
            axis == axis::vertical ? hi::corner_radii{layout().width() * 0.5f} : hi::corner_radii{layout().height() * 0.5f};
        context.draw_box(layout(), layout().rectangle(), background_color(), corner_radii);
    }

    void draw_slider(draw_context const& context) const noexcept
    {
        auto const corner_radii = axis == axis::vertical ? hi::corner_radii{_slider_rectangle.width() / 2.0f} :
                                                           hi::corner_radii{_slider_rectangle.height() / 2.0f};

        context.draw_box(layout(), translate_z(0.1f) * _slider_rectangle, foreground_color(), corner_radii);
    }
};

} // namespace v1
} // namespace hi::v1
