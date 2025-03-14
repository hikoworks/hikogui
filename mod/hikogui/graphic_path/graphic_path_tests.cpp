// Copyright Take Vos 2019, 2021.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

#include "graphic_path.hpp"
#include "bezier_curve.hpp"
#include "../macros.hpp"
#include <gtest/gtest.h>
#include <iostream>
#include <string>

using namespace std;
using namespace hi;

TEST(graphic_path, getBeziersOfLayer)
{
    auto path = graphic_path();
    path.moveTo(point2{1, 1});
    path.lineTo(point2{2, 1});
    path.lineTo(point2{2, 2});
    path.lineTo(point2{1, 2});
    path.closeContour();

    hilet beziers = path.getBeziers();
    ASSERT_EQ(beziers.size(), 4);
    ASSERT_EQ(beziers[0], bezier_curve(point2(1, 1), point2(2, 1)));
    ASSERT_EQ(beziers[1], bezier_curve(point2(2, 1), point2(2, 2)));
    ASSERT_EQ(beziers[2], bezier_curve(point2(2, 2), point2(1, 2)));
    ASSERT_EQ(beziers[3], bezier_curve(point2(1, 2), point2(1, 1)));
}

TEST(graphic_path, getbezier_pointsOfContour)
{
    auto path = graphic_path();
    path.moveTo(point2{1, 1});
    path.lineTo(point2{2, 1});
    path.lineTo(point2{2, 2});
    path.lineTo(point2{1, 2});
    path.closeContour();

    hilet points = path.getbezier_pointsOfContour(0);
    ASSERT_EQ(points.size(), 4);
    ASSERT_EQ(points[0], bezier_point(point2(1, 1), bezier_point::Type::Anchor));
    ASSERT_EQ(points[1], bezier_point(point2(2, 1), bezier_point::Type::Anchor));
    ASSERT_EQ(points[2], bezier_point(point2(2, 2), bezier_point::Type::Anchor));
    ASSERT_EQ(points[3], bezier_point(point2(1, 2), bezier_point::Type::Anchor));
}
