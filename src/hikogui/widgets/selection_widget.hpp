// Copyright Take Vos 2020-2022.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

/** @file widgets/selection_widget.hpp Defines selection_widget.
 * @ingroup widgets
 */

#pragma once

#include "widget.hpp"
#include "label_widget.hpp"
#include "overlay_widget.hpp"
#include "scroll_widget.hpp"
#include "grid_widget.hpp"
#include "radio_widget.hpp"
#include "selection_delegate.hpp"
#include "../observer/observer.hpp"
#include "../macros.hpp"
#include <memory>
#include <string>
#include <array>
#include <optional>
#include <future>
#include <coroutine>

hi_export_module(hikogui.widgets.selection_widget);

hi_export namespace hi {
inline namespace v1 {

/** A graphical control element that allows the user to choose only one of a
 * predefined set of mutually exclusive options.
 *
 * @image html selection_widget.gif
 *
 * The following example creates a selection widget with three options.
 * which will monitor and modify `value` to display the options from
 * the `option_list`. At application startup, value is zero and none
 * of the options is selected:
 *
 * @snippet widgets/selection_example_impl.cpp Create selection
 *
 * @ingroup widgets
 */
class selection_widget : public widget {
public:
    using super = widget;
    using delegate_type = selection_delegate;

    std::shared_ptr<delegate_type> delegate;

    observer<label> off_label = txt("N/A");
    observer<label> current_label = label{};

    template<typename... Args>
    [[nodiscard]] static std::shared_ptr<delegate_type> make_default_delegate(Args&&... args)
        requires requires { make_shared_ctad<default_selection_delegate>(std::forward<Args>(args)...); }
    {
        return make_shared_ctad<default_selection_delegate>(std::forward<Args>(args)...);
    }

    ~selection_widget()
    {
        delegate->deinit(this);
    }

    /** Construct a selection widget with a delegate.
     *
     * @param parent The owner of the selection widget.
     * @param delegate The delegate which will control the selection widget.
     */
    selection_widget(std::shared_ptr<delegate_type> delegate) noexcept : super(), delegate(std::move(delegate))
    {
        _current_label_widget = std::make_unique<label_widget>(current_label);
        _current_label_widget->set_parent(this);

        _off_label_widget = std::make_unique<label_widget>(off_label);
        _off_label_widget->set_parent(this);

        _overlay_widget = std::make_unique<overlay_widget>();
        _overlay_widget->set_parent(this);

        _scroll_widget = &_overlay_widget->emplace<vertical_scroll_widget>();
        _grid_widget = &_scroll_widget->emplace<grid_widget>();

        _off_label_cbt = this->off_label.subscribe([&](auto...) {
            ++global_counter<"selection_widget:off_label:constrain">;
            request_reconstrain();
        });

        _delegate_options_cbt = this->delegate->subscribe_on_options(
            this,
            [&] {
                update_options();
            },
            callback_flags::main);
        _delegate_options_cbt();

        _delegate_value_cbt = this->delegate->subscribe_on_value(
            this,
            [&] {
                update_value();
            },
            callback_flags::main);
        _delegate_value_cbt();

        hi_axiom_not_null(this->delegate);
        this->delegate->init(this);

        style.set_name("selection");
    }

    /** Construct a selection widget which will monitor an option list and a
     * value.
     *
     * @param parent The owner of the selection widget.
     * @param value The value or observer value to monitor.
     * @param option_list An vector or an observer vector of pairs of keys and
     *                    labels. The keys are of the same type as the @a value.
     *                    The labels are of type `label`.
     */
    template<typename Value, forward_of<observer<std::vector<std::pair<observer_decay_t<Value>, label>>>> OptionList>
    selection_widget(Value&& value, OptionList&& option_list) noexcept
        requires requires { make_default_delegate(std::forward<Value>(value), std::forward<OptionList>(option_list)); }
        : selection_widget(make_default_delegate(std::forward<Value>(value), std::forward<OptionList>(option_list)))
    {
    }

    /// @privatesection
    [[nodiscard]] generator<widget_intf&> children(bool include_invisible) const noexcept override
    {
        if (_overlay_state != overlay_state_type::closed or include_invisible) {
            co_yield *_overlay_widget;
        }
        if (_has_current_label or include_invisible) {
            co_yield *_current_label_widget;
        }
        if (not _has_current_label or include_invisible) {
            co_yield *_off_label_widget;
        }
    }

    [[nodiscard]] box_constraints update_constraints() noexcept override
    {
        hi_assert_not_null(_off_label_widget);
        hi_assert_not_null(_current_label_widget);
        hi_assert_not_null(_overlay_widget);

        _off_label_constraints = _off_label_widget->update_constraints();
        _current_label_constraints = _current_label_widget->update_constraints();
        _overlay_constraints = _overlay_widget->update_constraints();

        // Make it so that the scroll widget can scroll vertically.
        // XXX: This is a hack, the scroll widget should be able to calculate its own constraints.
        _scroll_widget->minimum->height() = style.height_px;

        auto const chevron_size = extent2{style.width_px, 0.0f};

        auto const overlay_minimum = extent2{_overlay_constraints.minimum.width(), 0.0f};
        auto const overlay_preferred = extent2{_overlay_constraints.preferred.width(), 0.0f};
        auto const overlay_maximum = extent2{_overlay_constraints.maximum.width(), 0.0f};

        auto const content_minimum = max(_off_label_constraints.minimum, _current_label_constraints.minimum, overlay_minimum);
        auto const content_preferred =
            max(_off_label_constraints.preferred, _current_label_constraints.preferred, overlay_preferred);
        auto const content_maximum = max(_off_label_constraints.maximum, _current_label_constraints.maximum, overlay_maximum);
        _content_padding = max(_off_label_constraints.margins, _current_label_constraints.margins, style.padding_px);
        auto const content_baseline = max(_off_label_constraints.baseline, _current_label_constraints.baseline);

        auto r = box_constraints{};
        r.minimum = content_minimum + chevron_size + _content_padding;
        r.preferred = content_preferred + chevron_size + _content_padding;
        r.maximum = content_maximum + chevron_size + _content_padding;
        r.margins = style.margins_px;
        r.baseline = embed(content_baseline, unit::pixels(_content_padding.bottom()), unit::pixels(_content_padding.top()));
        hi_axiom(r.holds_invariant());
        return r;
    }

    void set_layout(widget_layout const& context) noexcept override
    {
        super::set_layout(context);

        _chevron_box_rectangle = [&] {
            if (os_settings::left_to_right()) {
                return aarectangle{0.0f, 0.0f, style.width_px, context.height()};
            } else {
                return aarectangle{point2{context.right() - style.width_px, 0.0f}, point2{context.right(), context.top()}};
            }
        }();

        auto const content_rectangle = [&] {
            if (os_settings::left_to_right()) {
                return aarectangle{
                    point2{_chevron_box_rectangle.right() + _content_padding.left(), _content_padding.bottom()},
                    point2{context.right() - _content_padding.right(), context.top() - _content_padding.top()}};
            } else {
                return aarectangle{
                    point2{_content_padding.left(), _content_padding.bottom()},
                    point2{_chevron_box_rectangle.left() - _content_padding.right(), context.top() - _content_padding.top()}};
            }
        }();

        auto const content_shape = box_shape{
            content_rectangle,
            lift(context.baseline(), unit::pixels(_content_padding.bottom()), unit::pixels(_content_padding.top()))};

        _chevron_glyph = find_glyph(elusive_icon::ChevronUp);
        auto const chevron_glyph_bbox = _chevron_glyph.front_glyph_metrics().bounding_rectangle * style.font_size_px;
        _chevron_rectangle = align(_chevron_box_rectangle, chevron_glyph_bbox, alignment::middle_center());

        // The overlay itself will make sure the overlay fits the window, so we give the preferred size and position
        // from the point of view of the selection widget.
        // The overlay should start on the same left edge as the selection box and the same width.
        // The height of the overlay should be the maximum height, which will show all the options.
        auto const overlay_width = std::clamp(
            context.width() - style.width_px, _overlay_constraints.minimum.width(), _overlay_constraints.maximum.width());
        auto const overlay_height = _overlay_constraints.preferred.height();
        auto const overlay_x = os_settings::left_to_right() ? style.width_px : context.width() - style.width_px - overlay_width;
        auto const overlay_y = std::round((context.height() - overlay_height) / 2.0f);
        auto const overlay_rectangle_request = aarectangle{overlay_x, overlay_y, overlay_width, overlay_height};
        auto const overlay_rectangle = make_overlay_rectangle(overlay_rectangle_request);
        _overlay_widget->set_layout(context.transform(box_shape{overlay_rectangle}, transform_command::overlay));

        _off_label_widget->set_layout(context.transform(content_shape));
        _current_label_widget->set_layout(context.transform(content_shape));
    }

    void draw(draw_context const& context) const noexcept override
    {
        const_cast<selection_widget*>(this)->animate_overlay(context.display_time_point);

        if (overlaps(context, layout())) {
            draw_outline(context);
            draw_chevron_box(context);
            draw_chevron(context);
        }

        return super::draw(context);
    }

    bool handle_event(gui_event const& event) noexcept override
    {
        switch (event.type()) {
        case gui_event_type::mouse_up:
            if (enabled() and not delegate->empty(this) and layout().rectangle().contains(event.mouse().position)) {
                return handle_event(gui_event_type::gui_activate);
            }
            return true;

        case gui_event_type::gui_activate_next:
            // Handle gui_active_next so that the next widget will NOT get keyboard focus.
            // The previously selected item needs the get keyboard focus instead.
        case gui_event_type::gui_activate:
            if (enabled() and not delegate->empty(this)) {
                switch (_overlay_state) {
                case overlay_state_type::closed:
                    open_overlay();
                    break;
                case overlay_state_type::open:
                    close_overlay();
                    break;
                case overlay_state_type::closing:
                    break;
                default:
                    std::unreachable();
                }
            }
            ++global_counter<"selection_widget:gui_activate:relayout">;
            request_relayout();
            return true;

        case gui_event_type::gui_cancel:
            close_overlay();
            return true;

        default:;
        }

        return super::handle_event(event);
    }

    [[nodiscard]] hitbox hitbox_test(point2 position) const noexcept override
    {
        hi_axiom(loop::main().on_thread());

        if (not enabled()) {
            return {};
        }

        auto r = hitbox{};

        if (_overlay_state == overlay_state_type::open) {
            r = _overlay_widget->hitbox_test_from_parent(position);
        }

        if (layout().contains(position)) {
            r = std::max(
                r, hitbox{id(), layout().elevation, not delegate->empty(this) ? hitbox_type::button : hitbox_type::_default});
        }

        return r;
    }

    [[nodiscard]] bool accepts_keyboard_focus(keyboard_focus_group group) const noexcept override
    {
        hi_axiom(loop::main().on_thread());
        return enabled() and to_bool(group & hi::keyboard_focus_group::normal) and not delegate->empty(this);
    }

    /// @endprivatesection
private:
    enum class overlay_state_type { open, closing, closed };

    constexpr static std::chrono::nanoseconds _overlay_close_delay = std::chrono::milliseconds(200);

    overlay_state_type _overlay_state = overlay_state_type::closed;
    utc_nanoseconds _overlay_close_start = {};

    bool _notification_from_delegate = true;

    margins _content_padding;

    bool _has_current_label = false;
    std::unique_ptr<label_widget> _current_label_widget;
    box_constraints _current_label_constraints;

    std::unique_ptr<label_widget> _off_label_widget;
    box_constraints _off_label_constraints;

    aarectangle _chevron_box_rectangle;

    font_glyph_ids _chevron_glyph;
    aarectangle _chevron_rectangle;

    std::unique_ptr<overlay_widget> _overlay_widget;
    box_constraints _overlay_constraints;

    vertical_scroll_widget* _scroll_widget = nullptr;
    grid_widget* _grid_widget = nullptr;

    callback<void()> _delegate_options_cbt;
    callback<void()> _delegate_value_cbt;
    callback<void(label)> _off_label_cbt;

    void open_overlay() noexcept
    {
        hi_axiom(loop::main().on_thread());

        if (auto focus_id = delegate->keyboard_focus_id(this)) {
            _overlay_state = overlay_state_type::open;
            send_to_window(gui_event::window_set_keyboard_target(*focus_id, keyboard_focus_group::menu));
            request_redraw_window();
        }
    }

    void close_overlay() noexcept
    {
        hi_axiom(loop::main().on_thread());

        if (_overlay_state == overlay_state_type::open) {
            _overlay_state = overlay_state_type::closing;
            _overlay_close_start = std::chrono::utc_clock::now();
            request_redraw_window();
        }
    }

    void force_close_overlay() noexcept
    {
        if (_overlay_state != overlay_state_type::closed) {
            _overlay_state = overlay_state_type::closed;
            request_redraw_window();
        }
    }

    void animate_overlay(utc_nanoseconds display_time_point) noexcept
    {
        hi_axiom(loop::main().on_thread());

        switch (_overlay_state) {
        case overlay_state_type::open:
            break;
        case overlay_state_type::closing:
            if (display_time_point >= _overlay_close_start + _overlay_close_delay) {
                force_close_overlay();
            } else {
                request_redraw_window();
            }
            break;
        case overlay_state_type::closed:
            break;
        default:
            hi_no_default();
        }
    }

    void update_options() noexcept
    {
        _grid_widget->clear();
        for (auto i = 0_uz; i != delegate->size(this); ++i) {
            _grid_widget->push_bottom(delegate->make_option_widget(_grid_widget, i));
        }

        ++global_counter<"selection_widget:update_options:constrain">;
        request_reconstrain();
    }

    void update_value() noexcept
    {
        if (auto selected_label = delegate->selected_label(this)) {
            _has_current_label = true;
            current_label = *selected_label;

        } else {
            _has_current_label = false;
        }

        close_overlay();
    }

    void draw_outline(draw_context const& context) const noexcept
    {
        context.draw_box(
            layout(),
            layout().rectangle(),
            style.background_color,
            style.border_color,
            style.border_width_px,
            border_side::inside,
            style.border_radius_px);
    }

    void draw_chevron_box(draw_context const& context) const noexcept
    {
        auto const corner_radii = [&] {
            if (os_settings::left_to_right()) {
                return hi::corner_radii(style.border_bottom_left_radius_px, 0.0f, style.border_top_left_radius_px, 0.0f);
            } else {
                return hi::corner_radii(0.0f, style.border_bottom_right_radius_px, 0.0f, style.border_top_right_radius_px);
            }
        }();

        context.draw_box(layout(), translate_z(0.1f) * _chevron_box_rectangle, style.border_color, corner_radii);
    }

    void draw_chevron(draw_context const& context) const noexcept
    {
        context.draw_glyph(layout(), translate_z(0.2f) * _chevron_rectangle, _chevron_glyph, style.background_color);
    }
};

} // namespace v1
} // namespace hi::v1
