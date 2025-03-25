// Copyright Take Vos 2023.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "async_pool.hpp" // export
#include "atomic.hpp" // export
#include "callback_flags.hpp" // export
#include "callback.hpp" // export
#include "future_pool.hpp" // export
#include "global_state.hpp" // export
#include "id_factory.hpp" // export
#include "subsystem.hpp" // export
#include "thread.hpp" // export
#include "unfair_mutex.hpp" // export
#include "unfair_recursive_mutex.hpp" // export

hi_export_module(hikogui.concurrency);

hi_export namespace hi {
inline namespace v1 {

/** @defgroup concurrency Concurrency.

Types and functions for handling multiple threads of execution.
*/

}}
