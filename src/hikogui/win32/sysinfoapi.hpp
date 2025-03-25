
#include <sysinfoapi.h>
#include "../macros.hpp"

hi_export_module(hikogui.win32 : sysinfoapi)

hi_export namespace hi::inline v1 {

[[nodiscard]] inline SYSTEM_INFO win32_GetSystemInfo()
{
    auto r = SYSTEM_INFO{};
    GetSystemInfo(&r);
    return r;
}



}

