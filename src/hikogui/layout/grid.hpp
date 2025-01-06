// Copyright Take Vos 2024.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "constraints.hpp"
#include "../units/units.hpp"
#include "../utility/utility.hpp"
#include "../macros.hpp"

hi_export_module(hikogui.layout : grid);

hi_export namespace hi::inline v1 {
namespace detail {

template<typename T>
struct grid_cell {
    using value_type = T;

    size_t row_nr = 0;
    size_t column_nr = 0;
    size_t row_span = 1;
    size_t column_span = 1;

    layout_constraints vertical_constraints;
    layout_constraints horizontal_constraints;
    value_type value = {};
};

class grid_cells : public std::vector<grid_cell> {
public:
    using super = std::vector<grid_cell>;
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

    template<typename Member>
    [[nodiscard]] auto extent(size_t first, size_t last, Member member) const
    {
        auto _ = grid_cell_aggregate{};
        using value_type = decltype(_.*member){};

        assert(first <= last);
        assert(last <= size());

        auto r = value_type{};

        if (first != last) {
            r = (*this)[first].*member;
            for (auto row_nr = first + 1; row_nr != last; ++row_nr) {
                auto& row = (*this)[row_nr];

                r += row.*member;

                if constexpr (std::same_as<decltype(r), unit::pixels_f>) {
                    auto& previous_row = (*this)[row_nr - 1];
                    r += std::max(previous_row.margin_after, row.margin_before);
                }
            }
        }
        return r;
    }

    [[nodiscard]] auto extent() const
    {
        return extent(0, size());
    }

    void
    distribute_constraints(size_t first, size_t last, unit::pixels_f new_minimum, unit::pixels_f new_preferred, float new_weight)
    {
        assert(first <= last);
        assert(last <= size());

        auto const [original_minimum, original_preferred, original_weight] = extent(first, last);
        auto const extra_weight = std::max(0.0f, new_weight - original_weight);
        auto const extra_minimum = std::max(unit::pixels(0.0f), new_minimum - original_minimum);
        auto const extra_preferred = std::max(unit::pixels(0.0f), new_preferred - original_preferred);

        auto const has_extra_weight = extra_weight != 0.0f;
        auto const has_extra_size = extra_minimum != unit::pixels(0.0f) or extra_preferred != unit::pixels(0.0f);

        if (has_extra_weight or has_extra_size) {
            auto normalized_weight = 1.0f / (last - first);
            for (auto row_nr = first; row_nr != last; ++row_nr) {
                auto& row = (*this)[row_nr];

                if (original_weight != 0.0f) {
                    normalized_weight = row.weight / original_weight;
                }

                row.weight += extra_weight * normalized_weight;
                row.minimum += extra_minimum * normalized_weight;
                row.preferred += extra_preferred * normalized_weight;
                if (row.minimum > row.preferred) {
                    row.preferred = row.minimum;
                }
            }
        }
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
    void distribute_constraints(unit::pixels_f new_minimum, unit::pixels_f new_preferred, float new_weight)
    {
        distribute_constraints(0, size(), new_minimum, new_preferred, new_weight);
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
    template<typename F>
    [[nodiscard]] layout_constraints constraints(
        std::vector<grid_cell>& cells,
        size_t grid_cell::*first_member,
        size_t grid_cell::*size_member,
        layout_constraints grid_cell::*constraints_member,
        bool mirror,
        F const& f) requires(std::is_invocable_r_v<void, F, grid_cell&>)
    {
        set_constraints(cells, first_member, size_member, constraints_member, mirror, f);

        // Distribute the weight and size of multi-span cells to the rows and
        // columns. If we are lucky the single-span cells have already added
        // enough weight and size to the rows and columns, for the multi-span
        // cells to fit.
        distribute_constraints(cells, first_member, size_member, constraints_member);

        return constraints(mirror);
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

    [[nodiscard]] layout_constraints constraints(bool mirror) const
    {
        auto r = layout_constraints{};
        std::tie(r.minimum, r.preferred, r.weight) = extent();
        r.margin_before = margin_before();
        r.margin_after = margin_after();
        if (mirror) {
            std::swap(r.margin_before, r.marging_after);
        }
        return r;
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
    template<typename F>
    void set_constraints(
        std::vector<grid_cell>& cells,
        size_t grid_cell::*first_member,
        size_t grid_cell::*size_member,
        layout_constraints grid_cell::*constraints_member,
        bool mirror,
        F const& f) requires(std::is_invocable_r_v<void, F, grid_cell&>)
    {
        for (auto& cell : _cells) {
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
    void distribute_constraints(
        std::vector<grid_cell> const& cells,
        size_t grid_cell::*first_member,
        size_t grid_cell::*size_member,
        layout_constraints grid_cell::*constraints_member)
    {
        for (auto& cell : cells) {
            if (auto const size = cell.*size_member; size != 1) {
                auto const first = cell.*first_member;
                auto const last = first + size;
                auto const& constraints = cell.*constraints_member;

                // First distribute the extra weight among the rows in the span.
                // The new weights will be used to determine how to distribute
                // the extra size among the rows.
                _rows.distribute_constraints(first, last, unit::pixels(0.0f), unit::pixels(0.0f), constraints.weight);
                _rows.distribute_constraints(first, last, constraints.minimum_height, constraints.preferred_height, 0.0f);
            }
        }
        assert(_rows.holds_invariant());
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
            _cells,
            &detail::grid_cell::row_nr,
            &detail::grid_cell::row_span,
            &detail::grid_cell::vertical_constraints,
            false,
            [&f](grid_cell& cell) {
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
            &detail::grid_cell::col_nr,
            &detail::grid_cell::col_span,
            &detail::grid_cell::horizontal_constraints,
            not _left_to_right,
            [&_rows, &f](grid_cell& cell) {
                auto const [row_minimum, row_preferred, _] = _rows.extent(cell.row_nr, cell.row_nr + cell.row_span);
                cell.horizontal_constraints = f(cell.value, row_minimum, row_preferred);
            });
    }

private:
    std::vector<cell_type> _cells;
    detail::grid_header _rows;
    detail::grid_header _columns;
    bool _left_to_right = true;
};

} // namespace hi::v1
