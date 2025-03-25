// Copyright Take Vos 2021-2022.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

/** @file widgets/grid_widget.hpp Defines grid_widget.
 * @ingroup widgets
 */

#pragma once

#include "widget.hpp"
#include "../layout/layout.hpp"
#include "../macros.hpp"
#include <memory>
#include <coroutine>

hi_export_module(hikogui.widgets.grid_widget);

hi_export namespace hi {
inline namespace v1 {

/** A GUI widget that lays out child-widgets in a grid with variable sized cells.
 * @ingroup widgets
 *
 * The grid widget lays out child widgets in a grid pattern. Each child widget
 * occupies a single cell, which belongs into a single column and a single row.
 *
 * Columns are laid out from left to right, and rows from top to bottom. The row
 * and columns number may be specified as integers, or using an spreadsheet-like
 * cell-address:
 *  - `grid_widget::emplace<T>(std::size_t column_nr, std::size_t row_nr, ...)`
 *  - `grid_widget::emplace<T>(std::string address, ...)`
 *
 * The grid widget will calculate the size of each row and column based on the
 * minimum, preferred and maximum size of each child widget contained in them.
 * Margins are also taken into account in the spacing between columns and
 * between rows.
 *
 * When laid out, each child is sized to where it will occupy the full width and
 * height of each cell.
 *
 * @image html grid_widget.png
 */
class grid_widget : public widget {
public:
    using super = widget;

    ~grid_widget() {}

    /** Constructs an empty grid widget.
     *
     * @param parent The parent widget.
     */
    grid_widget() noexcept : widget()
    {
        style.set_name("grid-view");
    }

    /* Add a widget to the grid.
     */
    void insert(
        std::size_t first_column,
        std::size_t first_row,
        std::size_t last_column,
        std::size_t last_row,
        std::unique_ptr<widget> widget) noexcept
    {
        hi_axiom(loop::main().on_thread());
        hi_axiom(first_column < last_column);
        hi_axiom(first_row < last_row);

        if (_grid.cell_in_use(first_column, first_row, last_column, last_row)) {
            hi_log_fatal("cell ({},{}) of grid_widget is already in use", first_column, first_row);
        }

        widget->set_parent(this);
        _grid.add_cell(first_column, first_row, last_column, last_row, std::move(widget));
        hi_log_info("grid_widget::insert({}, {}, {}, {})", first_column, first_row, last_column, last_row);

        ++global_counter<"grid_widget:insert:constrain">;
        request_reconstrain();
    }

    /** Insert a widget to the front of the grid.
     *
     * All the widgets currently on the grid are moved 1 backward
     * and the new widget is added to the front-top cell.
     *
     * In left-to-right mode 'front' means 'left'.
     *
     * @param widget The widget to take ownership of
     * @return A reference to the widget being added.
     */
    void push_front(std::unique_ptr<widget> widget) noexcept
    {
        for (auto& cell : _grid) {
            ++cell.first_column;
            ++cell.last_column;
        }
        insert(0, 0, 1, 1, std::move(widget));
    }

    /** Insert a widget to the back of the grid.
     *
     * The widget is places at back-top.
     *
     * In left-to-right mode 'front' means 'left'.
     *
     * @param widget The widget to take ownership of
     * @return A reference to the widget being added.
     */
    void push_back(std::unique_ptr<widget> widget) noexcept
    {
        auto it = std::max_element(_grid.begin(), _grid.end(), [](auto const& a, auto const& b) {
            return a.last_column < b.last_column;
        });

        if (it == _grid.end()) {
            insert(0, 0, 1, 1, std::move(widget));
        } else {
            insert(it->last_column, 0, it->last_column + 1, 1, std::move(widget));
        }
    }

    /** Insert a widget to the top of the grid.
     *
     * All the widgets currently on the grid are moved 1 lower
     * and the new widget is added to the front-top cell.
     *
     * @param widget The widget to take ownership of
     * @return A reference to the widget being added.
     */
    void push_top(std::unique_ptr<widget> widget) noexcept
    {
        for (auto& cell : _grid) {
            ++cell.first_row;
            ++cell.last_row;
        }
        insert(0, 0, 1, 1, std::move(widget));
    }

    /** Insert a widget to the bottom of the grid.
     *
     * The widget is places at front-bottom.
     *
     * @param widget The widget to take ownership of
     * @return A reference to the widget being added.
     */
    void push_bottom(std::unique_ptr<widget> widget) noexcept
    {
        auto it = std::max_element(_grid.begin(), _grid.end(), [](auto const& a, auto const& b) {
            return a.last_row < b.last_row;
        });

        if (it == _grid.end()) {
            insert(0, 0, 1, 1, std::move(widget));
        } else {
            insert(0, it->last_row, 1, it->last_row + 1, std::move(widget));
        }
    }

    /** Add a widget directly to this grid-widget.
     *
     * @tparam Widget The type of the widget to be constructed.
     * @param first_column The first (left-most) column to place the widget in.
     * @param first_row The first (top-most) row to place the widget in.
     * @param last_column One beyond the last column to place the widget in.
     * @param last_row One beyond the last row to place the widget in.
     * @param args The arguments passed to the constructor of the widget.
     * @return A reference to the widget that was created.
     */
    template<typename Widget, typename... Args>
    Widget&
    emplace(std::size_t first_column, std::size_t first_row, std::size_t last_column, std::size_t last_row, Args&&... args)
    {
        hi_axiom(first_column < last_column);
        hi_axiom(first_row < last_row);
        auto tmp = std::make_unique<Widget>(std::forward<Args>(args)...);
        auto& ref = *tmp;
        insert(first_column, first_row, last_column, last_row, std::move(tmp));
        return ref;
    }

    /** Add a widget directly to this grid-widget.
     *
     * @tparam Widget The type of the widget to be constructed.
     * @param column The zero-based index from left-to-right.
     * @param row The zero-based index from top-to-bottom.
     * @param args The arguments passed to the constructor of the widget.
     * @return A reference to the widget that was created.
     */
    template<typename Widget, typename... Args>
    Widget& emplace(std::size_t column, std::size_t row, Args&&... args)
    {
        return emplace<Widget>(column, row, column + 1, row + 1, std::forward<Args>(args)...);
    }

    /** Add a widget directly to this grid-widget.
     *
     * @tparam Widget The type of the widget to be constructed.
     * @param address The spreadsheet-like address of the cell,
     *                see `parse_spreadsheet_address()`.
     * @param args The arguments passed to the constructor of the widget.
     * @return A reference to the widget that was created.
     */
    template<typename Widget, typename... Args>
    Widget& emplace(std::string_view address, Args&&... args)
    {
        auto const [column_first, row_first, column_last, row_last] = parse_spreadsheet_range(address);
        return emplace<Widget>(column_first, row_first, column_last, row_last, std::forward<Args>(args)...);
    }

    /** Emplace a widget to the front.
     *
     * All the widgets currently on the grid are moved 1 backward
     * and the new widget is added to the front-top cell.
     *
     * In left-to-right mode 'front' means 'left'.
     *
     * @tparam Widget The type of the widget to be constructed.
     * @param args The arguments passed to the constructor of the widget.
     * @return A reference to the widget that was created.
     */
    template<typename Widget, typename... Args>
    Widget& emplace_front(Args&&... args)
    {
        return static_cast<Widget&>(push_front(std::make_unique<Widget>(this, std::forward<Args>(args)...)));
    }

    /** Emplace a widget to the back.
     *
     * The widget is places at back-top.
     *
     * In left-to-right mode 'front' means 'left'.
     *
     * @tparam Widget The type of the widget to be constructed.
     * @param args The arguments passed to the constructor of the widget.
     * @return A reference to the widget that was created.
     */
    template<typename Widget, typename... Args>
    Widget& emplace_back(Args&&... args)
    {
        return static_cast<Widget&>(push_back(std::make_unique<Widget>(this, std::forward<Args>(args)...)));
    }

    /** Emplace a widget to the back.
     *
     * All the widgets currently on the grid are moved 1 lower
     * and the new widget is added to the front-top cell.
     *
     * @tparam Widget The type of the widget to be constructed.
     * @param args The arguments passed to the constructor of the widget.
     * @return A reference to the widget that was created.
     */
    template<typename Widget, typename... Args>
    Widget& emplace_top(Args&&... args)
    {
        return static_cast<Widget&>(push_top(std::make_unique<Widget>(this, std::forward<Args>(args)...)));
    }

    /** Emplace a widget to the back.
     *
     * The widget is places at front-bottom.
     *
     * @tparam Widget The type of the widget to be constructed.
     * @param args The arguments passed to the constructor of the widget.
     * @return A reference to the widget that was created.
     */
    template<typename Widget, typename... Args>
    Widget& emplace_bottom(Args&&... args)
    {
        auto tmp = std::make_unique<Widget>(std::forward<Args>(args)...);
        auto& ref = *tmp;
        push_bottom(std::move(tmp));
        return ref;
    }

    /** Remove all child widgets.
     */
    void clear() noexcept
    {
        _grid.clear();
    }

    /// @privatesection
    [[nodiscard]] generator<widget_intf&> children(bool include_invisible) const noexcept override
    {
        for (auto const& cell : _grid) {
            co_yield *cell.value;
        }
    }

    [[nodiscard]] box_constraints update_constraints() noexcept override
    {
        for (auto& cell : _grid) {
            cell.set_constraints(cell.value->update_constraints());
        }

        return _grid.constraints(os_settings::left_to_right(), style.vertical_alignment);
    }

    void set_layout(widget_layout const& context) noexcept override
    {
        super::set_layout(context);

        _grid.set_layout(context.shape);
        for (auto const& cell : _grid) {
            cell.value->set_layout(context.transform(cell.shape, transform_command::level));
        }
    }

    [[nodiscard]] hitbox hitbox_test(point2 position) const noexcept override
    {
        hi_axiom(loop::main().on_thread());

        auto r = hitbox{};
        for (auto const& cell : _grid) {
            r = cell.value->hitbox_test_from_parent(position, r);
        }
        return r;
    }
    /// @endprivatesection
private:
    grid_layout<std::unique_ptr<widget>> _grid;
};

} // namespace v1
} // namespace hi::v1
