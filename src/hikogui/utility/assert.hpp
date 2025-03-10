// Copyright Take Vos 2020-2022.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

/** @file assert.hpp Utilities to assert and bound check.
 */

#pragma once

#include "type_traits.hpp"
#include "terminate.hpp"
#include "../macros.hpp"
#include "exception.hpp"
#include <exception>
#include <ranges>

hi_export_module(hikogui.utility.assert);

hi_warning_push();
// "C26472: Don't use a static_cast for arithmetic", asserts use static_cast specifically for safety.
hi_warning_ignore_msvc(26472);

hi_export namespace hi { inline namespace v1 {

/** Check if an index is less than the bound.
 *
 * @param index The index to check.
 * @param upper The upper bound.
 * @return true If index is less than the upper bound.
 */
hi_export [[nodiscard]] constexpr bool bound_check(std::integral auto index, std::integral auto upper) noexcept
{
    return std::cmp_less(index, upper);
}

/** Check if an index is between the lower (inclusive) and upper (exclusive).
 *
 * @note It is undefined behavior when @a upper is lower than @a lower.
 * @param index The index to check.
 * @param lower The lower bound.
 * @param upper The upper bound.
 * @return true If index is greater or equal to lower bound and index is less than upper bound.
 */
hi_export [[nodiscard]] constexpr bool bound_check(std::integral auto index, std::integral auto lower, std::integral auto upper) noexcept
{
#ifndef NDEBUG
    if (std::cmp_greater(lower, upper)) {
        hi_assert_abort("bound_check() lower is greater than upper.");
    }
#else
    hi_assume(std::cmp_less_equal(lower, upper));
#endif

    return std::cmp_greater_equal(index, lower) and std::cmp_less(index, upper);
}

/** Check if an floating point value is between the lower (inclusive) and upper (inclusive).
 *
 * @note It is undefined behavior when @a upper is lower than @a lower.
 * @param index The index to check.
 * @param lower The lower bound.
 * @param upper The upper bound.
 * @return true If index is greater or equal to lower bound and index is less than upper bound.
 */
hi_export [[nodiscard]] constexpr bool bound_check(std::floating_point auto index, std::floating_point auto lower, std::floating_point auto upper) noexcept
{
#ifndef NDEBUG
    if (lower > upper) {
        hi_assert_abort("bound_check() lower is greater than upper.");
    }
#else
    hi_assume(lower <= upper);
#endif

    return index >= lower and index <= upper;
}

template<typename Context>
concept bound_check_range_helper = requires(Context&& range) {
    {
        std::ranges::size(range)
    } -> std::unsigned_integral;
};

/** Check if an index is within a range.
 *
 * @param index The index to check.
 * @param range The range object, such as a vector or array, which has a `std::ranges::size()`.
 * @return true If index natural and below the size of the array.
 */
hi_export [[nodiscard]] constexpr bool bound_check(std::integral auto index, bound_check_range_helper auto&& range) noexcept
{
    static_assert(sizeof(index) <= sizeof(size_t));

    if constexpr (std::is_signed_v<std::decay_t<decltype(index)>>) {
        if (index < 0) {
            return false;
        }
    }

    return static_cast<size_t>(index) < std::ranges::size(range);
}


}} // namespace hi::v1

hi_warning_pop();
