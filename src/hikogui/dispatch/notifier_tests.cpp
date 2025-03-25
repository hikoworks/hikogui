// Copyright Take Vos 2022.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

#include "notifier.hpp"
#include "task.hpp"
#include "loop_win32_intf.hpp"
#include <hikotest/hikotest.hpp>
#include <coroutine>

TEST_SUITE(notifier) {

TEST_CASE(local)
{
    auto a = 0;
    auto b = 0;

    auto n = hi::notifier{};

    auto a_cbt = n.subscribe(
        [&] {
            ++a;
        },
        hi::callback_flags::local);

    auto b_cbt = n.subscribe(
        [&] {
            ++b;
        },
        hi::callback_flags::local);

    // Post the functions to the local event-loop.
    // The two functions are not called immediately, not until the event-loop is resumed.
    n();
    REQUIRE(a == 0);
    REQUIRE(b == 0);

    hi::loop::local().resume_once();
    REQUIRE(a == 1);
    REQUIRE(b == 1);
}

TEST_CASE(local_unsubscribe)
{
    auto a = 0;
    auto b = 0;

    auto n = hi::notifier{};

    auto a_cbt = n.subscribe(
        [&] {
            ++a;
        },
        hi::callback_flags::local);

    auto b_cbt = n.subscribe(
        [&] {
            ++b;
        },
        hi::callback_flags::local);

    // Unsubscribe from a.
    a_cbt = {};

    // Post the callbacks to the local event-loop.
    n();
    REQUIRE(a == 0);
    REQUIRE(b == 0);

    hi::loop::local().resume_once();

    REQUIRE(a == 0);
    REQUIRE(b == 1);
}

static hi::scoped_task<> local_coroutine_func(int& a, int& b, hi::notifier<>& n)
{
    ++a;
    co_await n;
    ++b;
}

TEST_CASE(local_coroutine)
{
    auto a = 0;
    auto b = 0;

    auto n = hi::notifier{};

    REQUIRE(a == 0);
    REQUIRE(b == 0);

    // Start a coroutine, the first part of the coroutine is immediately executed.
    auto cr = local_coroutine_func(a, b, n);
    REQUIRE(a == 1);
    REQUIRE(b == 0);

    // Post the callbacks to the local event-loop.
    // The coroutine will not continue until the event loop is resumed.
    n();
    REQUIRE(a == 1);
    REQUIRE(b == 0);
    REQUIRE(not cr.done());

    hi::loop::local().resume_once();

    // Now the coroutine has continued and completed.
    REQUIRE(a == 1);
    REQUIRE(b == 1);
    REQUIRE(cr.done());
}

};