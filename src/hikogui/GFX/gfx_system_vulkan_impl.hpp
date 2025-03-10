
#pragma once

#include "gfx_system_vulkan_intf.hpp"
#include <vulkan/vulkan.hpp>

hi_export_module(hikogui.GFX : gfx_system_impl);

hi_export namespace hi { inline namespace v1 {

inline gfx_system::~gfx_system()
{
    auto const lock = std::scoped_lock(gfx_system_mutex);
#ifndef NDEBUG
    intrinsic.destroy(debugUtilsMessenger, nullptr, loader());
#endif
}

[[nodiscard]] inline gfx_system& gfx_system::global()
{
    if (not detail::gfx_system_global) {
        detail::gfx_system_global = std::make_unique<gfx_system>();
    }
    return *detail::gfx_system_global;
}

}} // namespace hi::v1
