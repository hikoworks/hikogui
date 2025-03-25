
#pragma once

#include <compare> // XXX #619
#include <cstddef> // XXX #619
#include <memory> // XXX #619
#include <chrono> // XXX #619
#include "awaitable_future_impl.hpp" // export
#include "awaitable_future_intf.hpp" // export
#include "awaitable_stop_token_intf.hpp" // export
#include "awaitable_stop_token_impl.hpp" // export
#include "awaitable_timer_intf.hpp" // export
#include "awaitable_timer_impl.hpp" // export
#include "awaitable.hpp" // export
#include "function_timer.hpp" // export
#include "loop_win32_intf.hpp" // export
#include "notifier.hpp" // export
#include "socket_event.hpp" // export
#include "stop_and_progress_source.hpp" // export
#include "stop_and_progress_token.hpp" // export
#include "stop_and_progress.hpp" // export
#include "task.hpp" // export
#include "when_any.hpp" // export

/** @module hikogui.dispatch
 * @brief Asynchronous dispatching and co-routine tasks.
 *
 * The Dispatch Loop
 * -----------------
 * Each thread has a local dispatch loop, which can be accessed using
 * the static function `hi::loop::local()`, which will return a `hi::loop`
 * instance.
 *
 * There are also two well-known loops which can be accessed using the
 * static functions `hi::loop::main()` and `hi::loop::timer()`.
 * 
 * The main-loop is used to handle events from the GUI, Audio-control and
 * network. The main-loop is latency sensitive and any event should be
 * handled quickly.
 *
 * The timer-loop is less latency sensitive and can be used for slower
 * maintenace tasks, such as: high-resolution clock synchronization and logging
 * of telemetry.
 *
 * To "pump" the events you can use either `hi::loop::resume_once()` which
 * will handle events for one iteration; or `hi::loop::resume()` which
 * will handle events until exit is requested, or until there are no more
 * event handlers registered.
 *
 * Currently you will need to use `hi::loop::resume()` to handle the GUI
 * system. One windows a complex setup of threads and priorities is needed
 * to properly handle redraws, windows-events and networking on the same
 * thread which is setup by `hi::loop:resume()`.
 *
 * Coroutines and Awaitables
 * -------------------------
 * HikoGUI implements a asynchronous task called hi::task. There are two forms;
 * the default is unscoped where the task will continue even if the hi::task
 * object is destroyed. The second is a scoped task, with the
 * alias hi::scoped_task, which will destroy a running task when
 * the hi::task object is destroyed.
 *
 * A hi::task can await on objects that match the concept
 * hi::convertible_to_awaitable. This handles:
 *  - An object that is awaitable, i.e. with the full set of await_*() member functions.
 *  - An object that has a `operator co_await()` operator member function.
 *  - An object that has a `operator co_await()` operator free function.
 *  - An object for which hi::awaitable_cast<> is specialized. This includes
 *    by default:
 *    + std::chrono::duration (including the std::chrono::literals).
 *    + std::chrono::time_stamp.
 *    + std::stop_token
 *
 */
hi_export_module(hikogui.dispatch);
