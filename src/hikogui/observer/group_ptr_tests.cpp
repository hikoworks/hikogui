// Copyright Take Vos 2022.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

#include "group_ptr.hpp"
#include <hikotest/hikotest.hpp>

hi_warning_push();
// C26414: Move, copy, reassign or reset a local smart pointer (r.5)
// We are testing a smart-pointer-like (group_ptr), so we hit this warning explicitly.
hi_warning_ignore_msvc(26414);

TEST_SUITE(group_ptr_suite) {

class A : public hi::enable_group_ptr<A> {
public:
    A(int value) noexcept : value(value) {}

    int value;
};

class B : public hi::enable_group_ptr<B, void(int)> {
public:
    B(int value) noexcept : value(value) {}

    int value;
};

class C : public hi::enable_group_ptr<C, void(int, int, int)> {
public:
    C(int value) noexcept : value(value) {}

    int value;
};

TEST_CASE(simple)
{
    auto a = hi::group_ptr<A>{};
    REQUIRE(not a);

    a = std::make_shared<A>(42);
    REQUIRE(static_cast<bool>(a));
    REQUIRE(a->value == 42);
}

TEST_CASE(chain)
{
    hi::group_ptr<A> a = std::make_shared<A>(1);
    hi::group_ptr<A> b = a;
    hi::group_ptr<A> c = b;
    REQUIRE(static_cast<bool>(a));
    REQUIRE(static_cast<bool>(b));
    REQUIRE(static_cast<bool>(c));
    REQUIRE(a->value == 1);
    REQUIRE(b->value == 1);
    REQUIRE(c->value == 1);
    REQUIRE(a.get() == b.get());
    REQUIRE(a.get() == c.get());
    auto* old_ptr = a.get();

    a = std::make_shared<A>(2);
    REQUIRE(static_cast<bool>(a));
    REQUIRE(static_cast<bool>(b));
    REQUIRE(static_cast<bool>(c));
    REQUIRE(a->value == 2);
    REQUIRE(b->value == 2);
    REQUIRE(c->value == 2);
    REQUIRE(a.get() != old_ptr);
    REQUIRE(a.get() == b.get());
    REQUIRE(a.get() == c.get());
}

TEST_CASE(no_chain)
{
    hi::group_ptr<A> a;
    hi::group_ptr<A> b = a;
    hi::group_ptr<A> c = b;
    REQUIRE(not a);
    REQUIRE(not b);
    REQUIRE(not c);

    a = std::make_shared<A>(2);
    REQUIRE(static_cast<bool>(a));
    REQUIRE(not b);
    REQUIRE(not c);
    REQUIRE(a->value == 2);
}

TEST_CASE(unlink_by_move)
{
    hi::group_ptr<A> a = std::make_shared<A>(1);
    hi::group_ptr<A> b = a;
    hi::group_ptr<A> c = a;
    REQUIRE(static_cast<bool>(a));
    REQUIRE(static_cast<bool>(b));
    REQUIRE(static_cast<bool>(c));
    REQUIRE(a->value == 1);
    REQUIRE(b->value == 1);
    REQUIRE(c->value == 1);

    a = std::move(b);
    REQUIRE(static_cast<bool>(a));
    REQUIRE(not b);
    REQUIRE(static_cast<bool>(c));
    REQUIRE(a->value == 1);
    REQUIRE(c->value == 1);

    c->value = 2;
    REQUIRE(static_cast<bool>(a));
    REQUIRE(not b);
    REQUIRE(static_cast<bool>(c));
    REQUIRE(a->value == 2);
    REQUIRE(c->value == 2);

    c = std::make_shared<A>(3);
    REQUIRE(static_cast<bool>(a));
    REQUIRE(not b);
    REQUIRE(static_cast<bool>(c));
    REQUIRE(a->value == 3);
    REQUIRE(c->value == 3);
}

TEST_CASE(unlink_by_reset)
{
    hi::group_ptr<A> a = std::make_shared<A>(1);
    hi::group_ptr<A> b = a;
    hi::group_ptr<A> c = a;
    REQUIRE(static_cast<bool>(a));
    REQUIRE(static_cast<bool>(b));
    REQUIRE(static_cast<bool>(c));
    REQUIRE(a->value == 1);
    REQUIRE(b->value == 1);
    REQUIRE(c->value == 1);

    a.reset();
    REQUIRE(not a);
    REQUIRE(static_cast<bool>(b));
    REQUIRE(static_cast<bool>(c));
    REQUIRE(b->value == 1);
    REQUIRE(c->value == 1);

    c->value = 2;
    REQUIRE(not a);
    REQUIRE(static_cast<bool>(b));
    REQUIRE(static_cast<bool>(c));
    REQUIRE(b->value == 2);
    REQUIRE(c->value == 2);

    c = std::make_shared<A>(3);
    REQUIRE(not a);
    REQUIRE(static_cast<bool>(b));
    REQUIRE(static_cast<bool>(c));
    REQUIRE(b->value == 3);
    REQUIRE(c->value == 3);
}

TEST_CASE(unlink_by_empty_shared_ptr)
{
    hi::group_ptr<A> a = std::make_shared<A>(1);
    hi::group_ptr<A> b = a;
    hi::group_ptr<A> c = a;
    REQUIRE(static_cast<bool>(a));
    REQUIRE(static_cast<bool>(b));
    REQUIRE(static_cast<bool>(c));
    REQUIRE(a->value == 1);
    REQUIRE(b->value == 1);
    REQUIRE(c->value == 1);

    std::shared_ptr<A> d;
    a = d;
    REQUIRE(not a);
    REQUIRE(static_cast<bool>(b));
    REQUIRE(static_cast<bool>(c));
    REQUIRE(b->value == 1);
    REQUIRE(c->value == 1);

    c->value = 2;
    REQUIRE(not a);
    REQUIRE(static_cast<bool>(b));
    REQUIRE(static_cast<bool>(c));
    REQUIRE(b->value == 2);
    REQUIRE(c->value == 2);

    c = std::make_shared<A>(3);
    REQUIRE(not a);
    REQUIRE(static_cast<bool>(b));
    REQUIRE(static_cast<bool>(c));
    REQUIRE(b->value == 3);
    REQUIRE(c->value == 3);
}

TEST_CASE(unlink_by_empty_group_ptr)
{
    hi::group_ptr<A> a = std::make_shared<A>(1);
    hi::group_ptr<A> b = a;
    hi::group_ptr<A> c = a;
    REQUIRE(static_cast<bool>(a));
    REQUIRE(static_cast<bool>(b));
    REQUIRE(static_cast<bool>(c));
    REQUIRE(a->value == 1);
    REQUIRE(b->value == 1);
    REQUIRE(c->value == 1);

    hi::group_ptr<A> d;
    a = d;
    REQUIRE(not a);
    REQUIRE(static_cast<bool>(b));
    REQUIRE(static_cast<bool>(c));
    REQUIRE(b->value == 1);
    REQUIRE(c->value == 1);

    c->value = 2;
    REQUIRE(not a);
    REQUIRE(static_cast<bool>(b));
    REQUIRE(static_cast<bool>(c));
    REQUIRE(b->value == 2);
    REQUIRE(c->value == 2);

    c = std::make_shared<A>(3);
    REQUIRE(not a);
    REQUIRE(static_cast<bool>(b));
    REQUIRE(static_cast<bool>(c));
    REQUIRE(b->value == 3);
    REQUIRE(c->value == 3);
}

TEST_CASE(unlink_by_nullptr)
{
    hi::group_ptr<A> a = std::make_shared<A>(1);
    hi::group_ptr<A> b = a;
    hi::group_ptr<A> c = a;
    REQUIRE(static_cast<bool>(a));
    REQUIRE(static_cast<bool>(b));
    REQUIRE(static_cast<bool>(c));
    REQUIRE(a->value == 1);
    REQUIRE(b->value == 1);
    REQUIRE(c->value == 1);

    a = nullptr;
    REQUIRE(not a);
    REQUIRE(static_cast<bool>(b));
    REQUIRE(static_cast<bool>(c));
    REQUIRE(b->value == 1);
    REQUIRE(c->value == 1);

    c->value = 2;
    REQUIRE(not a);
    REQUIRE(static_cast<bool>(b));
    REQUIRE(static_cast<bool>(c));
    REQUIRE(b->value == 2);
    REQUIRE(c->value == 2);

    c = std::make_shared<A>(3);
    REQUIRE(not a);
    REQUIRE(static_cast<bool>(b));
    REQUIRE(static_cast<bool>(c));
    REQUIRE(b->value == 3);
    REQUIRE(c->value == 3);
}

TEST_CASE(notify_no_arg)
{
    hi::group_ptr<A> a;
    hi::group_ptr<A> b;
    hi::group_ptr<A> c;

    int a_count = 0;
    int b_count = 0;
    int c_count = 0;

    a.subscribe([&]() {
        ++a_count;
    });
    b.subscribe([&]() {
        ++b_count;
    });
    c.subscribe([&]() {
        ++c_count;
    });

    a = std::make_shared<A>(1);
    b = a;
    c = b;
    REQUIRE(static_cast<bool>(a));
    REQUIRE(static_cast<bool>(b));
    REQUIRE(static_cast<bool>(c));
    REQUIRE(a->value == 1);
    REQUIRE(b->value == 1);
    REQUIRE(c->value == 1);
    REQUIRE(a.get() == b.get());
    REQUIRE(a.get() == c.get());

    REQUIRE(a_count == 0);
    REQUIRE(b_count == 0);
    REQUIRE(c_count == 0);
    a->notify_group_ptr();
    REQUIRE(a_count == 1);
    REQUIRE(b_count == 1);
    REQUIRE(c_count == 1);
}

TEST_CASE(notify_one_arg)
{
    hi::group_ptr<B> a;
    hi::group_ptr<B> b;
    hi::group_ptr<B> c;

    int a_count = 0;
    int b_count = 0;
    int c_count = 0;

    a.subscribe([&](int x) {
        a_count += x;
    });
    b.subscribe([&](int x) {
        b_count += x;
    });
    c.subscribe([&](int x) {
        c_count += x;
    });

    a = std::make_shared<B>(1);
    b = a;
    c = b;
    REQUIRE(static_cast<bool>(a));
    REQUIRE(static_cast<bool>(b));
    REQUIRE(static_cast<bool>(c));
    REQUIRE(a->value == 1);
    REQUIRE(b->value == 1);
    REQUIRE(c->value == 1);
    REQUIRE(a.get() == b.get());
    REQUIRE(a.get() == c.get());

    REQUIRE(a_count == 0);
    REQUIRE(b_count == 0);
    REQUIRE(c_count == 0);
    a->notify_group_ptr(2);
    REQUIRE(a_count == 2);
    REQUIRE(b_count == 2);
    REQUIRE(c_count == 2);
}

TEST_CASE(notify_three_args)
{
    hi::group_ptr<C> a;
    hi::group_ptr<C> b;
    hi::group_ptr<C> c;

    int a_count = 0;
    int b_count = 0;
    int c_count = 0;

    a.subscribe([&](int x, int y, int z) {
        a_count += x;
    });
    b.subscribe([&](int x, int y, int z) {
        b_count += y;
    });
    c.subscribe([&](int x, int y, int z) {
        c_count += z;
    });

    a = std::make_shared<C>(1);
    b = a;
    c = b;
    REQUIRE(static_cast<bool>(a));
    REQUIRE(static_cast<bool>(b));
    REQUIRE(static_cast<bool>(c));
    REQUIRE(a->value == 1);
    REQUIRE(b->value == 1);
    REQUIRE(c->value == 1);
    REQUIRE(a.get() == b.get());
    REQUIRE(a.get() == c.get());

    REQUIRE(a_count == 0);
    REQUIRE(b_count == 0);
    REQUIRE(c_count == 0);
    a->notify_group_ptr(2, 3, 4);
    REQUIRE(a_count == 2);
    REQUIRE(b_count == 3);
    REQUIRE(c_count == 4);
}
}; // TEST_SUITE(group_ptr_suite)

hi_warning_pop();
