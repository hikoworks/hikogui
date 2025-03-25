
#pragma once

#include "../macros.hpp"

hi_export_module(hikogui.geometry : object_fit);

hi_export namespace hi::inline v1 {

enum class object_fit {
    none,
    contain,
    cover,
    fill,
    scale_down,
};

} // namespace hi::v1
