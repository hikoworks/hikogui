// Copyright Take Vos 2020-2022.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

/** @file utility/cast.hpp Functions for casting values between types savely.
 * @ingroup utility
 */

#pragma once

#include "type_traits.hpp"
#include "terminate.hpp"
#include "../macros.hpp"
#include "assert.hpp"
#include "compare.hpp"
#include "concepts.hpp"
#include "exception.hpp"
#include <type_traits>
#include <typeinfo>
#include <concepts>
#include <limits>
#include <span>
#include <numeric>
#include <bit>
#include <cmath>

hi_export_module(hikogui.utility.cast);

hi_warning_push();
// C26472: Don't use static_cast for arithmetic conversions, Use brace initialization, gsl::narrow_cast or gsl::narrow (type.1).
// This file implements narrow_cast().
hi_warning_ignore_msvc(26472);
// C26467: Converting from floating point to unsigned integral types results in non-portable code if the double/float has
// a negative value. Use gsl::narrow_cast or gsl::naroow instead to guard against undefined behavior and potential data loss
// (es.46).
// This file implements narrow_cast().
hi_warning_ignore_msvc(26467);
// C26496: The variable 'r' does not change after construction, mark it as const (con.4).
// False positive
hi_warning_ignore_msvc(26496);
// C26466: Don't use static_cast downcast. A cast from a polymorphic type should use dynamic_cast (type.2)
// Used in down_cast<>() specifically for doing this savely.
hi_warning_ignore_msvc(26466);
// C26474: Don't cast between pointer types when the conversion could be implicit (type.1).
// Since these functions are templates this happens.
hi_warning_ignore_msvc(26474);
// C4702 unreachable code: Suppressed due intrinsics and std::is_constant_evaluated()
hi_warning_ignore_msvc(4702);

hi_export namespace hi { inline namespace v1 {

template<typename T>
[[nodiscard]] constexpr T copy(T value) noexcept
{
    return value;
}

/** Cast a pointer to a class to its base class or itself.
 *
 * @ingroup utility
 * @tparam Out The output type; a base-class of the input.
 * @param rhs A pointer to a object to cast.
 * @return A pointer to casted type.
 */
template<typename Out, typename In>
[[nodiscard]] constexpr Out up_cast(In *rhs) noexcept
{
    using out_type = std::remove_pointer_t<Out>;

    static_assert(std::is_pointer_v<Out>, "up_cast() Out template paramater must be a pointer if the input is a pointer.");
    static_assert(std::is_const_v<out_type> == std::is_const_v<In> or std::is_const_v<out_type>, "up_cast() can not cast away const.");
    static_assert(std::is_base_of_v<out_type, In>, "up_cast() may only be used to cast to a base-type.");

    return static_cast<Out>(rhs);
}

/** Cast a nullptr to a class.
 *
 * @ingroup utility
 * @tparam Out The output type.
 * @param rhs A nullptr.
 * @return A nullptr of the output type
 */
template<typename Out>
[[nodiscard]] constexpr Out up_cast(nullptr_t) noexcept
{
    static_assert(std::is_pointer_v<Out>, "up_cast() Out template paramater must be a pointer.");
    return nullptr;
}

/** Cast a reference to a class to its base class or itself.
 *
 * @ingroup utility
 * @tparam Out The output type; a base-class of the input.
 * @param rhs A reference to a object to cast.
 * @return A reference to casted type.
 */
template<typename Out, typename In>
[[nodiscard]] constexpr Out up_cast(In& rhs) noexcept
{
    using out_type = std::remove_reference_t<Out>;

    static_assert(std::is_reference_v<Out>, "up_cast() Out template paramater must be a reference if the input is a reference.");
    static_assert(std::is_const_v<out_type> == std::is_const_v<In> or std::is_const_v<out_type>, "up_cast() can not cast away const.");
    static_assert(std::is_base_of_v<out_type, In>, "up_cast() may only be used to cast to a base-type.");

    return static_cast<Out>(rhs);
}

/** Cast a pointer to a class to its derived class or itself.
 *
 * @ingroup utility
 * @note It is undefined behavior if the argument is not of type Out.
 * @param rhs A pointer to an object that is of type `Out`. Or a nullptr which will be
 *        passed through.
 * @return A pointer to the same object with a new type.
 */
template<typename Out, typename In>
[[nodiscard]] constexpr Out down_cast(In *rhs) noexcept
{
    using out_type = std::remove_pointer_t<Out>;

    static_assert(std::is_pointer_v<Out>, "down_cast() Out template paramater must be a pointer if the input is a pointer.");
    static_assert(std::is_const_v<out_type> == std::is_const_v<In> or std::is_const_v<out_type>, "down_cast() can not cast away const.");
    static_assert(std::is_base_of_v<out_type, In> or std::is_base_of_v<In, out_type>, "down_cast() may only be used to cast to a related type.");

    if constexpr (not std::is_base_of_v<out_type, In>) {
        hi_axiom(rhs == nullptr or dynamic_cast<Out>(rhs) != nullptr);
    }
    return static_cast<Out>(rhs);
}

/** Cast a pointer to a class to its derived class or itself.
 *
 * @ingroup utility
 * @return A pointer to the same object with a new type.
 */
template<typename Out>
[[nodiscard]] constexpr Out down_cast(nullptr_t) noexcept
    requires std::is_pointer_v<Out>
{
    return nullptr;
}

/** Cast a reference to a class to its derived class or itself.
 *
 * @ingroup utility
 * @note It is undefined behaviour if the argument is not of type Out.
 * @param rhs A reference to an object that is of type `Out`.
 * @return A reference to the same object with a new type.
 */
template<typename Out, typename In>
[[nodiscard]] constexpr Out down_cast(In& rhs) noexcept
{
    using out_type = std::remove_reference_t<Out>;

    static_assert(std::is_reference_v<Out>, "down_cast() Out template paramater must be a reference if the input is a reference.");
    static_assert(std::is_const_v<out_type> == std::is_const_v<In> or std::is_const_v<out_type>, "down_cast() can not cast away const.");
    static_assert(std::is_base_of_v<out_type, In> or std::is_base_of_v<In, out_type>, "down_cast() may only be used to cast to a related type.");

    if constexpr (not std::is_base_of_v<out_type, In>) {
        hi_axiom(dynamic_cast<std::add_pointer_t<out_type>>(std::addressof(rhs)) != nullptr);
    }
    return static_cast<Out>(rhs);
}

/** Cast to a type which can hold all values from the input type.
 *
 * @ingroup utility
 * @note This is the identity operation, when casting to the same type.
 * @tparam Out The same type as the input.
 * @param rhs The value of the input type.
 * @return A copy of the input value.
 */
template<typename Out, std::same_as<Out> In>
[[nodiscard]] constexpr Out wide_cast(In const& rhs) noexcept
{
    return rhs;
}

/** Cast a floating point number to a floating point type that is wider.
 *
 * @ingroup utility
 * @tparam Out A floating point type larger than the input type.
 * @param rhs The floating point input value.
 * @return The floating point value converted to a wider floating point type.
 */
template<std::floating_point Out, std::floating_point In>
[[nodiscard]] constexpr Out wide_cast(In const& rhs) noexcept
    requires(not std::same_as<In, Out>)
{
    static_assert(
        std::numeric_limits<In>::digits <= std::numeric_limits<Out>::digits,
        "wide_cast() is only allowed to a floating point of the same or larger size.");

    return static_cast<Out>(rhs);
}

/** Cast a integer to an integer type which is wider.
 *
 * @ingroup utility
 * @tparam Out An integer type that can hold all values of the input type.
 * @param rhs The integer input value.
 * @return The value converted to a wider integer type.
 */
template<std::integral Out, std::integral In>
[[nodiscard]] constexpr Out wide_cast(In rhs) noexcept
    requires(not std::same_as<In, Out>)
{
    static_assert(
        std::numeric_limits<In>::is_signed == std::numeric_limits<Out>::is_signed or not std::numeric_limits<In>::is_signed,
        "wide_cast() is only allowed if the input is unsigned or if both input and output have the same signess.");

    static_assert(
        std::numeric_limits<In>::digits <= std::numeric_limits<Out>::digits,
        "wide_cast() is only allowed to an integer of the same or larger size.");

    return static_cast<Out>(rhs);
}

/** Cast a integer to an float type which is wider.
 *
 * Since wide_cast() must be perfect the integers must be perfectly representable by a floating
 * point number. Integers that have number of binary digits less or equal to the size of the mantissa
 * of a floating point number can be perfectly represented.
 *
 *  - Up to uint16_t, int16_t can be wide_cast() to float.
 *  - Up to uint32_t, int32_t can be wide_cast() to double.
 *
 * @ingroup utility
 * @tparam Out An float type that can hold all values of the input type without loss of precission.
 * @param rhs The integer input value.
 * @return The value converted to a wider float type.
 */
template<std::floating_point Out, std::integral In>
[[nodiscard]] constexpr Out wide_cast(In rhs) noexcept
{
    static_assert(
        std::numeric_limits<In>::digits <= std::numeric_limits<Out>::digits,
        "wide_cast() is only allowed if the input can be represented with perfect accuracy by the floating point output type.");

    return static_cast<Out>(rhs);
}

/** Cast a numeric value to an integer saturating on overflow.
 *
 * @tparam Out the signed- or unsigned-integer type to cast to.
 * @param rhs The value to convert.
 * @return The converted value, which is saturated if @a rhs is over- or underflowing.
 */
template<std::integral Out, arithmetic In>
[[nodiscard]] constexpr Out saturate_cast(In rhs) noexcept
{
    if constexpr (std::is_floating_point_v<In>) {
        if (std::isnan(rhs)) {
            return Out{0};
        }
    }

    if (three_way_compare(rhs, std::numeric_limits<Out>::lowest()) != std::strong_ordering::greater) {
        return std::numeric_limits<Out>::lowest();
    } else if (three_way_compare(rhs, std::numeric_limits<Out>::max()) != std::strong_ordering::less) {
        return std::numeric_limits<Out>::max();
    } else {
        return static_cast<Out>(rhs);
    }
}

/** Check if a value can be casted to a narrow type.
 *
 * @ingroup utility
 * @tparam Out The output type.
 * @param rhs The input value to cast.
 * @return true if the value can be casted.
 */
template<typename Out, std::same_as<Out> In>
[[nodiscard]] constexpr bool can_narrow_cast(In const& rhs) noexcept
{
    return true;
}

/** Check if a value can be casted to a narrow type.
 *
 * @ingroup utility
 * @tparam Out The output type.
 * @param rhs The input value to cast.
 * @return true if the value can be casted.
 */
template<std::floating_point Out, std::floating_point In>
[[nodiscard]] constexpr bool can_narrow_cast(In const& rhs) noexcept
    requires(not std::same_as<In, Out>)
{
    if constexpr (std::numeric_limits<In>::digits > std::numeric_limits<Out>::digits) {
        // cast is allowed when the input is NaN, infinite or within the range of the output type.
        return rhs != rhs or rhs == std::numeric_limits<In>::infinity() or rhs == -std::numeric_limits<In>::infinity() or
            (rhs >= std::numeric_limits<Out>::lowest() and rhs <= std::numeric_limits<Out>::max());
    }

    return true;
}

/** Check if a value can be casted to a narrow type.
 *
 * @ingroup utility
 * @tparam Out The output type.
 * @param rhs The input value to cast.
 * @return true if the value can be casted.
 */
template<std::integral Out, std::integral In>
[[nodiscard]] constexpr bool can_narrow_cast(In const& rhs) noexcept
    requires(not std::same_as<In, Out>)
{
    if constexpr (std::numeric_limits<In>::is_signed == std::numeric_limits<Out>::is_signed) {
        if constexpr (std::numeric_limits<In>::digits > std::numeric_limits<Out>::digits) {
            if constexpr (std::numeric_limits<In>::is_signed) {
                if (rhs < static_cast<In>(std::numeric_limits<Out>::lowest())) {
                    return false;
                }
            }
            return rhs <= static_cast<In>(std::numeric_limits<Out>::max());
        }

    } else if constexpr (std::numeric_limits<In>::is_signed) {
        if (rhs < 0) {
            return false;
        }

        if constexpr (std::numeric_limits<In>::digits > std::numeric_limits<Out>::digits) {
            return rhs <= static_cast<In>(std::numeric_limits<Out>::max());
        }

    } else {
        if constexpr (std::numeric_limits<In>::digits > std::numeric_limits<Out>::digits) {
            return rhs <= static_cast<In>(std::numeric_limits<Out>::max());
        }
    }

    return true;
}

/** Check if a value can be casted to a narrow type.
 *
 * @ingroup utility
 * @tparam Out The output type.
 * @param rhs The input value to cast.
 * @return true if the value can be casted.
 */
template<std::floating_point Out, std::integral In>
[[nodiscard]] constexpr bool can_narrow_cast(In const& rhs) noexcept
{
    if constexpr (std::numeric_limits<In>::digits > std::numeric_limits<Out>::digits) {
        if constexpr (std::numeric_limits<In>::is_signed) {
            constexpr auto max = (1LL << std::numeric_limits<Out>::digits) - 1;
            constexpr auto lowest = -max;

            return rhs >= lowest and rhs <= max;

        } else {
            constexpr auto max = (1ULL << std::numeric_limits<Out>::digits) - 1;

            return rhs <= max;
        }
    }

    return true;
}

/** Cast numeric values without loss of precision.
 *
 * @ingroup utility
 * @note It is undefined behavior to cast a value which will cause a loss of precision.
 * @tparam Out The numeric type to cast to
 * @tparam In The numeric type to cast from
 * @param rhs The value to cast.
 * @return The value casted to a different type without loss of precision.
 */
template<typename Out, std::same_as<Out> In>
[[nodiscard]] constexpr Out narrow_cast(In const& rhs) noexcept
{
    return rhs;
}

/** Cast numeric values without loss of precision.
 *
 * @ingroup utility
 * @note It is undefined behavior to cast a value which will cause a loss of precision.
 * @tparam Out The numeric type to cast to
 * @tparam In The numeric type to cast from
 * @param rhs The value to cast.
 * @return The value casted to a different type without loss of precision.
 */
template<std::floating_point Out, std::floating_point In>
[[nodiscard]] constexpr Out narrow_cast(In const& rhs) noexcept
    requires(not std::same_as<In, Out>)
{
    if constexpr (std::numeric_limits<In>::digits > std::numeric_limits<Out>::digits) {
        // cast is allowed when the input is NaN, infinite or within the range of the output type.
        hi_axiom(
            rhs != rhs or rhs == std::numeric_limits<In>::infinity() or rhs == -std::numeric_limits<In>::infinity() or
            (rhs >= std::numeric_limits<Out>::lowest() and rhs <= std::numeric_limits<Out>::max()));
    }

    return static_cast<Out>(rhs);
}

/** Cast numeric values without loss of precision.
 *
 * @ingroup utility
 * @note It is undefined behavior to cast a value which will cause a loss of precision.
 * @tparam Out The numeric type to cast to
 * @tparam In The numeric type to cast from
 * @param rhs The value to cast.
 * @return The value casted to a different type without loss of precision.
 */
template<std::integral Out, std::integral In>
[[nodiscard]] constexpr Out narrow_cast(In const& rhs) noexcept
    requires(not std::same_as<In, Out>)
{
    if constexpr (std::numeric_limits<In>::is_signed == std::numeric_limits<Out>::is_signed) {
        if constexpr (std::numeric_limits<In>::digits > std::numeric_limits<Out>::digits) {
            if constexpr (std::numeric_limits<In>::is_signed) {
                hi_axiom(rhs >= static_cast<In>(std::numeric_limits<Out>::lowest()));
            }
            hi_axiom(rhs <= static_cast<In>(std::numeric_limits<Out>::max()));
        }

    } else if constexpr (std::numeric_limits<In>::is_signed) {
        hi_axiom(rhs >= 0);
        if constexpr (std::numeric_limits<In>::digits > std::numeric_limits<Out>::digits) {
            hi_axiom(rhs <= static_cast<In>(std::numeric_limits<Out>::max()));
        }

    } else {
        if constexpr (std::numeric_limits<In>::digits > std::numeric_limits<Out>::digits) {
            hi_axiom(rhs <= static_cast<In>(std::numeric_limits<Out>::max()));
        }
    }

    return static_cast<Out>(rhs);
}

/** Cast numeric values without loss of precision.
 *
 * @ingroup utility
 * @note It is undefined behavior to cast a value which will cause a loss of precision.
 * @tparam Out The numeric type to cast to
 * @tparam In The numeric type to cast from
 * @param rhs The value to cast.
 * @return The value casted to a different type without loss of precision.
 */
template<std::floating_point Out, std::integral In>
[[nodiscard]] constexpr Out narrow_cast(In const& rhs) noexcept
{
    if constexpr (std::numeric_limits<In>::digits > std::numeric_limits<Out>::digits) {
        if constexpr (std::numeric_limits<In>::is_signed) {
            constexpr auto max = (1LL << std::numeric_limits<Out>::digits) - 1;
            constexpr auto lowest = -max;

            hi_axiom(rhs >= lowest and rhs <= max);

        } else {
            constexpr auto max = (1ULL << std::numeric_limits<Out>::digits) - 1;

            hi_axiom(rhs <= max);
        }
    }

    return static_cast<Out>(rhs);
}

template<std::integral Out, std::floating_point In>
[[nodiscard]] constexpr bool can_round_cast(In rhs) noexcept
{
    auto const rhs_ = std::round(rhs);
    return rhs_ >= std::numeric_limits<Out>::lowest() and rhs_ <= std::numeric_limits<Out>::max();
}

template<std::integral Out, std::floating_point In>
[[nodiscard]] constexpr bool can_floor_cast(In rhs) noexcept
{
    auto const rhs_ = std::floor(rhs);
    return rhs_ >= std::numeric_limits<Out>::lowest() and rhs_ <= std::numeric_limits<Out>::max();
}

template<std::integral Out, std::floating_point In>
[[nodiscard]] constexpr bool can_ceil_cast(In rhs) noexcept
{
    auto const rhs_ = std::ceil(rhs);
    return rhs_ >= std::numeric_limits<Out>::lowest() and rhs_ <= std::numeric_limits<Out>::max();
}

template<std::integral Out, std::floating_point In>
[[nodiscard]] constexpr Out round_cast(In rhs) noexcept
{
    auto const lowest = static_cast<long double>(std::numeric_limits<Out>::lowest());
    auto const highest = static_cast<long double>(std::numeric_limits<Out>::max());

    auto const rhs_ = std::round(rhs);
    hi_axiom(rhs_ >= lowest and rhs_ <= highest);
    return static_cast<Out>(rhs_);
}

template<std::integral Out, std::floating_point In>
[[nodiscard]] constexpr Out floor_cast(In rhs) noexcept
{
    auto const lowest = static_cast<long double>(std::numeric_limits<Out>::lowest());
    auto const highest = static_cast<long double>(std::numeric_limits<Out>::max());

    auto const rhs_ = std::floor(rhs);
    hi_axiom(rhs_ >= lowest and rhs_ <= highest);
    return static_cast<Out>(rhs_);
}

template<std::integral Out, std::floating_point In>
[[nodiscard]] constexpr Out ceil_cast(In rhs) noexcept
{
    auto const lowest = static_cast<long double>(std::numeric_limits<Out>::lowest());
    auto const highest = static_cast<long double>(std::numeric_limits<Out>::max());

    auto const rhs_ = std::ceil(rhs);
    hi_axiom(rhs_ >= lowest and rhs_ <= highest);
    return static_cast<Out>(rhs_);
}

/** Cast an integral to an unsigned integral of the same size.
 */
template<std::integral In>
[[nodiscard]] constexpr std::make_unsigned_t<In> to_unsigned(In rhs) noexcept
{
    return static_cast<std::make_unsigned_t<In>>(rhs);
}

/** Cast an integral to an signed integral of the same size.
 */
template<std::integral In>
[[nodiscard]] constexpr std::make_signed_t<In> to_signed(In rhs) noexcept
{
    return static_cast<std::make_signed_t<In>>(rhs);
}

/** Cast between integral types truncating or zero-extending the result.
 */
template<std::integral Out, std::integral In>
[[nodiscard]] constexpr Out truncate(In rhs) noexcept
{
    return static_cast<Out>(to_unsigned(rhs));
}

/** Cast a character.
 *
 * Both the input and output types are interpreted as unsigned values, even if
 * they are signed values. For example `char` may be either signed or unsigned,
 * but you have to treat those as unsigned values.
 *
 * @note @a rhs value after casting, must fit in the output type.
 * @param rhs The value of the character.
 * @return The casted value.
 */
template<std::integral Out, std::integral In>
[[nodiscard]] constexpr Out char_cast(In rhs) noexcept
{
    using in_unsigned_type = std::make_unsigned_t<In>;
    using out_unsigned_type = std::make_unsigned_t<Out>;

    // We cast to unsigned of the same type, so that we don't accidentally sign extent 'char'.
    auto in_unsigned = static_cast<in_unsigned_type>(rhs);
    auto out_unsigned = narrow_cast<out_unsigned_type>(in_unsigned);
    return static_cast<Out>(out_unsigned);
}

/** Cast a character.
 *
 * Both the input and output types are interpreted as unsigned values, even if
 * they are signed values. For example `char` may be either signed or unsigned,
 * but you have to treat those as unsigned values.
 *
 * @note @a rhs value after casting, must fit in the output type.
 * @param rhs The value of the character.
 * @return The casted value.
 */
template<std::integral Out>
[[nodiscard]] constexpr Out char_cast(std::byte rhs) noexcept
{
    return char_cast<Out>(static_cast<uint8_t>(rhs));
}

/** Return the low half of the input value.
 */
template<std::unsigned_integral OutType, std::unsigned_integral InType>
[[nodiscard]] constexpr OutType low_bit_cast(InType value) noexcept
{
    static_assert(sizeof(OutType) * 2 == sizeof(InType), "Return value of low_bit_cast must be half the size of the input");
    return static_cast<OutType>(value);
}

/** Return the upper half of the input value.
 */
template<std::unsigned_integral OutType, std::unsigned_integral InType>
[[nodiscard]] constexpr OutType high_bit_cast(InType value) noexcept
{
    static_assert(sizeof(OutType) * 2 == sizeof(InType), "Return value of high_bit_cast must be half the size of the input");
    return static_cast<OutType>(value >> sizeof(OutType) * CHAR_BIT);
}

/** Return the low half of the input value.
 */
template<std::signed_integral OutType, std::signed_integral InType>
[[nodiscard]] constexpr OutType low_bit_cast(InType value) noexcept
{
    using UInType = std::make_unsigned_t<InType>;
    using UOutType = std::make_unsigned_t<OutType>;
    return static_cast<OutType>(low_bit_cast<UOutType>(static_cast<UInType>(value)));
}

/** Return the upper half of the input value.
 */
template<std::signed_integral OutType, std::signed_integral InType>
[[nodiscard]] constexpr OutType high_bit_cast(InType value) noexcept
{
    using UInType = std::make_unsigned_t<InType>;
    using UOutType = std::make_unsigned_t<OutType>;
    return static_cast<OutType>(high_bit_cast<UOutType>(static_cast<UInType>(value)));
}

/** Return the low half of the input value.
 */
template<std::unsigned_integral OutType, std::signed_integral InType>
[[nodiscard]] constexpr OutType low_bit_cast(InType value) noexcept
{
    using UInType = std::make_unsigned_t<InType>;
    return low_bit_cast<OutType>(static_cast<UInType>(value));
}

/** Return the upper half of the input value.
 */
template<std::unsigned_integral OutType, std::signed_integral InType>
[[nodiscard]] constexpr OutType high_bit_cast(InType value) noexcept
{
    using UInType = std::make_unsigned_t<InType>;
    return high_bit_cast<OutType>(static_cast<UInType>(value));
}

/** Return the upper half of the input value.
 */
template<std::unsigned_integral OutType, std::unsigned_integral InType>
[[nodiscard]] constexpr OutType merge_bit_cast(InType hi, InType lo) noexcept
{
    static_assert(sizeof(OutType) == sizeof(InType) * 2, "Return value of merge_bit_cast must be double the size of the input");

    OutType r = static_cast<OutType>(hi);
    r <<= sizeof(InType) * CHAR_BIT;
    r |= static_cast<OutType>(lo);
    return r;
}

/** Return the upper half of the input value.
 */
template<std::signed_integral OutType, std::signed_integral InType>
[[nodiscard]] constexpr OutType merge_bit_cast(InType hi, InType lo) noexcept
{
    using UInType = std::make_unsigned_t<InType>;
    using UOutType = std::make_unsigned_t<OutType>;
    return static_cast<OutType>(merge_bit_cast<UOutType>(static_cast<UInType>(hi), static_cast<UInType>(lo)));
}

/** Return the upper half of the input value.
 */
template<std::signed_integral OutType, std::unsigned_integral InType>
[[nodiscard]] constexpr OutType merge_bit_cast(InType hi, InType lo) noexcept
{
    using UOutType = std::make_unsigned_t<OutType>;
    return narrow_cast<OutType>(merge_bit_cast<UOutType>(hi, lo));
}

template<typename T>
[[nodiscard]] constexpr bool to_bool(T&& rhs) noexcept
    requires(requires(T&& x) { static_cast<bool>(std::forward<T>(x)); })
{
    return static_cast<bool>(std::forward<T>(rhs));
}

/** Create a mask from a boolean value.
 * 
 * @param v The boolean value to represent as a mask
 * @return A value which bit representations are all '1' when @a v == `true`, or
 *         all '0' when @a v == `false`.
*/
template<typename T>
[[nodiscard]] constexpr T to_mask(bool v) noexcept
{
    using large_bool = make_intxx_t<sizeof(T) * CHAR_BIT>;

    auto r = static_cast<large_bool>(v);
    r <<= sizeof(T) * CHAR_BIT - 1;
    r >>= sizeof(T) * CHAR_BIT - 1;
    return std::bit_cast<T>(r);
}

template<typename T>
[[nodiscard]] inline T to_ptr(std::intptr_t value) noexcept
    requires std::is_pointer_v<T>
{
    return reinterpret_cast<T>(value);
}

template<typename T>
[[nodiscard]] std::intptr_t to_int(T *ptr) noexcept
{
    return reinterpret_cast<std::intptr_t>(ptr);
}

/** Cast a byte-like buffer to a type with implicit lifetime.
 *
 * This function is used to cast a buffer to a type with an implicit lifetime.
 * For example when mapping a font-file into memory and casting parts of the
 * memory to easy to use structures.
 * 
 * @note Do not access the buffer during the implicit-lifetime of the returned
 *       value, this would be undefined behavior.
 * @note This function expects mmap() and other memory mapping functions to
 *       be blessed in respect to automatically creating implicit lifetimes
 *       objects.
 * @tparam T The type to cast to, must be an implicit lifetime type.
 * @param bytes The buffer to cast. The buffer must be aligned and at least the
 *              size of @a T.
 * @return A reference to the value in the buffer.
 * @throws std::bad_cast when the buffer is too small or not aligned.
 */
template<typename T, byte_like Byte>
[[nodiscard]] copy_cv_t<T, Byte>& implicit_cast(std::span<Byte> bytes)
{
    using value_type = copy_cv_t<T, Byte>;

    static_assert(std::is_trivially_default_constructible_v<value_type>);
    static_assert(std::is_trivially_destructible_v<value_type>);

    if (sizeof(value_type) > bytes.size()) {
        throw std::bad_cast();
    }
    hi_axiom_not_null(bytes.data());

    if constexpr (alignof(value_type) != 1) {
        if (std::bit_cast<std::uintptr_t>(bytes.data()) % alignof(value_type) != 0) {
            throw std::bad_cast();
        }
    }

    // XXX #708 use std::start_lifetime_as<T> here
    //return std::start_lifetime_as<value_type>(bytes.data());
    return *reinterpret_cast<value_type *>(bytes.data());
}

/** Cast a byte-like buffer to a array of type with implicit lifetime.
 *
 * This function is used to cast a buffer to am array of type with an implicit
 * lifetime. For example when mapping a font-file into memory and casting parts
 * of the memory to easy to use structures.
 *
 * @note Do not access the buffer during the implicit-lifetime of the returned
 *       value, this would be undefined behavior.
 * @note This function expects mmap() and other memory mapping functions to
 *       be blessed in respect to automatically creating implicit lifetimes
 *       objects.
 * @tparam T The type to cast to, must be an implicit lifetime type.
 * @param bytes The buffer to cast. The buffer must be aligned and at least the
 *              size of @a T.
 * @param n The number of values in the buffer.
 * @return A span of values in the buffer.
 * @throws std::bad_cast when the buffer is too small or not aligned.
 */
template<typename T, byte_like Byte>
[[nodiscard]] std::span<copy_cv_t<T, Byte>> implicit_cast(std::span<Byte> bytes, size_t n)
{
    using value_type = copy_cv_t<T, Byte>;

    static_assert(std::is_trivially_default_constructible_v<value_type>);
    static_assert(std::is_trivially_destructible_v<value_type>);

    if (sizeof(value_type) * n > bytes.size()) {
        throw std::bad_cast();
    }
    hi_axiom_not_null(bytes.data());

    if constexpr (alignof(value_type) != 1) {
        if (std::bit_cast<std::uintptr_t>(bytes.data()) % alignof(value_type) != 0) {
            throw std::bad_cast();
        }
    }

    // XXX #708 use std::start_lifetime_as<T> here
    //return {std::start_lifetime_as_array<value_type>(bytes.data(), n), n};
    return {reinterpret_cast<value_type *>(bytes.data()), n};
}

/** Cast a byte-like buffer to a array of type with implicit lifetime.
 *
 * This function is used to cast a buffer to am array of type with an implicit
 * lifetime. For example when mapping a font-file into memory and casting parts
 * of the memory to easy to use structures.
 *
 * @note Do not access the buffer during the implicit-lifetime of the returned
 *       value, this would be undefined behavior.
 * @note This function expects mmap() and other memory mapping functions to
 *       be blessed in respect to automatically creating implicit lifetimes
 *       objects.
 * @tparam T The type to cast to, must be an implicit lifetime type.
 * @param offset The offset into the buffer where the value is located. The
 *               offset will be incremented by the size of the value.
 * @param bytes The buffer to cast. The buffer must be aligned and at least the
 *              size of @a T.
 * @param n The number of values in the buffer.
 * @return A span of values in the buffer.
 * @throws std::bad_cast when the buffer is too small or not aligned.
 */
template<typename T, byte_like Byte>
[[nodiscard]] copy_cv_t<T, Byte>& implicit_cast(size_t& offset, std::span<Byte> bytes)
{
    using value_type = copy_cv_t<T, Byte>;

    static_assert(std::is_trivially_default_constructible_v<value_type>);
    static_assert(std::is_trivially_destructible_v<value_type>);

    if (sizeof(value_type) + offset > bytes.size()) {
        throw std::bad_cast();
    }
    hi_axiom_not_null(bytes.data());

    auto const data = bytes.data() + offset;

    if constexpr (alignof(value_type) != 1) {
        if (std::bit_cast<std::uintptr_t>(data) % alignof(value_type) != 0) {
            throw std::bad_cast();
        }
    }

    offset += sizeof(value_type);
    // XXX #708 use std::start_lifetime_as<T> here
    //return std::start_lifetime_as<value_type>(bytes.data());
    return *reinterpret_cast<value_type *>(data);
}

/** Cast a byte-like buffer to a array of type with implicit lifetime.
 *
 * This function is used to cast a buffer to am array of type with an implicit
 * lifetime. For example when mapping a font-file into memory and casting parts
 * of the memory to easy to use structures.
 *
 * @note Do not access the buffer during the implicit-lifetime of the returned
 *       value, this would be undefined behavior.
 * @note This function expects mmap() and other memory mapping functions to
 *       be blessed in respect to automatically creating implicit lifetimes
 *       objects.
 * @tparam T The type to cast to, must be an implicit lifetime type.
 * @param offset The offset into the buffer where the value is located. The
 *               offset will be incremented by the size of the array of values.
 * @param bytes The buffer to cast. The buffer must be aligned and at least the
 *              size of @a T.
 * @param n The number of values in the buffer.
 * @return A span of values in the buffer.
 * @throws std::bad_cast when the buffer is too small or not aligned.
 */
template<typename T, byte_like Byte>
[[nodiscard]] std::span<copy_cv_t<T, Byte>> implicit_cast(size_t& offset, std::span<Byte> bytes, size_t n)
{
    using value_type = copy_cv_t<T, Byte>;

    static_assert(std::is_trivially_default_constructible_v<value_type>);
    static_assert(std::is_trivially_destructible_v<value_type>);

    if (sizeof(value_type) * n + offset > bytes.size()) {
        throw std::bad_cast();
    }
    hi_axiom_not_null(bytes.data());

    auto const data = bytes.data() + offset;

    if constexpr (alignof(value_type) != 1) {
        if (std::bit_cast<std::uintptr_t>(data) % alignof(value_type) != 0) {
            throw std::bad_cast();
        }
    }

    offset += sizeof(value_type) * n;
    // XXX #708 use std::start_lifetime_as<T> here
    //return {std::start_lifetime_as_array<value_type>(bytes.data(), n), n};
    return {reinterpret_cast<value_type *>(data), n};
}

}} // namespace hi::inline v1

hi_warning_pop();
