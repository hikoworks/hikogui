// Copyright Take Vos 2022.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "../win32_headers.hpp"

#include "function_timer.hpp"
#include "function_predicate.hpp"
#include "socket_event.hpp"
#include "notifier.hpp"
#include "../container/container.hpp"
#include "../telemetry/telemetry.hpp"
#include "../concurrency/concurrency.hpp"
#include "../concurrency/unfair_mutex.hpp" // XXX #616
#include "../concurrency/thread.hpp" // XXX #616
#include "../time/time.hpp"
#include "../utility/utility.hpp"
#include "../macros.hpp"
#include <functional>
#include <type_traits>
#include <concepts>
#include <vector>
#include <memory>
#include <chrono>
#include <thread>

hi_export_module(hikogui.dispatch : loop_intf);

hi_export namespace hi::inline v1 {

class loop {
public:
    loop(loop const&) = delete;
    loop(loop&&) noexcept = delete;
    loop& operator=(loop const&) = delete;
    loop& operator=(loop&&) noexcept = delete;

    ~loop()
    {
        // Close all socket event handles.
        while (_handles.size() > _socket_handle_idx) {
            if (not WSACloseEvent(_handles.back())) {
                hi_log_error("Could not clock socket event handle for socket {}. {}", _sockets.back(), get_last_error_message());
            }

            _handles.pop_back();
            _sockets.pop_back();
        }

        if (_vsync_thread.joinable()) {
            _vsync_thread.request_stop();
            _vsync_thread.join();
        }

        if (not CloseHandle(_handles[_function_handle_idx])) {
            hi_log_error("Could not close async-event handle. {}", get_last_error_message());
        }
        if (not CloseHandle(_handles[_vsync_handle_idx])) {
            hi_log_error("Could not close vsync-event handle. {}", get_last_error_message());
        }
        if (not CloseHandle(_use_vsync_handle)) {
            hi_log_error("Could not close use-vsync handle. {}", get_last_error_message());
        }
    }

    loop() noexcept : _thread_id(current_thread_id())
    {
        // Create an level trigger event, to use as an on/off switch.
        if (auto handle = CreateEventW(NULL, TRUE, TRUE, NULL)) {
            _use_vsync_handle = handle;
        } else {
            hi_log_fatal("Could not create an use-vsync handle. {}", get_last_error_message());
        }

        // Create a pulse trigger event.
        if (auto handle = CreateEventW(NULL, FALSE, FALSE, NULL)) {
            _handles.push_back(handle);
            _sockets.push_back(-1);
            _socket_functions.emplace_back();
        } else {
            hi_log_fatal("Could not create an vsync-event handle. {}", get_last_error_message());
        }

        // Create a pulse trigger event.
        if (auto handle = CreateEventW(NULL, FALSE, FALSE, NULL)) {
            _handles.push_back(handle);
            _sockets.push_back(-1);
            _socket_functions.emplace_back();
        } else {
            hi_log_fatal("Could not create an async-event handle. {}", get_last_error_message());
        }
    }

    /** Get or create the thread-local loop.
     */
    [[nodiscard]] static loop& local() noexcept;

    /** Get or create the main-loop.
     *
     * @note The first time main() is called must be from the main-thread.
     *       In this case there is no race condition on the first time main() is called.
     */
    [[nodiscard]] hi_no_inline static loop& main() noexcept
    {
        if (auto ptr = _main.load(std::memory_order::acquire)) {
            return *ptr;
        }

        hi_axiom(_timer.load(std::memory_order::relaxed) == nullptr, "loop::main() must be called before loop::timer()");

        // This is the first time loop::main() is called so we must be on the main-thread
        // So name the thread "main" so we can find it during debugging.
        set_thread_name("main");

        auto ptr = std::addressof(local());
        _main.store(ptr, std::memory_order::release);
        return *ptr;
    }

    /** Get or create the timer event-loop.
     *
     * @note The first time this is called a thread is started to handle the timer events.
     */
    [[nodiscard]] hi_no_inline static loop& timer() noexcept
    {
        // The first time timer() is called, make sure that the main-loop exists,
        // or even create the main-loop on the current thread.
        [[maybe_unused]] auto const &tmp = loop::main();

        return *start_subsystem_or_terminate(_timer, nullptr, timer_init, timer_deinit);
    }

    /** Request all windows to be redrawn on the next frame.
     * 
     */
    void request_redraw() noexcept
    {
        _request_redraw.store(true, std::memory_order::relaxed);
    }

    /** Set maximum frame rate.
     *
     * A frame rate above 30.0 may will cause the vsync thread to block on
     *
     * @param frame_rate The maximum frame rate that a window will be updated.
     */
    void set_maximum_frame_rate(double frame_rate) noexcept
    {
        hi_axiom(on_thread());
    }

    /** Set the monitor id for vertical sync.
     */
    void set_vsync_monitor_id(uintptr_t id) noexcept
    {
        _selected_monitor_id.store(id, std::memory_order::relaxed);
    }

    /** Wait-free post a function to be called from the loop.
     *
     * @note It is safe to call this function from another thread.
     * @note The event loop is not directly notified that a new function exists
     *       and will be delayed until after the loop has woken for other work.
     * @note The post is only wait-free if the function fifo is not full,
     *       and the function is small enough to fit in a slot on the fifo.
     * @param func The function to call from the loop. The function must not take any arguments and return void.
     */
    template<forward_of<void()> Func>
    void wfree_post_function(Func&& func) noexcept
    {
        _function_fifo.add_function(std::forward<Func>(func));
    }

    /** Post a function to be called from the loop.
     *
     * @note It is safe to call this function from another thread.
     * @param func The function to call from the loop. The function must not take any arguments and return void.
     */
    template<forward_of<void()> Func>
    void post_function(Func&& func) noexcept
    {
        _function_fifo.add_function(std::forward<Func>(func));
        notify_has_send();
    }

    /** Call a function from the loop.
     *
     * @note It is safe to call this function from another thread.
     * @param func The function to call from the loop. The function must not take any argument,
     *             but may return a value.
     * @return A `std::future` for the return value.
     */
    template<typename Func>
    [[nodiscard]] auto async_function(Func&& func) noexcept
    {
        auto future = _function_fifo.add_async_function(std::forward<Func>(func));
        notify_has_send();
        return future;
    }

    

    /** Call a function at a certain time.
     *
     * @param time_point The time at which to call the function.
     * @param func The function to be called.
     */
    template<forward_of<void()> Func>
    [[nodiscard]] callback<void()> delay_function(utc_nanoseconds time_point, Func&& func) noexcept
    {
        auto [callback, first_to_call] = _function_timer.delay_function(time_point, std::forward<Func>(func));
        if (first_to_call) {
            // Notify if the added function is the next function to call.
            notify_has_send();
        }
        return std::move(callback);
    }

    /** Call a function repeatedly.
     *
     * @param period The period between calls to the function.
     * @param time_point The time at which to call the function.
     * @param func The function to be called.
     */
    template<forward_of<void()> Func>
    [[nodiscard]] callback<void()>
    repeat_function(std::chrono::nanoseconds period, utc_nanoseconds time_point, Func&& func) noexcept
    {
        auto [callback, first_to_call] = _function_timer.repeat_function(period, time_point, std::forward<Func>(func));
        if (first_to_call) {
            // Notify if the added function is the next function to call.
            notify_has_send();
        }
        return callback;
    }

    /** Call a function repeatedly.
     *
     * @param period The period between calls to the function.
     * @param func The function to be called.
     */
    template<forward_of<void()> Func>
    [[nodiscard]] callback<void()> repeat_function(std::chrono::nanoseconds period, Func&& func) noexcept
    {
        auto [callback, first_to_call] = _function_timer.repeat_function(period, std::forward<Func>(func));
        if (first_to_call) {
            // Notify if the added function is the next function to call.
            notify_has_send();
        }
        return std::move(callback);
    }

    
    template<forward_of<void()> Func>
    [[nodiscard]] callback<void()> delay_function_until(std::function<bool()> predicate, Func&& func) noexcept
    {
        return _function_predicate.add(std::move(predicate), std::forward<Func>(func));
    }

    //void subscribe_render(weak_callback<void(utc_nanoseconds)> callback) noexcept
    //{
    //}

    /** Subscribe a render function to be called on vsync.
     *
     * @param f A function to be called when vsync occurs. The function will be
     *          called with the time of the vsync and a boolean indicating if
     *          the window must be fully redrawn.
     */
    template<forward_of<void(utc_nanoseconds, bool)> Func>
    callback<void(utc_nanoseconds, bool)> subscribe_render(Func &&func) noexcept
    {
        hi_axiom(on_thread());

        auto cb = callback<void(utc_nanoseconds, bool)>{std::forward<Func>(func)};

        _render_functions.push_back(cb);

        // Startup the vsync thread once there is a window.
        if (not _vsync_thread.joinable()) {
            _vsync_thread = std::jthread{[this](std::stop_token token) {
                return vsync_thread_proc(std::move(token));
            }};
        }

        return cb;
    }

    /** Add a callback that reacts on a socket.
     *
     * In most cases @a mode is set to one of the following values:
     * - error | read: Unblock when there is data available for read.
     * - error | write: Unblock when there is buffer space available for write.
     * - error | read | write: Unblock when there is data available for read of when there is buffer space available for write.
     *
     * @note Only one callback can be associated with a socket.
     * @param fd File descriptor of the socket.
     * @param event_mask The socket events to wait for.
     * @param f The callback to call when the file descriptor unblocks.
     */
    void add_socket(int fd, socket_event event_mask, std::function<void(int, socket_events const&)> f)
    {
        hi_axiom(on_thread());
        hi_not_implemented();
    }

    /** Remove the callback associated with a socket.
     *
     * @param fd The file descriptor of the socket.
     */
    void remove_socket(int fd)
    {
        hi_axiom(on_thread());
        hi_not_implemented();
    }

    /** Resume the loop on the current thread.
     *
     * @param stop_token The thread's stop token to use to determine when to stop.
     *                   If not stop token is given, then resume will automatically stop when there
     *                   are no more windows, sockets, functions or timers.
     * @return Exit code when the loop is exited.
     */
    int resume(std::stop_token stop_token = {}) noexcept
    {
        auto const is_main = this == _main.load(std::memory_order::relaxed);

        // Microsoft recommends an event-loop that also renders to the screen to run at above normal priority.
        auto const thread_handle = GetCurrentThread();

        int original_thread_priority = GetThreadPriority(thread_handle);
        if (original_thread_priority == THREAD_PRIORITY_ERROR_RETURN) {
            original_thread_priority = THREAD_PRIORITY_NORMAL;
            hi_log_error("GetThreadPriority() for loop failed {}", get_last_error_message());
        }

        if (is_main and original_thread_priority < THREAD_PRIORITY_ABOVE_NORMAL) {
            if (not SetThreadPriority(thread_handle, THREAD_PRIORITY_ABOVE_NORMAL)) {
                hi_log_error("SetThreadPriority() for loop failed {}", get_last_error_message());
            }
        }

        _exit_code = {};
        while (not _exit_code) {
            resume_once(true);

            if (stop_token.stop_possible()) {
                if (stop_token.stop_requested()) {
                    // Stop immediately when stop is requested.
                    _exit_code = 0;
                }
            } else {
                if (_render_functions.empty() and _function_fifo.empty() and _function_timer.empty() and
                    _handles.size() <= _socket_handle_idx) {
                    // If there is not stop token, then exit when there are no more resources to wait on.
                    _exit_code = 0;
                }
            }
        }

        // Set the thread priority back to what is was before resume().
        if (is_main and original_thread_priority < THREAD_PRIORITY_ABOVE_NORMAL) {
            if (not SetThreadPriority(thread_handle, original_thread_priority)) {
                hi_log_error("SetThreadPriority() for loop failed {}", get_last_error_message());
            }
        }

        return *_exit_code;
    }

    /** Resume for a single iteration.
     *
     * The `resume_once(false)` may be used to continue processing events and
     * GUI redraws while the GUI event queue is blocked. This happens on win32 when
     * a window is being moved, resized, the title bar or system menu being clicked.
     *
     * It should be called often, as it will be used to process network messages and
     * latency of network processing will be increased based on the amount of times
     * this function is called.
     *
     * @note This function must be called from the same thread as `resume()`.
     * @param block Allow processing to block, this is normally done only inside `resume()`.
     */
    void resume_once(bool block = false) noexcept
    {
        using namespace std::chrono_literals;

        hi_axiom(on_thread());

        auto const is_main = this == _main.load(std::memory_order::relaxed);

        auto current_time = std::chrono::utc_clock::now();
        auto timeout = std::chrono::duration_cast<std::chrono::milliseconds>(_function_timer.current_deadline() - current_time);

        timeout = std::clamp(timeout, 0ms, 100ms);
        auto const timeout_ms = narrow_cast<DWORD>(timeout / 1ms);

        // Only handle win32 messages when blocking.
        // Since non-blocking is called from the win32 message-pump, we do not want to re-enter the loop.
        auto const message_mask = is_main and block ? QS_ALLINPUT : 0;

        auto const wait_r =
            MsgWaitForMultipleObjects(narrow_cast<DWORD>(_handles.size()), _handles.data(), FALSE, timeout_ms, message_mask);

        if (wait_r == WAIT_FAILED) {
            hi_log_fatal("Failed on MsgWaitForMultipleObjects(), {}", get_last_error_message());

        } else if (wait_r == WAIT_TIMEOUT) {
            // handle_functions() and handle_timers() is called after every wake-up of MsgWaitForMultipleObjects
            ;

        } else if (wait_r == WAIT_OBJECT_0 + _vsync_handle_idx) {
            // XXX Make sure this is not starving the win32 events.
            // should we just empty the win32 events after every unblock?
            handle_vsync();

        } else if (wait_r == WAIT_OBJECT_0 + _function_handle_idx) {
            // handle_functions() and handle_timers() is called after every
            // wake-up of MsgWaitForMultipleObjects(). This is done below the
            // if-cascade to make sure that the function-event is triggered
            // after any kind of wake-up.
            ;

        } else if (wait_r >= WAIT_OBJECT_0 + _socket_handle_idx and wait_r < WAIT_OBJECT_0 + _handles.size()) {
            auto const index = wait_r - WAIT_OBJECT_0;

            WSANETWORKEVENTS events;
            if (WSAEnumNetworkEvents(_sockets[index], _handles[index], &events) != 0) {
                switch (WSAGetLastError()) {
                case WSANOTINITIALISED:
                    hi_log_fatal("WSAStartup was not called.");
                case WSAENETDOWN:
                    hi_log_fatal("The network subsystem has failed.");
                case WSAEINVAL:
                    hi_log_fatal("One of the specified parameters was invalid.");
                case WSAEINPROGRESS:
                    hi_log_warning(
                        "A blocking Windows Sockets 1.1 call is in progress, or the service provider is still processing a "
                        "callback "
                        "function.");
                    break;
                case WSAEFAULT:
                    hi_log_fatal("The lpNetworkEvents parameter is not a valid part of the user address space.");
                case WSAENOTSOCK:
                    // If somehow the socket was destroyed, lets just remove it.
                    hi_log_error("Error during WSAEnumNetworkEvents on socket {}: {}", _sockets[index], get_last_error_message());
                    _handles.erase(_handles.begin() + index);
                    _sockets.erase(_sockets.begin() + index);
                    _socket_functions.erase(_socket_functions.begin() + index);
                    break;
                default:
                    hi_no_default();
                }

            } else {
                // Because of how WSAEnumNetworkEvents() work we must only handle this specific socket.
                _socket_functions[index](_sockets[index], socket_events_from_win32(events));
            }

        } else if (wait_r == WAIT_OBJECT_0 + _handles.size()) {
            handle_gui_events();

        } else if (wait_r >= WAIT_ABANDONED_0 and wait_r < WAIT_ABANDONED_0 + _handles.size()) {
            auto const index = wait_r - WAIT_ABANDONED_0;
            if (index == _vsync_handle_idx) {
                hi_log_fatal("The vsync-handle has been abandoned.");

            } else if (index == _function_handle_idx) {
                hi_log_fatal("The async-handle has been abandoned.");

            } else {
                // Socket handle has been abandoned. Remove it from the handles.
                hi_log_error("The socket-handle for socket {} has been abandoned.", _sockets[index]);
                _handles.erase(_handles.begin() + index);
                _sockets.erase(_sockets.begin() + index);
                _socket_functions.erase(_socket_functions.begin() + index);
            }

        } else {
            hi_no_default();
        }

        // Make sure timers are handled first, possibly they are time critical.
        handle_timers();

        // When functions are added wait-free, the function-event is never triggered.
        // So handle messages after any kind of wake up.
        handle_functions();

        // The predicate functions are functions that are called when a certain
        // condition is met.
        handle_predicate_functions();
    }

    /** Check if the current thread is the same as the loop's thread.
     *
     * The loop's thread is the thread that calls resume().
     */
    [[nodiscard]] bool on_thread() const noexcept
    {
        return current_thread_id() == _thread_id;
    }

private:
    /** Pointer to the main-loop.
     */
    inline static std::atomic<loop *> _main;

    /** Pointer to the timer-loop.
     */
    inline static std::atomic<loop *> _timer;

    inline static std::jthread _timer_thread;

    function_fifo<> _function_fifo;
    function_timer _function_timer;
    function_predicate _function_predicate;

    std::optional<int> _exit_code = {};
    double _maximum_frame_rate = 30.0;
    std::chrono::nanoseconds _minimum_frame_time = std::chrono::nanoseconds(33'333'333);
    thread_id _thread_id;
    std::vector<weak_callback<void(utc_nanoseconds, bool)>> _render_functions;
    std::atomic<bool> _request_redraw = false;

    struct socket_type {
        int fd;
        socket_event mode;
        std::function<void(int, socket_events const&)> callback;
    };

    constexpr static size_t _vsync_handle_idx = 0;
    constexpr static size_t _function_handle_idx = 1;
    constexpr static size_t _socket_handle_idx = 2;

    /** event-handle to continue the vsync.
     *
     * This event handle is a manual reset event.
     *
     * - set: Use `IDXGIOutput::WaitForVBlank()` at high priority.
     * - reset: Use `WaitForSingleObject()` timeout on low priority to about 30fps.
     */
    HANDLE _use_vsync_handle;

    /** Time when the last vertical blank happened.
     */
    std::atomic<utc_nanoseconds> _vsync_time;

    /** The last vsync_time update was made by a call to Sleep().
     */
    bool _vsync_time_from_sleep = true;

    /** pull down ratio for triggering SetEvent from WaitForVBlank.
     *
     * Format is in UQ8.8, this is done to reduce judder introduced by float precision.
     */
    std::atomic<uint16_t> _pull_down = 0x100;

    /** Sub-frame count in UQ56.8 format, incremented by `pull_down` on each vertical-blank.
     *
     * This is incremented only when blocking on vertical-blank.
     */
    uint64_t _sub_frame_count = 0;

    /** Frame count after pull-down.
     *
     * This is incremented only when blocking on vertical-blank.
     */
    uint64_t _frame_count = 0;

    /** The handles to block on.
     *
     * The following is the order of handles:
     * - 0 : vsync event-handle
     * - 1 : async-fifo event-handle
     * - x : A handle, one for each socket.
     *
     */
    std::vector<HANDLE> _handles;

    /** Socket file descriptors.
     *
     * This list contains one-to-one file descriptors with `handles`.
     * The first two file descriptors have the value -1 (for the non
     * socket handles).
     */
    std::vector<int> _sockets;

    /** A list of functions to call on an event to a socket.
     */
    std::vector<std::function<void(int, socket_events const&)>> _socket_functions;

    /** The vsync thread.
     */
    std::jthread _vsync_thread;

    /** The vsync thread handle.
     */
    HANDLE _vsync_thread_handle;

    /** The current priority of the vsync thread.
     */
    int _vsync_thread_priority = THREAD_PRIORITY_NORMAL;

    /** The monitor id that is selected for vsync.
     */
    std::atomic<std::uintptr_t> _selected_monitor_id = 0;

    /** The primary monitor id.
     * As returned by os_settings::primary_monitor_id().
     */
    std::uintptr_t _vsync_monitor_id = 0;

    /** The DXGI Output of the primary monitor.
     */
    IDXGIOutput *_vsync_monitor_output = nullptr;

    static loop *timer_init() noexcept
    {
        hi_assert(not _timer_thread.joinable());

        _timer_thread = std::jthread{[](std::stop_token stop_token) {
            _timer.store(std::addressof(loop::local()), std::memory_order::release);

            set_thread_name("timer");
            loop::local().resume(stop_token);
        }};

        while (true) {
            if (auto ptr = _timer.load(std::memory_order::relaxed)) {
                return ptr;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
        }
    }

    static void timer_deinit() noexcept
    {
        if (auto const *const ptr = _timer.exchange(nullptr, std::memory_order::acquire)) {
            hi_assert(_timer_thread.joinable());
            _timer_thread.request_stop();
            _timer_thread.join();
        }
    }

    /** Notify the event loop that a function was added to the _function_fifo.
     */
    void notify_has_send() noexcept
    {
        if (not SetEvent(_handles[_function_handle_idx])) {
            hi_log_error("Could not trigger async-event. {}", get_last_error_message());
        }
    }

    /** Call elapsed timers.
     *
     * @param deadline The deadline before all timers must be handled before moving on.
     */
    void handle_vsync() noexcept
    {
        // XXX Reduce the number of redraws for each window based on the refresh rate of the monitor they are located on.
        // XXX handle maximum frame rate and update vsync thread
        // XXX Update active windows more often than inactive windows.

        if (not _vsync_thread.joinable()) {
            // Fallback for the vsync_time advancing when the vsync thread is not running.
            _vsync_time.store(std::chrono::utc_clock::now());
        }

        auto const display_time = _vsync_time.load(std::memory_order::relaxed) + std::chrono::milliseconds(30);

        auto request_redraw = _request_redraw.exchange(false);
        for (auto& render_function : _render_functions) {
            if (auto rf = render_function.lock()) {
                rf(display_time, request_redraw);
            }
        }

        std::erase_if(_render_functions, [](auto& render_function) {
            return render_function.expired();
        });

        if (_render_functions.empty()) {
            // Stop the vsync thread when there are no more windows.
            if (_vsync_thread.joinable()) {
                _vsync_thread.request_stop();
            }
        }
    }

    /** Handle all function calls.
     *
     * @param deadline The deadline before all calls must be executed before moving on.
     */
    void handle_functions() noexcept
    {
        _function_fifo.run_all();
    }

    void handle_predicate_functions() noexcept
    {
        _function_predicate.run_all();
    }

    void handle_timers() noexcept
    {
        _function_timer.run_all(std::chrono::utc_clock::now());
    }

    /** Handle the gui events.
     *
     * @param deadline The deadline before all gui-events are handled before moving on.
     */
    void handle_gui_events() noexcept
    {
        MSG msg = {};
        auto const t1 = trace<"loop:gui-events">();
        while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE | PM_NOYIELD)) {
            auto const t2 = trace<"loop:gui-event">();

            if (msg.message == WM_QUIT) {
                _exit_code = narrow_cast<int>(msg.wParam);
                continue;
            }

            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }

    /** Update the dxgi_output to point to the primary-monitor.
     *
     * @note This function is cheap if the primary-monitor does not change.
     */
    void vsync_thread_update_dxgi_output() noexcept
    {
        if (not compare_store(_vsync_monitor_id, _selected_monitor_id.load(std::memory_order::relaxed))) {
            return;
        }

        if (_vsync_monitor_output) {
            _vsync_monitor_output->Release();
            _vsync_monitor_output = nullptr;
        }

        IDXGIFactory *factory = nullptr;
        if (FAILED(CreateDXGIFactory(__uuidof(IDXGIFactory), (void **)&factory))) {
            hi_log_error_once("vsync:error:CreateDXGIFactory", "Could not IDXGIFactory. {}", get_last_error_message());
            return;
        }
        hi_assert_not_null(factory);
        auto d1 = defer([&] {
            factory->Release();
        });

        IDXGIAdapter *adapter = nullptr;
        if (FAILED(factory->EnumAdapters(0, &adapter))) {
            hi_log_error_once("vsync:error:EnumAdapters", "Could not get IDXGIAdapter. {}", get_last_error_message());
            return;
        }
        hi_assert_not_null(adapter);
        auto d2 = defer([&] {
            adapter->Release();
        });

        if (FAILED(adapter->EnumOutputs(0, &_vsync_monitor_output))) {
            hi_log_error_once("vsync:error:EnumOutputs", "Could not get IDXGIOutput. {}", get_last_error_message());
            return;
        }

        DXGI_OUTPUT_DESC description;
        if (FAILED(_vsync_monitor_output->GetDesc(&description))) {
            hi_log_error_once("vsync:error:GetDesc", "Could not get IDXGIOutput description. {}", get_last_error_message());
            _vsync_monitor_output->Release();
            _vsync_monitor_output = nullptr;
            return;
        }

        if (description.Monitor != std::bit_cast<HMONITOR>(_vsync_monitor_id)) {
            hi_log_error_once("vsync:error:not-primary-monitor", "DXGI primary monitor does not match desktop primary monitor");
            _vsync_monitor_output->Release();
            _vsync_monitor_output = nullptr;
            return;
        }

        d2.cancel();
        d1.cancel();
    }

    /** Update the `vsync_time`.
     *
     * This function should be called directly after a vsync or sleep
     * to update the time when the last vsync happened. The `vsync_time`
     * is used to calculate the time when the next frame is displayed on the screen.
     *
     * @param on_sleep Set to true when this function was called after sleeping.
     * @return The duration since the last vsync. Used to determine if vsync didn't block.
     */
    std::chrono::nanoseconds vsync_thread_update_time(bool on_sleep)
    {
        auto const ts = time_stamp_count(time_stamp_count::inplace_with_cpu_id{});
        auto const new_time = time_stamp_utc::make(ts);

        auto const was_sleeping = std::exchange(_vsync_time_from_sleep, on_sleep);
        auto const old_time = _vsync_time.exchange(new_time, std::memory_order::acquire);

        // If old_time was caused by sleeping it can not be used to calculate how long vsync was blocking.
        return was_sleeping ? std::chrono::nanoseconds::max() : new_time - old_time;
    }

    void vsync_thread_wait_for_vblank() noexcept
    {
        using namespace std::chrono_literals;

        vsync_thread_update_dxgi_output();

        if (_vsync_monitor_output and FAILED(_vsync_monitor_output->WaitForVBlank())) {
            hi_log_error_once("vsync:error:WaitForVBlank", "WaitForVBlank() failed. {}", get_last_error_message());
        }

        if (vsync_thread_update_time(false) < 1ms) {
            hi_log_info_once("vsync:monitor-off", "WaitForVBlank() did not block; is the monitor turned off?");
            Sleep(16);

            // Fixup the time after the fallback sleep.
            vsync_thread_update_time(true);
        } else {
            ++global_counter<"vsync:vertical-blank">;
        }
    }

    /** The pull-down algorithm
     *
     * Handles pull-down from the frame rate of the primary monitor to the maximum frame-rate of all windows.
     * The calculation here uses fixed-point to get a fixed pattern/cadence of frame updates.
     *
     * @return True if the frame needs to be updated.
     */
    [[nodiscard]] bool vsync_thread_pull_down() noexcept
    {
        _sub_frame_count += _pull_down.load(std::memory_order::relaxed);
        return compare_store(_frame_count, _sub_frame_count >> 8);
    }

    /** Change the priority of the vsync-thread.
     *
     * @note This function is cheap when requesting the same priority multiple time.
     * @param new_priority A win32 thread priority; THREAD_PRIORITY_NORMAL or THREAD_PRIORITY_TIME_CRITICAL
     */
    void vsync_thread_update_priority(int new_priority) noexcept
    {
        if (std::exchange(_vsync_thread_priority, new_priority) != new_priority) {
            if (not SetThreadPriority(_vsync_thread_handle, new_priority)) {
                hi_log_error_once("vsync:error:SetThreadPriority", "Could not set the vsync thread priority to {}", new_priority);
            }
        }
    }

    void vsync_thread_proc(std::stop_token stop_token) noexcept
    {
        _vsync_thread_handle = GetCurrentThread();
        set_thread_name("vsync");

        while (not stop_token.stop_requested()) {
            switch (WaitForSingleObject(_use_vsync_handle, 30)) {
            case WAIT_TIMEOUT:
                // When use_vsync is off wake the main loop every 30ms.
                vsync_thread_update_time(true);

                vsync_thread_update_priority(THREAD_PRIORITY_NORMAL);

                ++global_counter<"vsync:low-priority">;
                ++global_counter<"vsync:frame">;
                SetEvent(_handles[_vsync_handle_idx]);
                break;

            case WAIT_OBJECT_0:
                // When use_vsync is on wake the main loop based on the vertical-sync and pull_down.
                vsync_thread_update_priority(THREAD_PRIORITY_TIME_CRITICAL);

                vsync_thread_wait_for_vblank();

                if (vsync_thread_pull_down()) {
                    ++global_counter<"vsync:frame">;
                    SetEvent(_handles[_vsync_handle_idx]);
                }

                break;

            case WAIT_ABANDONED:
                hi_log_error_once("vsync:error:WAIT_ABANDONED", "use_vsync_handle has been abandoned.");
                ResetEvent(_use_vsync_handle);
                break;

            case WAIT_FAILED:
                hi_log_error_once("vsync:error:WAIT_FAILED", "WaitForSingleObject failed. {}", get_last_error_message());
                ResetEvent(_use_vsync_handle);
                break;
            }
        }
    }
};

namespace detail {
inline thread_local std::unique_ptr<loop> thread_local_loop;
}

/** Get or create the thread-local loop.
 */
[[nodiscard]] hi_no_inline inline loop& loop::local() noexcept
{
    if (not detail::thread_local_loop) {
        detail::thread_local_loop = std::make_unique<loop>();
    }
    return *detail::thread_local_loop;
}

template<typename R, typename... Args>
template<forward_of<void()> Func>
void notifier<R(Args...)>::loop_local_post_function(Func&& func) const noexcept
{
    return loop::local().post_function(std::forward<Func>(func));
}

template<typename R, typename... Args>
template<forward_of<void()> Func>
void notifier<R(Args...)>::loop_main_post_function(Func&& func) const noexcept
{
    return loop::main().post_function(std::forward<Func>(func));
}

template<typename R, typename... Args>
template<forward_of<void()> Func>
void notifier<R(Args...)>::loop_timer_post_function(Func&& func) const noexcept
{
    return loop::timer().post_function(std::forward<Func>(func));
}

} // namespace hi::inline v1
