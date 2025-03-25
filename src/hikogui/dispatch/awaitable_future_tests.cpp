// Copyright Take Vos 2024.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

#include "dispatch.hpp"
#include <hikotest/hikotest.hpp>
#include <atomic>
#include <future>

TEST_SUITE(awaitable_future_suite) {

static int my_function(int* x)
{
    using namespace std::literals;

    std::this_thread::sleep_for(100ms);
    std::atomic_ref(*x).store(42);
    return *x + 1;
}

static hi::task<int> async_task(int* x)
{
    auto f = std::async(std::launch::async, [&]() -> int {
        return my_function(x);
    });

    auto r = co_await f;
    co_return r;
}

TEST_CASE(function_test)
{
    int x = 0;

    auto t = async_task(&x);
    
    REQUIRE(x == 0);
    
    auto start = std::chrono::high_resolution_clock::now();
    do {
        using namespace std::chrono_literals;

        hi::loop::local().resume_once();

        if (std::chrono::high_resolution_clock::now() - start > 10s) {
            break;
        }
    } while (not t.done());

    REQUIRE(x == 42);
    REQUIRE(t.value() == 43);
}

}; // TEST_SUITE(function_predicate_suite)
