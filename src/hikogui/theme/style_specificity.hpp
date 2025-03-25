

#pragma once

#include "../macros.hpp"

hi_export_module(hikogui.theme : style_specificity);

hi_export namespace hi::inline v1 {

enum class style_specificity : uint16_t {
    none = 0,

    /** A style set from within the source-code.
     *
     * @note This should be combined with `style_importance::author`.
     */
    style = 1000
};


}

