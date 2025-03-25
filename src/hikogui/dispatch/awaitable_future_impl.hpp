// Copyright Take Vos 2024.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "awaitable_future_intf.hpp"
#include "loop_win32_intf.hpp"
#include "../macros.hpp"
#include <utility>
#include <coroutine>
#include <chrono>

hi_export_module(hikogui.dispatch : awaitable_future_impl);

hi_export namespace hi::inline v1 {
template<typename T>
inline void awaitable_future<T>::await_suspend(std::coroutine_handle<> handle) noexcept
{
    _callback = loop::local().delay_function_until(
        [&]() {
            using namespace std::chrono_literals;
            
            assert(_future != nullptr);
            assert(_future->valid());
            return _future->wait_for(0ms) == std::future_status::ready;
        },
        [=]() {
            handle.resume();
        });
}
}
