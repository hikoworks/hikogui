

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

export module hikogui_SIMD : native_i16x8_sse2;
import : native_simd_utility;

export namespace hi {
inline namespace v1 {

#ifdef HI_HAS_SSE2


/** A int16_t x 8 (__m128i) SSE2 register.
 *
 *
 * When loading and storing from memory this is the order of the element in the register
 *
 * ```
 *   lo           hi lo           hi lo           hi lo           hi
 *  +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
 *  | E 0/a | E 1/b | E 2/c | E 3/d | E 4/e | E 5/f | E 6/g | E 7/h |
 *  +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
 *    0   1   2   3   4   5   6   7   8   9  10  11  12  13  14  15   memory address.
 * ```
 *
 * In the function below a `mask` values least-significan-bit corrosponds to element 0.
 *
 */
struct native_i16x8 {
    using value_type = int16_t;
    constexpr static size_t size = 8;
    using register_type = __m128i;

    register_type v;

    native_i16x8(native_i16x8 const&) noexcept = default;
    native_i16x8(native_i16x8 &&) noexcept = default;
    native_i16x8 &operator=(native_i16x8 const &) noexcept = default;
    native_i16x8 &operator=(native_i16x8 &&) noexcept = default;

    /** Initialize all elements to zero.
     */
    native_i16x8() noexcept : v(_mm_setzero_si128()) {}

    [[nodiscard]] explicit native_i16x8(register_type other) noexcept : v(other) {}

    [[nodiscard]] explicit operator register_type () const noexcept {
        return v;
    }

    /** Initialize the element to the values in the arguments.
     *
     * @param a The value for element 0.
     * @param b The value for element 1.
     * @param c The value for element 2.
     * @param d The value for element 3.
     */
    [[nodiscard]] native_i16x8(value_type a, value_type b = value_type{0}, value_type c = value_type{0}, value_type d = value_type{0},
                             value_type e = value_type{0}, value_type f = value_type{0}, value_type g = value_type{0},
                             value_type h = value_type{0}) noexcept :
        v(_mm_set_epi16(h, g, f, e, d, c, b, a)) {}

    [[nodiscard]] explicit native_i16x8(value_type const *other) noexcept : v(_mm_loadu_si128(reinterpret_cast<register_type const *>(other))) {}

    void store(value_type *out) const noexcept
    {
        hi_axiom_not_null(out);
        _mm_storeu_si128(reinterpret_cast<register_type *>(out), v);
    }

    [[nodiscard]] explicit native_i16x8(void const *other) noexcept : v(_mm_loadu_si128(static_cast<register_type const *>(other))) {}

    void store(void *out) const noexcept
    {
        hi_axiom_not_null(out);
        _mm_storeu_si128(static_cast<register_type *>(out), v);
    }

    [[nodiscard]] explicit native_i16x8(std::span<value_type const> other) noexcept
    {
        hi_axiom(other.size() >= size);
        v = _mm_loadu_si128(reinterpret_cast<register_type const *>(other.data()));
    }

    void store(std::span<value_type> out) const noexcept
    {
        hi_axiom(out.size() >= size);
        _mm_storeu_si128(reinterpret_cast<register_type *>(out.data()), v);
    }

    template<size_t N>
    [[nodiscard]] explicit native_i16x8(std::array<value_type, N> other) noexcept requires (N >= size) : v(_mm_loadu_si128(reinterpret_cast<register_type const *>(other.data()))) {}

    template<size_t N>
    [[nodiscard]] explicit operator std::array<value_type, N> () const noexcept requires (N >= size)
    {
        auto r = std::array<value_type, size>{};
        _mm_storeu_si128(reinterpret_cast<register_type *>(r.data()), v);
        return r;
    }


    /** Broadcast a single value to all the elements.
     *
     * ```
     * r[0] = a
     * r[1] = a
     * r[2] = a
     * r[3] = a
     * r[4] = a
     * r[5] = a
     * r[6] = a
     * r[7] = a
     * ```
     */
    [[nodiscard]] static native_i16x8 broadcast(int16_t a) noexcept
    {
        return native_i16x8{_mm_set1_epi16(a)};
    }

    /** Broadcast the first element to all the elements.
     *
     * ```
     * r[0] = a[0]
     * r[1] = a[0]
     * r[2] = a[0]
     * r[3] = a[0]
     * r[4] = a[0]
     * r[5] = a[0]
     * r[6] = a[0]
     * r[7] = a[0]
     * ```
     */
//    [[nodiscard]] static native_i16x8 broadcast(native_i16x8 a) noexcept
//    {
//#ifdef HI_HAS_AVX2
//        return native_i16x8{_mm_broadcastw_epi16(a.v)};
//#else
//        return permute<"xxxxxxxx">(a);
//#endif
//    }

    /** For each bit in mask set corresponding element to all-ones or all-zeros.
     */
    [[nodiscard]] static native_i16x8 from_mask(size_t mask) noexcept
    {
        hi_axiom(mask <= 0b1111'1111);

        return native_i16x8{
            truncate<value_type>(mask & 0b0000'0001 ? 0 : 0xffff),
            truncate<value_type>(mask & 0b0000'0010 ? 0 : 0xffff),
            truncate<value_type>(mask & 0b0000'0100 ? 0 : 0xffff),
            truncate<value_type>(mask & 0b0000'1000 ? 0 : 0xffff),
            truncate<value_type>(mask & 0b0001'0000 ? 0 : 0xffff),
            truncate<value_type>(mask & 0b0010'0000 ? 0 : 0xffff),
            truncate<value_type>(mask & 0b0100'0000 ? 0 : 0xffff),
            truncate<value_type>(mask & 0b1000'0000 ? 0 : 0xffff)};
    }

    /** Concatonate the top bit of each element.
     */
    [[nodiscard]] size_t mask() const noexcept
    {
        auto tmp = _mm_movemask_epi8(v);
        tmp &= 0b0101'0101;
        tmp |= tmp >> 1;
        tmp &= 0b0011'0011;
        tmp |= tmp >> 2;
        tmp &= 0b0000'1111;
        return narrow_cast<size_t>(tmp);
    }


    [[nodiscard]] friend native_i16x8 operator==(native_i16x8 a, native_i16x8 b) noexcept
    {
        return native_i16x8{_mm_cmpeq_epi16(a.v, b.v)};
    }

    [[nodiscard]] friend native_i16x8 operator!=(native_i16x8 a, native_i16x8 b) noexcept
    {
        return ~(a == b);
    }

    [[nodiscard]] friend native_i16x8 operator<(native_i16x8 a, native_i16x8 b) noexcept
    {
        return native_i16x8{_mm_cmplt_epi16(a.v, b.v)};
    }

    [[nodiscard]] friend native_i16x8 operator>(native_i16x8 a, native_i16x8 b) noexcept
    {
        return native_i16x8{_mm_cmpgt_epi16(a.v, b.v)};
    }

    [[nodiscard]] friend native_i16x8 operator<=(native_i16x8 a, native_i16x8 b) noexcept
    {
        return ~(a > b);
    }

    [[nodiscard]] friend native_i16x8 operator>=(native_i16x8 a, native_i16x8 b) noexcept
    {
        return ~(a < b);
    }

    [[nodiscard]] friend native_i16x8 operator+(native_i16x8 a, native_i16x8 b) noexcept
    {
        return native_i16x8{_mm_add_epi16(a.v, b.v)};
    }

    [[nodiscard]] friend native_i16x8 operator-(native_i16x8 a, native_i16x8 b) noexcept
    {
        return native_i16x8{_mm_sub_epi16(a.v, b.v)};
    }

    [[nodiscard]] friend native_i16x8 operator-(native_i16x8 a) noexcept
    {
        return native_i16x8{} - a;
    }

    [[nodiscard]] friend native_i16x8 operator*(native_i16x8 a, native_i16x8 b) noexcept
    {
        return native_i16x8{_mm_mullo_epi16(a.v, b.v)};
    }

    [[nodiscard]] friend native_i16x8 operator&(native_i16x8 a, native_i16x8 b) noexcept
    {
        return native_i16x8{_mm_and_si128(a.v, b.v)};
    }

    [[nodiscard]] friend native_i16x8 operator|(native_i16x8 a, native_i16x8 b) noexcept
    {
        return native_i16x8{_mm_or_si128(a.v, b.v)};
    }

    [[nodiscard]] friend native_i16x8 operator^(native_i16x8 a, native_i16x8 b) noexcept
    {
        return native_i16x8{_mm_xor_si128(a.v, b.v)};
    }

    [[nodiscard]] friend native_i16x8 operator~(native_i16x8 a) noexcept
    {
        auto ones = _mm_undefined_si128();
        ones = _mm_cmpeq_epi32(ones, ones);
        return native_i16x8{_mm_andnot_si128(a.v, ones)};
    }

    [[nodiscard]] friend native_i16x8 operator<<(native_i16x8 a, int b) noexcept
    {
        return native_i16x8{_mm_slli_epi16(a.v, b)};
    }

    [[nodiscard]] friend native_i16x8 operator>>(native_i16x8 a, int b) noexcept
    {
        return native_i16x8{_mm_srai_epi16(a.v, b)};
    }

    [[nodiscard]] friend native_i16x8 min(native_i16x8 a, native_i16x8 b) noexcept
    {
        return native_i16x8{_mm_min_epi16(a.v, b.v)};
    }

    [[nodiscard]] friend native_i16x8 max(native_i16x8 a, native_i16x8 b) noexcept
    {
        return native_i16x8{_mm_max_epi16(a.v, b.v)};
    }

    [[nodiscard]] friend native_i16x8 abs(native_i16x8 a) noexcept
    {
        return native_i16x8{_mm_abs_epi16(a.v)};
    }

    /** Set elements to zero.
     *
     * @tparam Mask A bit mask corresponding to each element.
     * @param a The value to modify.
     * @return argument @a with elements set to zero where the corresponding @a Mask bit was '1'.
     */
    template<size_t Mask>
    [[nodiscard]] friend native_i16x8 set_zero(native_i16x8 a) noexcept
    {
        static_assert(Mask <= 0b1111'1111);
        hilet mask = from_mask(Mask);
        return not_and(mask, a);
    }

    /** Insert a value into an element of a vector.
     *
     * @tparam Index the index of the element where insert the value.
     * @param a The vector to insert the value into.
     * @param b The value to insert.
     * @return The vector with the inserted value.
     */
    template<size_t Index>
    [[nodiscard]] friend native_i16x8 insert(native_i16x8 a, value_type b) noexcept
    {
        static_assert(Index < 4);
        return native_i16x8{_mm_insert_epi16(a, b, narrow_cast<int>(Index))};
    }

    /** Extract an element from a vector.
     *
     * @tparam Index the index of the element.
     * @param a The vector to select the element from.
     * @return The value of the selected element.
     */
    template<size_t Index>
    [[nodiscard]] friend float extract(native_i16x8 a) noexcept
    {
        return std::bit_cast<float>(_mm_extract_epi16(a, Index));
    }

    /** Select elements from two vectors.
     *
     * @tparam Mask A mask to select bits from @a a when '0'; or @a b when '1'. The
     *         lsb corrosponds with element zero.
     * @param a A vector for which element are selected when the bit in @a Mask is '0'.
     * @param b A vector for which element are selected when the bit in @a Mask is '1'.
     * @return A vector with element selected from @a a and @a b
     */
    template<size_t Mask>
    [[nodiscard]] friend native_i16x8 blend(native_i16x8 a, native_i16x8 b) noexcept
    {
#ifdef HI_HAS_SSE4_1
        return native_i16x8{_mm_blend_epi16(a, b, Mask)};
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
     * @tparam SourceElements A string reprecenting the order of elements. First character
     *         matches the first element.
     * @param a The vector to swizzle the elements
     * @returns A vector with the elements swizzled.
     */
    //template<fixed_string SourceElements>
    //[[nodiscard]] static native_i16x8 permute(native_i16x8 a) noexcept
    //{
    //    constexpr auto order = detail::native_swizzle_to_packed_indices<SourceElements, size>();
    //
    //    if constexpr (order == 0b111'110'101'100'011'010'001'000) {
    //        return a.v;
    //    } else {
    //        return native_i16x8{_mm_shuffle_epi16(a.v, order)};
    //    }
    //}

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
     * @tparam SourceElements A string reprecenting the order of elements. First character
     *         matches the first element.
     * @param a The vector to swizzle the elements
     * @returns A vector with the elements swizzled.
     */
    template<fixed_string SourceElements>
    [[nodiscard]] friend native_i16x8 swizzle(native_i16x8 a) noexcept
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
    [[nodiscard]] friend native_i16x8 horizontal_add(native_i16x8 a, native_i16x8 b) noexcept
    {
        return native_i16x8{_mm_hadd_epi16(a.v, b.v)};
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
    [[nodiscard]] friend native_i16x8 horizontal_sub(native_i16x8 a, native_i16x8 b) noexcept
    {
        return native_i16x8{_mm_hsub_epi16(a.v, b.v)};
    }
#endif

    /** Sum all elements of a vector.
     *
     * ```
     * r = broadcast(a[0] + a[1] + a[2] + a[3])
     * ```
     */
    //[[nodiscard]] friend native_i16x8 horizontal_sum(native_i16x8 a) noexcept
    //{
    //    auto tmp = a + permute<"cdab">(a);
    //    return tmp + permute<"badc">(tmp);
    //}

    /** Dot product.
     *
     * ```
     * tmp[0] = SourceMask[0] ? a[0] * b[0] : 0
     * tmp[1] = SourceMask[1] ? a[1] * b[1] : 0
     * tmp[2] = SourceMask[2] ? a[2] * b[2] : 0
     * tmp[3] = SourceMask[3] ? a[3] * b[3] : 0
     * r = broadcast(tmp[0] + tmp[1] + tmp[2] + tmp[3])
     * ```
     */
    template<size_t SourceMask>
    [[nodiscard]] friend native_i16x8 dot_product(native_i16x8 a, native_i16x8 b) noexcept
    {
        static_assert(SourceMask <= 0b1111);
        return horizontal_sum(set_zero<~SourceMask & 0b1111>(a * b));
    }


    /** not followed by and.
     *
     * r = ~a & b
     *
     */
    [[nodiscard]] friend native_i16x8 not_and(native_i16x8 a, native_i16x8 b) noexcept
    {
        return native_i16x8{_mm_andnot_si128(a.v, b.v)};
    }

    template<fixed_string SourceElements>
    [[nodiscard]] static native_i16x8 swizzle_numbers() noexcept
    {
        constexpr auto one_mask = detail::native_swizzle_to_mask<SourceElements, size, '1'>();
        constexpr auto zero_mask = detail::native_swizzle_to_mask<SourceElements, size, '0'>();
        constexpr auto number_mask = one_mask | zero_mask;
        constexpr auto alpha_mask = ~number_mask & 0b1111;

        if constexpr ((zero_mask | alpha_mask) == 0b1111) {
            return native_i16x8{_mm_setzero_si128()};

        } else if constexpr ((one_mask | alpha_mask)== 0b1111) {
            return native_i16x8{_mm_set1_epi16(1)};

        } else {
            return native_i16x8{_mm_set_epi16(
                to_bool(one_mask & 0b0001) ? 1 : 0,
                to_bool(one_mask & 0b0010) ? 1 : 0,
                to_bool(one_mask & 0b0100) ? 1 : 0,
                to_bool(one_mask & 0b1000) ? 1 : 0
            )};
        }

    }

};

#endif


}}

