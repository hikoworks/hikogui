// Copyright Take Vos 2019-2022.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

/** @file math.hpp Miscellaneous math functions.
 */

#pragma once

#include "type_traits.hpp"
#include "../macros.hpp"
#include "assert.hpp"
#include "cast.hpp"
#include "terminate.hpp"
#include "exception.hpp"
#include <complex>
#include <cmath>
#include <limits>
#include <span>
#include <tuple>
#include <numeric>
#include <iterator>
#include <bit>
#include <concepts>
#include <algorithm>
#include <numbers>

#if HI_COMPILER == HI_CC_MSVC
#include <intrin.h>
#endif
#if HI_PROCESSOR == HI_CPU_X86_64
#include <immintrin.h>
#endif

hi_export_module(hikogui.utility.math);

hi_export namespace hi::inline v1 {

template<typename Iterator>
[[nodiscard]] auto mean(Iterator first, Iterator last)
{
    auto const init = static_cast<typename std::iterator_traits<Iterator>::value_type>(0);

    auto const sum = std::reduce(first, last, init);
    auto const count = static_cast<decltype(sum)>(std::distance(first, last));

    return count > 0.0 ? sum / count : sum;
}

template<typename Iterator, typename T>
[[nodiscard]] auto stddev(Iterator first, Iterator last, T mean)
{
    auto const init = static_cast<typename std::iterator_traits<Iterator>::value_type>(0);

    auto const sum = std::accumulate(first, last, init, [=](auto const& acc, auto const& value) {
        auto const tmp = value - mean;
        return acc + tmp * tmp;
    });

    auto const count = static_cast<decltype(sum)>(std::distance(first, last));
    return count > 0.0 ? sum / count : sum;
}

template<std::equality_comparable T>
constexpr bool inplace_max(T& a, T const& b) noexcept
{
    using std::max;
    a = max(a, b);
    return a == b;
}

template<std::equality_comparable T>
constexpr bool inplace_min(T& a, T const& b) noexcept
{
    using std::min;
    a = min(a, b);
    return a == b;
}

template<typename T>
constexpr void inplace_max(T& a, T const& b) noexcept
{
    using std::max;
    a = max(a, b);
}

template<typename T>
constexpr void inplace_min(T& a, T const& b) noexcept
{
    using std::min;
    a = min(a, b);
}

template<typename T>
constexpr void inplace_clamp(T& a, T const& lo, T const& hi) noexcept
{
    hi_axiom(lo <= hi);

    using std::clamp;
    a = clamp(a, lo, hi);
}

template<typename T>
[[nodiscard]] constexpr T abs(T a) noexcept
{
    return a < T{} ? -a : a;
}

template<std::floating_point T>
[[nodiscard]] constexpr bool almost_equal(T a, T b) noexcept
{
    auto e = (a + b) * std::numeric_limits<T>::epsilon();
    return std::abs(a - b) <= e;
}

/** Convert degree to radian.
 *
 * @param degree The number of degrees.
 * @return The number of radians.
 */
template<std::floating_point T>
[[nodiscard]] constexpr T to_radian(T degree) noexcept
{
    return degree * (std::numbers::pi_v<T> / T{180.0});
}

/** The greatest multiple of alignment less than or equal to value.
 * @param value The unsigned value to round.
 * @param alignment The alignment.
 * @return The greatest multiple of alignment less than or equal to value.
 */
template<std::unsigned_integral T>
[[nodiscard]] constexpr T floor(T value, T alignment) noexcept
{
    return (value / alignment) * alignment;
}

/** The smallest multiple of alignment greater than or equal to value.
 * @param value The unsigned value to round.
 * @param alignment The alignment.
 * @return The smallest multiple of alignment greater than or equal to value.
 */
template<std::unsigned_integral T>
[[nodiscard]] constexpr T ceil(T value, T alignment) noexcept
{
    return floor(value + (alignment - 1), alignment);
}

template<std::floating_point T>
[[nodiscard]] constexpr bool isnan(T value) noexcept
{
    return not (value == value);
}

constexpr unsigned long long pow10_table[20] = {
    1ULL,
    10ULL,
    100ULL,
    1'000ULL,
    10'000ULL,
    100'000ULL,
    1'000'000ULL,
    10'000'000ULL,
    100'000'000ULL,
    1'000'000'000ULL,
    10'000'000'000ULL,
    100'000'000'000ULL,
    1'000'000'000'000ULL,
    10'000'000'000'000ULL,
    100'000'000'000'000ULL,
    1'000'000'000'000'000ULL,
    10'000'000'000'000'000ULL,
    100'000'000'000'000'000ULL,
    1'000'000'000'000'000'000ULL,
};

[[nodiscard]] constexpr uint64_t pow10(unsigned int x) noexcept
{
    hi_axiom_bounds(x, 20);
    return pow10_table[x];
}

template<std::unsigned_integral T>
[[nodiscard]] constexpr unsigned int decimal_width(T x)
{
    // clang-format off
    constexpr uint8_t guess_table[] = {
         0,  0,  0,  0,  1,  1,  1,  2,  2,  2,  3,  3,  3,  3,  4,  4,
         4,  5,  5,  5,  6,  6,  6,  6,  7,  7,  7,  8,  8,  8,  9,  9,
         9,  9, 10, 10, 10, 11, 11, 11, 12, 12, 12, 12, 13, 13, 13, 14,
        14, 14, 15, 15, 15, 15, 16, 16, 16, 17, 17, 17, 18, 18, 18, 18
    };
    // clang-format on

    auto const num_bits = std::bit_width(x);
    hi_axiom_bounds(num_bits, guess_table);
    auto const guess = guess_table[num_bits];
    return guess + wide_cast<unsigned int>(x >= pow10(guess));
}

template<std::signed_integral T>
[[nodiscard]] constexpr unsigned int decimal_width(T x)
{
    return decimal_width(narrow_cast<std::make_unsigned_t<T>>(abs(x)));
}

} // namespace hi::inline v1
