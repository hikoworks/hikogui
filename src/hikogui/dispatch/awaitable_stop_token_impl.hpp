// Copyright Take Vos 2023.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "awaitable_stop_token_intf.hpp"
#include "loop_win32_intf.hpp"
#include "../macros.hpp"
#include <utility>
#include <coroutine>
#include <chrono>
#include <stop_token>

hi_export_module(hikogui.dispatch : awaitable_stop_token_impl);

hi_export namespace hi::inline v1 {

inline void awaitable_stop_token::callback_wrapper::operator()() noexcept
{
    // Stop tokens are specifically designed to be called from a different thread,
    // so we will post the function to the same thread as the co_await.
    await_loop->post_function([this]() {
        if (handle and not handle.done()) {
            handle.resume();
        }
    });
}

inline void awaitable_stop_token::await_suspend(std::coroutine_handle<> handle) noexcept
{
    _stop_callback_ptr = std::make_unique<std::stop_callback<callback_wrapper>>(_stop_token, callback_wrapper{&loop::local(), handle});
}

}
