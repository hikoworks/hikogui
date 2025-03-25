

#pragma once

#include "../macros.hpp"
#include <cstdint>
#include <utility>

hi_export_module(hikogui.theme : style_importance);

hi_export namespace hi::inline v1 {


enum class style_importance : uint8_t {
    initial = 0,

    user = 1,

    theme = 2,

    author = 3,

    important_author = 4,

    important_theme = 5,

    important_user = 6,

    important_initial = 7
};

[[nodiscard]] constexpr style_importance make_important(style_importance rhs) noexcept
{
    switch (rhs) {
    case style_importance::initial:
        return style_importance::important_initial;
    case style_importance::user:
        return style_importance::important_user;
    case style_importance::theme:
        return style_importance::important_theme;
    case style_importance::author:
        return style_importance::important_author;
    default:
        return rhs;
    }
}

}
