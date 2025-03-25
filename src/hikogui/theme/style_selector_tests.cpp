// Copyright Take Vos 2024.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

#include "style_selector.hpp"
#include "style_path.hpp"
#include <hikotest/hikotest.hpp>

TEST_SUITE(style_selector_suite) {
    TEST_CASE(matches_descendant_combinator_test) {
        auto selector = hi::style_selector{{"foo"}, {"bar"}};
        selector[0].child_combinator = false;
        auto const path1 = hi::style_path{{"foo"}, {"bar"}};
        REQUIRE(hi::matches(selector, path1));

        auto const path2 = hi::style_path{{"foo"}, {"x"}, {"bar"}};
        REQUIRE(hi::matches(selector, path2));
    }

    TEST_CASE(matches_child_combinator_test) {
        auto selector = hi::style_selector{{"foo"}, {"bar"}};
        auto const path1 = hi::style_path{{"foo"}, {"bar"}};
        REQUIRE(hi::matches(selector, path1));

        auto const path2 = hi::style_path{{"foo"}, {"x"}, {"bar"}};
        REQUIRE(not hi::matches(selector, path2));
    }

    TEST_CASE(bug1_test) {
        auto selector = hi::style_selector{{"toggle"}};
        auto const path = hi::style_path{{"window"}, {"grid-view"}, {"tab-view"}, {"scroll-view"}, {"scroll-aperture"}, {"grid-view"}, {"with-label"}, {"checkbox"}};
        REQUIRE(not hi::matches(selector, path));
    }
};