// Copyright Take Vos 2021-2022.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

/** @file widgets/scroll_widget.hpp Defines scroll_widget.
 * @ingroup widgets
 */

module;
#include "../macros.hpp"

#include <coroutine>

export module hikogui_widgets_scroll_widget;
import hikogui_coroutine;
import hikogui_geometry;
import hikogui_layout;
import hikogui_widgets_scroll_aperture_widget;
import hikogui_widgets_scroll_bar_widget;
import hikogui_widgets_widget;

export namespace hi { inline namespace v1 {

/** The scroll widget allows a content widget to be shown in less space than is
 * required.
 *
 *  The user can then show the part of the content widget by using the
 * scroll-bar widgets which are provided by the scroll widget.
 *
 * The size of the scroll widget is based on the `widget::minimum_size()`,
 * `widget::preferred_size()` and `widget::maximum_size()` of the content widget
 * together with the space needed for the scroll bars.
 *
 * In the directions that are allowed for scrolling the minimum size of the
 * scroll widget is adjusted to be much smaller, up to the smallest size of the
 * scroll-bar widget in that direction.
 *
 * Scroll-bars are automatically added when the actual size of the scroll widget
 * is smaller than the content, this will happen even if the template parameters
 * given did not allow scrolling in that direction. This is useful behavior when
 * the scroll widget is part of an overlay widget which was unable to size to
 * the minimum size requested.
 *
 * @image html scroll_widget.png
 *
 * @ingroup widgets
 * @tparam Axis the axis that the content may be scrolled. Allowed values are
 *              `axis::horizontal`, `axis::vertical` or `axis::both`.
 */
template<axis Axis = axis::both>
class scroll_widget final : public widget {
public:
    using super = widget;
    using horizontal_scroll_bar_type = scroll_bar_widget<axis::horizontal>;
    using vertical_scroll_bar_type = scroll_bar_widget<axis::vertical>;

    constexpr static hi::axis axis = Axis;

    ~scroll_widget() {}

    /** Constructs an empty scroll widget.
     *
     * @param parent The parent widget.
     */
    scroll_widget(not_null<widget_intf const *> parent) noexcept : super(parent)
    {
        hi_axiom(loop::main().on_thread());

        auto aperture = std::make_unique<scroll_aperture_widget>(this);
        auto horizontal_scroll_bar = std::make_unique<horizontal_scroll_bar_type>(
            this, aperture->content_width, aperture->aperture_width, aperture->offset_x);
        auto vertical_scroll_bar = std::make_unique<vertical_scroll_bar_type>(
            this, aperture->content_height, aperture->aperture_height, aperture->offset_y);

        if (to_bool(axis & axis::horizontal)) {
            minimum.copy()->width() = 0;
        } else {
            horizontal_scroll_bar->mode = widget_mode::collapse;
        }

        if (to_bool(axis & axis::vertical)) {
            minimum.copy()->height() = 0;
        } else {
            vertical_scroll_bar->mode = widget_mode::collapse;
        }

        _aperture = aperture.get();
        _horizontal_scroll_bar = horizontal_scroll_bar.get();
        _vertical_scroll_bar = vertical_scroll_bar.get();

        _grid.add_cell(0, 0, std::move(aperture));
        _grid.add_cell(1, 0, std::move(vertical_scroll_bar));
        _grid.add_cell(0, 1, std::move(horizontal_scroll_bar));
    }

    /** Add a content widget directly to this scroll widget.
     *
     * This widget is added as the content widget.
     *
     * @pre No content widgets have been added before.
     * @tparam Widget The type of the widget to be constructed.
     * @param args The arguments passed to the constructor of the widget.
     * @return A reference to the widget that was created.
     */
    template<typename Widget, typename... Args>
    Widget& emplace(Args&&...args) noexcept
    {
        return _aperture->emplace<Widget>(std::forward<Args>(args)...);
    }

    /// @privatesection
    [[nodiscard]] generator<widget_intf &> children(bool include_invisible) noexcept override
    {
        co_yield *_aperture;
        co_yield *_vertical_scroll_bar;
        co_yield *_horizontal_scroll_bar;
    }

    [[nodiscard]] box_constraints update_constraints() noexcept override
    {
        _layout = {};

        for (auto& cell : _grid) {
            cell.set_constraints(cell.value->update_constraints());
        }
        auto grid_constraints = _grid.constraints(os_settings::left_to_right());
        return grid_constraints.constrain(*minimum, *maximum);
    }

    void set_layout(widget_layout const& context) noexcept override
    {
        if (compare_store(_layout, context)) {
            _grid.set_layout(context.shape, theme().baseline_adjustment());
        }

        for (hilet& cell : _grid) {
            auto shape = cell.shape;

            if (cell.value.get() == _aperture) {
                // This is the content. Move the content slightly when the scroll-bars aren't visible.
                // The grid cells are always ordered in row-major.
                // This the vertical scroll bar is _grid[1] and the horizontal scroll bar is _grid[2].
                if (not _vertical_scroll_bar->visible()) {
                    shape.rectangle = aarectangle{0, shape.y(), _layout.width(), shape.height()};
                }
                if (not _horizontal_scroll_bar->visible()) {
                    shape.rectangle = aarectangle{shape.x(), 0, shape.width(), _layout.height()};
                }
            }

            cell.value->set_layout(context.transform(shape, transform_command::level));
        }
    }

    void draw(draw_context const& context) noexcept override
    {
        if (*mode > widget_mode::invisible) {
            for (hilet& cell : _grid) {
                cell.value->draw(context);
            }
        }
    }

    [[nodiscard]] hitbox hitbox_test(point2 position) const noexcept override
    {
        hi_axiom(loop::main().on_thread());

        if (*mode >= widget_mode::partial) {
            auto r = _aperture->hitbox_test_from_parent(position);
            r = _horizontal_scroll_bar->hitbox_test_from_parent(position, r);
            r = _vertical_scroll_bar->hitbox_test_from_parent(position, r);

            if (layout().contains(position)) {
                r = std::max(r, hitbox{id, _layout.elevation});
            }
            return r;

        } else {
            return {};
        }
    }
    // @endprivatesection
private:
    grid_layout<std::unique_ptr<widget>> _grid;

    scroll_aperture_widget *_aperture;
    horizontal_scroll_bar_type *_horizontal_scroll_bar;
    vertical_scroll_bar_type *_vertical_scroll_bar;
};

/** Vertical scroll widget.
 * A scroll widget that only scrolls vertically.
 *
 * @ingroup widgets
 * @see scroll_widget
 * window gets a signal to resize to its preferred size.
 */
using vertical_scroll_widget = scroll_widget<axis::vertical>;

/** Horizontal scroll widget.
 * A scroll widget that only scrolls horizontally.
 *
 * @ingroup widgets
 * @see scroll_widget
 * window gets a signal to resize to its preferred size.
 */
using horizontal_scroll_widget = scroll_widget<axis::horizontal>;

}} // namespace hi::v1
