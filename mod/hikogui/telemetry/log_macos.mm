// Copyright Take Vos 2022.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

#include "trace.hpp"
#include "cpu_utc_clock.hpp"
#include "strings.hpp"
#include "utility/module.hpp"
#include "../macros.hpp"
#include <format>
#include <exception>
#include <memory>
#include <iostream>
#include <ostream>
#include <chrono>



namespace hi::inline v1 {

std::string get_last_error_message()
{
    return strerror(errno);
}

}
