// Copyright Take Vos 2019-2022.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

module;
#include "../macros.hpp"


#ifdef HI_HAS_SSE
#include <immintrin.h>
#endif
#ifdef HI_HAS_SSE2
#include <emmintrin.h>
#endif
#ifdef HI_HAS_SSE4_1
#include <smmintrin.h>
#endif
#if HI_COMPILER == HI_CC_MSVC
#include <stdlib.h>
#endif

#include <bit>
#include <concepts>

export module hikogui_utility_endian;
import hikogui_utility_cast;
import hikogui_utility_memory;
import hikogui_utility_reflection;

hi_warning_push();
// C26472: Don't use a static_cast for arithmetic conversions. Use brace initialization, gsl::narrow_cast or gsl::narrow
// (type.1).
// static_cast are used to cheaply cast integers to unsigned and back, for byteswapping.
hi_warning_ignore_msvc(26472);

export namespace hi { inline namespace v1 {

/** Convert an integral from little-to-native endian.
 */
template<std::integral T>
[[nodiscard]] constexpr T little_to_native(T x)
{
    if constexpr (std::endian::native == std::endian::little) {
        return x;
    } else {
        return std::byteswap(x);
    }
}

/** Convert an integral from big-to-native endian.
 */
template<std::integral T>
[[nodiscard]] constexpr T big_to_native(T x)
{
    if constexpr (std::endian::native == std::endian::big) {
        return x;
    } else {
        return std::byteswap(x);
    }
}

/** Convert an integral from native-to-little endian.
 */
template<std::integral T>
[[nodiscard]] constexpr T native_to_little(T x)
{
    if constexpr (std::endian::native == std::endian::little) {
        return x;
    } else {
        return std::byteswap(x);
    }
}

/** Convert an integral from native-to-big endian.
 */
template<std::integral T>
[[nodiscard]] constexpr T native_to_big(T x)
{
    if constexpr (std::endian::native == std::endian::big) {
        return x;
    } else {
        return std::byteswap(x);
    }
}

/** Unaligned Load of a numeric value from an array.
 *
 * @tparam Out The type of the integer or floating point value to load.
 * @tparam Endian The endianness of the data.
 * @param src A pointer to byte-like array.
 * @return The numeric value after endian conversion.
 */
template<std::integral Out, std::endian Endian = std::endian::native, typename In>
[[nodiscard]] constexpr Out load(In const *src) noexcept
{
    if constexpr (Endian != std::endian::native) {
        return std::byteswap(unaligned_load<Out>(src));
    } else {
        return unaligned_load<Out>(src);
    }
}

/** Unaligned Load of a numeric value from a byte-like array.
 *
 * @tparam T The type of the integer or floating point value to load.
 * @tparam Endian The endianness of the data.
 * @param src A pointer to memory.
 * @return The numeric value after endian conversion.
 */
template<std::integral T, std::endian Endian = std::endian::native>
[[nodiscard]] T load(void const *src) noexcept
{
    auto value = unaligned_load<T>(src);
    if constexpr (Endian != std::endian::native) {
        value = std::byteswap(value);
    }
    return value;
}

/** Load of a numeric value encoded in little-endian format.
 *
 * @tparam T The type of the integer or floating point value to load.
 * @param src A pointer to the numeric value.
 * @return The numeric value after endian conversion.
 */
template<std::integral T>
[[nodiscard]] constexpr T load_le(T const *src) noexcept
{
    return load<T, std::endian::little>(src);
}

/** Unaligned load of a numeric value encoded in little-endian format.
 *
 * @tparam T The type of the integer or floating point value to load.
 * @param src A pointer to a byte like memory.
 * @return The numeric value after endian conversion.
 */
template<std::integral T, byte_like B>
[[nodiscard]] constexpr T load_le(B const *src) noexcept
{
    return load<T, std::endian::little>(src);
}

/** Unaligned load of a numeric value encoded in little-endian format.
 *
 * @tparam T The type of the integer or floating point value to load.
 * @param src A pointer to memory.
 * @return The numeric value after endian conversion.
 */
template<std::integral T>
[[nodiscard]] T load_le(void const *src) noexcept
{
    return load<T, std::endian::little>(src);
}

/** Load of a numeric value encoded in big-endian format.
 *
 * @tparam T The type of the integer or floating point value to load.
 * @param src A pointer to a byte like memory.
 * @return The numeric value after endian conversion.
 */
template<std::integral T>
[[nodiscard]] constexpr T load_be(T const *src) noexcept
{
    return load<T, std::endian::big>(src);
}

/** Unaligned load of a numeric value encoded in byte-endian format.
 *
 * @tparam T The type of the integer or floating point value to load.
 * @param src A pointer to a byte like buffer.
 * @return The numeric value after endian conversion.
 */
template<std::integral T, byte_like B>
[[nodiscard]] constexpr T load_be(B const *src) noexcept
{
    return load<T, std::endian::big>(src);
}

/** Unaligned load of a numeric value encoded in byte-endian format.
 *
 * @tparam T The type of the integer or floating point value to load.
 * @param src A pointer to memory.
 * @return The numeric value after endian conversion.
 */
template<std::integral T>
[[nodiscard]] T load_be(void const *src) noexcept
{
    return load<T, std::endian::big>(src);
}

/** Unaligned load bits from a big-endian buffer at a bit-offset.
 *
 * To create the packed byte array from values.
 *  - Shift each value into a bigint object.
 *  - Shift by an additional 0 to 7 bits to align the first value to the MSB of a byte.
 *  - Shift by an additional 128 bits for the over-read extension.
 *  - Make a byte buffer with how many bits where added to the bigint.
 *  - Reverse iterate over the bytes in the buffer and shift out bytes from the bigint.
 *
 * @note The src buffer should be extended by 128-bits to allow over-reading beyond the end of the data.
 * @tparam NumBits the number of bits to read.
 * @param src A byte-like buffer to load bits from.
 * @param bit_index The bit offset into the buffer. 0 is the 7th bit of the 1st byte in @a src.
 * @return The loaded bits in the least-signigicant-bits of an unsigned integer type that can hold the bits requested.
 */
template<unsigned int NumBits, byte_like B>
[[nodiscard]] constexpr auto load_bits_be(B const *src, size_t bit_index) noexcept
{
    static_assert(NumBits <= sizeof(unsigned long long) * CHAR_BIT);

    constexpr auto num_bits = NumBits;
    constexpr auto num_bytes = (num_bits + CHAR_BIT - 1) / CHAR_BIT;

    // Determine an unsigned type that can be used to read NumBits in a single `load_be()` on every bit offset.
    // clang-format off
    using value_type =
        std::conditional_t<num_bytes < sizeof(unsigned short), unsigned short,
        std::conditional_t<num_bytes < sizeof(unsigned int), unsigned int,
        std::conditional_t<num_bytes < sizeof(unsigned long), unsigned long, unsigned long long>>>;
    // clang-format on

    constexpr auto value_bits = sizeof(value_type) * CHAR_BIT;

    hilet byte_offset = bit_index / CHAR_BIT;
    hilet bit_offset = bit_index % CHAR_BIT;

    // Optimization of reading a byte, aligned to a byte.
    if constexpr (num_bits == CHAR_BIT) {
        if (bit_offset == 0) {
            return char_cast<value_type>(src[byte_offset]);
        }
    }

    // load_be allows unaligned reads.
    auto r = load_be<value_type>(std::addressof(src[byte_offset]));

    // Align to most significant bit. In preparation for loading
    // one more byte.
    r <<= bit_offset;

    if constexpr (num_bytes == sizeof(value_type)) {
        // In this case it is possible we could not read the whole value in one go.
        // We may need to read one more byte.

        auto bits_done = value_bits - bit_offset;
        if (bits_done < num_bits) {
            auto rest = char_cast<value_type>(src[byte_offset + sizeof(value_type)]);
            rest >>= CHAR_BIT - bit_offset;
            r |= rest;
        }
    }

    // Align number to least significant bit.
    r >>= value_bits - num_bits;
    return r;
}

template<std::endian Endian = std::endian::native, std::integral T, byte_like B>
constexpr void store(T value, B const *dst) noexcept
{
    if constexpr (Endian != std::endian::native) {
        value = std::byteswap(value);
    }
    unaligned_store<T>(value, dst);
}

template<std::endian Endian = std::endian::native, std::integral T>
constexpr void store(T value, void const *dst) noexcept
{
    if constexpr (Endian != std::endian::native) {
        value = std::byteswap(value);
    }
    unaligned_store<T>(value, dst);
}

template<std::integral T, byte_like B>
constexpr void store_le(T value, B const *dst) noexcept
{
    store<std::endian::little>(value, dst);
}

template<std::integral T>
void store_le(T value, void const *dst) noexcept
{
    store<std::endian::little>(value, dst);
}

template<std::integral T, byte_like B>
constexpr void store_be(T value, B const *dst) noexcept
{
    store<std::endian::big>(value, dst);
}

template<std::integral T>
void store_be(T value, void const *dst) noexcept
{
    store<std::endian::big>(value, dst);
}

template<typename T, std::endian E, std::size_t A = alignof(T)>
struct endian_buf_t {
    using value_type = T;
    constexpr static std::endian endian = E;
    constexpr static std::size_t alignment = A;

    alignas(A) std::byte _value[sizeof(T)];

    [[nodiscard]] constexpr value_type operator*() const noexcept
    {
        return load<value_type, endian>(_value);
    }

    constexpr endian_buf_t& operator=(value_type x) noexcept
    {
        store<endian>(x, _value);
        return *this;
    }
};

using big_uint64_buf_t = endian_buf_t<uint64_t, std::endian::big, 1>;
using big_uint32_buf_t = endian_buf_t<uint32_t, std::endian::big, 1>;
using big_uint16_buf_t = endian_buf_t<uint16_t, std::endian::big, 1>;
using big_int64_buf_t = endian_buf_t<int64_t, std::endian::big, 1>;
using big_int32_buf_t = endian_buf_t<int32_t, std::endian::big, 1>;
using big_int16_buf_t = endian_buf_t<int16_t, std::endian::big, 1>;
using little_uint64_buf_t = endian_buf_t<uint64_t, std::endian::little, 1>;
using little_uint32_buf_t = endian_buf_t<uint32_t, std::endian::little, 1>;
using little_uint16_buf_t = endian_buf_t<uint16_t, std::endian::little, 1>;
using little_int64_buf_t = endian_buf_t<int64_t, std::endian::little, 1>;
using little_int32_buf_t = endian_buf_t<int32_t, std::endian::little, 1>;
using little_int16_buf_t = endian_buf_t<int16_t, std::endian::little, 1>;
using native_uint64_buf_t = endian_buf_t<uint64_t, std::endian::native, 1>;
using native_uint32_buf_t = endian_buf_t<uint32_t, std::endian::native, 1>;
using native_uint16_buf_t = endian_buf_t<uint16_t, std::endian::native, 1>;
using native_int64_buf_t = endian_buf_t<int64_t, std::endian::native, 1>;
using native_int32_buf_t = endian_buf_t<int32_t, std::endian::native, 1>;
using native_int16_buf_t = endian_buf_t<int16_t, std::endian::native, 1>;

using big_uint64_buf_at = endian_buf_t<uint64_t, std::endian::big>;
using big_uint32_buf_at = endian_buf_t<uint32_t, std::endian::big>;
using big_uint16_buf_at = endian_buf_t<uint16_t, std::endian::big>;
using big_int64_buf_at = endian_buf_t<int64_t, std::endian::big>;
using big_int32_buf_at = endian_buf_t<int32_t, std::endian::big>;
using big_int16_buf_at = endian_buf_t<int16_t, std::endian::big>;
using little_uint64_buf_at = endian_buf_t<uint64_t, std::endian::little>;
using little_uint32_buf_at = endian_buf_t<uint32_t, std::endian::little>;
using little_uint16_buf_at = endian_buf_t<uint16_t, std::endian::little>;
using little_int64_buf_at = endian_buf_t<int64_t, std::endian::little>;
using little_int32_buf_at = endian_buf_t<int32_t, std::endian::little>;
using little_int16_buf_at = endian_buf_t<int16_t, std::endian::little>;
using native_uint64_buf_at = endian_buf_t<uint64_t, std::endian::native>;
using native_uint32_buf_at = endian_buf_t<uint32_t, std::endian::native>;
using native_uint16_buf_at = endian_buf_t<uint16_t, std::endian::native>;
using native_int64_buf_at = endian_buf_t<int64_t, std::endian::native>;
using native_int32_buf_at = endian_buf_t<int32_t, std::endian::native>;
using native_int16_buf_at = endian_buf_t<int16_t, std::endian::native>;

}} // namespace hi::inline v1

hi_warning_pop();
