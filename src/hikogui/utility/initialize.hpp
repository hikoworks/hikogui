// Copyright Take Vos 2023.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "terminate.hpp"
#include "console_win32.hpp"
#include "debugger_intf.hpp"
#include <atomic>
#include <chrono>
#include <thread>
#include <cstdint>
#include <cstddef>

/**
 *
 * @module: hikogui.utility.initialize
 */
hi_export_module(hikogui.utility.initialize);

hi_export namespace hi {
inline namespace v1 {
namespace detail {

enum class initialize_state_type {
    uninitialized,
    running,
    finished,
};

inline thread_local uint16_t initialize_thread_id_dummy; 

/** Get a thread id.
 * @return A unique id for a thread, which is never 0 or 1.
 */
[[nodiscard]] inline uintptr_t initialize_thread_id() noexcept
{
    // The following is guaranteed:
    //  - 0: An address can never be a nullptr.
    //  - 1: initialize_thread_id_dummy is a uint16_t which is aligned to a multiple of two.
    return std::bit_cast<uintptr_t>(std::addressof(initialize_thread_id_dummy));
}

inline std::atomic<uintptr_t> initialize_state = 0;

}

/** Initialize base functionality of HikoGUI.
 *
 * This function is called as early in the life of an application as possible.
 * This function is called implicitly by:
 *  - Every function from hikogui.metadata.application_metadata.
 *  - When using the hikogui.crt.
 */
inline void initialize() noexcept
{
    using namespace std::literals;

    uintptr_t expected = 0;
    if (detail::initialize_state.compare_exchange_strong(expected, detail::initialize_thread_id(), std::memory_order::acquire)) {
        // Make sure stdin, stdout, stderr are attached to a console and that
        // std::print() works properly.
        start_console();

        // Install the terminate handle to make pretty error messages for
        // end users.
        //old_terminate_handler = std::set_terminate(hi::terminate_handler);

        // Install the handler for break-points and other traps.
        // Which will optionally start the just-in-time debugger, or call
        // std::terminate() with an appropriate error.
        //enable_debugger();

        // Mark initialization as "finished".
        detail::initialize_state.store(1, std::memory_order::release);

    } else if (expected == detail::initialize_thread_id()) {
        set_debug_message("hi::initialize() re-enterred from same thread.");
        std::terminate();

    } else {
        // We can not use std::mutex before main().
        // Wait until initialization on the other thread is finished.
        while (detail::initialize_state.load(std::memory_order::acquire) != 1) {
            std::this_thread::sleep_for(16ms);
        }
    }
}

} // namespace v1
}