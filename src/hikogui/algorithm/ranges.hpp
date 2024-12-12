// Copyright Take Vos 2021-2022.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "../utility/utility.hpp"
#include "../macros.hpp"
#include <ranges>
#include <algorithm>
#include <concepts>
#include <type_traits>
#include <vector>
#include <exception>
#include <stdexcept>

hi_export_module(hikogui.algorithm.ranges);

hi_export namespace hi::inline v1 {

template<typename Value, typename Range>
[[nodiscard]] constexpr Value get_first(Range &&range)
{
    auto it = std::ranges::begin(range);
    auto last = std::ranges::end(range);

    if (it == last) {
        throw std::out_of_range{"Range is empty"};
    }

    auto value = *it++;
    return Value{value};
}

template<typename Range>
[[nodiscard]] constexpr Range::value_type get_first(Range&& range)
{
    return get_first<typename Range::value_type>(std::forward<Range>(range));
}

/** clear and reserve space in a container.
 * 
 * This function will clear the container and reserve space for the given size.
 * This is useful when you want to reuse a container and avoid reallocations.
 * 
 * When the container is used for the first time, the reservation will match
 * the given size.
 * 
 * If the @a size is larger than the current capacity of the container, the
 * reservation will be increased 1.5 times the size.
 * 
 * If the @a size is larger than 1.5 times the current capacity of the container,
 * the reservation will be increased to the next power of two that is larger or
 * equal to the given size.
 * 
 * If the @a size is smaller than the current capacity of the container, the
 * reservation will not be changed.
 * 
 * @param container The container to clear and reserve space in.
 * @param size The size to reserve.
 */
template<std::ranges::range Container>
constexpr void clear_and_reserve(Container& container, size_t size)
    requires requires(Container& c) {
        c.clear();
        c.capacity();
        c.reserve(size);
    }
{
    auto const capacity = container.capacity();
    container.clear();

    if (size <= capacity) {
        return;
    }

    auto const next_golden_ratio = capacity + capacity / 2;
    if (size > next_golden_ratio) {
        auto const next_power_of_two = std::bit_ceil(size);
        container.reserve(next_power_of_two);
    } else {
        container.reserve(next_golden_ratio);
    }
}

/** clear and resize a container.
 * 
 * This function will clear the container and resize it to the given size.
 * This is useful when you want to reuse a container and avoid reallocations.
 * 
 * When the container is used for the first time, the size will match the given size.
 * 
 * If the @a size is larger than the current capacity of the container, the
 * reservation will be increased 1.5 times the size.
 * 
 * If the @a size is larger than 1.5 times the current capacity of the container,
 * the reservation will be increased to the next power of two that is larger or
 * equal to the given size.
 * 
 * If the @a size is smaller than the current capacity of the container, the
 * reservation will not be changed.
 * 
 * @param container The container to clear and resize.
 * @param size The size to resize to.
 * @param value The value to fill the container with.
 */
template<std::ranges::range Container>
constexpr void clear_and_resize(Container& container, size_t size, typename Container::value_type const& value)
    requires requires(Container& c) {
        c.clear();
        c.capacity();
        c.reserve(size);
        c.resize(size, value);
    }
{
    clear_and_reserve(container, size);
    container.resize(size, value);
}

} // namespace hi::inline v1
