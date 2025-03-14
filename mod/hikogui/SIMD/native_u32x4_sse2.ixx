// Copyright Take Vos 2022, 2023.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

module;
#include "../macros.hpp"

#include <span>
#include <array>
#include <ostream>

#ifdef HI_HAS_SSE
#include <xmmintrin.h>
#endif
#ifdef HI_HAS_SSE2
#include <emmintrin.h>
#endif
#ifdef HI_HAS_SSE3
#include <pmmintrin.h>
#endif
#ifdef HI_HAS_SSSE3
#include <tmmintrin.h>
#endif
#ifdef HI_HAS_SSE4_1
#include <smmintrin.h>
#endif
#ifdef HI_HAS_SSE4_2
#include <nmmintrin.h>
#endif
#ifdef HI_HAS_AVX
#include <immintrin.h>
#endif

export module hikogui_SIMD : native_u32x4_sse2;
import : native_simd_utility;
import hikogui_utility;


hi_warning_push();
// Ignore "C26490: Don't use reinterpret_cast", needed for intrinsic loads and stores.
hi_warning_ignore_msvc(26490);

export namespace hi { inline namespace v1 {

#ifdef HI_HAS_SSE2

/** A uint32_t x 4 (__m128i) SSE2 register.
 *
 *
 * When loading and storing from memory this is the order of the element in the register
 *
 * ```
 *   lo           hi lo           hi lo           hi lo           hi
 *  +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
 *  | element 0/a/x | element 1/b/y | element 2/c/z | element 3/d/w |
 *  +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
 *    0   1   2   3   4   5   6   7   8   9  10  11  12  13  14  15   memory address.
 * ```
 *
 * In the function below a `mask` values least-significant-bit corresponds to element 0.
 *
 */
template<>
struct native_simd<uint32_t,4> {
    using value_type = uint32_t;
    constexpr static size_t size = 4;
    using register_type = __m128i;
    using array_type = std::array<value_type, size>;

    register_type v;

    native_simd(native_simd const&) noexcept = default;
    native_simd(native_simd&&) noexcept = default;
    native_simd& operator=(native_simd const&) noexcept = default;
    native_simd& operator=(native_simd&&) noexcept = default;

    /** Initialize all elements to zero.
     */
    native_simd() noexcept : v(_mm_setzero_si128()) {}

    [[nodiscard]] explicit native_simd(register_type other) noexcept : v(other) {}

    [[nodiscard]] explicit operator register_type() const noexcept
    {
        return v;
    }

    /** Initialize the element to the values in the arguments.
     *
     * @param a The value for element 0.
     * @param b The value for element 1.
     * @param c The value for element 2.
     * @param d The value for element 3.
     */
    [[nodiscard]] native_simd(
        value_type a,
        value_type b = value_type{0},
        value_type c = value_type{0},
        value_type d = value_type{0}) noexcept :
        v(_mm_set_epi32(
            std::bit_cast<int32_t>(d),
            std::bit_cast<int32_t>(c),
            std::bit_cast<int32_t>(b),
            std::bit_cast<int32_t>(a)))
    {
    }

    [[nodiscard]] explicit native_simd(value_type const *other) noexcept :
        v(_mm_loadu_si128(reinterpret_cast<register_type const *>(other)))
    {
    }

    void store(value_type *out) const noexcept
    {
        hi_axiom_not_null(out);
        _mm_storeu_si128(reinterpret_cast<register_type *>(out), v);
    }

    [[nodiscard]] explicit native_simd(void const *other) noexcept : v(_mm_loadu_si128(static_cast<register_type const *>(other)))
    {
    }

    void store(void *out) const noexcept
    {
        hi_axiom_not_null(out);
        _mm_storeu_si128(static_cast<register_type *>(out), v);
    }

    [[nodiscard]] explicit native_simd(std::span<value_type const> other) noexcept
    {
        hi_axiom(other.size() >= size);
        v = _mm_loadu_si128(reinterpret_cast<register_type const *>(other.data()));
    }

    void store(std::span<value_type> out) const noexcept
    {
        hi_axiom(out.size() >= size);
        _mm_storeu_si128(reinterpret_cast<register_type *>(out.data()), v);
    }

    [[nodiscard]] explicit native_simd(array_type other) noexcept :
        v(_mm_loadu_si128(reinterpret_cast<register_type const *>(other.data())))
    {
    }

    [[nodiscard]] explicit operator array_type() const noexcept
    {
        auto r = array_type{};
        _mm_storeu_si128(reinterpret_cast<register_type *>(r.data()), v);
        return r;
    }

    [[nodiscard]] explicit native_simd(native_simd<int32_t,4> const &a) noexcept;

    /** Broadcast a single value to all the elements.
     *
     * ```
     * r[0] = a
     * r[1] = a
     * r[2] = a
     * r[3] = a
     * ```
     */
    [[nodiscard]] static native_simd broadcast(value_type a) noexcept
    {
        return native_simd{_mm_set1_epi32(std::bit_cast<int32_t>(a))};
    }

    /** Broadcast the first element to all the elements.
     *
     * ```
     * r[0] = a[0]
     * r[1] = a[0]
     * r[2] = a[0]
     * r[3] = a[0]
     * ```
     */
    [[nodiscard]] static native_simd broadcast(native_simd a) noexcept
    {
#ifdef HI_HAS_AVX2
        return native_simd{_mm_broadcastd_epi32(a.v)};
#else
        return native_simd{_mm_shuffle_epi32(a.v, 0b00'00'00'00)};
#endif
    }

    [[nodiscard]] static native_simd ones() noexcept
    {
        hilet tmp = _mm_undefined_si128();
        return native_simd{_mm_cmpeq_epi32(tmp, tmp)};
    }

    /** For each bit in mask set corresponding element to all-ones or all-zeros.
     */
    [[nodiscard]] static native_simd from_mask(size_t mask) noexcept
    {
        hi_axiom(mask <= 0b1111);

        constexpr auto ones_ = std::bit_cast<value_type>(0xffff'ffffU);
        return native_simd{
            mask & 0b0001 ? ones_ : 0, mask & 0b0010 ? ones_ : 0, mask & 0b0100 ? ones_ : 0, mask & 0b1000 ? ones_ : 0};
    }

    /** Concatenate the top bit of each element.
     */
    [[nodiscard]] size_t mask() const noexcept
    {
        return narrow_cast<size_t>(_mm_movemask_ps(_mm_castsi128_ps(v)));
    }

    [[nodiscard]] friend bool equal(native_simd a, native_simd b) noexcept
    {
        return (a == b).mask() == 0b1111;
    }

    [[nodiscard]] friend native_simd operator==(native_simd a, native_simd b) noexcept
    {
        return native_simd{_mm_cmpeq_epi32(a.v, b.v)};
    }

    [[nodiscard]] friend native_simd operator!=(native_simd a, native_simd b) noexcept
    {
        return ~(a == b);
    }

    [[nodiscard]] friend native_simd operator+(native_simd a) noexcept
    {
        return a;
    }

    [[nodiscard]] friend native_simd operator+(native_simd a, native_simd b) noexcept
    {
        return native_simd{_mm_add_epi32(a.v, b.v)};
    }

    [[nodiscard]] friend native_simd operator-(native_simd a, native_simd b) noexcept
    {
        return native_simd{_mm_sub_epi32(a.v, b.v)};
    }

    [[nodiscard]] friend native_simd operator*(native_simd a, native_simd b) noexcept
    {
        return native_simd{_mm_mullo_epi32(a.v, b.v)};
    }

    [[nodiscard]] friend native_simd operator&(native_simd a, native_simd b) noexcept
    {
        return native_simd{_mm_and_si128(a.v, b.v)};
    }

    [[nodiscard]] friend native_simd operator|(native_simd a, native_simd b) noexcept
    {
        return native_simd{_mm_or_si128(a.v, b.v)};
    }

    [[nodiscard]] friend native_simd operator^(native_simd a, native_simd b) noexcept
    {
        return native_simd{_mm_xor_si128(a.v, b.v)};
    }

    [[nodiscard]] friend native_simd operator~(native_simd a) noexcept
    {
        auto ones = _mm_undefined_si128();
        ones = _mm_cmpeq_epi32(ones, ones);
        return native_simd{_mm_andnot_si128(a.v, ones)};
    }

    [[nodiscard]] friend native_simd operator<<(native_simd a, unsigned int b) noexcept
    {
        hi_axiom_bounds(b, sizeof(value_type) * CHAR_BIT);
        return native_simd{_mm_slli_epi32(a.v, b)};
    }

    [[nodiscard]] friend native_simd operator>>(native_simd a, unsigned int b) noexcept
    {
        hi_axiom_bounds(b, sizeof(value_type) * CHAR_BIT);
        return native_simd{_mm_srli_epi32(a.v, b)};
    }

    [[nodiscard]] friend native_simd min(native_simd a, native_simd b) noexcept
    {
        return native_simd{_mm_min_epu32(a.v, b.v)};
    }

    [[nodiscard]] friend native_simd max(native_simd a, native_simd b) noexcept
    {
        return native_simd{_mm_max_epu32(a.v, b.v)};
    }

    /** Set elements to zero.
     *
     * @tparam Mask A bit mask corresponding to each element.
     * @param a The value to modify.
     * @return argument @a with elements set to zero where the corresponding @a Mask bit was '1'.
     */
    template<size_t Mask>
    [[nodiscard]] friend native_simd set_zero(native_simd a) noexcept
    {
        static_assert(Mask <= 0b1111);
#ifdef HI_HAS_SSE4_1
        return native_simd{_mm_castps_si128(_mm_insert_ps(_mm_castsi128_ps(a.v), _mm_castsi128_ps(a.v), Mask))};
#else
        hilet mask = from_mask(Mask);
        return not_and(mask, a);
#endif
    }

    /** Insert a value into an element of a vector.
     *
     * @tparam Index the index of the element where insert the value.
     * @param a The vector to insert the value into.
     * @param b The value to insert.
     * @return The vector with the inserted value.
     */
    template<size_t Index>
    [[nodiscard]] friend native_simd insert(native_simd a, value_type b) noexcept
    {
        static_assert(Index < 4);

#ifdef HI_HAS_SSE4_1
        return native_simd{_mm_insert_epi32(a.v, std::bit_cast<int32_t>(b), Index)};
#else
        hilet mask = from_mask(1_uz << Index);
        return not_and(mask, a) | (mask & broadcast(b));
#endif
    }

    /** Extract an element from a vector.
     *
     * @tparam Index the index of the element.
     * @param a The vector to select the element from.
     * @return The value of the selected element.
     */
    template<size_t Index>
    [[nodiscard]] friend value_type get(native_simd a) noexcept
    {
#ifdef HI_HAS_SSE4_1
        return std::bit_cast<value_type>(_mm_extract_epi32(a.v, Index));
#else
        auto r = static_cast<array_type>(a);
        return std::get<Index>(r);
#endif
    }

    /** Select elements from two vectors.
     *
     * @tparam Mask A mask to select bits from @a a when '0'; or @a b when '1'. The
     *         lsb corresponds with element zero.
     * @param a A vector for which element are selected when the bit in @a Mask is '0'.
     * @param b A vector for which element are selected when the bit in @a Mask is '1'.
     * @return A vector with element selected from @a a and @a b
     */
    template<size_t Mask>
    [[nodiscard]] friend native_simd blend(native_simd a, native_simd b) noexcept
    {
#ifdef HI_HAS_SSE4_1
        return native_simd{_mm_blend_epi32(a.v, b.v, Mask)};
#else
        hilet mask = from_mask(Mask);
        return not_and(mask, a) | (mask & b);
#endif
    }

    /** Permute elements, ignoring numeric elements.
     *
     * The characters in @a SourceElements mean the following:
     * - 'a' - 'p': The indices to elements 0 and 15 of @a a.
     * - 'x', 'y', 'z', 'w'': The indices to elements 0, 1, 2, 3 of @a a.
     * - Any other character is treated as if the original element was selected.
     *
     * @tparam SourceElements A string representing the order of elements. First character
     *         matches the first element.
     * @param a The vector to swizzle the elements
     * @returns A vector with the elements swizzled.
     */
    template<fixed_string SourceElements>
    [[nodiscard]] friend native_simd permute(native_simd a) noexcept
    {
        constexpr auto order = detail::native_swizzle_to_packed_indices<SourceElements, size>();

        if constexpr (order == 0b11'10'01'00) {
            return a;
        } else if constexpr (order == 0b00'00'00'00) {
            return broadcast(a);
        } else {
            return native_simd{_mm_shuffle_epi32(a.v, order)};
        }
    }

    /** Swizzle elements.
     *
     * The elements are swizzled in the order specified in @a SourceElements.
     * Each character in @a SourceElements is a index to an element in @a a or
     * a numeric value.
     *
     * The characters in @a SourceElements mean the following:
     * - 'a' - 'p': The indices to elements 0 and 15 of @a a.
     * - 'x', 'y', 'z', 'w'': The indices to elements 0, 1, 2, 3 of @a a.
     * - '0', '1': The values 0 and 1.
     *
     * @tparam SourceElements A string representing the order of elements. First character
     *         matches the first element.
     * @param a The vector to swizzle the elements
     * @returns A vector with the elements swizzled.
     */
    template<fixed_string SourceElements>
    [[nodiscard]] friend native_simd swizzle(native_simd a) noexcept
    {
        constexpr auto one_mask = detail::native_swizzle_to_mask<SourceElements, size, '1'>();
        constexpr auto zero_mask = detail::native_swizzle_to_mask<SourceElements, size, '0'>();
        constexpr auto number_mask = one_mask | zero_mask;

        if constexpr (number_mask == 0b1111) {
            // Swizzle was /[01][01][01][01]/.
            return swizzle_numbers<SourceElements>();

        } else if constexpr (number_mask == 0b0000) {
            // Swizzle was /[^01][^01][^01][^01]/.
            return permute<SourceElements>(a);

#ifdef HI_HAS_SSE4_1
        } else if constexpr (number_mask == zero_mask) {
            // Swizzle was /[^1][^1][^1][^1]/.
            hilet ordered = permute<SourceElements>(a);
            return set_zero<zero_mask>(ordered);
#endif

        } else {
            hilet ordered = permute<SourceElements>(a);
            hilet numbers = swizzle_numbers<SourceElements>();
            return blend<number_mask>(ordered, numbers);
        }
    }

#ifdef HI_HAS_SSE3
    /** Horizontal add.
     *
     * Add elements pair-wise in both vectors, then merge the results:
     * ```
     * r[0] = a[0] + a[1]
     * r[1] = a[2] + a[3]
     * r[2] = b[0] + b[1]
     * r[3] = b[2] + b[3]
     * ```
     */
    [[nodiscard]] friend native_simd horizontal_add(native_simd a, native_simd b) noexcept
    {
        return native_simd{_mm_hadd_epi32(a.v, b.v)};
    }
#endif

#ifdef HI_HAS_SSE3
    /** Horizontal subtract.
     *
     * Subtract elements pair-wise in both vectors, then merge the results:
     * ```
     * r[0] = a[0] - a[1]
     * r[1] = a[2] - a[3]
     * r[2] = b[0] - b[1]
     * r[3] = b[2] - b[3]
     * ```
     */
    [[nodiscard]] friend native_simd horizontal_sub(native_simd a, native_simd b) noexcept
    {
        return native_simd{_mm_hsub_epi32(a.v, b.v)};
    }
#endif

    /** Sum all elements of a vector.
     *
     * ```
     * r = broadcast(a[0] + a[1] + a[2] + a[3])
     * ```
     */
    [[nodiscard]] friend native_simd horizontal_sum(native_simd a) noexcept
    {
        hilet tmp = a + permute<"cdab">(a);
        return tmp + permute<"badc">(tmp);
    }

    /** not followed by and.
     *
     * r = ~a & b
     *
     */
    [[nodiscard]] friend native_simd not_and(native_simd a, native_simd b) noexcept
    {
        return native_simd{_mm_andnot_si128(a.v, b.v)};
    }

    friend std::ostream& operator<<(std::ostream& a, native_simd b) noexcept
    {
        return a << "(" << get<0>(b) << ", " << get<1>(b) << ", " << get<2>(b) << ", " << get<3>(b) << ")";
    }

    template<fixed_string SourceElements>
    [[nodiscard]] static native_simd swizzle_numbers() noexcept
    {
        constexpr auto one_mask = detail::native_swizzle_to_mask<SourceElements, size, '1'>();
        constexpr auto zero_mask = detail::native_swizzle_to_mask<SourceElements, size, '0'>();
        constexpr auto number_mask = one_mask | zero_mask;
        constexpr auto alpha_mask = ~number_mask & 0b1111;

        if constexpr ((zero_mask | alpha_mask) == 0b1111) {
            return native_simd{_mm_setzero_si128()};

        } else if constexpr ((one_mask | alpha_mask) == 0b1111) {
            return native_simd{_mm_set1_epi32(1)};

        } else {
            return native_simd{
                to_bool(one_mask & 0b0001) ? 1 : 0,
                to_bool(one_mask & 0b0010) ? 1 : 0,
                to_bool(one_mask & 0b0100) ? 1 : 0,
                to_bool(one_mask & 0b1000) ? 1 : 0};
        }
    }
};

#endif

}} // namespace hi::v1

hi_warning_pop();
