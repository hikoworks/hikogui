// Copyright Take Vos 2024.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "../utility/utility.hpp"
#include "../macros.hpp"
#include <stdexcept>

hi_export_module(hikogui.font : otype_feature_list);

hi_export namespace hi::inline v1 {

class otype_file_error : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

}
