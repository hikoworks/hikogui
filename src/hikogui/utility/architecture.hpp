// Copyright Take Vos 2019-2022.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

/** @file architecture.ixx
 *
 * Functions and macros for handling architectural difference between compilers, CPUs and operating systems.
 */

#pragma once

#include "../macros.hpp"

#include <exception>
#include <cstddef>
#include <type_traits>
#if defined(__APPLE__)
#include <TargetConditionals.h>
#endif

hi_export_module(hikogui.utility.architecture);

hi_export namespace hi::inline v1 {


#if (HI_COMPILER == HI_CC_GCC || HI_COMPILER == HI_CC_CLANG)
#if (HI_PROCESSOR == HI_CPU_X86_64 || HI_PROCESSOR == HI_CPU_ARM64)
#if (HI_STD_LIBRARY == HI_STL_GCC || HI_STD_LIBRARY == HI_STL_LLVM)
#define HI_HAS_INT128 1

/** Signed 128 bit integer.
 */
using int128_t = __int128;

/** Unsigned 128 bit integer.
 */
using uint128_t = unsigned __int128;

#endif
#endif
#endif

#if HI_OPERATING_SYSTEM == HI_OS_WINDOWS
using os_handle = void *;
//using file_handle = os_handle;
using thread_id = uint32_t;
constexpr std::size_t maximum_num_cpus = 64;

#elif HI_OPERATING_SYSTEM == HI_OS_MACOS
using os_handle = int;
//using file_handle = int;
using thread_id = uint32_t;
constexpr std::size_t maximum_num_cpus = CPU_SETSIZE;

#elif HI_OPERATING_SYSTEM == HI_OS_LINUX
using os_handle = int;
//using file_handle = int;
using thread_id = uint32_t;
constexpr std::size_t maximum_num_cpus = CPU_SETSIZE;

#else
#error "Not implemented."
#endif


} // namespace hi::inline v1

namespace std {

// Due to a bug in clang it will emit a undefined symbol to _Literal_zero_is_expected()
// Which is only called from a consteval function (which should never emit code, ever).
#if defined(__clang__) and defined(_CPPLIB_VER)
inline void _Literal_zero_is_expected(void)
{
    std::terminate();
}
#endif

}
