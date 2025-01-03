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

    layout_constraints constraints;
    layout_width_constraints width_constraints;
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

struct grid_row {
    unit::pixels_f minimum = unit::pixels(0.0f);
    unit::pixels_f preferred = unit::pixels(0.0f);
    float weight = 0.0f;
    unit::pixels_f margin_before = unit::pixels(0.0f);
    unit::pixels_f margin_after = unit::pixels(0.0f);

    unit::pixels_f baseline = unit::pixels(0.0f);
    unit::pixels_f height = unit::pixels(0.0f);
    unit::pixels_f position = unit::pixels(0.0f);
};

class grid_rows : public std::vector<grid_row> {
public:
    using super = std::vector<grid_row>;
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

    [[nodiscard]] auto extent(size_t first, size_t last) const
    {
        struct result_type {
            unit::pixels_f minimum = unit::pixels(0.0f);
            unit::pixels_f preferred = unit::pixels(0.0f);
            float weight = 0.0f;
        };

        assert(first <= last);
        assert(last <= size());

        auto r = result_type{};

        if (first == last) {
            return r;
        }

        r.minimum = (*this)[first].minimum;
        r.preferred = (*this)[first].preferred;
        r.weight = (*this)[first].weight;
        for (auto row_nr = first + 1; row_nr != last; ++row_nr) {
            auto& row = (*this)[row_nr];
            auto& previous_row = (*this)[row_nr - 1];

            r.minimum += row.minimum + std::max(previous_row.margin_after, row.margin_before);
            r.preferred += row.preferred + std::max(previous_row.margin_after, row.margin_before);
            r.weight += row.weight;
        }
        return r;
    }

    [[nodiscard]] auto extent() const
    {
        return extent(0, size());
    }

    [[nodiscard]] void
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

    [[nodiscard]] void distribute_constraints(unit::pixels_f new_minimum, unit::pixels_f new_preferred, float new_weight)
    {
        distribute_constraints(0, size(), new_minimum, new_preferred, new_weight);
    }

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
};

struct grid_column {
    unit::pixels_f minimum = unit::pixels(0.0f);
    unit::pixels_f preferred = unit::pixels(0.0f);
    float weight = 0.0f;
    unit::pixels_f margin_before = unit::pixels(0.0f);
    unit::pixels_f margin_after = unit::pixels(0.0f);

    unit::pixels_f width = unit::pixels(0.0f);
    unit::pixels_f position = unit::pixels(0.0f);
};

class grid_columns : public std::vector<grid_column> {
public:
    using super = std::vector<grid_column>;
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

    [[nodiscard]] auto extent(size_t first, size_t last) const
    {
        struct result_type {
            unit::pixels_f minimum = unit::pixels(0.0f);
            unit::pixels_f preferred = unit::pixels(0.0f);
            float weight = 0.0f;
        };

        assert(first <= last);
        assert(last <= size());

        auto r = result_type{};

        if (first == last) {
            return r;
        }

        r.minimum = (*this)[first].minimum;
        r.preferred = (*this)[first].preferred;
        r.weight = (*this)[first].weight;
        for (auto col_nr = first + 1; col_nr != last; ++col_nr) {
            auto& col = (*this)[col_nr];
            auto& previous_col = (*this)[col_nr - 1];

            r.minimum += col.minimum + std::max(previous_col.margin_after, col.margin_before);
            r.preferred += col.preferred + std::max(previous_col.margin_after, col.margin_before);
            r.weight += col.weight;
        }
        return r;
    }

    [[nodiscard]] auto extent() const
    {
        return extent(0, size());
    }

    [[nodiscard]] void
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
            for (auto col_nr = first; col_nr != last; ++col_nr) {
                auto& col = (*this)[col_nr];

                if (original_weight != 0.0f) {
                    normalized_weight = col.weight / original_weight;
                }

                col.weight += extra_weight * normalized_weight;
                col.minimum += extra_minimum * normalized_weight;
                col.preferred += extra_preferred * normalized_weight;
                if (col.minimum > col.preferred) {
                    col.preferred = col.minimum;
                }
            }
        }
    }

    [[nodiscard]] void distribute_constraints(unit::pixels_f new_minimum, unit::pixels_f new_preferred, float new_weight)
    {
        distribute_constraints(0, size(), new_minimum, new_preferred, new_weight);
    }

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
    [[nodiscard]] layout_constraints get_constraints(F const& f)
        requires(std::is_invocable_r_v<layout_constraints, F, value_type&>)
    {
        _rows.clear();
        _rows.resize(_cells.num_rows());
        _columns.clear();
        _columns.resize(_cells.num_columns());

        for (auto& cell : _cells) {
            cell.constraints = f(cell.value);

            inplace_max(_rows[cell.row_nr].margin_before, cell.constraints.margin_above);
            inplace_max(_rows[cell.row_nr + cell.row_span - 1].margin_after, cell.constraints.margin_below);
            if (_left_to_right) {
                inplace_max(_columns[cell.column_nr].margin_before, cell.constraints.margin_left);
                inplace_max(_columns[cell.column_nr + cell.column_span - 1].margin_after, cell.constraints.margin_right);
            } else {
                inplace_max(_columns[cell.column_nr + cell.column_span - 1].margin_before, cell.constraints.margin_left);
                inplace_max(_columns[cell.column_nr].margin_after, cell.constraints.margin_right);
            }

            if (cell.row_span == 1) {
                inplace_max(_rows[cell.row_nr].weight, cell.constraints.weight_height);
                inplace_max(_rows[cell.row_nr].minimum, cell.constraints.minimum_height);
                inplace_max(_rows[cell.row_nr].preferred, cell.constraints.preferred_height);
            }
            if (cell.column_span == 1) {
                inplace_max(_columns[cell.column_nr].weight, cell.constraints.weight_width);
            }
        }
        assert(_rows.holds_invariant());
        assert(_columns.holds_invariant());

        // Distribute the weight and size of multi-span cells to the rows and
        // columns. If we are lucky the single-span cells have already added
        // enough weight and size to the rows and columns, for the multi-span
        // cells to fit.
        for (auto& cell : cells) {
            auto const& constraints = cell.constraints;

            if (cell.row_span != 1) {
                auto const first = cell.row_nr;
                auto const last = cell.row_nr + cell.row_span;

                // First distribute the extra weight among the rows in the span.
                // The new weights will be used to determine how to distribute
                // the extra size among the rows.
                _rows.distribute_constraints(first, last, unit::pixels(0.0f), unit::pixels(0.0f), constraints.weight);
                _rows.distribute_constraints(first, last, constraints.minimum_height, constraints.preferred_height, 0.0f);
            }
            if (cell.col_span != 1) {
                auto const first = cell.column_nr;
                auto const last = cell.column_nr + cell.column_span;

                // We do not yet know the size of the columns, so just
                // distribute the weight.
                _columns.distribute_constraints(first, last, unit::pixels(0.0f), unit::pixels(0.0f), constraints.weight_width);
            }
        }
        assert(_rows.holds_invariant());
        assert(_columns.holds_invariant());

        auto r = layout_constraints{};
        std::tie(r.minimum_height, r.preferred_height, r.weight_height) = _rows.extent();
        std::tie(std::ignore, std::ignore, r.weight_width) = _columns.extent();
        r.margin_left = _left_to_right ? _columns.margin_before() : _columns.margin_after();
        r.margin_right = _left_to_right ? _columns.margin_after() : _columns.margin_before();
        r.margin_below = _rows.margin_before();
        r.margin_above = _rows.margin_after();
        return r;
    }

    template<typename F>
    [[nodiscard]] layout_width_constraints
    set_width_constraints(unit::pixels_f minimum_height, unit::pixels_f preferred_height, F const& f)
        requires(std::is_invocable_r_v<void, F, value_type&, unit::pixels_f, unit::pixels_f>)
    {
        _rows.distribute_constraints(minimum_height, preferred_height, 0.0f);

        for (auto& cell : _cells) {
            auto const [row_minimum, row_preferred, _] = _rows.extent(cell.row_nr, cell.row_nr + cell.row_span);
            cell.width_constraints = f(cell.value, row_minimum, row_preferred);

            if (cell.column_span == 1) {
                inplace_max(_columns[cell.column_nr].minimum, cell.width_constraints.minimum);
                inplace_max(_columns[cell.column_nr].preferred, cell.width_constraints.preferred);
            }
        }
        assert(_columns.holds_invariant());

        for (auto& cell : _cells) {
            if (cell.column_span != 1) {
                auto const first = cell.column_nr;
                auto const last = cell.column_nr + cell.column_span;

                _columns.distribute_constraints(first, last, cell.width_constraints.minimum, cell.width_constraints.preferred, 0.0f);
            }
        }
        assert(_columns.holds_invariant());

        auto r = layout_width_constraints{};
        std::tie(r.minimum_width, r.preferred_width, std::ignore) = _columns.extent();
        return r;
    }

private:
    std::vector<cell_type> _cells;
    detail::grid_rows _rows;
    std::vector<detail::grid_column> _columns;
    bool _left_to_right = true;
};

} // namespace hi::v1
