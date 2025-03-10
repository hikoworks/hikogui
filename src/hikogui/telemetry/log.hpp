// Copyright Take Vos 2021-2022.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "delayed_format.hpp"
#include "format_check.hpp"
#include "../container/container.hpp"
#include "../time/time.hpp"
#include "../utility/utility.hpp"
#include "../concurrency/concurrency.hpp"
#include "../char_maps/char_maps.hpp" // XXX #619
#include "../macros.hpp"
#include <chrono>
#include <format>
#include <string>
#include <string_view>
#include <tuple>
#include <mutex>
#include <atomic>
#include <memory>
#include <thread>
#include <filesystem>
#include <cstdio>

hi_export_module(hikogui.telemetry : log);


hi_export namespace hi { inline namespace v1 {
namespace detail {

class log_message_base {
public:
    hi_force_inline log_message_base() noexcept = default;
    virtual ~log_message_base() = default;

    [[nodiscard]] virtual std::string format() const noexcept = 0;
    [[nodiscard]] virtual std::unique_ptr<log_message_base> make_unique_copy() const noexcept = 0;
};

template<global_state_type Level, fixed_string SourcePath, int SourceLine, fixed_string Fmt, typename... Values>
class log_message : public log_message_base {
public:
    static_assert(std::popcount(std::to_underlying(Level)) == 1);

    // clang-format off
    constexpr static char const *log_level_name =
        Level == global_state_type::log_fatal ? "fatal" :
        Level == global_state_type::log_error ? "error" :
        Level == global_state_type::log_warning ? "warning" :
        Level == global_state_type::log_info ? "info" :
        Level == global_state_type::log_debug ? "debug" :
        Level == global_state_type::log_trace ? "trace" :
        Level == global_state_type::log_audit ? "audit" :
        Level == global_state_type::log_statistics ? "stats" :
        "<unknown log level>";
    // clang-format on

    log_message(log_message const&) noexcept = default;
    log_message& operator=(log_message const&) noexcept = default;

    template<typename... Args>
    hi_force_inline log_message(Args&&...args) noexcept :
        _time_stamp(time_stamp_count::inplace_with_thread_id{}), _what(std::forward<Args>(args)...)
    {
    }

    std::string format() const noexcept override
    {
        auto const utc_time_point = time_stamp_utc::make(_time_stamp);
        auto const sys_time_point = std::chrono::clock_cast<std::chrono::system_clock>(utc_time_point);
        auto const local_time_point = cached_current_zone().to_local(sys_time_point);

        auto const cpu_id = _time_stamp.cpu_id();
        auto const thread_id = _time_stamp.thread_id();
        auto const thread_name = get_thread_name(thread_id);

        if constexpr (to_bool(Level & global_state_type::log_statistics)) {
            return std::format("{} {}({}) {:5} {}", local_time_point, thread_name, cpu_id, log_level_name, _what());
        } else {
            auto source_filename = std::filesystem::path{static_cast<std::string_view>(SourcePath)}.filename().generic_string();
            return std::format(
                "{} {}({}) {:5} {} ({}:{})",
                local_time_point,
                thread_name,
                cpu_id,
                log_level_name,
                _what(),
                source_filename,
                SourceLine);
        }
    }

    [[nodiscard]] std::unique_ptr<log_message_base> make_unique_copy() const noexcept override
    {
        return std::make_unique<log_message>(*this);
    }

private:
    time_stamp_count _time_stamp;
    delayed_format<Fmt, Values...> _what;
};

} // namespace detail

class log {
public:
    /** Log a message.
     * @tparam Level log level of message, must be greater or equal to the log level of the `system_status`.
     * @tparam SourcePath The source file where this function was called.
     * @tparam SourceLine The source line where this function was called.
     * @tparam Fmt The format string.
     * @param args Arguments to `std::format()`.
     */
    template<global_state_type Level, fixed_string SourcePath, int SourceLine, fixed_string Fmt, typename... Args>
    hi_force_inline void add(Args&&...args) noexcept
    {
        static_assert(std::popcount(std::to_underlying(Level)) == 1);

        auto const state = global_state.load(std::memory_order::relaxed);
        if (not to_bool(state & Level)) {
            return;
        }

        // Add messages in the queue, block when full.
        // * This reduces amount of instructions needed to be executed during logging.
        // * Simplifies logged_fatal_message logic.
        // * Will make sure everything gets logged.
        // * Blocking is bad in a real time thread, so maybe count the number of times it is blocked.

        // Emplace a message directly on the queue.
        _fifo.emplace<detail::log_message<Level, SourcePath, SourceLine, Fmt, forward_value_t<Args>...>>(
            std::forward<Args>(args)...);

        if (to_bool(Level & global_state_type::log_fatal) or not to_bool(state & global_state_type::log_is_running)) [[unlikely]] {
            // If the logger did not start we will log in degraded mode and log from the current thread.
            // On fatal error we also want to log from the current thread.
            flush();
        }
    }

    /** Flush all messages from the log_queue directly from this thread.
     * Flushing includes writing the message to a log file or displaying
     * them on the console.
     */
    hi_no_inline void flush() noexcept
    {
        bool wrote_message;
        do {
            std::unique_ptr<detail::log_message_base> copy_of_message;

            {
                auto const lock = std::scoped_lock(_mutex);

                wrote_message = _fifo.take_one([&copy_of_message](auto& message) {
                    copy_of_message = message.make_unique_copy();
                });
            }

            if (wrote_message) {
                hi_assert_not_null(copy_of_message);
                write(copy_of_message->format());
            }
        } while (wrote_message);
    }

    /** Start the logger system.
     *
     * Initialize the logger system if it is not already initialized and while the system is not in shutdown-mode.
     *
     * @param log_level The level at which to log.
     * @return true if the logger system is initialized, false when the system is being shutdown.
     */
    static bool start_subsystem(global_state_type log_level = global_state_type::log_level_default);

    /** Stop the logger system.
     * De-initialize the logger system if it is initialized.
     */
    static void stop_subsystem()
    {
        return hi::stop_subsystem(log::subsystem_deinit);
    }

private:
    /** The global log queue contains messages to be displayed by the logger thread.
     */
    wfree_fifo<detail::log_message_base, 64> _fifo;
    mutable unfair_mutex _mutex;

    /** Write to a log file and console.
     * This will write to the console if one is open.
     * It will also create a log file in the application-data directory.
     */
    void write(std::string const& str) const noexcept
    {
        std::println(stderr, "{}", str);
    }

    /** The global logger thread.
     */
    static inline std::jthread _log_thread;

    /** The function of the logger thread.
     *
     * @note implementation is in counters.hpp
     */
    static void log_thread_main(std::stop_token stop_token) noexcept;

    /** Deinitalize the logger system.
     */
    static void subsystem_deinit() noexcept;

    /** Initialize the log system.
     * This will start the logging threads which periodically
     * checks the log_queue for new messages and then
     * call log_flush_messages().
     */
    static bool subsystem_init() noexcept
    {
        _log_thread = std::jthread(log_thread_main);
        return true;
    }
};

inline log log_global;

inline bool log::start_subsystem(global_state_type log_level)
{
    set_log_level(log_level);
    if (hi::start_subsystem(global_state_type::log_is_running, log::subsystem_init, log::subsystem_deinit)) {
        on_terminate([]() {
            log_global.flush();
        });
        return true;
    } else {
        return false;
    }
}

/** Deinitalize the logger system.
 */
inline void log::subsystem_deinit() noexcept
{
    if (global_state_disable(global_state_type::log_is_running)) {
        if (_log_thread.joinable()) {
            _log_thread.request_stop();
            _log_thread.join();
        }

        log_global.flush();
    }
}

}} // namespace hi::v1
