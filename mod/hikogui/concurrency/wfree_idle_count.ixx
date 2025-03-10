// Copyright Take Vos 2022.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

module;
#include "../macros.hpp"

#include <limits>
#include <atomic>
#include <cstdint>

export module hikogui_concurrency_wfree_idle_count;
import hikogui_utility;

export namespace hi::inline v1 {

/** Counts how many times a critical section was idle.
 *
 * A reader thread.
 * ```cpp
 * auto lock = std::scoped_lock(idle_count);
 * ... read protected data ...
 * ```
 *
 * A write thread.
 * ```cpp
 * idle_count.lock();
 * ... write protected data ...
 * auto old_version = idle_count.version();
 * idle_count.unlock();
 *
 * ... wait some time ...
 *
 * auto new_version = idle_count.expanded_version(version);
 * if (new_version > old_version) {
 *   // All threads now see the new data.
 *   ... Delete old data ...
 * }
 * ```
 */
class wfree_idle_count {
public:
    constexpr wfree_idle_count() noexcept = default;
    ~wfree_idle_count() = default;
    wfree_idle_count(wfree_idle_count const&) = delete;
    wfree_idle_count(wfree_idle_count&&) = delete;
    wfree_idle_count& operator=(wfree_idle_count const&) = delete;
    wfree_idle_count& operator=(wfree_idle_count&&) = delete;

    /** Check if the critical section is locked.
     *
     * @note This is only reliably `true` when inside a critical section.
     */
    [[nodiscard]] hi_force_inline bool is_locked() const noexcept
    {
        return to_bool(_lock_count.load(std::memory_order::relaxed));
    }

    /** Start the critical section.
     *
     * @note lock is allowed to be called reentered.
     */
    hi_force_inline void lock() noexcept
    {
        hilet lock_count = _lock_count.fetch_add(1, std::memory_order::acquire);
        hi_axiom(lock_count != std::numeric_limits<uint32_t>::max());
    }

    /** End the critical section.
     *
     * @note It is undefined behavior to call unlock() when not holding the lock.
     */
    hi_force_inline void unlock() noexcept
    {
        auto lock_count = _lock_count.fetch_sub(1, std::memory_order::release);
        hi_axiom(lock_count != 0);
        if (lock_count == 1) {
            // No one is locking, increment the idle count.
            _version.fetch_add(1, std::memory_order::relaxed);
        }
    }

    /** Get the current idle-count.
     *
     * @return The number of times the critical section became idle, modulo 2^23.
     */
    hi_force_inline uint64_t operator*() const noexcept
    {
        return _version.load(std::memory_order::acquire);
    }

private:
    std::atomic<uint64_t> _version = 0;
    std::atomic<uint64_t> _lock_count = 0;
};

} // namespace hi::inline v1
