

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

export module hikogui_SIMD : native_f16x8_sse2;
import : native_simd_utility;
import hikogui_utility;


export namespace hi {
inline namespace v1 {

#ifdef HI_HAS_SSE2


/** A float16 x 8 (__m128i) SSE2 register.
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
template<>
struct native_simd<float16,8> {
    using value_type = float16;
    constexpr static size_t size = 8;
    using register_type = __m128i;

    register_type v;

    native_simd(native_simd const&) noexcept = default;
    native_simd(native_simd &&) noexcept = default;
    native_simd &operator=(native_simd const &) noexcept = default;
    native_simd &operator=(native_simd &&) noexcept = default;

    /** Initialize all elements to zero.
     */
    native_simd() noexcept : v(_mm_setzero_si128()) {}

    explicit native_simd(native_simd<float,8> const &a) noexcept;
    native_simd(native_simd<float,4> const &a, native_simd<float,4> const &b) noexcept;

    [[nodiscard]] explicit native_simd(register_type other) noexcept : v(other) {}

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
    [[nodiscard]] native_simd(value_type a, value_type b = value_type{}, value_type c = value_type{}, value_type d = value_type{},
                             value_type e = value_type{}, value_type f = value_type{}, value_type g = value_type{},
                             value_type h = value_type{}) noexcept :
        v(_mm_set_epi16(h.v, g.v, f.v, e.v, d.v, c.v, b.v, a.v)) {}

    [[nodiscard]] explicit native_simd(value_type const *other) noexcept : v(_mm_loadu_si128(reinterpret_cast<register_type const *>(other))) {}

    void store(value_type *out) const noexcept
    {
        hi_axiom_not_null(out);
        _mm_storeu_si128(reinterpret_cast<register_type *>(out), v);
    }

    [[nodiscard]] explicit native_simd(void const *other) noexcept : v(_mm_loadu_si128(static_cast<register_type const *>(other))) {}

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

    template<size_t N>
    [[nodiscard]] explicit native_simd(std::array<value_type, N> other) noexcept requires (N >= size) : v(_mm_loadu_si128(reinterpret_cast<register_type const *>(other.data()))) {}

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
    [[nodiscard]] static native_simd broadcast(int16_t a) noexcept
    {
        return native_simd{_mm_set1_epi16(a)};
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
    [[nodiscard]] static native_simd broadcast(native_simd a) noexcept
    {
#ifdef HI_HAS_AVX2
        return native_simd{_mm_broadcastw_epi16(a.v)};
#else
        // Create a mask for 1 word each dword, AND it with a.v.
        auto tmp = _mm_undefined_si128();
        tmp = _mm_cmpeq_epi32(tmp, tmp);
        tmp = _mm_slli_epi32(tmp, 16);
        tmp = _mm_and_si128(tmp, a.v);

        // Broadcast the first word to all the bytes in the first dword.
        tmp = _mm_or_si128(tmp, _mm_slli_epi32(tmp, 16));

        // Broadcast the first dword to all 4 dwords.
        tmp = _mm_shuffle_epi32(tmp, 0b00'00'00'00);
        return native_simd{tmp};
#endif
    }

    /** For each bit in mask set corresponding element to all-ones or all-zeros.
     */
    [[nodiscard]] static native_simd from_mask(size_t mask) noexcept
    {
        hi_axiom(mask <= 0b1111'1111);

        return native_simd{
            mask & 0b0000'0001 ? value_type{} : value_type(intrinsic, 0xffff),
            mask & 0b0000'0010 ? value_type{} : value_type(intrinsic, 0xffff),
            mask & 0b0000'0100 ? value_type{} : value_type(intrinsic, 0xffff),
            mask & 0b0000'1000 ? value_type{} : value_type(intrinsic, 0xffff),
            mask & 0b0001'0000 ? value_type{} : value_type(intrinsic, 0xffff),
            mask & 0b0010'0000 ? value_type{} : value_type(intrinsic, 0xffff),
            mask & 0b0100'0000 ? value_type{} : value_type(intrinsic, 0xffff),
            mask & 0b1000'0000 ? value_type{} : value_type(intrinsic, 0xffff)};
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


    [[nodiscard]] friend native_simd operator==(native_simd a, native_simd b) noexcept
    {
        return native_simd{_mm_cmpeq_epi16(a.v, b.v)};
    }

    [[nodiscard]] friend native_simd operator!=(native_simd a, native_simd b) noexcept
    {
        return ~(a == b);
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

    /** Set elements to zero.
     *
     * @tparam Mask A bit mask corresponding to each element.
     * @param a The value to modify.
     * @return argument @a with elements set to zero where the corresponding @a Mask bit was '1'.
     */
    template<size_t Mask>
    [[nodiscard]] friend native_simd set_zero(native_simd a) noexcept
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
    [[nodiscard]] friend native_simd insert(native_simd a, value_type b) noexcept
    {
        static_assert(Index < 4);
        return native_simd{_mm_insert_epi16(a, b.v, narrow_cast<int>(Index))};
    }

    /** Extract an element from a vector.
     *
     * @tparam Index the index of the element.
     * @param a The vector to select the element from.
     * @return The value of the selected element.
     */
    template<size_t Index>
    [[nodiscard]] friend value_type extract(native_simd a) noexcept
    {
        return std::bit_cast<value_type>(_mm_extract_epi16(a, Index));
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
    [[nodiscard]] friend native_simd blend(native_simd a, native_simd b) noexcept
    {
#ifdef HI_HAS_SSE4_1
        return native_simd{_mm_blend_epi16(a, b, Mask)};
#else
        hilet mask = from_mask(Mask);
        return not_and(mask, a) | (mask & b);
#endif
    }

    /** Permute elements, ignoring numeric elements.
     *
     * The characters in @a SourceElements mean the following:
     * - 'a' - 'p': The indices to elements 0 and 15 of @a a.
     * - Any other character is treated as if the original element was selected.
     *
     * @tparam SourceElements A string reprecenting the order of elements. First character
     *         matches the first element.
     * @param a The vector to swizzle the elements
     * @returns A vector with the elements swizzled.
     */
    //template<fixed_string SourceElements>
    //[[nodiscard]] static native_simd permute(native_simd a) noexcept
    //{
    //    constexpr auto order = detail::native_swizzle_to_packed_indices<SourceElements, size>();
    //
    //    if constexpr (order == 0b111'110'101'100'011'010'001'000) {
    //        return a.v;
    //    } else {
    //        auto tmp = _mm_shufflelo(a.v,
    //        return native_simd{_mm_shuffle_epi16(a.v, order)};
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
    [[nodiscard]] friend native_simd swizzle(native_simd a) noexcept
    {
        constexpr auto one_mask = detail::native_swizzle_to_mask<SourceElements, size, '1'>();
        constexpr auto zero_mask = detail::native_swizzle_to_mask<SourceElements, size, '0'>();
        constexpr auto number_mask = one_mask | zero_mask;

        if constexpr (number_mask == 0b11111111) {
            // Swizzle was /[01][01][01][01]/.
            return swizzle_numbers<SourceElements>();

        } else if constexpr (number_mask == 0b00000000) {
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

    /** not followed by and.
     *
     * r = ~a & b
     *
     */
    [[nodiscard]] friend native_simd not_and(native_simd a, native_simd b) noexcept
    {
        return native_simd{_mm_andnot_si128(a.v, b.v)};
    }

    template<fixed_string SourceElements>
    [[nodiscard]] static native_simd swizzle_numbers() noexcept
    {
        constexpr auto one_mask = detail::native_swizzle_to_mask<SourceElements, size, '1'>();
        constexpr auto zero_mask = detail::native_swizzle_to_mask<SourceElements, size, '0'>();
        constexpr auto number_mask = one_mask | zero_mask;
        constexpr auto alpha_mask = ~number_mask & 0b11111111;

        if constexpr ((zero_mask | alpha_mask) == 0b11111111) {
            return native_simd{_mm_setzero_si128()};

        } else if constexpr ((one_mask | alpha_mask)== 0b11111111) {
            return native_simd::broadcast(value_type::from_uint16_t(0x3c00)); // 1.0

        } else {
            return native_simd{
                to_bool(one_mask & 0b00000001) ? value_type::from_uint16_t(0x3c00) : value_type{},
                to_bool(one_mask & 0b00000010) ? value_type::from_uint16_t(0x3c00) : value_type{},
                to_bool(one_mask & 0b00000100) ? value_type::from_uint16_t(0x3c00) : value_type{},
                to_bool(one_mask & 0b00001000) ? value_type::from_uint16_t(0x3c00) : value_type{},
                to_bool(one_mask & 0b00010000) ? value_type::from_uint16_t(0x3c00) : value_type{},
                to_bool(one_mask & 0b00100000) ? value_type::from_uint16_t(0x3c00) : value_type{},
                to_bool(one_mask & 0b01000000) ? value_type::from_uint16_t(0x3c00) : value_type{},
                to_bool(one_mask & 0b10000000) ? value_type::from_uint16_t(0x3c00) : value_type{}
            };
        }

    }

};

#endif


}}

