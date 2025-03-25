// Copyright Take Vos 2023.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "hresult_error_intf.hpp" // export
#include "hresult_error_impl.hpp" // export
#include "libloaderapi.hpp" // export
#include "processthreadsapi.hpp" // export
#include "shlobj_core.hpp" // export
#include "stringapiset.hpp" // export
#include "synchapi.hpp" // export
#include "sysinfoapi.hpp" // export
#include "utility.hpp" // export
#include "win32_error_intf.hpp" // export
#include "win32_error_impl.hpp" // export
#include "winbase.hpp" // export
#include "winnls.hpp" // export
#include "winreg.hpp" // export
#include "winuser.hpp" // export

/** Win32 wrapping functions.
 *
 * The functions are wrapped with the following rules:
 *  - The functions that are wrapped use the `W` suffix.
 *  - The functions are `noexcept`.
 *  - Use `std::expected<>` when a function can result in a error.
 *  - String arguments and result are `std::string` or `std::optional<std::string>`.
 *  - non-optional struct arguments are passed by reference.
 *  - `in` struct arguments are passed by const reference.
 *  - output arguments are returned as value, even when a struct.
 *  - an struct argument with expanded versions are passed by template argument.
 *
 * @module hikogui.win32
 */
hi_export_module(hikogui.win32);

