

#pragma once

#include "array_intrinsic.hpp"
#include "macros.hpp"
#include <cstddef>
#include <cmath>
#include <array>
#include <type_traits>
#include <bit>
#include <algorithm>

hi_export_module(hikocpu : array_generic);

hi_warning_push();
// False positive: warning C4702: unreachable code.
hi_warning_ignore_msvc(4702);

hi_export namespace hi {
inline namespace v1 {

template<typename Context, typename T>
concept array_generic_convertible_to = requires (Context a) { static_cast<T>(a); };

/** Intrinsic operations on arrays.
 *
 */
template<typename T, std::size_t N>
struct array_generic {
    static_assert(N > 1);
    static_assert(std::has_single_bit(N));

    using value_type = T;
    using array_type = std::array<T, N>;
    using intrinsic_type = array_intrinsic<T, N>;

    // clang-format off
    using mask_type =
        std::conditional_t<sizeof(T) * CHAR_BIT == 8, uint8_t,
        std::conditional_t<sizeof(T) * CHAR_BIT == 16, uint16_t,
        std::conditional_t<sizeof(T) * CHAR_BIT == 32, uint32_t,
        std::conditional_t<sizeof(T) * CHAR_BIT == 64, uint64_t, void>>>>;
    // clang-format on
    using signed_mask_type = std::make_signed_t<mask_type>;

    [[nodiscard]] hi_force_inline constexpr static mask_type to_mask(value_type a) noexcept
    {
        return std::bit_cast<mask_type>(a);
    }

    [[nodiscard]] hi_force_inline constexpr static signed_mask_type to_signed_mask(value_type a) noexcept
    {
        return std::bit_cast<signed_mask_type>(a);
    }

    template<std::unsigned_integral M>
    [[nodiscard]] hi_force_inline constexpr static value_type to_value(M a) noexcept
    {
        return std::bit_cast<value_type>(static_cast<mask_type>(a));
    }

    template<std::signed_integral M>
    [[nodiscard]] hi_force_inline constexpr static value_type to_value(M a) noexcept
    {
        return std::bit_cast<value_type>(static_cast<signed_mask_type>(a));
    }

    constexpr static value_type _zero_mask = to_value(mask_type{0});
    constexpr static value_type _ones_mask = to_value(~mask_type{0});

    [[nodiscard]] hi_force_inline static array_type undefined() noexcept
    {
        if (not std::is_constant_evaluated()) {
            if constexpr (requires { intrinsic_type::undefined(); }) {
                return intrinsic_type::undefined();
            }
        }
        array_type r;
        return r;
    }

    template<std::same_as<value_type>... Args>
    [[nodiscard]] hi_force_inline constexpr static array_type set(Args... args) noexcept
        requires(sizeof...(Args) == N)
    {
        if (not std::is_constant_evaluated()) {
            if constexpr (requires { intrinsic_type::set(args...); }) {
                return intrinsic_type::set(args...);
            }
        }
        return array_type{args...};
    }

    [[nodiscard]] hi_force_inline constexpr static array_type set(value_type arg) noexcept
    {
        if (not std::is_constant_evaluated()) {
            if constexpr (requires { intrinsic_type::set(arg); }) {
                return intrinsic_type::set(arg);
            }
        }
        auto r = array_type{};
        std::get<0>(r) = arg;
        return r;
    }

    [[nodiscard]] hi_force_inline constexpr static array_type set_zero() noexcept
    {
        if (not std::is_constant_evaluated()) {
            if constexpr (requires { intrinsic_type::set_zero(); }) {
                return intrinsic_type::set_zero();
            }
        }
        auto r = array_type{};
        for (std::size_t i = 0; i != N; ++i) {
            r[i] = _zero_mask;
        }
        return r;
    }

    [[nodiscard]] hi_force_inline constexpr static array_type set_all_ones() noexcept
    {
        if (not std::is_constant_evaluated()) {
            if constexpr (requires { intrinsic_type::set_all_ones(); }) {
                return intrinsic_type::set_all_ones();
            }
        }
        auto r = array_type{};
        for (std::size_t i = 0; i != N; ++i) {
            r[i] = _ones_mask;
        }
        return r;
    }

    [[nodiscard]] hi_force_inline constexpr static array_type set_one() noexcept
    {
        if (not std::is_constant_evaluated()) {
            if constexpr (requires { intrinsic_type::set_one(); }) {
                return intrinsic_type::set_one();
            }
        }
        auto r = array_type{};
        for (std::size_t i = 0; i != N; ++i) {
            r[i] = value_type{1};
        }
        return r;
    }

    [[nodiscard]] hi_force_inline constexpr static array_type broadcast(value_type a) noexcept
    {
        if (not std::is_constant_evaluated()) {
            if constexpr (requires { intrinsic_type::broadcast(a); }) {
                return intrinsic_type::broadcast(a);
            }
        }
        auto r = array_type{};
        for (std::size_t i = 0; i != N; ++i) {
            r[i] = a;
        }
        return r;
    }

    [[nodiscard]] hi_force_inline constexpr static array_type broadcast(array_type a) noexcept
    {
        if (not std::is_constant_evaluated()) {
            if constexpr (requires { intrinsic_type::broadcast(a); }) {
                return intrinsic_type::broadcast(a);
            }
        }
        auto r = array_type{};
        for (std::size_t i = 0; i != N; ++i) {
            r[i] = std::get<0>(a);
        }
        return r;
    }

    template<size_t I>
    [[nodiscard]] hi_force_inline constexpr static value_type get(array_type a) noexcept
    {
        if (not std::is_constant_evaluated()) {
            if constexpr (requires { intrinsic_type::template get<I>(a); }) {
                return intrinsic_type::template get<I>(a);
            }
        }
        return std::get<I>(a);
    }

    /** Set each element to all ones or all zero based on the bits of the mask
     *
     *
     */
    [[nodiscard]] hi_force_inline constexpr static array_type set_mask(size_t mask) noexcept
    {
        if (not std::is_constant_evaluated()) {
            if constexpr (requires { intrinsic_type::set_mask(mask); }) {
                return intrinsic_type::set_mask(mask);
            }
        }
        auto r = array_type{};
        for (std::size_t i = 0; i != N; ++i) {
            r[i] = mask & 1 ? _ones_mask : _zero_mask;
            mask >>= 1;
        }
        return r;
    }

    /** Get an integer mask where each bit in the mask corresponds with the top-bit of each element.
     *
     *
     */
    [[nodiscard]] hi_force_inline constexpr static size_t get_mask(array_type a) noexcept
    {
        if (not std::is_constant_evaluated()) {
            if constexpr (requires { intrinsic_type::get_mask(a); }) {
                return intrinsic_type::get_mask(a);
            }
        }
        size_t mask = 0;
        for (std::size_t i = 0; i != N; ++i) {
            auto const tmp = to_signed_mask(a[i]) < 0 ? size_t{1} : size_t{0};
            mask |= tmp << i;
        }
        return mask;
    }

    template<array_generic_convertible_to<value_type> O>
    [[nodiscard]] hi_force_inline constexpr static array_type convert(std::array<O, N> a) noexcept
    {
        if (not std::is_constant_evaluated()) {
            if constexpr (requires { intrinsic_type::convert(a); }) {
                return intrinsic_type::convert(a);
            }
        }

        auto r = array_type{};
        for (std::size_t i = 0; i != N; ++i) {
            r[i] = static_cast<value_type>(a[i]);
        }
        return r;
    }

    [[nodiscard]] hi_force_inline constexpr static array_type neg(array_type a) noexcept
    {
        if (not std::is_constant_evaluated()) {
            if constexpr (requires { intrinsic_type::neg(a); }) {
                return intrinsic_type::neg(a);
            }
        }

        for (std::size_t i = 0; i != N; ++i) {
            a[i] = -a[i];
        }
        return a;
    }

    template<std::size_t Mask>
    [[nodiscard]] hi_force_inline constexpr static array_type neg_mask(array_type a) noexcept
    {
        if (not std::is_constant_evaluated()) {
            if constexpr (requires { intrinsic_type::template neg_mask<Mask>(a); }) {
                return intrinsic_type::template neg_mask<Mask>(a);
            }
        }

        return blend<Mask>(a, neg(a));
    }

    [[nodiscard]] hi_force_inline constexpr static array_type inv(array_type a) noexcept
    {
        if (not std::is_constant_evaluated()) {
            if constexpr (requires { intrinsic_type::inv(a); }) {
                return intrinsic_type::inv(a);
            }
        }

        for (std::size_t i = 0; i != N; ++i) {
            a[i] = to_value(~to_mask(a[i]));
        }
        return a;
    }

    [[nodiscard]] hi_force_inline constexpr static array_type rcp(array_type a) noexcept
    {
        if (not std::is_constant_evaluated()) {
            if constexpr (requires { intrinsic_type::rcp(a); }) {
                return intrinsic_type::rcp(a);
            }
        }

        for (std::size_t i = 0; i != N; ++i) {
            a[i] = value_type{1} / a[i];
        }
        return a;
    }

    [[nodiscard]] hi_force_inline static array_type sqrt(array_type a) noexcept
    {
        if (not std::is_constant_evaluated()) {
            if constexpr (requires { intrinsic_type::sqrt(a); }) {
                return intrinsic_type::sqrt(a);
            }
        }

        for (std::size_t i = 0; i != N; ++i) {
            a[i] = std::sqrt(a[i]);
        }
        return a;
    }

    [[nodiscard]] hi_force_inline static array_type rsqrt(array_type a) noexcept
    {
        if (not std::is_constant_evaluated()) {
            if constexpr (requires { intrinsic_type::rsqrt(a); }) {
                return intrinsic_type::rsqrt(a);
            }
        }

        for (std::size_t i = 0; i != N; ++i) {
            a[i] = value_type{1} / std::sqrt(a[i]);
        }
        return a;
    }

    [[nodiscard]] hi_force_inline constexpr static array_type abs(array_type a) noexcept
    {
        if (not std::is_constant_evaluated()) {
            if constexpr (requires { intrinsic_type::abs(a); }) {
                return intrinsic_type::abs(a);
            }
        }

        for (std::size_t i = 0; i != N; ++i) {
            a[i] = std::abs(a[i]);
        }
        return a;
    }

    [[nodiscard]] hi_force_inline constexpr static array_type round(array_type a) noexcept
    {
        if (not std::is_constant_evaluated()) {
            if constexpr (requires { intrinsic_type::round(a); }) {
                return intrinsic_type::round(a);
            }
        }

        for (std::size_t i = 0; i != N; ++i) {
            a[i] = std::round(a[i]);
        }
        return a;
    }

    [[nodiscard]] hi_force_inline constexpr static array_type floor(array_type a) noexcept
    {
        if (not std::is_constant_evaluated()) {
            if constexpr (requires { intrinsic_type::floor(a); }) {
                return intrinsic_type::floor(a);
            }
        }

        for (std::size_t i = 0; i != N; ++i) {
            a[i] = std::floor(a[i]);
        }
        return a;
    }

    [[nodiscard]] hi_force_inline constexpr static array_type ceil(array_type a) noexcept
    {
        if (not std::is_constant_evaluated()) {
            if constexpr (requires { intrinsic_type::ceil(a); }) {
                return intrinsic_type::ceil(a);
            }
        }

        for (std::size_t i = 0; i != N; ++i) {
            a[i] = std::ceil(a[i]);
        }
        return a;
    }

    [[nodiscard]] hi_force_inline constexpr static array_type add(array_type a, array_type b) noexcept
    {
        if (not std::is_constant_evaluated()) {
            if constexpr (requires { intrinsic_type::add(a, b); }) {
                return intrinsic_type::add(a, b);
            }
        }

        for (std::size_t i = 0; i != N; ++i) {
            a[i] = a[i] + b[i];
        }
        return a;
    }

    [[nodiscard]] hi_force_inline constexpr static array_type sub(array_type a, array_type b) noexcept
    {
        if (not std::is_constant_evaluated()) {
            if constexpr (requires { intrinsic_type::sub(a, b); }) {
                return intrinsic_type::sub(a, b);
            }
        }

        for (std::size_t i = 0; i != N; ++i) {
            a[i] = a[i] - b[i];
        }
        return a;
    }

    /** Add or subtract based on the mask
     *
     * If the mask bit is '1' then add, if the bit is '0' then substract.
     */
    template<std::size_t Mask>
    [[nodiscard]] hi_force_inline constexpr static array_type addsub_mask(array_type a, array_type b) noexcept
    {
        if (not std::is_constant_evaluated()) {
            if constexpr (requires { intrinsic_type::template addsub_mask<Mask>(a, b); }) {
                return intrinsic_type::template addsub_mask<Mask>(a, b);
            }
        }

        return blend<Mask>(sub(a, b), add(a, b));
    }

    [[nodiscard]] hi_force_inline constexpr static array_type mul(array_type a, array_type b) noexcept
    {
        if (not std::is_constant_evaluated()) {
            if constexpr (requires { intrinsic_type::mul(a, b); }) {
                return intrinsic_type::mul(a, b);
            }
        }

        for (std::size_t i = 0; i != N; ++i) {
            a[i] = a[i] * b[i];
        }
        return a;
    }

    [[nodiscard]] hi_force_inline constexpr static array_type div(array_type a, array_type b) noexcept
    {
        if (not std::is_constant_evaluated()) {
            if constexpr (requires { intrinsic_type::div(a, b); }) {
                return intrinsic_type::div(a, b);
            }
        }

        for (std::size_t i = 0; i != N; ++i) {
            a[i] = a[i] / b[i];
        }
        return a;
    }

    [[nodiscard]] hi_force_inline constexpr static array_type mod(array_type a, array_type b) noexcept
    {
        if (not std::is_constant_evaluated()) {
            if constexpr (requires { intrinsic_type::mod(a, b); }) {
                return intrinsic_type::mod(a, b);
            }
        }

        for (std::size_t i = 0; i != N; ++i) {
            if constexpr (std::floating_point<T>) {
                a[i] = std::fmod(a[i], b[i]);
            } else {
                a[i] = a[i] % b[i];
            }
        }
        return a;
    }

    [[nodiscard]] hi_force_inline constexpr static array_type eq(array_type a, array_type b) noexcept
    {
        if (not std::is_constant_evaluated()) {
            if constexpr (requires { intrinsic_type::eq(a, b); }) {
                return intrinsic_type::eq(a, b);
            }
        }

        for (std::size_t i = 0; i != N; ++i) {
            a[i] = a[i] == b[i] ? _ones_mask : _zero_mask;
        }
        return a;
    }

    [[nodiscard]] hi_force_inline constexpr static array_type ne(array_type a, array_type b) noexcept
    {
        if (not std::is_constant_evaluated()) {
            if constexpr (requires { intrinsic_type::ne(a, b); }) {
                return intrinsic_type::ne(a, b);
            }
        }

        for (std::size_t i = 0; i != N; ++i) {
            a[i] = a[i] != b[i] ? _ones_mask : _zero_mask;
        }
        return a;
    }

    [[nodiscard]] hi_force_inline constexpr static array_type lt(array_type a, array_type b) noexcept
    {
        if (not std::is_constant_evaluated()) {
            if constexpr (requires { intrinsic_type::lt(a, b); }) {
                return intrinsic_type::lt(a, b);
            }
        }

        for (std::size_t i = 0; i != N; ++i) {
            a[i] = a[i] < b[i] ? _ones_mask : _zero_mask;
        }
        return a;
    }

    [[nodiscard]] hi_force_inline constexpr static array_type gt(array_type a, array_type b) noexcept
    {
        if (not std::is_constant_evaluated()) {
            if constexpr (requires { intrinsic_type::gt(a, b); }) {
                return intrinsic_type::gt(a, b);
            }
        }

        for (std::size_t i = 0; i != N; ++i) {
            a[i] = a[i] > b[i] ? _ones_mask : _zero_mask;
        }
        return a;
    }

    [[nodiscard]] hi_force_inline constexpr static array_type le(array_type a, array_type b) noexcept
    {
        if (not std::is_constant_evaluated()) {
            if constexpr (requires { intrinsic_type::le(a, b); }) {
                return intrinsic_type::le(a, b);
            }
        }

        for (std::size_t i = 0; i != N; ++i) {
            a[i] = a[i] <= b[i] ? _ones_mask : _zero_mask;
        }
        return a;
    }

    [[nodiscard]] hi_force_inline constexpr static array_type ge(array_type a, array_type b) noexcept
    {
        if (not std::is_constant_evaluated()) {
            if constexpr (requires { intrinsic_type::ge(a, b); }) {
                return intrinsic_type::ge(a, b);
            }
        }

        for (std::size_t i = 0; i != N; ++i) {
            a[i] = a[i] >= b[i] ? _ones_mask : _zero_mask;
        }
        return a;
    }

    /** Test the two operands.
     *
     * This will AND the two operands together and if all the bits are zero
     * the test returns true.
     */
    [[nodiscard]] hi_force_inline constexpr static bool test(array_type a, array_type b) noexcept
    {
        if (not std::is_constant_evaluated()) {
            if constexpr (requires { intrinsic_type::test(a, b); }) {
                return intrinsic_type::test(a, b);
            }
        }

        auto r = mask_type{0};
        for (std::size_t i = 0; i != N; ++i) {
            r |= to_mask(a[i]) & to_mask(b[i]);
        }
        return r == 0;
    }

    /** Compare the two operands.
     *
     * This will compare both operands bit-wise and if equal return true.
     */
    [[nodiscard]] hi_force_inline constexpr static bool all_equal(array_type a, array_type b) noexcept
    {
        // All bits will be zero when a == b.
        auto const tmp = ne(a, b);
        // Return true is all bits in tmp are zero.
        return test(tmp, tmp);
    }

    [[nodiscard]] hi_force_inline constexpr static array_type max(array_type a, array_type b) noexcept
    {
        if (not std::is_constant_evaluated()) {
            if constexpr (requires { intrinsic_type::max(a, b); }) {
                return intrinsic_type::max(a, b);
            }
        }

        for (std::size_t i = 0; i != N; ++i) {
            a[i] = std::max(a[i], b[i]);
        }
        return a;
    }

    [[nodiscard]] hi_force_inline constexpr static array_type min(array_type a, array_type b) noexcept
    {
        if (not std::is_constant_evaluated()) {
            if constexpr (requires { intrinsic_type::min(a, b); }) {
                return intrinsic_type::min(a, b);
            }
        }

        for (std::size_t i = 0; i != N; ++i) {
            a[i] = std::min(a[i], b[i]);
        }
        return a;
    }

    [[nodiscard]] hi_force_inline constexpr static array_type clamp(array_type v, array_type lo, array_type hi) noexcept
    {
        if (not std::is_constant_evaluated()) {
            if constexpr (requires { intrinsic_type::clamp(v, lo, hi); }) {
                return intrinsic_type::clamp(v, lo, hi);
            }
        }

        for (std::size_t i = 0; i != N; ++i) {
            v[i] = std::clamp(v[i], lo[i], hi[i]);
        }
        return v;
    }

    [[nodiscard]] hi_force_inline constexpr static array_type _or(array_type a, array_type b) noexcept
    {
        if (not std::is_constant_evaluated()) {
            if constexpr (requires { intrinsic_type::_or(a, b); }) {
                return intrinsic_type::_or(a, b);
            }
        }

        for (std::size_t i = 0; i != N; ++i) {
            a[i] = to_value(to_mask(a[i]) | to_mask(b[i]));
        }
        return a;
    }

    [[nodiscard]] hi_force_inline constexpr static array_type _and(array_type a, array_type b) noexcept
    {
        if (not std::is_constant_evaluated()) {
            if constexpr (requires { intrinsic_type::_and(a, b); }) {
                return intrinsic_type::_and(a, b);
            }
        }

        for (std::size_t i = 0; i != N; ++i) {
            a[i] = to_value(to_mask(a[i]) & to_mask(b[i]));
        }
        return a;
    }

    [[nodiscard]] hi_force_inline constexpr static array_type _xor(array_type a, array_type b) noexcept
    {
        if (not std::is_constant_evaluated()) {
            if constexpr (requires { intrinsic_type::_xor(a, b); }) {
                return intrinsic_type::_xor(a, b);
            }
        }

        for (std::size_t i = 0; i != N; ++i) {
            a[i] = to_value(to_mask(a[i]) ^ to_mask(b[i]));
        }
        return a;
    }

    /** andnot of two operands
     *
     * The following calculation is done on the bits of the two operands:
     *  `result = ~a & b`
     *
     */
    [[nodiscard]] hi_force_inline constexpr static array_type andnot(array_type a, array_type b) noexcept
    {
        if (not std::is_constant_evaluated()) {
            if constexpr (requires { intrinsic_type::andnot(a, b); }) {
                return intrinsic_type::andnot(a, b);
            }
        }

        for (std::size_t i = 0; i != N; ++i) {
            a[i] = to_value(~to_mask(a[i]) & to_mask(b[i]));
        }
        return a;
    }

    [[nodiscard]] hi_force_inline constexpr static array_type sll(array_type a, unsigned int b) noexcept
    {
        if (not std::is_constant_evaluated()) {
            if constexpr (requires { intrinsic_type::sll(a, b); }) {
                return intrinsic_type::sll(a, b);
            }
        }

        if (b >= sizeof(value_type) * CHAR_BIT) {
            a = {};
        } else {
            for (std::size_t i = 0; i != N; ++i) {
                a[i] = to_value(to_mask(a[0]) << b);
            }
        }
        return a;
    }

    [[nodiscard]] hi_force_inline constexpr static array_type srl(array_type a, unsigned int b) noexcept
    {
        if (not std::is_constant_evaluated()) {
            if constexpr (requires { intrinsic_type::srl(a, b); }) {
                return intrinsic_type::srl(a, b);
            }
        }

        if (b >= sizeof(value_type) * CHAR_BIT) {
            a = {};
        } else {
            for (std::size_t i = 0; i != N; ++i) {
                a[i] = to_value(to_mask(a[0]) >> b);
            }
        }
        return a;
    }

    [[nodiscard]] hi_force_inline constexpr static array_type sra(array_type a, unsigned int b) noexcept
    {
        if (not std::is_constant_evaluated()) {
            if constexpr (requires { intrinsic_type::sra(a, b); }) {
                return intrinsic_type::sra(a, b);
            }
        }

        if (b >= sizeof(value_type) * CHAR_BIT) {
            b = sizeof(value_type) * CHAR_BIT - 1;
        }

        for (std::size_t i = 0; i != N; ++i) {
            a[i] = to_value(to_signed_mask(a[0]) >> b);
        }
        return a;
    }

    /** Add the elements of both operands pairwise and return the packed result.
     *
     */
    [[nodiscard]] hi_force_inline constexpr static array_type hadd(array_type a, array_type b) noexcept
    {
        if (not std::is_constant_evaluated()) {
            if constexpr (requires { intrinsic_type::hadd(a, b); }) {
                return intrinsic_type::hadd(a, b);
            }
        }

        auto r = array_type{};
        auto dst_i = 0;
        
        for (std::size_t src_i = 0; src_i != N; src_i += 2) {
            r[dst_i++] = a[src_i] + a[src_i + 1];
        }

        for (std::size_t src_i = 0; src_i != N; src_i += 2) {
            r[dst_i++] = b[src_i] + b[src_i + 1];
        }
        return r;
    }

    [[nodiscard]] hi_force_inline constexpr static array_type hsub(array_type a, array_type b) noexcept
    {
        if (not std::is_constant_evaluated()) {
            if constexpr (requires { intrinsic_type::hsub(a, b); }) {
                return intrinsic_type::hsub(a, b);
            }
        }

        auto r = array_type{};
        auto dst_i = 0;
        
        for (std::size_t src_i = 0; src_i != N; src_i += 2) {
            r[dst_i++] = a[src_i] - a[src_i + 1];
        }

        for (std::size_t src_i = 0; src_i != N; src_i += 2) {
            r[dst_i++] = b[src_i] - b[src_i + 1];
        }
        return r;
    }

    template<size_t I, int First, int... Rest>
    hi_force_inline constexpr static void _shuffle(array_type& r, array_type a) noexcept
    {
        static_assert(std::cmp_less(First, N));

        if constexpr (First < 0) {
            std::get<I>(r) = std::get<I>(a);
        } else {
            std::get<I>(r) = std::get<First>(a);
        }

        if constexpr (sizeof...(Rest)) {
            _shuffle<I + 1, Rest...>(r, a);
        }
    }

    template<size_t I, int First, int... Rest>
    [[nodiscard]] constexpr static bool _have_to_shuffle() noexcept
    {
        static_assert(std::cmp_less(First, N));

        if constexpr (First >= 0 and First != I) {
            return true;
        }

        if constexpr (sizeof...(Rest)) {
            return _have_to_shuffle<I + 1, Rest...>();
        } else {
            return false;
        }
    }

    template<int... Indices>
    [[nodiscard]] hi_force_inline constexpr static array_type shuffle(array_type a) noexcept
    {
        static_assert(sizeof...(Indices) == N);

        if constexpr (not _have_to_shuffle<0, Indices...>()) {
            // Short circuit for performance.
            return a;
        }

        if (not std::is_constant_evaluated()) {
            if constexpr (requires { intrinsic_type::template shuffle<Indices...>(a); }) {
                return intrinsic_type::template shuffle<Indices...>(a);
            }
        }

        auto r = array_type{};
        _shuffle<0, Indices...>(r, a);
        return r;
    }

    template<size_t Mask>
    [[nodiscard]] hi_force_inline constexpr static array_type blend(array_type a, array_type b) noexcept
    {
        if constexpr (Mask == 0) {
            // Short circuit for performance.
            return a;
        } else if constexpr (Mask == (1ULL << N) - 1) {
            // Short circuit for performance.
            return b;
        }

        if (not std::is_constant_evaluated()) {
            if constexpr (requires { intrinsic_type::template blend<Mask>(a, b); }) {
                return intrinsic_type::template blend<Mask>(a, b);
            }
        }

        auto mask = Mask;
        for (std::size_t i = 0; i != N; ++i) {
            a[i] = mask & 1 ? b[i] : a[i];
            mask >>= 1;
        }
        return a;
    }

    hi_warning_push();
    // C26494 Variable '...' is uninitialized. Always initialize an object (type.5).
    // Internal to _MM_TRANSPOSE4_PS
    hi_warning_ignore_msvc(26494);
    template<std::derived_from<array_type>... Columns>
    [[nodiscard]] constexpr static std::array<array_type, N> transpose(Columns... columns) noexcept
        requires(sizeof...(Columns) == N)
    {
        if (not std::is_constant_evaluated()) {
            if constexpr (requires { intrinsic_type::transpose(columns...); }) {
                return intrinsic_type::transpose(columns...);
            }
        }

        auto r = std::array<array_type, N>{};
        auto f = [&r, &columns... ]<std::size_t... Ints>(std::index_sequence<Ints...>)
        {
            auto tf = [&r](auto i, auto v) {
                for (std::size_t j = 0; j != N; ++j) {
                    r[j][i] = v[j];
                }
                return 0;
            };
            static_cast<void>((tf(Ints, columns) + ...));
        };
        f(std::make_index_sequence<sizeof...(columns)>{});
        return r;
    }
    hi_warning_pop();

    template<size_t I, int Value, int First, int... Rest>
    constexpr static void _make_swizzle_blend_mask(std::size_t& r) noexcept
    {
        if constexpr (First == Value) {
            r |= 1ULL << I;
        }

        if constexpr (sizeof...(Rest)) {
            _make_swizzle_blend_mask<I + 1, Value, Rest...>(r);
        }
    }

    template<int Value, int... Indices>
    [[nodiscard]] constexpr static std::size_t _make_swizzle_blend_mask() noexcept
    {
        auto r = std::size_t{0};
        _make_swizzle_blend_mask<0, Value, Indices...>(r);
        return r;
    }

    /** Swizzle elements.
     *
     * This function shuffles the elements or inserts the constants 0 or 1
     * depending on the value of the @a Indices.
     *
     * The @a Indices can have the following values:
     *  * -2: The value 1.
     *  * -1: The value 0.
     *  * other: The index of an element of @a a.
     */
    template<int... Indices>
    [[nodiscard]] hi_force_inline constexpr static array_type swizzle(array_type a) noexcept
    {
        constexpr auto zero_mask = _make_swizzle_blend_mask<-1, Indices...>();
        constexpr auto one_mask = _make_swizzle_blend_mask<-2, Indices...>();

        auto tmp = shuffle<Indices...>(a);
        if constexpr (zero_mask != 0) {
            tmp = blend<zero_mask>(tmp, set_zero());
        }
        if constexpr (one_mask != 0) {
            tmp = blend<one_mask>(tmp, set_one());
        }
        return tmp;
    }

    [[nodiscard]] hi_force_inline constexpr static array_type sum(array_type a) noexcept
    {
        if (not std::is_constant_evaluated()) {
            if constexpr (requires { intrinsic_type::sum(a); }) {
                return intrinsic_type::sum(a);
            }
        }

        auto r = value_type{0};
        for (std::size_t i = 0; i != N; ++i) {
            r = r + a[i];
        }
        return broadcast(r);
    }

    template<size_t Mask>
    [[nodiscard]] hi_force_inline constexpr static array_type dot(array_type a, array_type b) noexcept
    {
        if (not std::is_constant_evaluated()) {
            if constexpr (requires { intrinsic_type::template dot<Mask>(a, b); }) {
                return intrinsic_type::template dot<Mask>(a, b);
            }
        }

        auto const tmp1 = mul(a, b);
        auto const tmp2 = blend<Mask>(set_zero(), tmp1);
        return sum(tmp2);
    }

    template<size_t Mask>
    [[nodiscard]] hi_force_inline constexpr static array_type hypot(array_type a) noexcept
    {
        if (not std::is_constant_evaluated()) {
            if constexpr (requires { intrinsic_type::template hypot<Mask>(a); }) {
                return intrinsic_type::template hypot<Mask>(a);
            }
        }

        return sqrt(dot<Mask>(a, a));
    }

    template<size_t Mask>
    [[nodiscard]] hi_force_inline constexpr static array_type rhypot(array_type a) noexcept
    {
        if (not std::is_constant_evaluated()) {
            if constexpr (requires { intrinsic_type::template rhypot<Mask>(a); }) {
                return intrinsic_type::template rhypot<Mask>(a);
            }
        }

        return rsqrt(dot<Mask>(a, a));
    }

    template<size_t Mask>
    [[nodiscard]] hi_force_inline constexpr static array_type normalize(array_type a) noexcept
    {
        if (not std::is_constant_evaluated()) {
            if constexpr (requires { intrinsic_type::template normalize<Mask>(a); }) {
                return intrinsic_type::template normalize<Mask>(a);
            }
        }

        return mul(rhypot<Mask>(a), a);
    }

};

static_assert(array_generic<float, 4>::neg(std::array{1.0f, 2.0f, -2.0f, 0.0f}) == std::array{-1.0f, -2.0f, 2.0f, 0.0f});

} // namespace v1
}

hi_warning_pop();
