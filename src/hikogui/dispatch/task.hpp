// Copyright Take Vos 2022.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "notifier.hpp"
#include "awaitable.hpp"
#include "loop_win32_intf.hpp"
#include "../utility/utility.hpp"
#include "../concurrency/unfair_mutex.hpp" // XXX #616
#include "../concurrency/thread.hpp" // XXX #616
#include "../macros.hpp"
#include <coroutine>
#include <type_traits>
#include <memory>
#include <exception>
#include <optional>

hi_export_module(hikogui.dispatch : task);

hi_export namespace hi::inline v1 {
/** A task.
 *
 * This implements a asynchronous co-routine task.
 *
 * @tparam T The type returned by co_return.
 * @tparam DestroyFrame Destroy the coroutine frame when `task` goes out of scope.
 */
template<typename T = void, bool DestroyFrame = false>
class task {
public:
    using value_type = T;
    using notifier_type = notifier<void(value_type)>;
    using callback_type = notifier_type::callback_type;

    struct promise_type {
        notifier_type notifier;
        std::optional<value_type> value = {};
        std::exception_ptr exception = nullptr;

        template<std::convertible_to<value_type> NewValue>
        void return_value(NewValue&& new_value) noexcept
        {
            value = std::forward<NewValue>(new_value);
        }

        void unhandled_exception() noexcept
        {
            exception = std::current_exception();
        }

        std::suspend_always final_suspend() noexcept
        {
            // The notifier is posted on the loop to ensure the coroutine
            // gets completed. That way the callback will see the coroutine
            // as completed.
            if (value) {
                // Trigger the notifier with the co_return value.
                loop::local().post_function([this] {
                    this->notifier(*this->value);
                });

            } else {
                // Notify also in case of exception.
                hi_assert_not_null(exception);
                if constexpr (std::is_default_constructible_v<value_type>) {
                    loop::local().post_function([this] {
                        this->notifier(value_type{});
                    });
                }
            }

            return {};
        }

        task get_return_object() noexcept
        {
            return task{handle_type::from_promise(*this)};
        }

        /** Before we enter the coroutine, allow the caller to set the callback.
         */
        std::suspend_never initial_suspend() noexcept
        {
            return {};
        }

        template<convertible_to_awaitable RHS>
        decltype(auto) await_transform(RHS&& rhs)
        {
            return awaitable_cast<std::remove_cvref_t<RHS>>{}(std::forward<RHS>(rhs));
        }
    };

    using handle_type = std::coroutine_handle<promise_type>;

    task(handle_type coroutine) noexcept : _coroutine(coroutine) {}

    ~task()
    {
        if (DestroyFrame and _coroutine) {
            _coroutine.destroy();
        }
    }

    task() = default;

    // task can not be copied because it tracks if the co-routine must be destroyed by the
    // shared_ptr to the value shared between task and the promise.
    task(task const&) = delete;
    task& operator=(task const&) = delete;

    task(task&& other) noexcept
    {
        _coroutine = std::exchange(other._coroutine, {});
    }

    task& operator=(task&& other) noexcept
    {
        _coroutine = std::exchange(other._coroutine, {});
        return *this;
    }

    /** Check if the co-routine was started.
     */
    [[nodiscard]] bool started() const noexcept
    {
        return _coroutine;
    }

    /** Check if the co-routine is running
     */
    [[nodiscard]] bool running() const noexcept
    {
        return _coroutine and not _coroutine.done();
    }

    /** Check if the co-routine has completed.
     */
    [[nodiscard]] bool done() const noexcept
    {
        return _coroutine and _coroutine.done();
    }

    /** Get the return value returned from co_return.
     *
     * @note It is undefined behavior to call this function if the co-routine is incomplete.
     * @throws The exception thrown from the co-routine.
     */
    [[nodiscard]] value_type const& value() const
    {
        hi_axiom(done());

        auto const& promise = _coroutine.promise();
        if (promise.value) {
            return *promise.value;

        } else {
            hi_axiom(promise.exception);
            std::rethrow_exception(promise.exception);
        }
    }

    /** Get the return value returned from co_return.
     *
     * @note It is undefined behavior to call this function if the co-routine is incomplete.
     * @throws The exception thrown from the co-routine.
     */
    [[nodiscard]] value_type const& operator*() const
    {
        return value();
    }

    /** Subscribe a callback for when the co-routine is completed.
     *
     * @param flags The callback flags used to be passed to the notifier.
     * @param callback The callback to call when the co-routine executed co_return. If co_return
     *                 has a non-void expression then the callback must accept the expression as an argument.
     * @return The callback token used to manage the lifetime of the callback
     */
    template<forward_of<void(value_type)> Func>
    [[nodiscard]] callback<void(value_type)> subscribe(Func&& func, callback_flags flags = callback_flags::synchronous) noexcept
    {
        return _coroutine.promise().notifier.subscribe(std::forward<Func>(func), flags);
    }

    /** Create an awaiter that can await on this task.
     */
    auto operator co_await() const noexcept
    {
        return _coroutine.promise().notifier.operator co_await();
    }

private:
    // Optional value type
    handle_type _coroutine;
};

/**
 * @sa task<>
 */
template<bool DestroyFrame>
class task<void, DestroyFrame> {
public:
    using value_type = void;
    using notifier_type = notifier<void()>;
    using callback_type = notifier_type::callback_type;

    struct promise_type {
        notifier_type notifier;
        std::exception_ptr exception = nullptr;

        void return_void() noexcept {}

        void unhandled_exception() noexcept
        {
            exception = std::current_exception();
        }

        std::suspend_always final_suspend() noexcept
        {
            // The notifier is posted on the loop to ensure the coroutine
            // gets completed. That way the callback will see the coroutine
            // as completed.
            loop::local().post_function([this] {
                notifier();
            });
            return {};
        }

        task get_return_object() noexcept
        {
            return task{handle_type::from_promise(*this)};
        }

        std::suspend_never initial_suspend() noexcept
        {
            return {};
        }

        template<convertible_to_awaitable RHS>
        decltype(auto) await_transform(RHS&& rhs)
        {
            return awaitable_cast<std::remove_cvref_t<RHS>>{}(std::forward<RHS>(rhs));
        }
    };

    using handle_type = std::coroutine_handle<promise_type>;

    task(handle_type coroutine) noexcept : _coroutine(coroutine) {}

    ~task()
    {
        if (DestroyFrame and _coroutine) {
            _coroutine.destroy();
        }
    }

    task() = default;
    task(task const&) = delete;
    task& operator=(task const&) = delete;

    task(task&& other) noexcept
    {
        _coroutine = std::exchange(other._coroutine, {});
    }

    task& operator=(task&& other) noexcept
    {
        _coroutine = std::exchange(other._coroutine, {});
        return *this;
    }

    /** Check if the co-routine was started.
     */
    [[nodiscard]] bool started() const noexcept
    {
        return _coroutine;
    }

    /** Check if the co-routine is running
     */
    [[nodiscard]] bool running() const noexcept
    {
        return _coroutine and not _coroutine.done();
    }

    /** Check if the co-routine has completed.
     */
    [[nodiscard]] bool done() const noexcept
    {
        return _coroutine and _coroutine.done();
    }

    /** Get the return value returned from co_return.
     *
     * @note It is undefined behavior to call this function if the co-routine is incomplete.
     * @throws The exception thrown from the co-routine.
     */
    void value() const
    {
        hi_axiom(done());

        auto const& promise = _coroutine.promise();
        if (promise.exception) {
            std::rethrow_exception(promise.exception);
        }
    }

    /**
     * @sa notifier<>::subscribe()
     */
    template<forward_of<void()> Func>
    [[nodiscard]] callback<void()> subscribe(Func&& func, callback_flags flags = callback_flags::synchronous) noexcept
    {
        return _coroutine.promise().notifier.subscribe(std::forward<Func>(func), flags);
    }

    /** Create an awaiter that can await on this task.
     */
    auto operator co_await() const noexcept
    {
        return _coroutine.promise().notifier.operator co_await();
    }

private:
    // Optional value type
    handle_type _coroutine;
};

template<typename T = void>
using scoped_task = task<T, true>;

/** type-trait to determine if the given type @a T is a task.
 */
template<typename T>
struct is_task : std::false_type {};

template<typename ResultType, bool DestroyFrame>
struct is_task<hi::task<ResultType, DestroyFrame>> : std::true_type {};

/** type-trait to determine if the given type @a T is a task.
 */
template<typename T>
constexpr bool is_task_v = is_task<T>::value;

template<typename T>
struct task_value_type;

template<typename ResultType, bool DestroyFrame>
struct task_value_type<hi::task<ResultType, DestroyFrame>> {
    using type = ResultType;
};

template<typename T>
using task_value_type_t = task_value_type<T>::type;

/** type-trait to determining if the given invocable @a Func is a task.
 */
template<typename Func, typename... ArgTypes>
struct is_invocable_task {
    constexpr static bool value = is_task_v<std::invoke_result_t<Func, ArgTypes...>>;
};

/** type-trait to determining if the given invocable @a Func is a task.
 */
template<typename Func, typename... ArgTypes>
constexpr bool is_invocable_task_v = is_invocable_task<Func, ArgTypes...>::value;

/** Concept for a invocable that returns a task.
 */
template<typename Func, typename... ArgTypes>
concept invocable_task = std::invocable<Func, ArgTypes...> and is_invocable_task_v<Func, ArgTypes...>;

template<typename Func, typename... ArgTypes>
struct invoke_task_result {
    using type = task_value_type_t<std::invoke_result_t<Func, ArgTypes...>>;
};

template<typename Func, typename... ArgTypes>
using invoke_task_result_t = invoke_task_result<Func, ArgTypes...>::type;

} // namespace hi::inline v1
