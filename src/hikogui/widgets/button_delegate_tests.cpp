// Copyright Take Vos 2024.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

#include "button_delegate.hpp"
#include <hikotest/hikotest.hpp>
#include <atomic>

TEST_SUITE(button_delegate_suite) {

static void my_function(int* x)
{
    using namespace std::literals;

    std::this_thread::sleep_for(100ms);
    std::atomic_ref(*x).store(42);
}

static void my_stoppable_function(std::stop_token token, int* x)
{
    using namespace std::literals;

    do {
        std::this_thread::sleep_for(100ms);
        std::atomic_ref(*x).fetch_add(1);
    } while (not token.stop_requested());
}

static hi::task<void> my_task(int* x)
{
    using namespace std::literals;

    co_await 100ms;

    std::atomic_ref(*x).store(42);
}

static hi::task<void> my_stoppable_task(std::stop_token token, int* x)
{
    using namespace std::literals;

    do {
        co_await 100ms;
        std::atomic_ref(*x).fetch_add(1);
    } while (not token.stop_requested());

    co_return;
}

TEST_CASE(function_test)
{
    int x = 0;

    auto delegate = hi::make_shared_ctad<hi::default_button_delegate>(my_function, &x);

    REQUIRE(x == 0);
    REQUIRE(delegate->state(nullptr) == hi::widget_value::off);

    delegate->activate(nullptr);
    REQUIRE(delegate->state(nullptr) == hi::widget_value::on);

    auto start = std::chrono::high_resolution_clock::now();
    do {
        using namespace std::chrono_literals;

        hi::loop::local().resume_once();

        if (std::chrono::high_resolution_clock::now() - start > 10s) {
            break;
        }
    } while (delegate->state(nullptr) != hi::widget_value::off);

    REQUIRE(delegate->state(nullptr) == hi::widget_value::off);
    REQUIRE(x == 42);
}

TEST_CASE(task_test)
{
    int x = 0;

    auto delegate = hi::make_shared_ctad<hi::default_button_delegate>(my_task, &x);

    REQUIRE(x == 0);
    REQUIRE(delegate->state(nullptr) == hi::widget_value::off);

    delegate->activate(nullptr);
    REQUIRE(delegate->state(nullptr) == hi::widget_value::on);

    auto start = std::chrono::high_resolution_clock::now();
    do {
        using namespace std::chrono_literals;

        hi::loop::local().resume_once();

        if (std::chrono::high_resolution_clock::now() - start > 10s) {
            break;
        }
    } while (delegate->state(nullptr) != hi::widget_value::off);

    REQUIRE(delegate->state(nullptr) == hi::widget_value::off);
    REQUIRE(x == 42);
}

TEST_CASE(stoppable_function_test)
{
    int x = 0;

    auto delegate = hi::make_shared_ctad<hi::default_button_delegate>(my_stoppable_function, &x);

    REQUIRE(x == 0);
    REQUIRE(delegate->state(nullptr) == hi::widget_value::off);

    delegate->activate(nullptr);
    REQUIRE(delegate->state(nullptr) == hi::widget_value::on);

    auto start = std::chrono::high_resolution_clock::now();
    do {
        using namespace std::chrono_literals;

        hi::loop::local().resume_once();

        if (std::chrono::high_resolution_clock::now() - start > 200ms and delegate->state(nullptr) == hi::widget_value::on) {
            // Cancel the task. This is only called once, due to the state being
            // either 'off' or 'other' after the cancellation.
            delegate->activate(nullptr);
        }

        if (std::chrono::high_resolution_clock::now() - start > 10s) {
            break;
        }
    } while (delegate->state(nullptr) != hi::widget_value::off);

    REQUIRE(delegate->state(nullptr) == hi::widget_value::off);
    REQUIRE(x > 0);
    REQUIRE(x < 80);
}

TEST_CASE(stoppable_task_test)
{
    int x = 0;

    auto delegate = hi::make_shared_ctad<hi::default_button_delegate>(my_stoppable_task, &x);

    REQUIRE(x == 0);
    REQUIRE(delegate->state(nullptr) == hi::widget_value::off);

    delegate->activate(nullptr);
    REQUIRE(delegate->state(nullptr) == hi::widget_value::on);

    auto start = std::chrono::high_resolution_clock::now();
    do {
        using namespace std::chrono_literals;

        hi::loop::local().resume_once();

        if (std::chrono::high_resolution_clock::now() - start > 200ms and delegate->state(nullptr) == hi::widget_value::on) {
            // Cancel the task. This is only called once, due to the state being
            // either 'off' or 'other' after the cancellation.
            delegate->activate(nullptr);
        }

        if (std::chrono::high_resolution_clock::now() - start > 10s) {
            break;
        }
    } while (delegate->state(nullptr) != hi::widget_value::off);

    REQUIRE(delegate->state(nullptr) == hi::widget_value::off);
    REQUIRE(x > 0);
    REQUIRE(x < 80);
}

}; // TEST_SUITE(button_delegate_suite)
