// Copyright Take Vos 2021.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

#include "bezier_curve.hpp"
#include "../macros.hpp"
#include <gtest/gtest.h>
#include <iostream>
#include <string>

using namespace std;
using namespace hi;

namespace bezier_curve_tests {

template<typename T, typename U>
double maxAbsDiff(hi::lean_vector<T> const &lhs, hi::lean_vector<U> const &rhs)
{
    if (lhs.size() != rhs.size()) {
        return std::numeric_limits<double>::infinity();
    }

    double max_diff = 0.0;
    for (hilet lhs_value: lhs) {
        // Compare with the closest value in rhs.
        double min_diff = std::numeric_limits<double>::infinity();
        for (hilet rhs_value: rhs) {
            hi::inplace_min(min_diff, std::abs(lhs_value - rhs_value));
        }

        hi::inplace_max(max_diff, min_diff);
    }
    return max_diff;
}

template<typename T, typename U>
testing::AssertionResult ResultsNearPredFormat(
    const char *expr1,
    const char *expr2,
    const char *abs_error_expr,
    hi::lean_vector<T> val1,
    hi::lean_vector<U> val2,
    double abs_error)
{
    hilet diff = maxAbsDiff(val1, val2);
    if (diff <= abs_error)
        return testing::AssertionSuccess();

    auto val1_str = std::string{""};
    for (auto x: val1) {
        val1_str += val1_str.empty() ? "(" : ", ";
        val1_str += std::format("{}", x);
    }
    val1_str += ")";

    auto val2_str = std::string{""};
    for (auto x: val2_str) {
        val2_str += val2_str.empty() ? "(" : ", ";
        val2_str += std::format("{}", x);
    }
    val2_str += ")";


    return testing::AssertionFailure() << "The difference between " << expr1 << " and " << expr2 << " is " << diff
                                       << ", which exceeds " << abs_error_expr << ", where\n"
                                       << expr1 << " evaluates to " << val1_str << ",\n"
                                       << expr2 << " evaluates to " << val2_str << ", and\n"
                                       << abs_error_expr << " evaluates to " << abs_error << ".";
}
}

#define ASSERT_RESULTS_NEAR(val1, val2, abs_error) ASSERT_PRED_FORMAT3(bezier_curve_tests::ResultsNearPredFormat, val1, val2, abs_error)

#define ASSERT_RESULTS(val1, val2) ASSERT_RESULTS_NEAR(val1, val2, 0.000001)

TEST(bezier_curve, solve_x_by_y)
{
    ASSERT_RESULTS(bezier_curve(point2(1.0f, 1.0f), point2(1.5f, 1.0f), point2(2.0f, 1.0f)).solveXByY(1.5f), make_lean_vector<double>());
    ASSERT_RESULTS(bezier_curve(point2(2.0f, 1.0f), point2(2.0f, 1.5f), point2(2.0f, 2.0f)).solveXByY(1.5f), make_lean_vector<double>(2.0f));
    ASSERT_RESULTS(bezier_curve(point2(2.0f, 2.0f), point2(1.5f, 2.0f), point2(1.0f, 2.0f)).solveXByY(1.5f), make_lean_vector<double>());
    ASSERT_RESULTS(bezier_curve(point2(1.0f, 2.0f), point2(1.0f, 1.5f), point2(1.0f, 1.0f)).solveXByY(1.5f), make_lean_vector<double>(1.0f));
}
