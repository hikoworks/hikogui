// Copyright Take Vos 2020-2022.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

module;
#include "../macros.hpp"

#include <span>
#include <cstddef>
#include <exception>
#include <bit>

export module hikogui_utility_bits;
import hikogui_utility_cast;
import hikogui_utility_debugger;
import hikogui_utility_exception;
import hikogui_utility_misc;

export namespace hi { inline namespace v1 {

/** Read a single bit from span of bytes
 * Bits are ordered LSB first.
 *
 * @param buffer The buffer of bytes to extract the bit from.
 * @param index The index of the bit in the byte span.
 */
[[nodiscard]] bool get_bit(std::span<std::byte const> buffer, std::size_t &index) noexcept
{
    hilet byte_index = index >> 3;
    hilet bit_index = index & 7;
    ++index;

    hi_axiom(byte_index < buffer.size());
    return to_bool(std::bit_cast<uint8_t>(buffer[byte_index] >> bit_index) & 1);
}

/** Read a bits from of span of bytes
 * Bits are ordered LSB first.
 * Bits are copied as if the byte array is laid out from right to left, example:
 *
 *  7 6 5 4 3 2 1 0 7 6 5 4 3 2 1 0
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |    byte 1     |    byte 0     |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *           :         :
 * index=6   +-+-+-+-+-+
 * length=5  | Return  |
 *           +-+-+-+-+-+
 *            4 3 2 1 0
 *
 * @param buffer The buffer of bytes to extract bits from.
 * @param index The index of the bit in the byte span.
 * @param length the number of bits to return.
 */
[[nodiscard]] std::size_t get_bits(std::span<std::byte const> buffer, std::size_t &index, std::size_t length) noexcept
{
    hi_axiom(length <= sizeof(std::size_t) * CHAR_BIT);

    auto value = 0;

    auto todo = length;
    auto done = 0_uz;
    while (todo) {
        hilet byte_index = index >> 3;
        hilet bit_index = index & 7;
        hi_axiom(byte_index < buffer.size());

        hilet available_bits = 8 - bit_index;
        hilet nr_bits = std::min(available_bits, todo);

        hilet mask = (1 << nr_bits) - 1;

        hilet tmp = static_cast<int>(buffer[byte_index] >> bit_index) & mask;
        value |= tmp << done;

        todo -= nr_bits;
        done += nr_bits;
        index += nr_bits;
    }

    return value;
}

}} // namespace hi::inline v1
