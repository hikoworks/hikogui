// Copyright Take Vos 2024.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

#include "dispatch.hpp"
#include <hikotest/hikotest.hpp>
#include <atomic>
#include <future>

TEST_SUITE(function_predicate_suite) {

static int my_function(int* x)
{
    using namespace std::literals;

    std::this_thread::sleep_for(100ms);
    std::atomic_ref(*x).store(42);
    return *x + 1;
}

TEST_CASE(function_test)
{
    int x = 0;
    int y = 0;

    auto f = std::async(std::launch::async, [&]() {
        return my_function(&x);
    });

    auto cbt = hi::loop::local().delay_function_until(
        [&] {
            using namespace std::chrono_literals;

            assert(f.valid());
            return f.wait_for(0ms) == std::future_status::ready;
        },
        [&] {
            y = f.get();
        });

    REQUIRE(x == 0);
    REQUIRE(y == 0);
    
    auto start = std::chrono::high_resolution_clock::now();
    do {
        using namespace std::chrono_literals;

        hi::loop::local().resume_once();

        if (std::chrono::high_resolution_clock::now() - start > 10s) {
            break;
        }
    } while (y == 0);

    REQUIRE(x == 42);
    REQUIRE(y == 43);
}

}; // TEST_SUITE(function_predicate_suite)
