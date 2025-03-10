// Copyright Take Vos 2022.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "wfree_fifo.hpp"
#include "functional.hpp"
#include "../macros.hpp"
#include <future>

hi_export_module(hikogui.container.function_fifo);

hi_export namespace hi::inline v1 {

/** A fifo (First-in, First-out) for asynchronous calls.
 *
 * This fifo is used to handle asynchronous calls from an event-loop.
 *
 * @tparam Proto The `std::function` prototype.
 * @tparam SlotSize The size in bytes of each slot. This determines the maximum
 *                  number of functions that can be stored on the fifo and if
 *                  functions can be completely stored on the fifo or are allocated on the heap.
 */
template<typename Proto = void(), std::size_t SlotSize = 64>
class function_fifo {
public:
    constexpr function_fifo() noexcept = default;
    function_fifo(function_fifo const&) = delete;
    function_fifo(function_fifo&&) = delete;
    function_fifo& operator=(function_fifo const&) = delete;
    function_fifo& operator=(function_fifo&&) = delete;

    /** Check if there are not functions added to the fifo.
     */
    [[nodiscard]] bool empty() const noexcept
    {
        return _fifo.empty();
    }

    /** Run one of the function that was posted or send.
     *
     * @retval true One async function has been taken from the fifo and run.
     * @retval false The fifo was empty.
     */
    template<typename... Args>
    auto run_one(Args const &...args) noexcept
    {
        return _fifo.take_one([args...](auto& item) {
            return item(args...);
        });
    }

    /** Run all the functions posted or send on the fifo.
     *
     * This function calls `run_one()` until the fifo is empty.
     */
    void run_all() noexcept
    {
        while (run_one()) {}
    }

    /** Asynchronously post a functor to the fifo to be executed later.
     *
     * The function object and arguments are stored within the fifo and does
     * not need allocation.
     *
     * @note wait-free if the function object and arguments fit in the message slot of the fifo.
     * @param func A function object.
     */
    template<typename Func>
    void add_function(Func&& func) noexcept
    {
        _fifo.insert(make_function<Proto>(std::forward<Func>(func)));
    }

    /** Asynchronously send a functor to the fifo to be executed later.
     *
     * The function object and arguments are stored within the fifo and does
     * not need allocation. However the `std::promise` object will allocate the
     * return object on the heap as it must be shared with `std::future`.
     *
     * @param func A function object.
     * @param args The arguments to pass to the function when called.
     * @return A `std::future` with the result of `func`. The result type may be `void`.
     */
    template<typename Func, typename... Args>
    auto add_async_function(Func&& func, Args&&...args) noexcept
        requires(std::is_invocable_v<std::decay_t<Func>, std::decay_t<Args>...>)
    {
        return _fifo.insert_and_invoke(
            [](auto& item) {
                return item.get_future();
            },
            make_function<Proto>(std::forward<Func>(func)));
    }

private : wfree_fifo<function<Proto>, SlotSize> _fifo;
};

} // namespace hi::inline v1
