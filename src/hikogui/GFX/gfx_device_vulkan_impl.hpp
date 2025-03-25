// Copyright Take Vos 2019-2022.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "gfx_device_vulkan_intf.hpp"
#include "gfx_system_vulkan_intf.hpp"
#include "gfx_surface_vulkan_intf.hpp"
#include "../file/file.hpp"
#include "../utility/utility.hpp"
#include "../macros.hpp"
#include <vulkan/vulkan.hpp>
#include <span>

hi_export_module(hikogui.GFX : gfx_device_impl);

hi_export namespace hi::inline v1 {

inline gfx_device::gfx_device(vk::PhysicalDevice physicalDevice) :
    physicalIntrinsic(std::move(physicalDevice))
{
    auto result = physicalIntrinsic.getProperties2KHR<vk::PhysicalDeviceProperties2, vk::PhysicalDeviceIDProperties>(
        vulkan_loader());

    auto resultDeviceProperties2 = result.get<vk::PhysicalDeviceProperties2>();
    auto resultDeviceIDProperties = result.get<vk::PhysicalDeviceIDProperties>();

    requiredExtensions.push_back(VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME);
    requiredExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
    requiredExtensions.push_back(VK_KHR_MAINTENANCE3_EXTENSION_NAME);
    // requiredExtensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
    requiredExtensions.push_back(VK_KHR_IMAGE_FORMAT_LIST_EXTENSION_NAME);
    requiredExtensions.push_back(VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME);

    deviceID = resultDeviceProperties2.properties.deviceID;
    vendorID = resultDeviceProperties2.properties.vendorID;
    deviceName = std::string(resultDeviceProperties2.properties.deviceName.data());
    deviceUUID = uuid::from_big_endian(resultDeviceIDProperties.deviceUUID);

    physicalProperties = physicalIntrinsic.getProperties();

    initialize_device();
}

inline int gfx_device::score(vk::SurfaceKHR surface) const
{
    hi_axiom(gfx_system_mutex.recurse_lock_count());

    auto total_score = 0;

    hi_log_info("Scoring device: {}", string());
    if (not hasRequiredFeatures(physicalIntrinsic, gfx_system::global().requiredFeatures)) {
        hi_log_info(" - Does not have the required features.");
        return -1;
    }

    if (not meetsRequiredLimits(physicalIntrinsic, gfx_system::global().requiredLimits)) {
        hi_log_info(" - Does not meet the required limits.");
        return -1;
    }

    if (!hasRequiredExtensions(physicalIntrinsic, requiredExtensions)) {
        hi_log_info(" - Does not have the required extensions.");
        return -1;
    }

    bool device_has_graphics = false;
    bool device_has_present = false;
    bool device_has_compute = false;
    bool device_shares_graphics_and_present = false;
    for (auto const& queue : _queues) {
        auto const has_present = to_bool(physicalIntrinsic.getSurfaceSupportKHR(queue.family_queue_index, surface));
        auto const has_graphics = to_bool(queue.flags & vk::QueueFlagBits::eGraphics);
        auto const has_compute = to_bool(queue.flags & vk::QueueFlagBits::eCompute);

        device_has_graphics |= has_graphics;
        device_has_present |= has_present;
        device_has_compute |= has_compute;
        if (has_present and has_graphics) {
            device_shares_graphics_and_present = true;
        }
    }

    if (not device_has_graphics) {
        hi_log_info(" - Does not have a graphics queue.");
        return -1;
    }

    if (not device_has_present) {
        hi_log_info(" - Does not have a present queue.");
        return -1;
    }

    if (device_has_compute) {
        hi_log_info(" - Device has compute queue.");
        total_score += 1;
    }

    if (device_shares_graphics_and_present) {
        hi_log_info(" - Device shares graphics and present on same queue.");
        total_score += 10;
    }

    hi_log_info(" - Surface formats:");
    int surface_format_score = 0;
    [[maybe_unused]] auto surface_format = get_surface_format(surface, &surface_format_score);
    if (surface_format_score <= 0) {
        hi_log_info(" - Does not have a suitable surface format.");
        return -1;
    }
    total_score += surface_format_score;

    hi_log_info(" - Present modes:");
    int present_mode_score = 0;
    [[maybe_unused]] auto present_mode = get_present_mode(surface, &present_mode_score);
    if (present_mode_score <= 0) {
        hi_log_info(" - Does not have a suitable present mode.");
        return -1;
    }
    total_score += present_mode_score;

    // Give score based on the performance of the device.
    auto device_type_score = 0;
    auto const properties = physicalIntrinsic.getProperties();
    switch (properties.deviceType) {
    case vk::PhysicalDeviceType::eCpu:
        device_type_score = 1;
        break;
    case vk::PhysicalDeviceType::eOther:
        device_type_score = 1;
        break;
    case vk::PhysicalDeviceType::eVirtualGpu:
        device_type_score = 2;
        break;
    case vk::PhysicalDeviceType::eIntegratedGpu:
        device_type_score = 3;
        break;
    case vk::PhysicalDeviceType::eDiscreteGpu:
        device_type_score = 4;
        break;
    }
    hi_log_info(" - device-type={}, score={}", vk::to_string(properties.deviceType), device_type_score);
    total_score += device_type_score;

    hi_log_info(" - total score {}", total_score);
    return total_score;
}

inline void gfx_device::initialize_device()
{
    auto const default_queue_priority = std::array{1.0f};
    uint32_t queue_family_index = 0;

    auto device_queue_create_infos = std::vector<vk::DeviceQueueCreateInfo>{};
    for (auto queue_family_properties : physicalIntrinsic.getQueueFamilyProperties()) {
        auto const num_queues = 1;
        hi_assert(size(default_queue_priority) >= num_queues);
        device_queue_create_infos.emplace_back(vk::DeviceQueueCreateFlags(), queue_family_index++, num_queues, default_queue_priority.data());
    }

    auto const available_device_features = physicalIntrinsic.getFeatures();

    // Enable optional features.
    device_features = gfx_system::global().requiredFeatures;
    device_features.setDualSrcBlend(available_device_features.dualSrcBlend);
    device_features.setShaderSampledImageArrayDynamicIndexing(VK_TRUE);
    auto physical_device_features = vk::PhysicalDeviceFeatures2{device_features};

    auto device_descriptor_indexing_features = vk::PhysicalDeviceDescriptorIndexingFeatures{};
    device_descriptor_indexing_features.setPNext(&physical_device_features);
    device_descriptor_indexing_features.setShaderSampledImageArrayNonUniformIndexing(VK_TRUE);

    auto device_create_info = vk::DeviceCreateInfo{
        vk::DeviceCreateFlags(),
        narrow_cast<uint32_t>(device_queue_create_infos.size()),
        device_queue_create_infos.data(),
        0,
        nullptr,
        narrow_cast<uint32_t>(requiredExtensions.size()),
        requiredExtensions.data(),
        nullptr};
    device_create_info.setPNext(&device_descriptor_indexing_features);

    intrinsic = physicalIntrinsic.createDevice(device_create_info);

    VmaAllocatorCreateInfo allocatorCreateInfo = {};
    allocatorCreateInfo.physicalDevice = physicalIntrinsic;
    allocatorCreateInfo.device = intrinsic;
    allocatorCreateInfo.instance = vulkan_instance();
    vmaCreateAllocator(&allocatorCreateInfo, &allocator);

    VmaAllocationCreateInfo lazyAllocationInfo = {};
    lazyAllocationInfo.flags = VMA_ALLOCATION_CREATE_USER_DATA_COPY_STRING_BIT;
    lazyAllocationInfo.pUserData = const_cast<char *>("lazy transient image check");
    lazyAllocationInfo.usage = VMA_MEMORY_USAGE_GPU_LAZILY_ALLOCATED;
    uint32_t typeIndexOut = 0;
    supportsLazyTransientImages =
        vmaFindMemoryTypeIndex(allocator, 0, &lazyAllocationInfo, &typeIndexOut) != VK_ERROR_FEATURE_NOT_PRESENT;

    if (supportsLazyTransientImages) {
        lazyMemoryUsage = VMA_MEMORY_USAGE_GPU_LAZILY_ALLOCATED;
        transientImageUsageFlags = vk::ImageUsageFlagBits::eTransientAttachment;
    }

    initialize_queues(device_queue_create_infos);
    queue_family_indices.clear();
    queue_family_indices.push_back(get_graphics_queue().family_queue_index);
    initialize_quad_index_buffer();

    box_pipeline = std::make_unique<gfx_pipeline_box::device_shared>(*this);
    image_pipeline = std::make_unique<gfx_pipeline_image::device_shared>(*this);
    SDF_pipeline = std::make_unique<gfx_pipeline_SDF::device_shared>(*this);
    override_pipeline = std::make_unique<gfx_pipeline_override::device_shared>(*this);
    tone_mapper_pipeline = std::make_unique<gfx_pipeline_tone_mapper::device_shared>(*this);
}

inline void gfx_device::setDebugUtilsObjectNameEXT(vk::DebugUtilsObjectNameInfoEXT const& name_info) const
{
#ifndef NDEBUG
    hi_axiom(gfx_system_mutex.recurse_lock_count());
    return intrinsic.setDebugUtilsObjectNameEXT(name_info, vulkan_loader());
#endif
}

inline void gfx_device::cmdBeginDebugUtilsLabelEXT(vk::CommandBuffer buffer, vk::DebugUtilsLabelEXT const& create_info) const
{
#ifndef NDEBUG
    buffer.beginDebugUtilsLabelEXT(create_info, vulkan_loader());
#endif
}

inline void gfx_device::cmdEndDebugUtilsLabelEXT(vk::CommandBuffer buffer) const
{
#ifndef NDEBUG
    buffer.endDebugUtilsLabelEXT(vulkan_loader());
#endif
}

} // namespace hi::inline v1
