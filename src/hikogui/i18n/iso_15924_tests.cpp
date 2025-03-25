// Copyright Take Vos 2022.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

#include "iso_15924.hpp"
#include <hikotest/hikotest.hpp>

TEST_SUITE(iso_15924_suite)
{

TEST_CASE(from_code4_test)
{
    REQUIRE(hi::iso_15924{"Latn"}.number() == 215);
    REQUIRE(hi::iso_15924{"LATN"}.number() == 215);
    REQUIRE(hi::iso_15924{"latn"}.number() == 215);

    REQUIRE(hi::iso_15924{"Yiii"}.number() == 460);
    REQUIRE(hi::iso_15924{"YIII"}.number() == 460);
    REQUIRE(hi::iso_15924{"yiii"}.number() == 460);

    REQUIRE_THROWS(hi::iso_15924{"yi  "}, hi::parse_error);
    REQUIRE_THROWS(hi::iso_15924{"Foob"}, hi::parse_error);
}

TEST_CASE(to_code4_test)
{
    REQUIRE(hi::iso_15924{215}.code4() == "Latn");
    REQUIRE(hi::iso_15924{460}.code4() == "Yiii");
}

TEST_CASE(to_code4_open_type_test)
{
    REQUIRE(hi::iso_15924{215}.code4_open_type() == hi::fourcc<"latn">());
    REQUIRE(hi::iso_15924{460}.code4_open_type() == hi::fourcc<"yi  ">());
}

};
