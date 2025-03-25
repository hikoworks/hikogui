// Copyright Take Vos 2019.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

#include "strings.hpp"
#include <hikotest/hikotest.hpp>

TEST_SUITE(strings_suite)
{

TEST_CASE(split_test1)
{
    auto const result = hi::split_string("path1/path2", "/");
    auto const check_value = std::vector<std::string>{"path1", "path2"};
    REQUIRE(result == check_value);
}

TEST_CASE(split_test2)
{
    auto const result = hi::split_string("path1/path2/", "/");
    auto const check_value = std::vector<std::string>{"path1", "path2", ""};
    REQUIRE(result == check_value);
}

TEST_CASE(split_test3)
{
    auto const result = hi::split_string("path1/path2/", "/", "p");
    auto const check_value = std::vector<std::string>{"", "ath1", "", "ath2", ""};
    REQUIRE(result == check_value);
}

TEST_CASE(split_test4)
{
    auto const result = hi::split_string("", "/");
    auto const check_value = std::vector<std::string>{""};
    REQUIRE(result == check_value);
}

TEST_CASE(split_test5)
{
    auto const result = hi::split_string("/", "/");
    auto const check_value = std::vector<std::string>{"", ""};
    REQUIRE(result == check_value);
}

TEST_CASE(normalize_lf_test)
{
    REQUIRE(hi::normalize_lf("hello\nworld\n\nFoo\n") == "hello\nworld\n\nFoo\n");
    REQUIRE(hi::normalize_lf("hello\rworld\r\rFoo\r") == "hello\nworld\n\nFoo\n");
    REQUIRE(hi::normalize_lf("hello\r\nworld\r\n\r\nFoo\r\n") == "hello\nworld\n\nFoo\n");
}


};
