// Copyright Take Vos 2019, 2021-2022.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

module;
#include "../macros.hpp"

#include <atomic>
#include <thread>
#include <chrono>

export module hikogui_concurrency_atomic;
import hikogui_utility;

hi_warning_push();
// C26403: Reset or explicitly delete and owner<T> pointer '...' (r.3).: ...
// The static analyzer is very confused about the get_or_make() function.
hi_warning_ignore_msvc(26403);

export namespace hi::inline v1 {

/** Lock-free fetch-then-max operation on an atomic.
 */
template<typename T>
T fetch_max(std::atomic<T> &lhs, T rhs, std::memory_order order) noexcept
{
    auto expected = lhs.load(order);
    while (expected < rhs) {
        if (lhs.compare_exchange_weak(expected, rhs, order)) {
            return expected;
        }
    }
    return expected;
}

/** Lock-free fetch-then-min operation on an atomic.
 */
template<typename T>
T fetch_min(std::atomic<T> &lhs, T rhs, std::memory_order order) noexcept
{
    auto expected = lhs.load(order);
    while (rhs < expected) {
        if (lhs.compare_exchange_weak(expected, rhs, order)) {
            return expected;
        }
    }
    return expected;
}

template<typename T>
class atomic_unique_ptr {
public:
    using element_type = T;
    using pointer = element_type *;
    using reference = element_type &;

    ~atomic_unique_ptr() noexcept
    {
        delete _pointer.load(std::memory_order_relaxed);
    }

    constexpr atomic_unique_ptr() noexcept = default;
    atomic_unique_ptr(atomic_unique_ptr const &) = delete;
    atomic_unique_ptr &operator=(atomic_unique_ptr const &) = delete;

    atomic_unique_ptr(atomic_unique_ptr &&other) noexcept : _pointer(other._pointer.exchange(nullptr)) {}

    atomic_unique_ptr &operator=(atomic_unique_ptr &&other) noexcept
    {
        hi_return_on_self_assignment(other);

        delete _pointer.exchange(other._pointer.exchange(nullptr));
        return *this;
    }

    constexpr atomic_unique_ptr(std::nullptr_t) noexcept : _pointer(nullptr) {}

    atomic_unique_ptr &operator=(std::nullptr_t) noexcept
    {
        delete _pointer.exchange(nullptr);
        return *this;
    }

    constexpr atomic_unique_ptr(pointer other) noexcept : _pointer(other) {}

    atomic_unique_ptr &operator=(pointer other) noexcept
    {
        delete _pointer.exchange(other);
        return *this;
    }

    /** Get the raw pointer.
     */
    [[nodiscard]] pointer get(std::memory_order order = std::memory_order::seq_cst) const noexcept
    {
        return _pointer.load(order);
    }

    /** Get or make an object.
     *
     * This function will return a previously created object, or if the internal pointer is
     * null a new element_type is constructed and returned.
     *
     * It is possible for the element_type to be constructed multiple times during a race,
     * but only one will be returned from this function.
     *
     * @param args The arguments passed to the constructor
     * @return A reference to an existing or just constructed object.
     */
    template<typename... Args>
    [[nodiscard]] reference get_or_make(Args &&...args) noexcept
    {
        auto expected = _pointer.load(std::memory_order::acquire);
        if (expected != nullptr) {
            return *expected;
        }

        auto desired = new element_type(std::forward<Args>(args)...);
        if (not _pointer.compare_exchange_strong(expected, desired, std::memory_order::release, std::memory_order::acquire)) {
            // Lost construction race.
            delete desired;
            return *expected;
        } else {
            return *desired;
        }
    }

private:
    std::atomic<pointer> _pointer;
};

} // namespace hi::inline v1

hi_warning_pop();
