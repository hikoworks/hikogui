// Copyright Take Vos 2023.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "../macros.hpp"
#include "progress.hpp"
#include "task.hpp"
#include "awaitable.hpp"
#include "awaitable_timer_intf.hpp"
#include <future>

hi_export_module(hikogui.dispatch.async_task);

hi_export namespace hi {
inline namespace v1 {

/** Run a function asynchronously as a co-routine task.
 *
 * @param func The function to be called.
 * @param args... The arguments forwarded to @a func.
 */
template<typename Func, typename... Args>
[[nodiscard]] std::invoke_result_t<Func, Args...> async_task(Func func, Args... args) requires(is_invocable_task_v<Func, Args...>)
{
    return func(args...);
}

/** Run a function asynchronously as a co-routine task.
 *
 * @note This will check every 15ms if the function is completed.
 * @param func The function to be called.
 * @param args... The arguments forwarded to @a func.
 */
template<typename Func, typename... Args>
[[nodiscard]] task<std::invoke_result_t<Func, Args...>> async_task(Func func, Args... args)
    requires(not is_invocable_task_v<Func, Args...>)
{
    using namespace std::literals;

    auto future = std::async(std::launch::async, [func = std::move(func), ... args = std::move(args)] {
        return func(args...);
    });

    if (not future.valid()) {
        throw std::future_error(std::future_errc::no_state);
    }

    while (true) {
        switch (future.wait_for(0s)) {
        case std::future_status::deferred:
            // This shouldn't really happen, this will cause the GUI to block.
            co_return future.get();

        case std::future_status::ready:
            co_return future.get();

        case std::future_status::timeout:
            co_await 15ms;
            break;

        default:
            hi_no_default();
        }
    }
}

/** Features of an invocable.
 */
enum class cancel_features_type {
    /** This invocable does not have extra arguments.
     */
    none = 0,

    /** The extra argument is a std::stop_token.
     */
    stop = 1,

    /** The extra argument is a hi::progress_token.
     */
    progress = 2,

    /** The extra arguments are a std::stop_token, followed by a hi::progress_token.
     */
    stop_and_progress = 3
};

/** Type trait to retrieve the cancel features of a invocable.
 *
 * @tparam Func The invocable to check.
 * @tparam Args The arguments to the invocable.
 */
template<typename Func, typename... Args>
struct cancel_features {
    // clang-format off
    constexpr static cancel_features_type value =
        std::is_invocable_v<Func, std::stop_token, ::hi::progress_token, Args...> ? cancel_features_type::stop_and_progress :
        std::is_invocable_v<Func, ::hi::progress_token, Args...> ? cancel_features_type::progress :
        std::is_invocable_v<Func, std::stop_token, Args...> ? cancel_features_type::stop :
        cancel_features_type::none;
    // clang-format on
};

/** The value of the hi::cancel_features<> type trait.
 */
template<typename Func, typename... Args>
constexpr auto cancel_features_v = cancel_features<Func, Args...>::value;


template<typename ResultType, typename FuncType, typename... ArgTypes>
concept compatible_cancelable_async_function_none = requires(FuncType f, ArgTypes... args) {
    {
        f(args...)
    } -> std::same_as<ResultType>;
};

template<typename ResultType, typename FuncType, typename... ArgTypes>
concept compatible_cancelable_async_function_stop = requires(FuncType f, std::stop_token st, ArgTypes... args) {
    {
        f(st, args...)
    } -> std::same_as<ResultType>;
};

template<typename ResultType, typename FuncType, typename... ArgTypes>
concept compatible_cancelable_async_function_progress = requires(FuncType f, progress_token pt, ArgTypes... args) {
    {
        f(pt, args...)
    } -> std::same_as<ResultType>;
};

template<typename ResultType, typename FuncType, typename... ArgTypes>
concept compatible_cancelable_async_function_stop_and_progress =
    requires(FuncType f, std::stop_token st, progress_token pt, ArgTypes... args) {
        {
            f(st, pt, args...)
        } -> std::same_as<ResultType>;
    };

template<typename ResultType, typename FuncType, typename... ArgTypes>
concept compatible_cancelable_async_task_none = requires(FuncType f, ArgTypes... args) {
    {
        f(args...)
    } -> std::same_as<task<ResultType>>;
};

template<typename ResultType, typename FuncType, typename... ArgTypes>
concept compatible_cancelable_async_task_stop = requires(FuncType f, std::stop_token st, ArgTypes... args) {
    {
        f(st, args...)
    } -> std::same_as<task<ResultType>>;
};

template<typename ResultType, typename FuncType, typename... ArgTypes>
concept compatible_cancelable_async_task_progress = requires(FuncType f, progress_token pt, ArgTypes... args) {
    {
        f(pt, args...)
    } -> std::same_as<task<ResultType>>;
};

template<typename ResultType, typename FuncType, typename... ArgTypes>
concept compatible_cancelable_async_task_stop_and_progress =
    requires(FuncType f, std::stop_token st, progress_token pt, ArgTypes... args) {
        {
            f(st, pt, args...)
        } -> std::same_as<task<ResultType>>;
    };

/** A concept for a callable that may be use in cancelable_async_task().
 */
// clang-format off
template<typename ResultType, typename FuncType, typename... ArgTypes>
concept compatible_cancelable_async_callable =
    compatible_cancelable_async_function_none<ResultType, FuncType, ArgTypes...> or
    compatible_cancelable_async_function_stop<ResultType, FuncType, ArgTypes...> or
    compatible_cancelable_async_function_progress<ResultType, FuncType, ArgTypes...> or
    compatible_cancelable_async_function_stop_and_progress<ResultType, FuncType, ArgTypes...> or
    compatible_cancelable_async_task_none<ResultType, FuncType, ArgTypes...> or
    compatible_cancelable_async_task_stop<ResultType, FuncType, ArgTypes...> or
    compatible_cancelable_async_task_progress<ResultType, FuncType, ArgTypes...> or
    compatible_cancelable_async_task_stop_and_progress<ResultType, FuncType, ArgTypes...>;
// clang-format on

/** Run a function asynchronously as a co-routine task.
 *
 * If the function @a func accepts the `std::stop_token` and/or
 * `hi::progress_token` then those arguments are passed to the function.
 *
 * @note This will call `async_task()`.
 * @param func The function to be called.
 * @param stop_token The stop token to optionally pass to the @a func.
 * @param progress_token The progress token to optionally pass to the @a func.
 * @param args... The arguments forwarded to @a func.
 * @return A hi::task object for the running @a func.
 */
template<typename Func, typename... Args>
[[nodiscard]] auto cancelable_async_task(Func func, std::stop_token stop_token, ::hi::progress_token progress_token, Args... args)
{
    constexpr auto features = cancel_features_v<Func, Args...>;

    if constexpr (features == cancel_features_type::stop_and_progress) {
        return async_task(std::move(func), std::move(stop_token), std::move(progress_token), std::move(args)...);

    } else if constexpr (features == cancel_features_type::progress) {
        return async_task(std::move(func), std::move(progress_token), std::move(args)...);

    } else if constexpr (features == cancel_features_type::stop) {
        return async_task(std::move(func), std::move(stop_token), std::move(args)...);

    } else if constexpr (features == cancel_features_type::none) {
        return async_task(std::move(func), std::move(args)...);

    } else {
        hi_static_no_default();
    }
}

} // namespace v1
}
