// Copyright Take Vos 2022.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

#include "group_ptr.hpp"
#include "../macros.hpp"
#include <gtest/gtest.h>

hi_warning_push();
// C26414: Move, copy, reassign or reset a local smart pointer (r.5)
// We are testing a smart-pointer-like (group_ptr), so we hit this warning explicitly.
hi_warning_ignore_msvc(26414)

namespace test_group_ptr {

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

}

TEST(group_ptr, simple)
{
    using namespace test_group_ptr;

    auto a = hi::group_ptr<A>{};
    ASSERT_FALSE(a);

    a = std::make_shared<A>(42);
    ASSERT_TRUE(a);
    ASSERT_EQ(a->value, 42);
}

TEST(group_ptr, chain)
{
    using namespace test_group_ptr;

    hi::group_ptr<A> a = std::make_shared<A>(1);
    hi::group_ptr<A> b = a;
    hi::group_ptr<A> c = b;
    ASSERT_TRUE(a);
    ASSERT_TRUE(b);
    ASSERT_TRUE(c);
    ASSERT_EQ(a->value, 1);
    ASSERT_EQ(b->value, 1);
    ASSERT_EQ(c->value, 1);
    ASSERT_EQ(a.get(), b.get());
    ASSERT_EQ(a.get(), c.get());
    auto *old_ptr = a.get();

    a = std::make_shared<A>(2);
    ASSERT_TRUE(a);
    ASSERT_TRUE(b);
    ASSERT_TRUE(c);
    ASSERT_EQ(a->value, 2);
    ASSERT_EQ(b->value, 2);
    ASSERT_EQ(c->value, 2);
    ASSERT_NE(a.get(), old_ptr);
    ASSERT_EQ(a.get(), b.get());
    ASSERT_EQ(a.get(), c.get());
}

TEST(group_ptr, no_chain)
{
    using namespace test_group_ptr;

    hi::group_ptr<A> a;
    hi::group_ptr<A> b = a;
    hi::group_ptr<A> c = b;
    ASSERT_FALSE(a);
    ASSERT_FALSE(b);
    ASSERT_FALSE(c);

    a = std::make_shared<A>(2);
    ASSERT_TRUE(a);
    ASSERT_FALSE(b);
    ASSERT_FALSE(c);
    ASSERT_EQ(a->value, 2);
}

TEST(group_ptr, unlink_by_move)
{
    using namespace test_group_ptr;

    hi::group_ptr<A> a = std::make_shared<A>(1);
    hi::group_ptr<A> b = a;
    hi::group_ptr<A> c = a;
    ASSERT_TRUE(a);
    ASSERT_TRUE(b);
    ASSERT_TRUE(c);
    ASSERT_EQ(a->value, 1);
    ASSERT_EQ(b->value, 1);
    ASSERT_EQ(c->value, 1);

    a = std::move(b);
    ASSERT_TRUE(a);
    ASSERT_FALSE(b);
    ASSERT_TRUE(c);
    ASSERT_EQ(a->value, 1);
    ASSERT_EQ(c->value, 1);

    c->value = 2;
    ASSERT_TRUE(a);
    ASSERT_FALSE(b);
    ASSERT_TRUE(c);
    ASSERT_EQ(a->value, 2);
    ASSERT_EQ(c->value, 2);

    c = std::make_shared<A>(3);
    ASSERT_TRUE(a);
    ASSERT_FALSE(b);
    ASSERT_TRUE(c);
    ASSERT_EQ(a->value, 3);
    ASSERT_EQ(c->value, 3);
}

TEST(group_ptr, unlink_by_reset)
{
    using namespace test_group_ptr;

    hi::group_ptr<A> a = std::make_shared<A>(1);
    hi::group_ptr<A> b = a;
    hi::group_ptr<A> c = a;
    ASSERT_TRUE(a);
    ASSERT_TRUE(b);
    ASSERT_TRUE(c);
    ASSERT_EQ(a->value, 1);
    ASSERT_EQ(b->value, 1);
    ASSERT_EQ(c->value, 1);

    a.reset();
    ASSERT_FALSE(a);
    ASSERT_TRUE(b);
    ASSERT_TRUE(c);
    ASSERT_EQ(b->value, 1);
    ASSERT_EQ(c->value, 1);

    c->value = 2;
    ASSERT_FALSE(a);
    ASSERT_TRUE(b);
    ASSERT_TRUE(c);
    ASSERT_EQ(b->value, 2);
    ASSERT_EQ(c->value, 2);

    c = std::make_shared<A>(3);
    ASSERT_FALSE(a);
    ASSERT_TRUE(b);
    ASSERT_TRUE(c);
    ASSERT_EQ(b->value, 3);
    ASSERT_EQ(c->value, 3);
}

TEST(group_ptr, unlink_by_empty_shared_ptr)
{
    using namespace test_group_ptr;

    hi::group_ptr<A> a = std::make_shared<A>(1);
    hi::group_ptr<A> b = a;
    hi::group_ptr<A> c = a;
    ASSERT_TRUE(a);
    ASSERT_TRUE(b);
    ASSERT_TRUE(c);
    ASSERT_EQ(a->value, 1);
    ASSERT_EQ(b->value, 1);
    ASSERT_EQ(c->value, 1);

    std::shared_ptr<A> d;
    a = d;
    ASSERT_FALSE(a);
    ASSERT_TRUE(b);
    ASSERT_TRUE(c);
    ASSERT_EQ(b->value, 1);
    ASSERT_EQ(c->value, 1);

    c->value = 2;
    ASSERT_FALSE(a);
    ASSERT_TRUE(b);
    ASSERT_TRUE(c);
    ASSERT_EQ(b->value, 2);
    ASSERT_EQ(c->value, 2);

    c = std::make_shared<A>(3);
    ASSERT_FALSE(a);
    ASSERT_TRUE(b);
    ASSERT_TRUE(c);
    ASSERT_EQ(b->value, 3);
    ASSERT_EQ(c->value, 3);
}

TEST(group_ptr, unlink_by_empty_group_ptr)
{
    using namespace test_group_ptr;

    hi::group_ptr<A> a = std::make_shared<A>(1);
    hi::group_ptr<A> b = a;
    hi::group_ptr<A> c = a;
    ASSERT_TRUE(a);
    ASSERT_TRUE(b);
    ASSERT_TRUE(c);
    ASSERT_EQ(a->value, 1);
    ASSERT_EQ(b->value, 1);
    ASSERT_EQ(c->value, 1);

    hi::group_ptr<A> d;
    a = d;
    ASSERT_FALSE(a);
    ASSERT_TRUE(b);
    ASSERT_TRUE(c);
    ASSERT_EQ(b->value, 1);
    ASSERT_EQ(c->value, 1);

    c->value = 2;
    ASSERT_FALSE(a);
    ASSERT_TRUE(b);
    ASSERT_TRUE(c);
    ASSERT_EQ(b->value, 2);
    ASSERT_EQ(c->value, 2);

    c = std::make_shared<A>(3);
    ASSERT_FALSE(a);
    ASSERT_TRUE(b);
    ASSERT_TRUE(c);
    ASSERT_EQ(b->value, 3);
    ASSERT_EQ(c->value, 3);
}

TEST(group_ptr, unlink_by_nullptr)
{
    using namespace test_group_ptr;

    hi::group_ptr<A> a = std::make_shared<A>(1);
    hi::group_ptr<A> b = a;
    hi::group_ptr<A> c = a;
    ASSERT_TRUE(a);
    ASSERT_TRUE(b);
    ASSERT_TRUE(c);
    ASSERT_EQ(a->value, 1);
    ASSERT_EQ(b->value, 1);
    ASSERT_EQ(c->value, 1);

    a = nullptr;
    ASSERT_FALSE(a);
    ASSERT_TRUE(b);
    ASSERT_TRUE(c);
    ASSERT_EQ(b->value, 1);
    ASSERT_EQ(c->value, 1);

    c->value = 2;
    ASSERT_FALSE(a);
    ASSERT_TRUE(b);
    ASSERT_TRUE(c);
    ASSERT_EQ(b->value, 2);
    ASSERT_EQ(c->value, 2);

    c = std::make_shared<A>(3);
    ASSERT_FALSE(a);
    ASSERT_TRUE(b);
    ASSERT_TRUE(c);
    ASSERT_EQ(b->value, 3);
    ASSERT_EQ(c->value, 3);
}

TEST(group_ptr, notify_no_arg)
{
    using namespace test_group_ptr;

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
    ASSERT_TRUE(a);
    ASSERT_TRUE(b);
    ASSERT_TRUE(c);
    ASSERT_EQ(a->value, 1);
    ASSERT_EQ(b->value, 1);
    ASSERT_EQ(c->value, 1);
    ASSERT_EQ(a.get(), b.get());
    ASSERT_EQ(a.get(), c.get());

    ASSERT_EQ(a_count, 0);
    ASSERT_EQ(b_count, 0);
    ASSERT_EQ(c_count, 0);
    a->notify_group_ptr();
    ASSERT_EQ(a_count, 1);
    ASSERT_EQ(b_count, 1);
    ASSERT_EQ(c_count, 1);
}

TEST(group_ptr, notify_one_arg)
{
    using namespace test_group_ptr;

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
    ASSERT_TRUE(a);
    ASSERT_TRUE(b);
    ASSERT_TRUE(c);
    ASSERT_EQ(a->value, 1);
    ASSERT_EQ(b->value, 1);
    ASSERT_EQ(c->value, 1);
    ASSERT_EQ(a.get(), b.get());
    ASSERT_EQ(a.get(), c.get());

    ASSERT_EQ(a_count, 0);
    ASSERT_EQ(b_count, 0);
    ASSERT_EQ(c_count, 0);
    a->notify_group_ptr(2);
    ASSERT_EQ(a_count, 2);
    ASSERT_EQ(b_count, 2);
    ASSERT_EQ(c_count, 2);
}

TEST(group_ptr, notify_three_args)
{
    using namespace test_group_ptr;

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
    ASSERT_TRUE(a);
    ASSERT_TRUE(b);
    ASSERT_TRUE(c);
    ASSERT_EQ(a->value, 1);
    ASSERT_EQ(b->value, 1);
    ASSERT_EQ(c->value, 1);
    ASSERT_EQ(a.get(), b.get());
    ASSERT_EQ(a.get(), c.get());

    ASSERT_EQ(a_count, 0);
    ASSERT_EQ(b_count, 0);
    ASSERT_EQ(c_count, 0);
    a->notify_group_ptr(2, 3, 4);
    ASSERT_EQ(a_count, 2);
    ASSERT_EQ(b_count, 3);
    ASSERT_EQ(c_count, 4);
}

hi_warning_pop();
