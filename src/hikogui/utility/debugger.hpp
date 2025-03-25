
#pragma once

#include "debugger_utils.hpp" // export
#include "debugger_intf.hpp" // export
#if defined(HI_GENERIC)
#include "debugger_generic_impl.hpp" // export
#else
#include "debugger_win32_impl.hpp" // export
#endif

/** Support for debugging.
 * 
 * On Windows:
 *  - hi_debug_break() will break if a debugger is attached; otherwise the
 *    application will continue.
 *    This macro will yield a single-byte `int 3` instruction.
 *  - hi_assert_break() will give a continuable error if a debugger is attached;
 *    otherwise std::terminate() will be called and a error message will
 *    be displayed.
 *    This macro will yield an assignment to a global variable and the `int 2c`
 *    instruction.
 *  - hi_assert_break() and hi_debug_break() will optionally launch the
 *    just-in-time debugger if this was configured.
 *  
 * @module hikogui.utility.debugger
 */
hi_export_module(hikogui.utility.debugger);
