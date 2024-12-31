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

struct grid_row {
    height_constraints constraints;

    unit::pixels_f baseline = unit::pixels(0.0f);
    unit::pixels_f height = unit::pixels(0.0f);
    unit::pixels_f position = unit::pixels(0.0f);
};

template<typename T>
struct grid_cell {
    using value_type = T;

    size_t row_nr = 0;
    size_t column_nr = 0;
    size_t row_span = 1;
    size_t column_span = 1;

    value_type value = {};
};

}

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

    template<std::invocable<value_type&> F>
    [[nodiscard]] hi::height_constraints height_constraints(F const& f)
    {
        _rows.clear();
        for (auto &cell : _cells) {
            if (cell.row_span == 1) {
                _rows.resize(std::max(_rows.size(), cell.row + 1));
                inplace_max(_rows[cell.row_nr].constraints, f(cell.value));
            }
        }

        for (auto &cell : _cells) {
            if (cell.row_span != 1) {
                auto const last_row_nr = cell.row_nr + cell.row_span;
                _rows.resize(std::max(_rows.size(), last_row_nr));

                auto const constraints = f(cell.value);
                inplace_max(_rows[cell.row_nr].constraints.margin_above, constraints.margin_above);
                inplace_max(_rows[last_row_nr].constraints.margin_below, constraints.margin_below);

                auto span_constraints = hi::height_constraints{};
                for (auto row_nr = cell.row_nr; row_nr != last_row_nr; ++row_nr) {
                    span_constraints += _rows[row_nr].constraints;
                }

                // Distribute extra weight among the rows in the span.
                // Based on the weights of the rows already in the span. 
                if (auto const extra_weight = constraints.weight - span_constraints.weight; extra_weight != 0.0f) {
                    for (auto row_nr = cell.row_nr; row_nr != last_row_nr; ++row_nr) {
                        auto const normalized_weight = span.constraints.weight == 0.0f ?
                            1.0f / cell.row_span : 1.0f :
                            _rows[row_nr].constraints.weight / span.constraints.weight;

                        _rows[row_nr].constraints.weight += extra_weight * normalized_weight;
                    }
                    span_constraints.weight += extra_weight;
                }

                // Distribute extra height among the rows in the span.
                auto const extra_minimum = constraints.minimum - span_constraints.minimum;
                auto const extra_preferred = constraints.preferred - span_constraints.preferred;
                if (extra_minimum != unit::pixels(0.0f) or extra_preferred != unit::pixels(0.0f)) {
                    auto normalized_weight = 1.0f / cell.row_span;
                    for (auto row_nr = cell.row_nr; row_nr != last_row_nr; ++row_nr) {
                        if (span_constraints.weight != 0.0f) {
                            normalized_weight = _rows[row_nr].constraints.weight / span_constraints.weight;
                        }

                        _rows[row_nr].constraints.minimum += extra_minimum * normalized_weight;
                        _rows[row_nr].constraints.preferred += extra_preferred * normalized_weight;
                        if (_rows[row_nr].constraints.minimum > _rows[row_nr].constraints.preferred) {
                            _rows[row_nr].constraints.preferred = _rows[row_nr].constraints.minimum;
                        }
                    }
                }
            }
        }

        auto total_constraints = hi::height_constraints{};
        for (auto row_nr = size_t{0}; row_nr != _rows.size(); ++row_nr) {
            total_constraints += _rows[row_nr].constraints;
        }
        return total_constraints;
    }

    template<std::invocable<value_type&, unit::pixels_f, unit::pixels_f> F>
    [[nodiscard]] hi::width_constraints width_constraints(unit::pixels_f minimum_height, unit::pixels_f preferred_height, F const& f)
    {
        // Adjust the row constraints to the external height constraints.
        auto total_constraints = hi::height_constraints{};
        for (auto row_nr = size_t{0}; row_nr != _rows.size(); ++row_nr) {
            total_constraints += _rows[row_nr].constraints;
        }

        if (minimum_height > total_constraints.minimum_height or preferred_height > total_constraints.preferred_height) {
            auto const extra_minimum = minimum_height - total_constraints.minimum_height;
            auto const extra_preferred = preferred_height - total_constraints.preferred_height;

            auto normalized_weight = 1.0f / _rows.size();
            for (auto &row : _rows) {
                if (total_constraints.weight != 0.0f) {
                    normalized_weight = row.constraints.weight / total_constraints.weight;
                }

                row.constraints.minimum += extra_minimum * normalized_weight;
                row.constraints.preferred += extra_preferred * normalized_weight;
                if (row.constraints.minimum > row.constraints.preferred) {
                    row.constraints.preferred = row.constraints.minimum;
                }
            }

            total_constraints.minimum_height = minimum_height;
            total_constraints.preferred_height = preferred_height;
        }


        _columns.clear();
        for (auto &cell : _cells) {
            if (cell.column_span == 1) {
                _columns.resize(std::max(_columns.size(), cell.column + 1));

                auto row_span_constraints = hi::height_constraints{};
                for (auto row_nr = cell.row_nr; row_nr != cell.row_nr + cell.row_span; ++row_nr) {
                    row_span_constraints += _rows[row_nr].constraints;
                }

                _columns[cell.column_nr].constraints.inplace_max(f(cell.value, row_span_constraints.minimum_height, row_span_constraints.preferred_height));
            }
        }

        for (auto &cell : _cells) {
            if (cell.column_span != 1) {
                auto const last_column_nr = cell.column_nr + cell.column_span;
                _columns.resize(std::max(_columns.size(), last_column_nr));

                auto const row_span_constraints = hi::height_constraints{};
                for (auto row_nr = cell.row_nr; row_nr != cell.row_nr + cell.row_span; ++row_nr) {
                    row_span_constraints += _rows[row_nr].constraints;
                }

                auto const constraints = f(cell.value, row_span_constraints.minimum_height, row_span_constraints.preferred_height);
                inplace_max(_columns[cell.column_nr].constraints.margin_before, constraints.margin_before);
                inplace_max(_columns[last_column_nr].constraints.margin_after, constraints.margin_after);

                auto span_constraints = hi::width_constraints{};
                for (auto column_nr = cell.column_nr; column_nr != last_column_nr; ++column_nr) {
                    span_constraints += _columns[column_nr].constraints;
                }

                // Distribute extra weight among the columns in the span.
                // Based on the weights of the columns already in the span. 
                if (auto const extra_weight = constraints.weight - span_constraints.weight; extra_weight != 0.0f) {
                    auto normalized_weight = 1.0f / cell.column_span;
                    for (auto column_nr = cell.column_nr; column_nr != last_column_nr; ++column_nr) {
                        if (span_constraints.weight != 0.0f) {
                            normalized_weight = _columns[column_nr].constraints.weight / span_constraints.weight;
                        }
                        _columns[column_nr].constraints.weight += extra_weight * normalized_weight;
                    }
                    span_constraints.weight += extra_weight;
                }

                // Distribute extra width among the columns in the span.
                auto const extra_minimum = constraints.minimum_width - span_constraints.minimum_width;
                auto const extra_preferred = constraints.preferred_width - span_constraints.preferred_width;
                if (extra_minimum != unit::pixels(0.0f) or extra_preferred != unit::pixels(0.0f)) {
                    auto normalized_weight = 1.0f / cell.column_span;
                    for (auto column_nr = cell.column_nr; column_nr != last_column_nr; ++column_nr) {
                        if (span_constraints.weight != 0.0f) {
                            normalized_weight = _columns[column_nr].constraints.weight / span_constraints.weight;
                        }
                        _columns[column_nr].constraints.minimum_width += extra_minimum * normalized_weight;
                        _columns[column_nr].constraints.preferred_width += extra_preferred * normalized_weight;
                        if (_columns[column_nr].constraints.minimum_width > _columns[column_nr].constraints.preferred_width) {
                            _columns[column_nr].constraints.preferred_width = _columns[column_nr].constraints.minimum_width;
                        }
                    }
                } 
            }
        }

        auto total_width_constraints = hi::width_constraints{};
        for (auto column_nr = size_t{0}; column_nr != _columns.size(); ++column_nr) {
            total_width_constraints += _columns[column_nr].constraints;
        }
        return total_width_constraints;
    }

private:
    std::vector<cell_type> _cells;
    std::vector<detail::grid_row> _rows;
    std::vector<detail::grid_column> _columns;
};

} // namespace hi::v1

