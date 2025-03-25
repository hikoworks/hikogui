// Copyright Take Vos 2020-2021.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

#include "algorithm_misc.hpp"
#include <hikotest/hikotest.hpp>

TEST_SUITE(algorithm_suite)
{

TEST_CASE(shuffle_by_index1)
{
    auto items = std::string{"abcde"};
    auto const indices = std::vector<int>{4, 3, 2, 1, 0};

    auto last = hi::shuffle_by_index(begin(items), end(items), begin(indices), end(indices));
    REQUIRE(last == end(items));
    REQUIRE(items == "edcba");
}

TEST_CASE(shuffle_by_index2)
{
    auto items = std::string{"abcde"};
    auto const indices = std::vector<int>{4, 3, 2};

    auto last = hi::shuffle_by_index(begin(items), end(items), begin(indices), end(indices));
    items.erase(last, end(items));

    REQUIRE(last == end(items));
    REQUIRE(items == "edc");
}

TEST_CASE(shuffle_by_index3)
{
    auto items = std::string{"abcde"};
    auto const indices = std::vector<int>{0, 1, 3, 2, 4};

    auto last = hi::shuffle_by_index(begin(items), end(items), begin(indices), end(indices));
    REQUIRE(last == end(items));
    REQUIRE(items == "abdce");
}

TEST_CASE(shuffle_by_index4)
{
    auto items = std::vector<int>{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13};
    auto const indices = std::vector<int>{12, 13, 11, 10, 9, 7, 8, 6, 5, 4, 3, 2, 1, 0};

    auto last = hi::shuffle_by_index(begin(items), end(items), begin(indices), end(indices));
    REQUIRE(last == end(items));
    REQUIRE(items == indices);
}

TEST_CASE(remove_transform_if_test)
{
    auto input = std::vector<int>{1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    auto output = std::vector<int>{};
    auto const expected_output = std::vector<int>{43, 45, 47, 49, 51};
    auto const expected_input = std::vector<int>{2, 4, 6, 8, 10};

    auto const it = hi::v1::remove_transform_if(
        input.begin(), input.end(), std::back_inserter(output),
        [](int i) -> std::optional<int> {
            if (i % 2 == 0) {
                return std::nullopt;
            } else {
                return i + 42;
            }
        });
    input.erase(it, input.end());

    REQUIRE(output == expected_output);
    REQUIRE(input == expected_input);
}

};
