// Copyright Take Vos 2023.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

#include "style_parser.hpp"
#include <hikotest/hikotest.hpp>

TEST_SUITE(style_parser_suite) {
    TEST_CASE(id_test) {
        auto result = hi::parse_style("#foo width=5");
        REQUIRE(result.has_value());
        auto [attributes, id, classes] = *result;
        REQUIRE(id == "foo");
        REQUIRE(std::holds_alternative<hi::unit::dips_f>(attributes.width()));
        REQUIRE(std::get<hi::unit::dips_f>(attributes.width()) == hi::unit::dips(5));
    }

    TEST_CASE(id_error_test) {
        auto result = hi::parse_style("#foo width=5 #bar");
        REQUIRE(result.has_error());
    }

    TEST_CASE(class_test) {
        auto result = hi::parse_style(".foo width=5 .bar");
        REQUIRE(result.has_value());
        auto [attributes, id, classes] = *result;
        REQUIRE(classes.size() == 2);
        REQUIRE(classes[0] == "foo");
        REQUIRE(classes[1] == "bar");
        REQUIRE(std::holds_alternative<hi::unit::dips_f>(attributes.width()));
        REQUIRE(std::get<hi::unit::dips_f>(attributes.width()) == hi::unit::dips(5));
    }

    TEST_CASE(length_dips_test) {
        auto result = hi::parse_style("width=5 height = 42");
        REQUIRE(result.has_value());
        auto [attributes, id, classes] = *result;
        REQUIRE(std::holds_alternative<hi::unit::dips_f>(attributes.width()));
        REQUIRE(std::holds_alternative<hi::unit::dips_f>(attributes.height()));
        REQUIRE(std::get<hi::unit::dips_f>(attributes.width()) == hi::unit::dips(5));
        REQUIRE(std::get<hi::unit::dips_f>(attributes.height()) == hi::unit::dips(42));
    }

    TEST_CASE(length_pixel_test) {
        auto result = hi::parse_style("width=5 px height = 42px");
        REQUIRE(result.has_value());
        auto [attributes, id, classes] = *result;
        REQUIRE(std::holds_alternative<hi::unit::pixels_f>(attributes.width()));
        REQUIRE(std::holds_alternative<hi::unit::pixels_f>(attributes.height()));
        REQUIRE(std::get<hi::unit::pixels_f>(attributes.width()) == hi::unit::pixels(5));
        REQUIRE(std::get<hi::unit::pixels_f>(attributes.height()) == hi::unit::pixels(42));
    }

    TEST_CASE(length_points_test) {
        auto result = hi::parse_style("width=5 pt height = 42pt");
        REQUIRE(result.has_value());
        auto [attributes, id, classes] = *result;
        REQUIRE(std::holds_alternative<hi::unit::points_f>(attributes.width()));
        REQUIRE(std::holds_alternative<hi::unit::points_f>(attributes.height()));
        REQUIRE(std::get<hi::unit::points_f>(attributes.width()) == hi::unit::points(5));
        REQUIRE(std::get<hi::unit::points_f>(attributes.height()) == hi::unit::points(42));
    }

    TEST_CASE(length_cm_in_test) {
        auto result = hi::parse_style("width=2.54 cm height = 2.0in");
        REQUIRE(result.has_value());
        auto [attributes, id, classes] = *result;
        REQUIRE(std::holds_alternative<hi::unit::points_f>(attributes.width()));
        REQUIRE(std::holds_alternative<hi::unit::points_f>(attributes.height()));
        REQUIRE(std::get<hi::unit::points_f>(attributes.width()) == hi::unit::points(72));
        REQUIRE(std::get<hi::unit::points_f>(attributes.height()) == hi::unit::points(144));
    }

    TEST_CASE(length_error_test) {
        auto result = hi::parse_style("width=a");
        REQUIRE(result.has_error());
    }

    TEST_CASE(color_named_test) {
        hi::named_color<"test1"> = hi::color{1.0, 2.0, 3.0, 0.5};
        hi::named_color<"test2"> = hi::color{2.0, 3.0, 4.0, 0.5};

        auto result = hi::parse_style("color=test1 background-color = 'test2'");
        REQUIRE(result.has_value());
        auto [attributes, id, classes] = *result;
        REQUIRE(attributes.color() == hi::color(1.0, 2.0, 3.0, 0.5));
        REQUIRE(attributes.background_color() == hi::color(2.0, 3.0, 4.0, 0.5));
    }

    TEST_CASE(color_rgb_test) {
        auto result = hi::parse_style("color=rgb(1.0, 2.0, 3.0) background-color = rgba(2.0,3, 4.0, 0.5)");
        REQUIRE(result.has_value());
        auto [attributes, id, classes] = *result;
        REQUIRE(attributes.color() == hi::color(1.0, 2.0, 3.0, 1.0));
        REQUIRE(attributes.background_color() == hi::color(2.0, 3.0, 4.0, 0.5));
    }

    TEST_CASE(color_hex_test) {
        auto result = hi::parse_style("color='#112233' background-color = '#22334455'");
        REQUIRE(result.has_value());
        auto [attributes, id, classes] = *result;
        REQUIRE(attributes.color() == hi::color_from_sRGB("#112233"));
        REQUIRE(attributes.background_color() == hi::color_from_sRGB("#22334455"));
    }

    TEST_CASE(color_error_test) {
        REQUIRE(hi::parse_style("color=a").has_error());
        REQUIRE(hi::parse_style("color=12").has_error());
        REQUIRE(hi::parse_style("color=12.3").has_error());
        REQUIRE(hi::parse_style("color=rgba(1.0, 2.0, 3.0)").has_error());
        REQUIRE(hi::parse_style("color=rgb(1.0, 2.0)").has_error());
    }

    TEST_CASE(alignment_test) {
        auto result = hi::parse_style("horizontal-alignment=right vertical-alignment = middle");
        REQUIRE(result.has_value());
        auto [attributes, id, classes] = *result;
        REQUIRE(attributes.horizontal_alignment() == hi::horizontal_alignment::right);
        REQUIRE(attributes.vertical_alignment() == hi::vertical_alignment::middle);
    }

};
