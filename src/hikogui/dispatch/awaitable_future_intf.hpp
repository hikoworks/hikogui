// Copyright Take Vos 20242.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "awaitable.hpp"
#include "../concurrency/concurrency.hpp"
#include "../macros.hpp"
#include <future>
#include <coroutine>
#include <functional>

hi_export_module(hikogui.dispatch : awaitable_future_intf);

hi_export namespace hi::inline v1 {

template<typename T>
class awaitable_future {
public:
    using value_type = T;

    awaitable_future(awaitable_future &&) noexcept = default;
    awaitable_future &operator=(awaitable_future &&) noexcept = default;
    awaitable_future(awaitable_future const &) noexcept = delete;
    awaitable_future &operator=(awaitable_future const &) noexcept = delete;

    awaitable_future(std::future<value_type>& future) noexcept : _future(std::addressof(future))
    {
    }

    [[nodiscard]] bool await_ready() const noexcept
    {
        using namespace std::chrono_literals;
        assert(_future != nullptr);
        assert(_future->valid());
        return _future->wait_for(0ms) == std::future_status::ready;
    }

    void await_suspend(std::coroutine_handle<> handle) noexcept;

    value_type await_resume() const noexcept
    {
        assert(_future != nullptr);
        assert(_future->valid());
        return _future->get();
    }

private:
    std::future<value_type> *_future = nullptr;
    callback<void()> _callback;
};

template<typename T>
struct awaitable_cast<std::future<T>> {
    awaitable_future<T> operator()(std::future<T>& rhs) noexcept
    {
        return awaitable_future<T>{rhs};
    }
};

} // namespace hi::inline v1
