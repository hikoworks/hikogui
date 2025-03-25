// Copyright Take Vos 2019, 2021-2022.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

#include "URL.hpp"
#include "glob.hpp"
#include "../algorithm/algorithm.hpp"
#include <hikotest/hikotest.hpp>

TEST_SUITE(URL_suite) {

TEST_CASE(parsing)
{
    auto const a = hi::URL("scheme://user:password@hostname:1234/path1/path2?query#fragment");

    REQUIRE(a.scheme() == "scheme");
    REQUIRE(a.path().absolute() == true);
    REQUIRE(a.path().at(0) == "");
    REQUIRE(a.path().at(1) == "path1");
    REQUIRE(a.path().at(2) == "path2");
    REQUIRE(a.query() == "query");
    REQUIRE(a.fragment() == "fragment");
}

TEST_CASE(relativePath)
{
    auto const a = hi::URL("file:foo/bar.txt");

    REQUIRE(a.filesystem_path() == "foo/bar.txt");
}

TEST_CASE(glob1)
{
    auto txt_files = std::ranges::to<std::vector>(hi::glob(hi::library_test_data_dir() / "*.txt"));

    REQUIRE(std::any_of(txt_files.begin(), txt_files.end(), [](auto x) {
        return x.filename() == "file_view.txt";
    }));
    REQUIRE(not std::any_of(txt_files.begin(), txt_files.end(), [](auto x) {
        return x.filename() == "HikoGUI_Foundation.lib";
    }));
}

TEST_CASE(glob2)
{
    auto txt_files = std::ranges::to<std::vector>(hi::glob(hi::library_test_data_dir() / "**" / "*.txt"));

    REQUIRE(std::any_of(txt_files.begin(), txt_files.end(), [](auto x) {
        return x.filename() == "glob2.txt";
    }));
}

};
