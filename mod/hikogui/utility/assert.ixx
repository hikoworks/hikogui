// Copyright Take Vos 2020-2022.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

/** @file assert.hpp Utilities to assert and bound check.
 */

module;
#include "../macros.hpp"

#include <exception>
#include <ranges>

export module hikogui_utility_assert;
import hikogui_utility_debugger;
import hikogui_utility_exception;
import hikogui_utility_type_traits;

hi_warning_push();
// "C26472: Don't use a static_cast for arithmetic", asserts use static_cast specifically for safety.
hi_warning_ignore_msvc(26472);

export namespace hi { inline namespace v1 {

/** Check if an unsigned index is less than the bound.
 *
 * @param index The unsigned index to check.
 * @param upper The upper bound.
 * @return true If index is less than the upper bound.
 */
export [[nodiscard]] constexpr bool bound_check(std::unsigned_integral auto index, std::unsigned_integral auto upper) noexcept
{
    using value_type = common_integer_t<decltype(index), decltype(upper)>;

    hilet index_ = static_cast<value_type>(index);
    hilet upper_ = static_cast<value_type>(upper);
    return index_ < upper_;
}

export [[nodiscard]] constexpr bool bound_check(std::unsigned_integral auto index, std::signed_integral auto upper) noexcept
{
    if (upper <= 0) {
        return false;
    }
    return bound_check(index, static_cast<std::make_unsigned_t<decltype(upper)>>(upper));
}

/** Check if an index is between the lower (inclusive) and upper (exclusive).
 *
 * @note It is undefined behavior when @a upper is lower than @a lower.
 * @param index The index to check.
 * @param lower The lower bound.
 * @param upper The upper bound.
 * @return true If index is greater or equal to lower bound and index is less than upper bound.
 */
export [[nodiscard]] constexpr bool bound_check(std::integral auto index, std::integral auto lower, std::integral auto upper) noexcept
{
    using value_type = common_integer_t<decltype(index), decltype(lower), decltype(upper)>;

    auto index_ = static_cast<value_type>(index);
    auto lower_ = static_cast<value_type>(lower);
    auto upper_ = static_cast<value_type>(upper);

#ifndef NDEBUG
    if (not(lower_ < upper_)) {
        hi_debug_abort("bound_check() lower is greater than upper.");
    }
#else
    hi_assume(lower_ < upper_);
#endif

    return index_ >= lower_ and index_ < upper_;
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
export [[nodiscard]] constexpr bool bound_check(std::integral auto index, bound_check_range_helper auto&& range) noexcept
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
