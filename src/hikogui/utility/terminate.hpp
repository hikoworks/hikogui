// Copyright Take Vos 2023.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

/** @file terminate.hpp Utilities for throwing exceptions and terminating the application.
 */

#pragma once

#include "../macros.hpp"
#include "dialog.hpp"
#include "debugger.hpp"
#include <exception>
#include <stdexcept>
#include <atomic>
#include <bit>
#include <format>
#include <iostream>
#include <functional>
#include <mutex>
#include <print>
#include <cstdio>
#include <stacktrace>

hi_export_module(hikogui.utility.terminate);

hi_export namespace hi {
inline namespace v1 {
namespace detail {

inline std::mutex terminate_mutex;

inline std::vector<std::function<void()>> on_terminate_functions;

inline void call_on_terminate() noexcept
{
    auto const lock = std::scoped_lock(terminate_mutex);
    for (auto it = on_terminate_functions.rbegin(); it != on_terminate_functions.rend(); ++it) {
        (*it)();
    }
}

} // namespace detail

/** Register functions that need to be called on std::terminate().
 *
 */
inline void on_terminate(std::function<void()> f) noexcept
{
    auto const lock = std::scoped_lock(detail::terminate_mutex);
    detail::on_terminate_functions.push_back(std::move(f));
}

/** The old terminate handler.
 *
 * This is the handler returned by `std::set_terminate()`.
 */
inline std::terminate_handler old_terminate_handler;

/** The HikoGUI terminate handler.
 *
 * This handler will print an error message on the console or pop-up a dialogue box.
 *
 * @note Use `set_debug_message()` to set a message.
 */
inline void terminate_handler() noexcept
{
    using namespace std::literals;

    detail::call_on_terminate();

    auto title = std::string{};
    auto message = std::string{};

    auto const ep = std::current_exception();
    if (ep) {
        try {
            std::rethrow_exception(ep);

        } catch (std::exception const& e) {
            title = "Unhandled std::exception."s;
            message += e.what();

        } catch (...) {
            title = "Unhandled unknown exception."s;
            message += "<no data>"s;
        }

    } else {
        title = "Abnormal termination."s;
        if (auto message_cstr = get_debug_message()) {
            message += message_cstr;
        } else {
            message += "<unknown>";
        }
    }

    // Append a stacktrace.
    message += "\n\nStack Trace:\n";

    auto stack_trace = std::stacktrace::current(0, 25);

    auto depth = 0;
    for (auto entry : stack_trace) {
        if (entry and not(entry.source_file().empty() and entry.source_line() == 0 and entry.description().empty())) {
            message += std::format("{}. {}:{} {}\n", depth, entry.source_file(), entry.source_line(), entry.description());
        } else {
            message += std::format("{}. <information is unavailable>\n", depth);
        }
        ++depth;
    }

    if (not dialog(title, message)) {
        // Failed to show the dialog box.
        std::println(stderr, "{}\n{}", title, message);
    }

    // Chain the optional older terminate handler.
    if (old_terminate_handler) {
        old_terminate_handler();
    }
}

} // namespace v1
} // namespace hi::v1
