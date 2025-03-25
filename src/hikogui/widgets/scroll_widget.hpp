// Copyright Take Vos 2021-2022.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

/** @file widgets/scroll_widget.hpp Defines scroll_widget.
 * @ingroup widgets
 */

#pragma once

#include "widget.hpp"
#include "scroll_bar_widget.hpp"
#include "scroll_aperture_widget.hpp"
#include "../geometry/geometry.hpp"
#include "../layout/layout.hpp"
#include "../macros.hpp"
#include <coroutine>

hi_export_module(hikogui.widgets.scroll_widget);

hi_export namespace hi {
inline namespace v1 {

/** The scroll widget allows a content widget to be shown in less space than is
 * required.
 *
 *  The user can then show the part of the content widget by using the
 * scroll-bar widgets which are provided by the scroll widget.
 *
 * The size of the scroll widget is based on the `widget::minimum_size()`,
 * `widget::preferred_size()` and `widget::maximum_size()` of the content widget
 * together with the space needed for the scrollbars.
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
class scroll_widget : public widget {
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
    scroll_widget() noexcept : super()
    {
        hi_axiom(loop::main().on_thread());

        _aperture = std::make_unique<scroll_aperture_widget>();
        _aperture->set_parent(this);
        _grid.add_cell(0, 0, _aperture.get());

        if constexpr (std::to_underlying(axis & axis::horizontal)) {
            _horizontal_scroll_bar = std::make_unique<horizontal_scroll_bar_type>(
                _aperture->content_width, _aperture->aperture_width, _aperture->offset_x);
            _horizontal_scroll_bar->set_parent(this);
            _grid.add_cell(1, 0, _horizontal_scroll_bar.get());
        }

        if constexpr (std::to_underlying(axis & axis::vertical)) {
            _vertical_scroll_bar = std::make_unique<vertical_scroll_bar_type>(
                _aperture->content_height, _aperture->aperture_height, _aperture->offset_y);
            _vertical_scroll_bar->set_parent(this);
            _grid.add_cell(0, 1, _vertical_scroll_bar.get());
        }

        style.set_name("scroll-view");
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
    Widget& emplace(Args&&... args) noexcept
    {
        return _aperture->emplace<Widget>(std::forward<Args>(args)...);
    }

    /// @privatesection
    [[nodiscard]] generator<widget_intf&> children(bool include_invisible) const noexcept override
    {
        co_yield *_aperture;
        if constexpr (std::to_underlying(axis & axis::vertical)) {
            co_yield *_vertical_scroll_bar;
        }
        if constexpr (std::to_underlying(axis & axis::horizontal)) {
            co_yield *_horizontal_scroll_bar;
        }
    }

    [[nodiscard]] box_constraints update_constraints() noexcept override
    {
        for (auto& cell : _grid) {
            cell.set_constraints(cell.value->update_constraints());
        }
        auto grid_constraints = _grid.constraints(os_settings::left_to_right(), style.vertical_alignment);
        return grid_constraints.constrain(*minimum, *maximum);
    }

    void set_layout(widget_layout const& context) noexcept override
    {
        super::set_layout(context);

        _grid.set_layout(context.shape);

        for (auto const& cell : _grid) {
            auto shape = cell.shape;

            if (cell.value == _aperture.get()) {
                // This is the content. If the scroll bars are not visible make
                // the shape of this cell overlap the entire layout.
                auto x = shape.x();
                auto y = shape.y();
                auto width = shape.width();
                auto height = shape.height();

                if (not std::to_underlying(axis & axis::horizontal) or not _horizontal_scroll_bar->visible()) {
                    y = 0;
                    height = layout().height();
                }
                if (not std::to_underlying(axis & axis::vertical) or not _vertical_scroll_bar->visible()) {
                    x = 0;
                    width = layout().width();
                }

                shape.rectangle = aarectangle{x, y, width, height};
            }

            cell.value->set_layout(context.transform(shape, transform_command::level));
        }
    }

    [[nodiscard]] hitbox hitbox_test(point2 position) const noexcept override
    {
        hi_axiom(loop::main().on_thread());

        if (enabled()) {
            auto r = _aperture->hitbox_test_from_parent(position);
            if constexpr (std::to_underlying(axis & axis::horizontal)) {
                r = _horizontal_scroll_bar->hitbox_test_from_parent(position, r);
            }
            if constexpr (std::to_underlying(axis & axis::vertical)) {
                r = _vertical_scroll_bar->hitbox_test_from_parent(position, r);
            }

            if (layout().contains(position)) {
                r = std::max(r, hitbox{id(), layout().elevation});
            }
            return r;

        } else {
            return {};
        }
    }
    // @endprivatesection
private:
    grid_layout<widget_intf*> _grid;

    std::unique_ptr<scroll_aperture_widget> _aperture;
    std::unique_ptr<horizontal_scroll_bar_type> _horizontal_scroll_bar;
    std::unique_ptr<vertical_scroll_bar_type> _vertical_scroll_bar;
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

} // namespace v1
} // namespace hi::v1
