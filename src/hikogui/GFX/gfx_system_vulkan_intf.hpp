// Copyright Take Vos 2020-2021.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "gfx_device_vulkan_intf.hpp"
#include "../macros.hpp"
#include <vulkan/vulkan.hpp>
#include <unordered_set>

hi_export_module(hikogui.GFX : gfx_system_intf);

hi_export namespace hi::inline v1 {

/** Vulkan gfx_device controller.
 * Manages Vulkan device and a set of Windows.
 */
class gfx_system {
public:
    //! Vulkan instance.
    vk::Instance intrinsic;

    //! List of extension that where requested when the instance was created.
    std::vector<const char *> requiredExtensions;

    //! List of extension that where requested when the instance was created.
    std::vector<const char *> requiredLayers;

    //! List of required features for each device.
    vk::PhysicalDeviceFeatures requiredFeatures;

    //! List of required limits for each device.
    vk::PhysicalDeviceLimits requiredLimits;

    //! application info passed when the instance was created.
    vk::ApplicationInfo applicationInfo;

    /** Create an instance of a gfx_device.
     *
     * After the constructor is completed it may be used to get a
     * Vulkan surface and passed to `Window` constructors.
     *
     */
    gfx_system()
    {
#if HI_OPERATING_SYSTEM == HI_OS_WINDOWS
        requiredExtensions = {VK_KHR_WIN32_SURFACE_EXTENSION_NAME};
#else
#error "Not Implemented"
#endif

        applicationInfo = vk::ApplicationInfo(
            get_application_name().c_str(),
            VK_MAKE_VERSION(get_application_version().major, get_application_version().minor, get_application_version().patch),
            get_library_name().c_str(),
            VK_MAKE_VERSION(get_library_version().major, get_library_version().minor, get_library_version().patch),
            VK_API_VERSION_1_2);

        // VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2 extension is needed to retrieve unique identifiers for
        // each GPU in the system, so that we can select the same one on each startup and so that the
        // user could select a different one.
        requiredExtensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);

        // VK_KHR_SURFACE extension is needed to draw in a window.
        requiredExtensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);

#ifndef NDEBUG
        requiredExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif

        if (!has_foundation_extensions(requiredExtensions)) {
            throw gui_error("Vulkan instance does not have the required extensions");
        }

        auto instanceCreateInfo = vk::InstanceCreateInfo(vk::InstanceCreateFlags(), &applicationInfo);
        instanceCreateInfo.setEnabledExtensionCount(narrow_cast<uint32_t>(requiredExtensions.size()));
        instanceCreateInfo.setPpEnabledExtensionNames(requiredExtensions.data());

#ifndef NDEBUG
        requiredFeatures.robustBufferAccess = VK_TRUE;
#endif

#ifndef NDEBUG
        auto const requested_layers = std::vector<char const *>{
            "VK_LAYER_KHRONOS_validation", "VK_LAYER_KHRONOS_synchronization2"
            //"VK_LAYER_LUNARG_api_dump"
        };

        requiredLayers = filter_available_layers(requested_layers);
#endif

        instanceCreateInfo.setEnabledLayerCount(narrow_cast<uint32_t>(requiredLayers.size()));
        instanceCreateInfo.setPpEnabledLayerNames(requiredLayers.data());

        hi_log_info("Creating Vulkan instance.");
        intrinsic = vk_create_instance_no_asan(instanceCreateInfo);

#if (VK_HEADER_VERSION == 97)
        _loader = vk::DispatchLoaderDynamic(intrinsic);
#else
        _loader = vk::DispatchLoaderDynamic(intrinsic, vkGetInstanceProcAddr);
#endif

#ifndef NDEBUG
        debugUtilsMessenger = intrinsic.createDebugUtilsMessengerEXT(
            {vk::DebugUtilsMessengerCreateFlagsEXT(),
             vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose |
                 // vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo |
                 vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning | vk::DebugUtilsMessageSeverityFlagBitsEXT::eError,
             vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation |
                 vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance,
             debug_utils_message_callback,
             this},
            nullptr,
            loader());
#endif
    }

    ~gfx_system();
    gfx_system(const gfx_system&) = delete;
    gfx_system& operator=(const gfx_system&) = delete;
    gfx_system(gfx_system&&) = delete;
    gfx_system& operator=(gfx_system&&) = delete;

    [[nodiscard]] static gfx_system &global();

    void log_memory_usage() const noexcept
    {
        for (auto const& device : devices) {
            device->log_memory_usage();
        }
    }


    [[nodiscard]] gfx_device *find_best_device(vk::SurfaceKHR surface)
    {
        enumerate_devices();

        auto const lock = std::scoped_lock(gfx_system_mutex);

        int best_score = -1;
        gfx_device *best_device = nullptr;

        for (auto const& device : devices) {
            auto const score = device->score(surface);
            if (score >= best_score) {
                best_score = score;
                best_device = device.get();
            }
        }

        if (best_score <= 0) {
            hi_log_fatal("Could not find a graphics device suitable for presenting this window.");
        }
        return best_device;
    }

    vk::DispatchLoaderDynamic loader() const noexcept
    {
        hi_axiom(gfx_system_mutex.recurse_lock_count());
        return _loader;
    }

    void destroySurfaceKHR(vk::SurfaceKHR surface)
    {
        hi_axiom(gfx_system_mutex.recurse_lock_count());
        intrinsic.destroySurfaceKHR(surface);
    }

private:
    //! List of all devices.
    std::vector<std::shared_ptr<gfx_device>> devices;

    //! Vulkan dynamic loader of library functions.
    vk::DispatchLoaderDynamic _loader;

    vk::DebugUtilsMessengerEXT debugUtilsMessenger;

    void enumerate_devices() noexcept
    {
        auto const lock = std::scoped_lock(gfx_system_mutex);

        if (not devices.empty()) {
            return;
        }

        for (auto physical_device : intrinsic.enumeratePhysicalDevices()) {
            devices.push_back(std::make_shared<gfx_device>(physical_device));
        }
    }

    [[nodiscard]] static VkBool32 debug_utils_message_callback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
        void *pUserData)
    {
        auto message = std::string_view(pCallbackData->pMessage);

        if (messageSeverity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT) {
            hi_log_info("Vulkan: {}", message);

        } else if (messageSeverity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
            hi_log_warning("Vulkan: {}", message);

        } else if (messageSeverity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
            if (message.starts_with("Failed to open dynamic library")) {
                // Steelseries mouse driver will inject:
                // C:\ProgramData\obs-studio-hook\graphics-hook{32,64}.dll
                // One of them will always fail to load.
                hi_log_warning("Vulkan: {}", pCallbackData->pMessage);

            } else {
                hi_log_error("Vulkan: {}", pCallbackData->pMessage);
            }
        }

        return VK_FALSE;
    }

    [[nodiscard]] static bool has_foundation_extensions(const std::vector<const char *>& requiredExtensions)
    {
        auto availableExtensions = std::unordered_set<std::string>();
        for (auto availableExtensionProperties : vk::enumerateInstanceExtensionProperties()) {
            availableExtensions.insert(std::string(availableExtensionProperties.extensionName.data()));
        }

        for (auto requiredExtension : requiredExtensions) {
            if (availableExtensions.count(requiredExtension) == 0) {
                return false;
            }
        }
        return true;
    }

    [[nodiscard]] static std::vector<const char *> filter_available_layers(std::vector<const char *> const& requested_layers)
    {
        auto available_layers = vk::enumerateInstanceLayerProperties();

        hi_log_info("Available vulkan layers:");
        auto r = std::vector<const char *>{};
        for (auto const& available_layer : available_layers) {
            auto const layer_name = std::string{available_layer.layerName.data()};

            auto const it = std::find(begin(requested_layers), end(requested_layers), layer_name);

            if (it != end(requested_layers)) {
                // Use the *it, because the lifetime of its `char const *` is still available after the function call.
                r.push_back(*it);
                hi_log_info("  * {}", layer_name);
            } else {
                hi_log_info("    {}", layer_name);
            }
        }
        return r;
    }

    [[nodiscard]] hi_no_sanitize_address static vk::Instance vk_create_instance_no_asan(vk::InstanceCreateInfo instance_create_info)
    {
        return vk::createInstance(instance_create_info);
    }
};

namespace detail {
inline std::unique_ptr<gfx_system> gfx_system_global = {};
}

inline vk::Instance vulkan_instance() noexcept
{
    return gfx_system::global().intrinsic;
}

inline vk::DispatchLoaderDynamic vulkan_loader() noexcept
{
    return gfx_system::global().loader();
}

/** Find the best device for a Vulkan surface.
 *
 * @param surface The surface to find the best device for.
 * @return A pointer to a gfx device.
 * @retval nullptr Could not find a Vulkan device for this surface.
 */
[[nodiscard]] inline gfx_device *find_best_device(vk::SurfaceKHR surface)
{
    return gfx_system::global().find_best_device(surface);
}

/** Find the best device for a surface.
 *
 * @param surface The surface to find the best device for.
 * @return A pointer to a fdx device.
 * @retval nullptr Could not find a Vulkan device for this surface.
 */
[[nodiscard]] gfx_device *find_best_device(gfx_surface const &surface);

} // namespace hi::inline v1
