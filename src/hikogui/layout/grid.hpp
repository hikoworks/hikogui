// Copyright Take Vos 2024.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "constraints.hpp"
#include "../units/units.hpp"
#include "../utility/utility.hpp"
#include "../macros.hpp"

hi_export_module(hikogui.layout : grid);

hi_export namespace hi::inline v1::layout {
namespace detail {

template<typename T>
struct grid_cell {
    using value_type = T;

    size_t row_nr = 0;
    size_t column_nr = 0;
    size_t row_span = 1;
    size_t column_span = 1;

    layout_constraints vertical_constraints = {};
    layout_constraints horizontal_constraints = {};
    value_type value = {};
};

template<typename T>
class grid_cells : public std::vector<grid_cell<T>> {
public:
    using super = std::vector<grid_cell<T>>;
    using super::super;

    [[nodiscard]] size_t num_rows() const noexcept
    {
        size_t r = 0;
        for (auto const& cell : *this) {
            r = std::max(r, cell.row_nr + cell.row_span);
        }
        return r;
    }

    [[nodiscard]] size_t num_columns() const noexcept
    {
        size_t r = 0;
        for (auto const& cell : *this) {
            r = std::max(r, cell.column_nr + cell.column_span);
        }
        return r;
    }
};

struct grid_cell_aggregate {
    unit::pixels_f minimum = unit::pixels(0.0f);
    unit::pixels_f preferred = unit::pixels(0.0f);
    float weight = 0.0f;
    unit::pixels_f margin_before = unit::pixels(0.0f);
    unit::pixels_f margin_after = unit::pixels(0.0f);

    unit::pixels_f extent = unit::pixels(0.0f);
    unit::pixels_f position = unit::pixels(0.0f);
};

class grid_header : public std::vector<grid_cell_aggregate> {
public:
    using super = std::vector<grid_cell_aggregate>;
    using super::super;

    [[nodiscard]] unit::pixels_f margin_before() const noexcept
    {
        if (empty()) {
            return unit::pixels(0.0f);
        } else {
            return front().margin_before;
        }
    }

    [[nodiscard]] unit::pixels_f margin_after() const noexcept
    {
        if (empty()) {
            return unit::pixels(0.0f);
        } else {
            return back().margin_after;
        }
    }

    [[nodiscard]] layout_constraints constraints(size_t first, size_t last, bool mirror = false) const
    {
        assert(first <= last);
        assert(last <= size());

        auto r = layout_constraints{};

        if (first != last) {
            r.minimum = (*this)[first].minimum;
            r.preferred = (*this)[first].preferred;
            r.weight = (*this)[first].weight;
            for (auto row_nr = first + 1; row_nr != last; ++row_nr) {
                auto& row = (*this)[row_nr];
                auto& previous_row = (*this)[row_nr - 1];

                auto const margin = std::max(previous_row.margin_after, row.margin_before);
                r.minimum += row.minimum + margin;
                r.preferred += row.preferred + margin;
                r.weight += row.weight;
            }
        }

        r.margin_before = margin_before();
        r.margin_after = margin_after();
        if (mirror) {
            std::swap(r.margin_before, r.margin_after);
        }
        return r;
    }

    [[nodiscard]] layout_constraints constraints(bool mirror = false) const
    {
        return constraints(0, size(), mirror);
    }

    [[nodiscard]] unit::pixels_f extent(size_t first, size_t last) const
    {
        assert(first <= last);
        assert(last <= size());

        auto r = unit::pixels(0.0f);

        if (first != last) {
            r = (*this)[first].extent;
            for (auto row_nr = first + 1; row_nr != last; ++row_nr) {
                auto& row = (*this)[row_nr];
                auto& previous_row = (*this)[row_nr - 1];

                r += row.extent + std::max(previous_row.margin_after, row.margin_before);
            }
        }
        return r;
    }

    [[nodiscard]] auto extent() const
    {
        return extent(0, size());
    }

    /** Distribute the extra weight and size of multi-span cells.
     *
     * This function will distribute the extra weight and size of multi-span
     * cells to the rows or columns that the cell spans. This distributes
     * the weight and size at the same time, so you should call this function
     * first with only the weight, followed by a call with only the size.
     *
     * @param first The index of the first row or column.
     * @param last The index after the last row or column.
     * @param new_minimum The new minimum size of the rows or columns.
     * @param new_preferred The new preferred size of the rows or columns.
     * @param new_weight The new weight of the rows or columns.
     * @param mirror If true the grid is vertically or horizontally mirrored.
     *               Used for right-to-left layout.
     * @return The constraints of the rows or columns.
     */
    layout_constraints distribute_constraints(
        size_t first,
        size_t last,
        unit::pixels_f new_minimum,
        unit::pixels_f new_preferred,
        float new_weight,
        bool mirror = false)
    {
        assert(first <= last);
        assert(last <= size());
        assert(new_minimum >= unit::pixels(0.0f));
        assert(new_minimum <= new_preferred);
        assert(new_weight >= 0.0f);

        auto r = constraints(first, last, mirror);
        auto const extra_minimum = std::max(unit::pixels(0.0f), new_minimum - r.minimum);
        auto const extra_preferred = std::max(unit::pixels(0.0f), new_preferred - r.preferred);
        auto const extra_weight = std::max(0.0f, new_weight - r.weight);

        auto const total_weight = r.weight + extra_weight;

        auto const has_extra_minium = extra_minimum != unit::pixels(0.0f);
        auto const has_extra_preferred = extra_preferred != unit::pixels(0.0f);
        auto const has_extra_weight = extra_weight != 0.0f;
        if (has_extra_weight or has_extra_minium or has_extra_preferred) {
            auto minimum_todo = extra_minimum;
            auto preferred_todo = extra_preferred;
            for (auto i = first; i != last; ++i) {
                auto& item = (*this)[i];

                // First distribute the extra weight to this row or column,
                // based on the total current weight and weight of this row
                // or column.
                if (r.weight != 0.0f) {
                    item.weight += extra_weight * (item.weight / r.weight);
                } else {
                    item.weight += extra_weight / (last - first);
                }

                // Then calculate a normalized weight for this row or column
                // to distribute the extra size.
                auto const weight = total_weight != 0.0f ? item.weight / total_weight : 1.0f / (last - first);
                r.weight += item.weight;

                FIRST WE NEED TO SET THE PREFERRED WIDTH, BECAUSE THE MINIMUM WIDTH MAY NEVER BE LARGER THAN THE PREFERRED WIDTH.

                auto const item_extra_preferred = std::min(preferred_todo, ceil_as(unit::pixels, extra_preferred * weight));
                preferred_todo -= item_extra_preferred;
                item.preferred += item_extra_preferred;
                r.preferred += item_extra_preferred;

                auto const item_extra_minimum = std::min(minimum_todo, ceil_as(unit::pixels, extra_minimum * weight));
                minimum_todo -= item_extra_minimum;
                item.minimum += item_extra_minimum;
                r.minimum += item_extra_minimum;
            }
        }

        assert(r.minimum <= r.preferred);
        assert(r.weight >= 0.0f);
        return r;
    }

    /** Distribute the extra weight and size of multi-span cells.
     *
     * This function will distribute the extra minimum and preferred size which
     * was calculated from parent widgets.
     *
     * @param new_minimum The new minimum size of the rows or columns.
     * @param new_preferred The new preferred size of the rows or columns.
     * @param new_weight The new weight of the rows or columns.
     */
    layout_constraints
    distribute_constraints(unit::pixels_f new_minimum, unit::pixels_f new_preferred, float new_weight, bool mirror = false)
    {
        return distribute_constraints(0, size(), new_minimum, new_preferred, new_weight, mirror);
    }

    /** Set the constraints from the cells in the grid.
     *
     * This function will calculate the margins, weight, minimum and preferred
     * size of the rows or columns in the grid.
     *
     * @param cells The cells in the grid.
     * @param first_member The member of the cell that contains the index of the
     *                     first row or column.
     * @param size_member The member of the cell that contains the size of the
     *                    span of rows or columns.
     * @param constraints_member The member of the cell that contains the
     *                           vertical or horizontal constraints.
     * @param mirror If true the grid is vertically or horizontally mirrored.
     *               Used for right-to-left layout.
     * @param f A function that sets the constraints of a cell, recursively
     *          requesting the constraints of child widgets. The function's
     *          signature is `void f(grid_cell&)`.
     * @return The constraints of the grid as a whole.
     */
    template<typename T, typename F>
    [[nodiscard]] layout_constraints constraints(
        grid_cells<T>& cells,
        size_t grid_cell<T>::*first_member,
        size_t grid_cell<T>::*size_member,
        layout_constraints grid_cell<T>::*constraints_member,
        bool mirror,
        F const& f) requires(std::is_invocable_r_v<void, F, grid_cell<T>&>)
    {
        set_constraints(cells, first_member, size_member, constraints_member, mirror, f);

        // Distribute the weight and size of multi-span cells to the rows and
        // columns. If we are lucky the single-span cells have already added
        // enough weight and size to the rows and columns, for the multi-span
        // cells to fit.
        return distribute_constraints(cells, first_member, size_member, constraints_member, mirror);
    }

    /** Set the layout of the rows or columns.
     *
     * This function will set the position and extent of the rows or columns.
     *
     * @param start The start position of the rows or columns.
     * @param extent The size of the rows or columns.
     * @param mirror If true the grid is vertically or horizontally mirrored.
     *               Used for right-to-left layout.
     */
    void set_layout(unit::pixels_f start, unit::pixels_f extent, bool mirror) noexcept
    {
        auto const constraints_ = constraints(mirror);
        assert(extent >= constraints_.minimum);

        for (auto& item : *this) {
            item.extent = item.preferred;
        }

        if (extent > constraints_.preferred) {
            auto const extra = extent - constraints_.preferred;
            auto normalized_weight = 1.0f / size();
            for (auto& item : *this) {
                if (constraints_.weight != 0.0f) {
                    normalized_weight = item.weight / constraints_.weight;
                }
                THIS NEEDS ROUNDING item.extent += extra * normalized_weight;
            }

        } else if (extent < constraints_.preferred) {
            auto less = constraints_.preferred - extent;
            for (auto& item : *this) {
                if (less <= unit::pixels(0.0f)) {
                    break;
                }
                auto const available = item.preferred - item.minimum;
                auto const shrink = std::min(available, less);
                item.extent -= shrink;
                less -= shrink;
            }
        }
    }

private:
    [[nodiscard]] bool holds_invariant() const noexcept
    {
        for (auto const& col : *this) {
            if (col.margin_before < unit::pixels(0.0f)) {
                return false;
            }
            if (col.margin_after < unit::pixels(0.0f)) {
                return false;
            }
            if (col.minimum < unit::pixels(0.0f)) {
                return false;
            }
            if (col.minimum > col.preferred) {
                return false;
            }
            if (col.weight < 0.0f) {
                return false;
            }
        }
        return true;
    }

    /** Set the constraints from the cells in the grid.
     *
     * This function will set the margins of it's row or column. And for cells
     * with a span of 1, it will set the weight, minimum and preferred size of
     * it's row or column.
     *
     * @param cells The cells in the grid.
     * @param first_member The member of the cell that contains the index of the
     *                     first row or column.
     * @param size_member The member of the cell that contains the size of the
     *                    span of rows or columns.
     * @param constraints_member The member of the cell that contains the
     *                           vertical or horizontal constraints.
     * @param mirror If true the grid is vertically or horizontally mirrored.
     *               Used for right-to-left layout.
     * @param f A function that sets the constraints of a cell, recursively
     *          requesting the constraints of child widgets. The function's
     *          signature is `void f(grid_cell&)`.
     */
    template<typename T, typename F>
    void set_constraints(
        grid_cells<T>& cells,
        size_t grid_cell<T>::*first_member,
        size_t grid_cell<T>::*size_member,
        layout_constraints grid_cell<T>::*constraints_member,
        bool mirror,
        F const& f) requires(std::is_invocable_r_v<void, F, grid_cell<T>&>)
    {
        for (auto& cell : cells) {
            f(cell);

            auto const first = cell.*first_member;
            auto const size = cell.*size_member;
            auto const last = first + size;
            auto const& constraints = cell.*constraints_member;

            auto margin_before = constraints.margin_before;
            auto margin_after = constraints.margin_after;
            if (mirror) {
                std::swap(margin_before, margin_after);
            }

            inplace_max((*this)[first].margin_before, margin_before);
            inplace_max((*this)[last - 1].margin_after, margin_after);

            if (size == 1) {
                inplace_max((*this)[first].weight, constraints.weight);
                inplace_max((*this)[first].minimum, constraints.minimum);
                inplace_max((*this)[first].preferred, constraints.preferred);
            }
        }
        assert(holds_invariant());
    }

    /** Distribute the extra weight and size of multi-span cells.
     *
     * This function will distribute the extra weight and size of multi-span
     * cells to the rows or columns that the cell spans. This is done by
     * distributing the extra weight first, and then distributing the extra
     * size based on the new weights.
     *
     * @param cells The cells in the grid.
     * @param first_member The member of the cell that contains the index of the
     *                     first row or column.
     * @param size_member The member of the cell that contains the size of the
     *                    span of rows or columns.
     * @param constraints_member The member of the cell that contains the
     *                           vertical or horizontal constraints.
     */
    template<typename T>
    void distribute_constraints(
        grid_cells<T> const& cells,
        size_t grid_cell<T>::*first_member,
        size_t grid_cell<T>::*size_member,
        layout_constraints grid_cell<T>::*constraints_member)
    {
        for (auto& cell : cells) {
            if (auto const size = cell.*size_member; size != 1) {
                auto const first = cell.*first_member;
                auto const last = first + size;
                auto const& constraints = cell.*constraints_member;

                // First distribute the extra weight among the rows in the span.
                // The new weights will be used to determine how to distribute
                // the extra size among the rows.
                distribute_constraints(first, last, unit::pixels(0.0f), unit::pixels(0.0f), constraints.weight);
                distribute_constraints(first, last, constraints.minimum, constraints.preferred, 0.0f);
            }
        }
        assert(holds_invariant());
    }
};

} // namespace detail

template<typename T>
class grid {
public:
    using value_type = T;
    using cell_type = detail::grid_cell<value_type>;

    constexpr grid() noexcept = default;
    constexpr grid(grid const&) noexcept = default;
    constexpr grid(grid&&) noexcept = default;
    constexpr grid& operator=(grid const&) noexcept = default;
    constexpr grid& operator=(grid&&) noexcept = default;

    void clear() noexcept
    {
        _cells.clear();
        _rows.clear();
        _columns.clear();
    }

    template<typename F>
    [[nodiscard]] layout_constraints vertical_constraints(F const& f)
        requires(std::is_invocable_r_v<layout_constraints, F, value_type&>)
    {
        _rows.clear();
        _rows.resize(_cells.num_rows());
        _columns.clear();
        _columns.resize(_cells.num_columns());

        return _rows.constraints(
            _cells, &cell_type::row_nr, &cell_type::row_span, &cell_type::vertical_constraints, false, [&f](cell_type& cell) {
                cell.vertical_constraints = f(cell.value);
            });
    }

    template<typename F>
    [[nodiscard]] layout_constraints
    horizontal_constraints(unit::pixels_f minimum_height, unit::pixels_f preferred_height, F const& f)
        requires(std::is_invocable_r_v<void, F, value_type&, unit::pixels_f, unit::pixels_f>)
    {
        // At this point we have a better idea of the minimum and preferred
        // height of the rows. This would allow columns to be smaller than
        // before.
        _rows.distribute_constraints(minimum_height, preferred_height, 0.0f);

        return _columns.constraints(
            _cells,
            &cell_type::col_nr,
            &cell_type::col_span,
            &cell_type::horizontal_constraints,
            not _left_to_right,
            [this, &f](cell_type& cell) {
                auto const [row_minimum, row_preferred, _] = this->_rows.extent(cell.row_nr, cell.row_nr + cell.row_span);
                cell.horizontal_constraints = f(cell.value, row_minimum, row_preferred);
            });
    }

    void set_layout(box_shape const& shape) noexcept
    {
        _columns.set_layout(shape.x(), shape.width(), not _left_to_right);
        // Rows in the grid are laid out from top to bottom which is reverse from the y-axis up.
        _rows.set_layout(shape.y(), shape.height(), true);

        for (auto& cell : _cells) {
            auto const left = _columns[cell.column_nr].position;
            auto const right =
                _columns[cell.column_nr + cell.column_span - 1].position + _columns[cell.column_nr + cell.column_span - 1].extent;
            auto const top = _rows[cell.row_nr].position;
            auto const bottom = _rows[cell.row_nr + cell.row_span - 1].position - _rows[cell.row_nr + cell.row_span - 1].extent;

            cell.shape.rectangle = {point2{left, bottom}, point2{right, top}};
        }
    }

private:
    detail::grid_cells<value_type> _cells;
    detail::grid_header _rows;
    detail::grid_header _columns;
    bool _left_to_right = true;
};

} // namespace hi::v1
