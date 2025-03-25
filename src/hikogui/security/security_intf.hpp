// Copyright Take Vos 2022.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "../win32_headers.hpp"

#include "../utility/utility.hpp"
#include "../macros.hpp"
#include <iterator>
#include <memory>

hi_export_module(hikogui.security : intf);


hi_export namespace hi::inline v1 {

/** Securely clear memory.
 *
 * This function uses an operating system service for erasing memory securely.
 *
 * @param ptr The pointer to the memory to clear.
 * @param size The number of bytes to clear.
 */
void secure_clear(void *ptr, size_t size) noexcept;

/** Securely clear an object.
 *
 * This function uses an operating system service for erasing memory securely.
 *
 * @param object The object to securely clear to zeroes.
 */
inline void secure_clear(trivially_copyable auto &object) noexcept
{
    secure_clear(&object, sizeof(object));
}

/** Securely clear a set of objects.
 *
 * This function uses an operating system service for erasing memory securely.
 *
 * @param first An iterator pointing to the first object to clear.
 * @param last An iterator pointing to one beyond to clear.
 */
template<typename It>
void secure_clear(It first, It last) noexcept
{
    using value_type = decltype(*first);

    if constexpr (requires { std::distance(first, last); }) {
        secure_clear(std::addressof(*first), std::distance(first, last) * sizeof(value_type));

    } else {
        for (auto it = first; it != last; ++it) {
            secure_clear(*it);
        }
    }
}

void secure_destroy_at(auto *p)
{
    std::destroy_at(p);
    secure_clear(*p);
}

/** Securely destroy objects.
 *
 * Destroy objects and overwrite its memory with all '1' followed by all '0'.
 *
 * @note Internal allocations of objects will not be securily destroyed.
 * @param first An iterator to the first object.
 * @param last An iterator to beyond the last object.
 */
template<typename It>
void secure_destroy(It first, It last)
{
    std::destroy(first, last);
    secure_clear(first, last);
}

/** Securely move objects.
 *
 * First calls std::uinitialized_move() then overwrites memory with all '1'
 * followed by all '0'.
 *
 * @param first An iterator to the first object to move
 * @param last An iterator to beyond the last object to move
 * @param d_first An destination iterator to uninitialized memory.
 */
template<typename It, typename OutIt>
void secure_uninitialized_move(It first, It last, OutIt d_first)
{
    std::uninitialized_move(first, last, d_first);
    secure_erase(first, last);
}

}

