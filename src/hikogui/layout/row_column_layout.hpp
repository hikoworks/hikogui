// Copyright Take Vos 2022.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "grid_layout.hpp"
#include "box_constraints.hpp"
#include "box_shape.hpp"
#include "../geometry/geometry.hpp"
#include "../macros.hpp"
#include <concepts>
#include <iterator>

hi_export_module(hikogui.layout.row_column_layout);

hi_export namespace hi { inline namespace v1 {

template<axis Axis, typename T>
class row_column_layout {
public:
    static_assert(Axis == axis::x or Axis == axis::y);

    using value_type = T;
    using grid_type = grid_layout<T>;
    using cell_type = grid_type::cell_type;
    using iterator = grid_type::iterator;
    using const_iterator = grid_type::const_iterator;

    ~row_column_layout() = default;
    constexpr row_column_layout() noexcept = default;
    constexpr row_column_layout(row_column_layout const&) noexcept = default;
    constexpr row_column_layout(row_column_layout&&) noexcept = default;
    constexpr row_column_layout& operator=(row_column_layout const&) noexcept = default;
    constexpr row_column_layout& operator=(row_column_layout&&) noexcept = default;
    [[nodiscard]] constexpr friend bool operator==(row_column_layout const&, row_column_layout const&) noexcept = default;

    [[nodiscard]] constexpr bool empty() const noexcept
    {
        return _grid.empty();
    }

    [[nodiscard]] constexpr size_t size() const noexcept
    {
        return _grid.size();
    }

    [[nodiscard]] constexpr iterator begin() noexcept
    {
        return _grid.begin();
    }

    [[nodiscard]] constexpr const_iterator begin() const noexcept
    {
        return _grid.begin();
    }

    [[nodiscard]] constexpr const_iterator cbegin() const noexcept
    {
        return _grid.cbegin();
    }

    [[nodiscard]] constexpr iterator end() noexcept
    {
        return _grid.end();
    }

    [[nodiscard]] constexpr const_iterator end() const noexcept
    {
        return _grid.end();
    }

    [[nodiscard]] constexpr const_iterator cend() const noexcept
    {
        return _grid.cend();
    }

    [[nodiscard]] constexpr cell_type& operator[](size_t index) noexcept
    {
        // Grids in a cell are ordered by row_nr.
        return _grid[index];
    }

    [[nodiscard]] constexpr cell_type const& operator[](size_t index) const noexcept
    {
        // Grids in a cell are ordered by row_nr.
        return _grid[index];
    }

    template<std::convertible_to<T> Value>
    cell_type& insert(const_iterator pos, Value&& value) noexcept
    {
        auto const index = std::distance(cbegin(), pos);

        for (auto it = begin() + index; it != end(); ++it) {
            if constexpr (Axis == axis::x) {
                ++(it->first_column);
                ++(it->last_column);
            } else {
                ++(it->first_row);
                ++(it->last_row);
            }
        }

        if constexpr (Axis == axis::x) {
            return _grid.add_cell(index, 0, std::forward<Value>(value));
        } else {
            return _grid.add_cell(0, index, std::forward<Value>(value));
        }
    }

    template<std::convertible_to<T> Value>
    cell_type& push_front(Value&& value) noexcept
    {
        return insert(cbegin(), std::forward<Value>(value));
    }

    template<std::convertible_to<T> Value>
    cell_type& push_back(Value&& value) noexcept
    {
        return insert(cend(), std::forward<Value>(value));
    }

    void clear() noexcept
    {
        return _grid.clear();
    }

    [[nodiscard]] box_constraints constraints(bool left_to_right, vertical_alignment alignment) const noexcept
    {
        return _grid.constraints(left_to_right, alignment);
    }

    void set_layout(box_shape const &shape) noexcept
    {
        return _grid.set_layout(shape);
    }

private:
    grid_type _grid;
};

template<typename T>
using row_layout = row_column_layout<axis::x, T>;

template<typename T>
using column_layout = row_column_layout<axis::y, T>;

}} // namespace hi::v1
