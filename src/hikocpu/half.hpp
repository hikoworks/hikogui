// Copyright Take Vos 2020-2022.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "macros.hpp"
#include "half_to_float.hpp"
#include "float_to_half.hpp"
#include <cstdint>
#include <type_traits>
#include <bit>
#include <algorithm>
#include <numeric>
#include <format>

hi_export_module(hikocpu : half);

hi_warning_push();
// C26472: Don't use static_cast for arithmetic conversions, Use brace initialization, gsl::narrow_cast or gsl::narrow (type.1).
// static_cast here is used to extract bits and cause sign-extension.
hi_warning_ignore_msvc(26472);

hi_export namespace hi {
inline namespace v1 {

struct half {
    uint16_t v = 0;

    constexpr half() noexcept = default;
    constexpr half(half const&) noexcept = default;
    constexpr half(half&&) noexcept = default;
    constexpr half& operator=(half const&) noexcept = default;
    constexpr half& operator=(half&&) noexcept = default;
    constexpr half(std::in_place_t, uint16_t v) noexcept : v(v) {}

    constexpr explicit half(float other) noexcept : v(float_to_half(other)) {}

    constexpr operator float() const noexcept
    {
        return half_to_float(v);
    }

    [[nodiscard]] constexpr uint16_t const& intrinsic() const noexcept
    {
        return v;
    }

    [[nodiscard]] constexpr uint16_t& intrinsic() noexcept
    {
        return v;
    }

    constexpr half& operator=(float other) noexcept
    {
        v = float_to_half(other);
        return *this;
    }

    [[nodiscard]] constexpr friend bool isnan(half const &rhs) noexcept
    {
        auto const e = (rhs.v >> 10) & 0x1f;
        auto const m = rhs.v & 0x3ff;
        return e == 0x1f and m != 0;
    }

    [[nodiscard]] constexpr friend bool iszero(half const &rhs) noexcept
    {
        return rhs.v == 0 or rhs.v == 0x8000;
    }

    [[nodiscard]] constexpr friend bool isinf(half const &rhs) noexcept
    {
        return rhs.v == 0x7c00 or rhs.v == 0xfc00;
    }

    [[nodiscard]] constexpr friend bool isnormal(half const &rhs) noexcept
    {
        auto const e = (rhs.v >> 10) & 0x1f;
        return e != 0 and e != 0x1f;
    }

    [[nodiscard]] constexpr friend bool issubnormal(half const &rhs) noexcept
    {
        auto const e = (rhs.v >> 10) & 0x1f;
        auto const m = rhs.v & 0x3ff;
        return e == 0 and m != 0;
    }

    [[nodiscard]] constexpr friend bool isfinite(half const &rhs) noexcept
    {
        return not isnan(rhs) and not isinf(rhs);
    }

    [[nodiscard]] constexpr friend half abs(half const &rhs) noexcept
    {
        return half(std::in_place, rhs.v & 0x7fff);
    }

    [[nodiscard]] constexpr friend half copysign(half const &lhs, half const &rhs) noexcept
    {
        return half(std::in_place, (lhs.v & 0x7fff) | (rhs.v & 0x8000));
    }

    [[nodiscard]] std::size_t hash() const noexcept
    {
        return std::hash<uint16_t>{}(v);
    }

    [[nodiscard]] constexpr friend bool operator==(half lhs, half rhs) noexcept
    {
        if (isnan(lhs) or isnan(rhs)) [[unlikely]] {
            return false;
        }
        if (iszero(lhs) and iszero(rhs)) [[unlikely]] {
            return true;
        }
        return lhs.v == rhs.v;
    }

    template<typename LHS, typename RHS>
    [[nodiscard]] constexpr friend bool operator==(LHS const &lhs, RHS const &rhs) noexcept requires requires {
        static_cast<float>(lhs);
        static_cast<float>(rhs);
    }
    {
        return static_cast<float>(lhs) == static_cast<float>(rhs);
    }

    template<typename LHS, typename RHS>
    [[nodiscard]] constexpr friend auto operator<=>(LHS const &lhs, RHS const &rhs) noexcept requires requires {
        static_cast<float>(lhs);
        static_cast<float>(rhs);
    }
    {
        return static_cast<float>(lhs) <=> static_cast<float>(rhs);
    }

#define HI_X_binary_math_op(op) \
    template<typename LHS, typename RHS> \
    [[nodiscard]] constexpr friend float operator op(LHS const &lhs, RHS const &rhs) noexcept requires requires { \
        static_cast<float>(lhs); \
        static_cast<float>(rhs); \
    } \
    { \
        return half{static_cast<float>(lhs) op static_cast<float>(rhs)}; \
    }

    // clang-format off
    HI_X_binary_math_op(+)
    HI_X_binary_math_op(-)
    HI_X_binary_math_op(*)
    HI_X_binary_math_op(/)
    // clang-format on
#undef HI_X_binary_math_op

    //[[nodiscard]] constexpr friend half operator%(half const& lhs, half const& rhs) noexcept
    //{
    //    auto const lhs_ = static_cast<float>(lhs);
    //    auto const rhs_ = static_cast<float>(rhs);
    //    auto const div_result = std::floor(lhs_ / rhs_);
    //    return half{lhs_ - (div_result * rhs_)};
    //}

#define HI_X_binary_bit_op(op) \
    [[nodiscard]] constexpr friend half operator op(half const& lhs, half const& rhs) noexcept \
    { \
        return half(std::in_place, lhs.v op rhs.v); \
    }

        // clang-format off
    HI_X_binary_bit_op(|)
    HI_X_binary_bit_op(&)
    HI_X_binary_bit_op(^)
    // clang-format on
#undef HI_X_binary_bit_op
};

// Check if half can be std::bit_cast<uint16_t>().
static_assert(sizeof(half) == sizeof(uint16_t));
static_assert(std::is_trivially_copy_constructible_v<half>);
static_assert(std::is_trivially_move_constructible_v<half>);
static_assert(std::is_trivially_copy_assignable_v<half>);
static_assert(std::is_trivially_move_assignable_v<half>);
static_assert(std::is_trivially_destructible_v<half>);

static_assert(requires(half a) { std::bit_cast<uint16_t>(a); });
static_assert(requires(uint16_t a) { std::bit_cast<half>(a); });

} // namespace v1
} // namespace hi::inline v1

hi_export namespace std {
template<>
struct hash<::hi::half> {
    size_t operator()(::hi::half const& rhs) noexcept
    {
        return rhs.hash();
    }
};

// XXX #617 MSVC bug does not handle partial specialization in modules.
template<>
struct formatter<::hi::half, char> : formatter<float, char> {
    constexpr auto format(::hi::half const& t, auto& fc) const
    {
        return formatter<float, char>::format(static_cast<float>(t), fc);
    }
};

template<>
struct numeric_limits<::hi::half> {
    using value_type = ::hi::half;

    constexpr static bool is_specialized = true;
    constexpr static bool is_signed = true;
    constexpr static bool is_integer = false;
    constexpr static bool is_exact = false;
    constexpr static bool has_infinity = true;
    constexpr static bool has_quiet_NaN = true;
    constexpr static bool has_signaling_NaN = false;
    constexpr static float_round_style round_style = round_to_nearest;
    constexpr static bool is_iec559 = true;
    constexpr static bool is_bounded = true;
    constexpr static bool is_modulo = false;
    constexpr static int digits = 10;
    constexpr static int digits10 = 4;
    constexpr static int max_digits10 = 4;
    constexpr static int min_exponent = -14;
    constexpr static int min_exponent10 = -3;
    constexpr static int max_exponent = 15;
    constexpr static int max_exponent10 = 3;
    constexpr static bool traps = false;
    constexpr static bool tinyness_before = false;

    constexpr static value_type min() noexcept
    {
        return ::hi::half(std::in_place, 0x0400);
    }

    constexpr static value_type lowest() noexcept
    {
        return ::hi::half(std::in_place, 0xfbff);
    }

    constexpr static value_type max() noexcept
    {
        return ::hi::half(std::in_place, 0x7bff);
    }

    constexpr static value_type epsilon() noexcept
    {
        return ::hi::half(std::in_place, 0xfbff);
    }

    constexpr static value_type round_error() noexcept
    {
        return ::hi::half(std::in_place, 0x3800); // 0.5
    }

    constexpr static value_type infinity() noexcept
    {
        return ::hi::half(std::in_place, 0x7c00);
    }

    constexpr static value_type quiet_NaN() noexcept
    {
        return ::hi::half(std::in_place, 0x7c01);
    }

    constexpr static value_type signaling_NaN() noexcept
    {
        return ::hi::half(std::in_place, 0x7e01);
    }

    constexpr static value_type denorm_min() noexcept
    {
        return ::hi::half(std::in_place, 0x0001);
    }
};
}

hi_warning_pop();
