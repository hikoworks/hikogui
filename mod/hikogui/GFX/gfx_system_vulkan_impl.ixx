
module;

#include <vulkan/vulkan.hpp>

export module hikogui_GFX : gfx_system_impl;
import : gfx_system_intf;

export namespace hi { inline namespace v1 {

gfx_system::~gfx_system()
{
    hilet lock = std::scoped_lock(gfx_system_mutex);
#ifndef NDEBUG
    intrinsic.destroy(debugUtilsMessenger, nullptr, loader());
#endif
}

[[nodiscard]] gfx_system& gfx_system::global()
{
    if (not detail::gfx_system_global) {
        detail::gfx_system_global = std::make_unique<gfx_system>();
    }
    return *detail::gfx_system_global;
}

}} // namespace hi::v1
