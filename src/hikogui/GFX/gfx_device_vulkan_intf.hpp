// Copyright Take Vos 2019-2022.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "gfx_system_globals.hpp"
#include "gfx_queue_vulkan.hpp"
#include "gfx_pipeline_image_vulkan_intf.hpp"
#include "gfx_pipeline_box_vulkan_intf.hpp"
#include "gfx_pipeline_SDF_vulkan_intf.hpp"
#include "gfx_pipeline_override_vulkan_intf.hpp"
#include "gfx_pipeline_tone_mapper_vulkan_intf.hpp"
#include "../settings/settings.hpp"
#include "../macros.hpp"
#include <vulkan/vulkan.hpp>
#include <vma/vk_mem_alloc.h>
#include <filesystem>
#include <unordered_set>
#include <string>

hi_export_module(hikogui.GFX : gfx_device_intf);

hi_export namespace hi::inline v1 {
class gfx_device {
public:
    std::string deviceName = "<no device>";
    uint32_t vendorID = 0;
    uint32_t deviceID = 0;
    uuid deviceUUID = {};

    vk::PhysicalDevice physicalIntrinsic;
    vk::Device intrinsic;
    VmaAllocator allocator;

    vk::PhysicalDeviceType deviceType = vk::PhysicalDeviceType::eOther;
    vk::PhysicalDeviceProperties physicalProperties;

    std::vector<gfx_queue_vulkan> _queues;

    std::vector<uint32_t> queue_family_indices;

    /** The device features that have been turned on for this device.
     */
    vk::PhysicalDeviceFeatures device_features;

    /** Shared index buffer containing indices for drawing quads.
     * The index buffer uses the following index order: 0, 1, 2, 2, 1, 3
     * ```
     * 2<--3
     * |\  ^
     * | \ |
     * v  \|
     * 0-->1
     * ```
     */
    vk::Buffer quadIndexBuffer;
    VmaAllocation quadIndexBufferAllocation = {};

    std::unique_ptr<gfx_pipeline_box::device_shared> box_pipeline;
    std::unique_ptr<gfx_pipeline_image::device_shared> image_pipeline;
    std::unique_ptr<gfx_pipeline_SDF::device_shared> SDF_pipeline;
    std::unique_ptr<gfx_pipeline_override::device_shared> override_pipeline;
    std::unique_ptr<gfx_pipeline_tone_mapper::device_shared> tone_mapper_pipeline;

    /*! List if extension required on this device.
     */
    std::vector<const char*> requiredExtensions;

    bool supportsLazyTransientImages = false;
    vk::ImageUsageFlags transientImageUsageFlags = vk::ImageUsageFlags{};
    VmaMemoryUsage lazyMemoryUsage = VMA_MEMORY_USAGE_GPU_ONLY;

    ~gfx_device()
    {
        try {
            auto const lock = std::scoped_lock(gfx_system_mutex);

            tone_mapper_pipeline->destroy(this);
            tone_mapper_pipeline = nullptr;
            override_pipeline->destroy(this);
            override_pipeline = nullptr;
            SDF_pipeline->destroy(this);
            SDF_pipeline = nullptr;
            image_pipeline->destroy(this);
            image_pipeline = nullptr;
            box_pipeline->destroy(this);
            box_pipeline = nullptr;

            destroy_quad_index_buffer();

            vmaDestroyAllocator(allocator);

            for (auto const& queue : _queues) {
                intrinsic.destroy(queue.command_pool);
            }

            intrinsic.destroy();

        } catch (std::exception const& e) {
            hi_log_fatal("Could not properly destruct gfx_device. '{}'", e.what());
        }
    }

    gfx_device(const gfx_device&) = delete;
    gfx_device& operator=(const gfx_device&) = delete;
    gfx_device(gfx_device&&) = delete;
    gfx_device& operator=(gfx_device&&) = delete;
    gfx_device(vk::PhysicalDevice physicalDevice);

    std::string string() const noexcept
    {
        auto const lock = std::scoped_lock(gfx_system_mutex);

        return std::format("{0:04x}:{1:04x} {2} {3}", vendorID, deviceID, deviceName, deviceUUID.uuid_string());
    }


    /** Get a graphics queue.
     * Always returns the first queue that can handle graphics.
     */
    [[nodiscard]] gfx_queue_vulkan const& get_graphics_queue() const noexcept
    {
        for (auto& queue : _queues) {
            if (queue.flags & vk::QueueFlagBits::eGraphics) {
                return queue;
            }
        }
        hi_no_default();
    }

    /** Get a graphics queue.
     * Always returns the first queue that can handle both graphics and presenting;
     * or as fallback the first graphics queue.
     */
    [[nodiscard]] gfx_queue_vulkan const& get_graphics_queue(vk::SurfaceKHR surface) const noexcept
    {
        // First try to find a graphics queue which can also present.
        gfx_queue_vulkan const* graphics_queue = nullptr;
        for (auto& queue : _queues) {
            if (queue.flags & vk::QueueFlagBits::eGraphics) {
                if (physicalIntrinsic.getSurfaceSupportKHR(queue.family_queue_index, surface)) {
                    return queue;
                }
                if (not graphics_queue) {
                    graphics_queue = &queue;
                }
            }
        }

        hi_assert_not_null(graphics_queue);
        return *graphics_queue;
    }

    /** Get a present queue.
     * Always returns the first queue that can handle both graphics and presenting;
     * or as fallback the first present queue.
     */
    [[nodiscard]] gfx_queue_vulkan const& get_present_queue(vk::SurfaceKHR surface) const noexcept
    {
        // First try to find a graphics queue which can also present.
        gfx_queue_vulkan const* present_queue = nullptr;
        for (auto& queue : _queues) {
            if (physicalIntrinsic.getSurfaceSupportKHR(queue.family_queue_index, surface)) {
                if (queue.flags & vk::QueueFlagBits::eGraphics) {
                    return queue;
                }
                if (not present_queue) {
                    present_queue = &queue;
                }
            }
        }

        hi_assert_not_null(present_queue);
        return *present_queue;
    }

    /** Get the surface format.
     * Always returns the best suitable surface format.
     *
     * Prioritizes HDR, followed by sRGB.
     *
     * @param surface The surface to determine the surface format for.
     * @param[out] score Optional return parameter for the quality of the surface format.
     */
    [[nodiscard]] vk::SurfaceFormatKHR get_surface_format(vk::SurfaceKHR surface, int* score = nullptr) const noexcept
    {
        auto best_surface_format = vk::SurfaceFormatKHR{};
        auto best_surface_format_score = 0;
        for (auto surface_format : physicalIntrinsic.getSurfaceFormatsKHR(surface)) {
            auto surface_format_score = 0;

            switch (surface_format.colorSpace) {
            case vk::ColorSpaceKHR::eSrgbNonlinear:
                surface_format_score += 1;
                break;
            case vk::ColorSpaceKHR::eExtendedSrgbNonlinearEXT:
                surface_format_score += 10;
                break;
            default:;
            }

            switch (surface_format.format) {
            case vk::Format::eR16G16B16A16Sfloat:
                if (os_settings::uniform_HDR()) {
                    surface_format_score += 12;
                } else {
                    // XXX add override for application that require HDR.
                    surface_format_score -= 100;
                }
                break;
            case vk::Format::eR16G16B16Sfloat:
                if (os_settings::uniform_HDR()) {
                    surface_format_score += 11;
                } else {
                    // XXX add override for application that require HDR.
                    surface_format_score -= 100;
                }
                break;
            case vk::Format::eA2B10G10R10UnormPack32:
                // This is a wire format for HDR, the GPU will not automatically convert linear shader-space to this wire format.
                surface_format_score -= 100;
                break;
            case vk::Format::eR8G8B8A8Srgb:
                surface_format_score += 4;
                break;
            case vk::Format::eB8G8R8A8Srgb:
                surface_format_score += 4;
                break;
            case vk::Format::eR8G8B8Srgb:
                surface_format_score += 3;
                break;
            case vk::Format::eB8G8R8Srgb:
                surface_format_score += 3;
                break;
            case vk::Format::eB8G8R8A8Unorm:
                surface_format_score += 2;
                break;
            case vk::Format::eR8G8B8A8Unorm:
                surface_format_score += 2;
                break;
            case vk::Format::eB8G8R8Unorm:
                surface_format_score += 1;
                break;
            case vk::Format::eR8G8B8Unorm:
                surface_format_score += 1;
                break;
            default:;
            }

            if (score) {
                hi_log_info(
                    "    - color-space={}, format={}, score={}",
                    vk::to_string(surface_format.colorSpace),
                    vk::to_string(surface_format.format),
                    surface_format_score);
            }

            if (surface_format_score > best_surface_format_score) {
                best_surface_format_score = surface_format_score;
                best_surface_format = surface_format;
            }
        }

        if (score) {
            *score = best_surface_format_score;
        }
        return best_surface_format;
    }

    /** Get the present mode.
     * Always returns the best suitable present mode.
     *
     * Prioritized a double buffering mode.
     *
     * @param surface The surface to determine the present mode for.
     * @param[out] score Optional return parameter for the quality of the present mode.
     */
    [[nodiscard]] vk::PresentModeKHR get_present_mode(vk::SurfaceKHR surface, int* score = nullptr) const noexcept
    {
        auto best_present_mode = vk::PresentModeKHR{};
        auto best_present_mode_score = 0;
        for (auto const& present_mode : physicalIntrinsic.getSurfacePresentModesKHR(surface)) {
            int present_mode_score = 0;

            switch (present_mode) {
            case vk::PresentModeKHR::eImmediate:
                present_mode_score += 1;
                break;
            case vk::PresentModeKHR::eFifoRelaxed:
                present_mode_score += 2;
                break;
            case vk::PresentModeKHR::eFifo:
                present_mode_score += 3;
                break;
            case vk::PresentModeKHR::eMailbox:
                present_mode_score += 1;
                break; // mailbox does not wait for vsync.
            default:
                continue;
            }

            if (score) {
                hi_log_info("    - present-mode={} score={}", vk::to_string(present_mode), present_mode_score);
            }

            if (present_mode_score > best_present_mode_score) {
                best_present_mode_score = present_mode_score;
                best_present_mode = present_mode;
            }
        }

        if (score) {
            *score = best_present_mode_score;
        }
        return best_present_mode;
    }

    /*! Check if this device is a good match for this surface.
     *
     * It is possible for a surface to be created that is not presentable, in case of a headless-virtual-display,
     * however in this case it may still be able to be displayed by any device.
     *
     * \returns -1 When not viable, 0 when not presentable, positive values for increasing score.
     */
    int score(vk::SurfaceKHR surface) const;

    std::pair<vk::Buffer, VmaAllocation>
    createBuffer(const vk::BufferCreateInfo& bufferCreateInfo, const VmaAllocationCreateInfo& allocationCreateInfo) const
    {
        hi_axiom(gfx_system_mutex.recurse_lock_count());

        VkBuffer buffer;
        VmaAllocation allocation;

        auto const bufferCreateInfo_ = static_cast<VkBufferCreateInfo>(bufferCreateInfo);
        auto const result =
            vk::Result{vmaCreateBuffer(allocator, &bufferCreateInfo_, &allocationCreateInfo, &buffer, &allocation, nullptr)};

        if (result != vk::Result::eSuccess) {
            throw gui_error(std::format("vmaCreateBuffer() failed {}", to_string(result)));
        }

        return {buffer, allocation};
    }

    void destroyBuffer(const vk::Buffer& buffer, const VmaAllocation& allocation) const
    {
        hi_axiom(gfx_system_mutex.recurse_lock_count());

        vmaDestroyBuffer(allocator, buffer, allocation);
    }

    std::pair<vk::Image, VmaAllocation>
    createImage(const vk::ImageCreateInfo& imageCreateInfo, const VmaAllocationCreateInfo& allocationCreateInfo) const
    {
        hi_axiom(gfx_system_mutex.recurse_lock_count());

        VkImage image;
        VmaAllocation allocation;

        auto const imageCreateInfo_ = static_cast<VkImageCreateInfo>(imageCreateInfo);
        auto const result =
            vk::Result{vmaCreateImage(allocator, &imageCreateInfo_, &allocationCreateInfo, &image, &allocation, nullptr)};

        if (result != vk::Result::eSuccess) {
            throw gui_error(std::format("vmaCreateImage() failed {}", to_string(result)));
        }

        return {image, allocation};
    }

    void destroyImage(const vk::Image& image, const VmaAllocation& allocation) const
    {
        hi_axiom(gfx_system_mutex.recurse_lock_count());

        vmaDestroyImage(allocator, image, allocation);
    }

    vk::CommandBuffer beginSingleTimeCommands() const
    {
        hi_axiom(gfx_system_mutex.recurse_lock_count());

        auto const& queue = get_graphics_queue();
        auto const commandBuffers = intrinsic.allocateCommandBuffers({queue.command_pool, vk::CommandBufferLevel::ePrimary, 1});
        auto const commandBuffer = commandBuffers.at(0);

        commandBuffer.begin({vk::CommandBufferUsageFlagBits::eOneTimeSubmit});
        return commandBuffer;
    }

    void endSingleTimeCommands(vk::CommandBuffer commandBuffer) const
    {
        hi_axiom(gfx_system_mutex.recurse_lock_count());

        commandBuffer.end();

        std::vector<vk::CommandBuffer> const commandBuffers = {commandBuffer};

        auto const& queue = get_graphics_queue();
        queue.queue.submit(
            {{
                0,
                nullptr,
                nullptr, // wait semaphores, wait stages
                narrow_cast<uint32_t>(commandBuffers.size()),
                commandBuffers.data(),
                0,
                nullptr // signal semaphores
            }},
            vk::Fence());

        queue.queue.waitIdle();
        intrinsic.freeCommandBuffers(queue.command_pool, commandBuffers);
    }

    static void transition_layout(
        vk::CommandBuffer command_buffer,
        vk::Image image,
        vk::Format format,
        vk::ImageLayout src_layout,
        vk::ImageLayout dst_layout)
    {
        hi_axiom(gfx_system_mutex.recurse_lock_count());

        auto const [srcAccessMask, srcStage] = access_and_stage_from_layout(src_layout);
        auto const [dstAccessMask, dstStage] = access_and_stage_from_layout(dst_layout);

        std::vector<vk::ImageMemoryBarrier> barriers = {
            {srcAccessMask,
             dstAccessMask,
             src_layout,
             dst_layout,
             VK_QUEUE_FAMILY_IGNORED,
             VK_QUEUE_FAMILY_IGNORED,
             image,
             {
                 vk::ImageAspectFlagBits::eColor,
                 0, // baseMipLevel
                 1, // levelCount
                 0, // baseArrayLayer
                 1 // layerCount
             }}};

        command_buffer.pipelineBarrier(
            srcStage,
            dstStage,
            vk::DependencyFlags(),
            0,
            nullptr,
            0,
            nullptr,
            narrow_cast<uint32_t>(barriers.size()),
            barriers.data());
    }

    void transition_layout(vk::Image image, vk::Format format, vk::ImageLayout src_layout, vk::ImageLayout dst_layout) const
    {
        hi_axiom(gfx_system_mutex.recurse_lock_count());

        auto const command_buffer = beginSingleTimeCommands();

        transition_layout(command_buffer, image, format, src_layout, dst_layout);

        endSingleTimeCommands(command_buffer);
    }

    void copyImage(
        vk::Image srcImage,
        vk::ImageLayout srcLayout,
        vk::Image dstImage,
        vk::ImageLayout dstLayout,
        vk::ArrayProxy<vk::ImageCopy const> regions) const
    {
        hi_axiom(gfx_system_mutex.recurse_lock_count());

        auto const commandBuffer = beginSingleTimeCommands();

        commandBuffer.copyImage(srcImage, srcLayout, dstImage, dstLayout, regions);

        endSingleTimeCommands(commandBuffer);
    }

    void copyBufferToImage(
        vk::Buffer srcBuffer,
        vk::Image dstImage,
        vk::ImageLayout dstLayout,
        vk::ArrayProxy<vk::BufferImageCopy const> regions) const
    {
        hi_axiom(gfx_system_mutex.recurse_lock_count());

        auto const commandBuffer = beginSingleTimeCommands();

        commandBuffer.copyBufferToImage(srcBuffer, dstImage, dstLayout, regions);

        endSingleTimeCommands(commandBuffer);
    }

    void clearColorImage(
        vk::Image image,
        vk::ImageLayout layout,
        vk::ClearColorValue const& color,
        vk::ArrayProxy<const vk::ImageSubresourceRange> ranges) const
    {
        hi_axiom(gfx_system_mutex.recurse_lock_count());

        auto const commandBuffer = beginSingleTimeCommands();

        commandBuffer.clearColorImage(image, layout, color, ranges);

        endSingleTimeCommands(commandBuffer);
    }

    template<typename T>
    std::span<T> mapMemory(const VmaAllocation& allocation) const
    {
        hi_axiom(gfx_system_mutex.recurse_lock_count());

        void* mapping;
        auto const result = vk::Result{vmaMapMemory(allocator, allocation, &mapping)};
        if (result != vk::Result::eSuccess) {
            throw gui_error(std::format("vmaMapMemory failed {}", to_string(result)));
        }

        VmaAllocationInfo allocationInfo;
        vmaGetAllocationInfo(allocator, allocation, &allocationInfo);

        // Should we launder the pointer? The GPU has created the objects, not the C++ application.
        T* mappingT = static_cast<T*>(mapping);
        return std::span<T>{mappingT, allocationInfo.size / sizeof(T)};
    }

    void unmapMemory(const VmaAllocation& allocation) const
    {
        hi_axiom(gfx_system_mutex.recurse_lock_count());

        vmaUnmapMemory(allocator, allocation);
    }

    void flushAllocation(const VmaAllocation& allocation, VkDeviceSize offset, VkDeviceSize size) const
    {
        hi_axiom(gfx_system_mutex.recurse_lock_count());

        auto const alignment = physicalProperties.limits.nonCoherentAtomSize;

        auto const alignedOffset = (offset / alignment) * alignment;
        auto const adjustedSize = size + (offset - alignedOffset);
        auto const alignedSize = ((adjustedSize + (alignment - 1)) / alignment) * alignment;

        vmaFlushAllocation(allocator, allocation, alignedOffset, alignedSize);
    }

    vk::ShaderModule loadShader(uint32_t const* data, std::size_t size) const
    {
        hi_axiom(gfx_system_mutex.recurse_lock_count());

        hi_log_info("Loading shader");

        // Check uint32_t alignment of pointer.
        hi_assert((reinterpret_cast<std::uintptr_t>(data) & 3) == 0);

        return intrinsic.createShaderModule({vk::ShaderModuleCreateFlags(), size, data});
    }

    vk::ShaderModule loadShader(std::span<std::byte const> shaderObjectBytes) const
    {
        // no lock, only local variable.

        // Make sure the address is aligned to uint32_t;
        auto const address = reinterpret_cast<uintptr_t>(shaderObjectBytes.data());
        hi_assert((address & 2) == 0);

        auto const shaderObjectBytes32 = reinterpret_cast<uint32_t const*>(shaderObjectBytes.data());
        return loadShader(shaderObjectBytes32, shaderObjectBytes.size());
    }

    vk::ShaderModule loadShader(std::filesystem::path const& path) const
    {
        // no lock, only local variable.

        return loadShader(as_span<std::byte const>(file_view{path}));
    }

    void waitIdle() const
    {
        hi_axiom(gfx_system_mutex.recurse_lock_count());
        return intrinsic.waitIdle();
    }

    vk::Result waitForFences(vk::ArrayProxy<const vk::Fence> fences, vk::Bool32 waitAll, uint64_t timeout) const
    {
        hi_axiom(gfx_system_mutex.recurse_lock_count());
        return intrinsic.waitForFences(fences, waitAll, timeout);
    }

    vk::Result acquireNextImageKHR(
        vk::SwapchainKHR swapchain,
        uint64_t timeout,
        vk::Semaphore semaphore,
        vk::Fence fence,
        uint32_t* pImageIndex) const
    {
        hi_axiom(gfx_system_mutex.recurse_lock_count());
        return intrinsic.acquireNextImageKHR(swapchain, timeout, semaphore, fence, pImageIndex);
    }

    void resetFences(vk::ArrayProxy<const vk::Fence> fences) const
    {
        hi_axiom(gfx_system_mutex.recurse_lock_count());
        return intrinsic.resetFences(fences);
    }

    vk::Result createSwapchainKHR(
        const vk::SwapchainCreateInfoKHR* pCreateInfo,
        const vk::AllocationCallbacks* pAllocator,
        vk::SwapchainKHR* pSwapchain) const
    {
        hi_axiom(gfx_system_mutex.recurse_lock_count());
        return intrinsic.createSwapchainKHR(pCreateInfo, pAllocator, pSwapchain);
    }

    std::vector<vk::Image> getSwapchainImagesKHR(vk::SwapchainKHR swapchain) const
    {
        hi_axiom(gfx_system_mutex.recurse_lock_count());
        return intrinsic.getSwapchainImagesKHR(swapchain);
    }

    vk::ImageView createImageView(const vk::ImageViewCreateInfo& createInfo) const
    {
        hi_axiom(gfx_system_mutex.recurse_lock_count());
        return intrinsic.createImageView(createInfo);
    }

    vk::Framebuffer createFramebuffer(const vk::FramebufferCreateInfo& createInfo) const
    {
        hi_axiom(gfx_system_mutex.recurse_lock_count());
        return intrinsic.createFramebuffer(createInfo);
    }

    vk::RenderPass createRenderPass(const vk::RenderPassCreateInfo& createInfo) const
    {
        hi_axiom(gfx_system_mutex.recurse_lock_count());
        return intrinsic.createRenderPass(createInfo);
    }

    vk::Extent2D getRenderAreaGranularity(const vk::RenderPass& render_pass) const noexcept
    {
        hi_axiom(gfx_system_mutex.recurse_lock_count());
        vk::Extent2D r;
        intrinsic.getRenderAreaGranularity(render_pass, &r);
        return r;
    }

    vk::Semaphore createSemaphore(const vk::SemaphoreCreateInfo& createInfo = vk::SemaphoreCreateInfo{}) const
    {
        hi_axiom(gfx_system_mutex.recurse_lock_count());
        return intrinsic.createSemaphore(createInfo);
    }

    vk::Fence createFence(const vk::FenceCreateInfo& createInfo) const
    {
        hi_axiom(gfx_system_mutex.recurse_lock_count());
        return intrinsic.createFence(createInfo);
    }

    vk::DescriptorSetLayout createDescriptorSetLayout(const vk::DescriptorSetLayoutCreateInfo& createInfo) const
    {
        hi_axiom(gfx_system_mutex.recurse_lock_count());
        return intrinsic.createDescriptorSetLayout(createInfo);
    }

    vk::DescriptorPool createDescriptorPool(const vk::DescriptorPoolCreateInfo& createInfo) const
    {
        hi_axiom(gfx_system_mutex.recurse_lock_count());
        return intrinsic.createDescriptorPool(createInfo);
    }

    vk::PipelineLayout createPipelineLayout(const vk::PipelineLayoutCreateInfo& createInfo) const
    {
        hi_axiom(gfx_system_mutex.recurse_lock_count());
        return intrinsic.createPipelineLayout(createInfo);
    }

    vk::Pipeline createGraphicsPipeline(vk::PipelineCache pipelineCache, const vk::GraphicsPipelineCreateInfo& createInfo) const
    {
        hi_axiom(gfx_system_mutex.recurse_lock_count());
        return intrinsic.createGraphicsPipeline(pipelineCache, createInfo).value;
    }

    vk::Sampler createSampler(const vk::SamplerCreateInfo& createInfo) const
    {
        hi_axiom(gfx_system_mutex.recurse_lock_count());
        return intrinsic.createSampler(createInfo);
    }

    std::vector<vk::DescriptorSet> allocateDescriptorSets(const vk::DescriptorSetAllocateInfo& allocateInfo) const
    {
        hi_axiom(gfx_system_mutex.recurse_lock_count());
        return intrinsic.allocateDescriptorSets(allocateInfo);
    }

    std::vector<vk::CommandBuffer> allocateCommandBuffers(const vk::CommandBufferAllocateInfo& allocateInfo) const
    {
        hi_axiom(gfx_system_mutex.recurse_lock_count());
        return intrinsic.allocateCommandBuffers(allocateInfo);
    }

    void updateDescriptorSets(
        vk::ArrayProxy<const vk::WriteDescriptorSet> descriptorWrites,
        vk::ArrayProxy<const vk::CopyDescriptorSet> descriptorCopies) const
    {
        hi_axiom(gfx_system_mutex.recurse_lock_count());
        return intrinsic.updateDescriptorSets(descriptorWrites, descriptorCopies);
    }

    void freeCommandBuffers(vk::CommandPool commandPool, vk::ArrayProxy<const vk::CommandBuffer> commandBuffers) const
    {
        hi_axiom(gfx_system_mutex.recurse_lock_count());
        return intrinsic.freeCommandBuffers(commandPool, commandBuffers);
    }

    void setDebugUtilsObjectNameEXT(vk::DebugUtilsObjectNameInfoEXT const& name_info) const;

    void setDebugUtilsObjectNameEXT(vk::Image image, char const* name) const
    {
        return setDebugUtilsObjectNameEXT(
            vk::DebugUtilsObjectNameInfoEXT{vk::ObjectType::eImage, std::bit_cast<uint64_t>(image), name});
    }

    void setDebugUtilsObjectNameEXT(vk::Buffer buffer, char const* name) const
    {
        return setDebugUtilsObjectNameEXT(
            vk::DebugUtilsObjectNameInfoEXT{vk::ObjectType::eBuffer, std::bit_cast<uint64_t>(buffer), name});
    }

    void setDebugUtilsObjectNameEXT(vk::Sampler sampler, char const* name) const
    {
        return setDebugUtilsObjectNameEXT(
            vk::DebugUtilsObjectNameInfoEXT{vk::ObjectType::eSampler, std::bit_cast<uint64_t>(sampler), name});
    }

    void setDebugUtilsObjectNameEXT(vk::ShaderModule shader_module, char const* name) const
    {
        return setDebugUtilsObjectNameEXT(
            vk::DebugUtilsObjectNameInfoEXT{vk::ObjectType::eShaderModule, std::bit_cast<uint64_t>(shader_module), name});
    }

    void cmdBeginDebugUtilsLabelEXT(vk::CommandBuffer buffer, vk::DebugUtilsLabelEXT const& create_info) const;

    void cmdEndDebugUtilsLabelEXT(vk::CommandBuffer buffer) const;

    void cmdBeginDebugUtilsLabelEXT(vk::CommandBuffer buffer, char const* name) const
    {
        return cmdBeginDebugUtilsLabelEXT(buffer, vk::DebugUtilsLabelEXT{name});
    }

    template<typename T>
    void destroy(T x) const
    {
        hi_axiom(gfx_system_mutex.recurse_lock_count());
        intrinsic.destroy(x);
    }

    vk::SurfaceCapabilitiesKHR getSurfaceCapabilitiesKHR(vk::SurfaceKHR surface) const
    {
        hi_axiom(gfx_system_mutex.recurse_lock_count());
        return physicalIntrinsic.getSurfaceCapabilitiesKHR(surface);
    }

    void log_memory_usage() const noexcept
    {
        hi_log_info("Memory usage for gfx device {}:", string());

        char* stat_string;
        vmaBuildStatsString(allocator, &stat_string, VK_TRUE);
        hi_log_info(" * {}", stat_string);
        vmaFreeStatsString(allocator, stat_string);
    }

private:
    static bool
    hasRequiredExtensions(const vk::PhysicalDevice& physicalDevice, const std::vector<const char*>& requiredExtensions)
    {
        auto availableExtensions = std::unordered_set<std::string>();
        for (auto availableExtensionProperties : physicalDevice.enumerateDeviceExtensionProperties()) {
            availableExtensions.insert(std::string(availableExtensionProperties.extensionName.data()));
        }

        for (auto requiredExtension : requiredExtensions) {
            if (availableExtensions.count(requiredExtension) == 0) {
                return false;
            }
        }
        return true;
    }

    static bool meetsRequiredLimits(const vk::PhysicalDevice& physicalDevice, const vk::PhysicalDeviceLimits& requiredLimits)
    {
        return true;
    }

    static bool hasRequiredFeatures(const vk::PhysicalDevice& physicalDevice, const vk::PhysicalDeviceFeatures& requiredFeatures)
    {
        auto const availableFeatures = physicalDevice.getFeatures();
        auto meetsRequirements = true;

        meetsRequirements &=
            (requiredFeatures.robustBufferAccess == VK_TRUE) ? (availableFeatures.robustBufferAccess == VK_TRUE) : true;
        meetsRequirements &=
            (requiredFeatures.fullDrawIndexUint32 == VK_TRUE) ? (availableFeatures.fullDrawIndexUint32 == VK_TRUE) : true;
        meetsRequirements &= (requiredFeatures.imageCubeArray == VK_TRUE) ? (availableFeatures.imageCubeArray == VK_TRUE) : true;
        meetsRequirements &=
            (requiredFeatures.independentBlend == VK_TRUE) ? (availableFeatures.independentBlend == VK_TRUE) : true;
        meetsRequirements &= (requiredFeatures.geometryShader == VK_TRUE) ? (availableFeatures.geometryShader == VK_TRUE) : true;
        meetsRequirements &=
            (requiredFeatures.tessellationShader == VK_TRUE) ? (availableFeatures.tessellationShader == VK_TRUE) : true;
        meetsRequirements &=
            (requiredFeatures.sampleRateShading == VK_TRUE) ? (availableFeatures.sampleRateShading == VK_TRUE) : true;
        meetsRequirements &= (requiredFeatures.dualSrcBlend == VK_TRUE) ? (availableFeatures.dualSrcBlend == VK_TRUE) : true;
        meetsRequirements &= (requiredFeatures.logicOp == VK_TRUE) ? (availableFeatures.logicOp == VK_TRUE) : true;
        meetsRequirements &=
            (requiredFeatures.multiDrawIndirect == VK_TRUE) ? (availableFeatures.multiDrawIndirect == VK_TRUE) : true;
        meetsRequirements &= (requiredFeatures.drawIndirectFirstInstance == VK_TRUE) ?
            (availableFeatures.drawIndirectFirstInstance == VK_TRUE) :
            true;
        meetsRequirements &= (requiredFeatures.depthClamp == VK_TRUE) ? (availableFeatures.depthClamp == VK_TRUE) : true;
        meetsRequirements &= (requiredFeatures.depthBiasClamp == VK_TRUE) ? (availableFeatures.depthBiasClamp == VK_TRUE) : true;
        meetsRequirements &=
            (requiredFeatures.fillModeNonSolid == VK_TRUE) ? (availableFeatures.fillModeNonSolid == VK_TRUE) : true;
        meetsRequirements &= (requiredFeatures.depthBounds == VK_TRUE) ? (availableFeatures.depthBounds == VK_TRUE) : true;
        meetsRequirements &= (requiredFeatures.wideLines == VK_TRUE) ? (availableFeatures.wideLines == VK_TRUE) : true;
        meetsRequirements &= (requiredFeatures.largePoints == VK_TRUE) ? (availableFeatures.largePoints == VK_TRUE) : true;
        meetsRequirements &= (requiredFeatures.alphaToOne == VK_TRUE) ? (availableFeatures.alphaToOne == VK_TRUE) : true;
        meetsRequirements &= (requiredFeatures.multiViewport == VK_TRUE) ? (availableFeatures.multiViewport == VK_TRUE) : true;
        meetsRequirements &=
            (requiredFeatures.samplerAnisotropy == VK_TRUE) ? (availableFeatures.samplerAnisotropy == VK_TRUE) : true;
        meetsRequirements &=
            (requiredFeatures.textureCompressionETC2 == VK_TRUE) ? (availableFeatures.textureCompressionETC2 == VK_TRUE) : true;
        meetsRequirements &= (requiredFeatures.textureCompressionASTC_LDR == VK_TRUE) ?
            (availableFeatures.textureCompressionASTC_LDR == VK_TRUE) :
            true;
        meetsRequirements &=
            (requiredFeatures.textureCompressionBC == VK_TRUE) ? (availableFeatures.textureCompressionBC == VK_TRUE) : true;
        meetsRequirements &=
            (requiredFeatures.occlusionQueryPrecise == VK_TRUE) ? (availableFeatures.occlusionQueryPrecise == VK_TRUE) : true;
        meetsRequirements &=
            (requiredFeatures.pipelineStatisticsQuery == VK_TRUE) ? (availableFeatures.pipelineStatisticsQuery == VK_TRUE) : true;
        meetsRequirements &= (requiredFeatures.vertexPipelineStoresAndAtomics == VK_TRUE) ?
            (availableFeatures.vertexPipelineStoresAndAtomics == VK_TRUE) :
            true;
        meetsRequirements &= (requiredFeatures.fragmentStoresAndAtomics == VK_TRUE) ?
            (availableFeatures.fragmentStoresAndAtomics == VK_TRUE) :
            true;
        meetsRequirements &= (requiredFeatures.shaderTessellationAndGeometryPointSize == VK_TRUE) ?
            (availableFeatures.shaderTessellationAndGeometryPointSize == VK_TRUE) :
            true;
        meetsRequirements &= (requiredFeatures.shaderImageGatherExtended == VK_TRUE) ?
            (availableFeatures.shaderImageGatherExtended == VK_TRUE) :
            true;
        meetsRequirements &= (requiredFeatures.shaderStorageImageExtendedFormats == VK_TRUE) ?
            (availableFeatures.shaderStorageImageExtendedFormats == VK_TRUE) :
            true;
        meetsRequirements &= (requiredFeatures.shaderStorageImageMultisample == VK_TRUE) ?
            (availableFeatures.shaderStorageImageMultisample == VK_TRUE) :
            true;
        meetsRequirements &= (requiredFeatures.shaderStorageImageReadWithoutFormat == VK_TRUE) ?
            (availableFeatures.shaderStorageImageReadWithoutFormat == VK_TRUE) :
            true;
        meetsRequirements &= (requiredFeatures.shaderStorageImageWriteWithoutFormat == VK_TRUE) ?
            (availableFeatures.shaderStorageImageWriteWithoutFormat == VK_TRUE) :
            true;
        meetsRequirements &= (requiredFeatures.shaderUniformBufferArrayDynamicIndexing == VK_TRUE) ?
            (availableFeatures.shaderUniformBufferArrayDynamicIndexing == VK_TRUE) :
            true;
        meetsRequirements &= (requiredFeatures.shaderSampledImageArrayDynamicIndexing == VK_TRUE) ?
            (availableFeatures.shaderSampledImageArrayDynamicIndexing == VK_TRUE) :
            true;
        meetsRequirements &= (requiredFeatures.shaderStorageBufferArrayDynamicIndexing == VK_TRUE) ?
            (availableFeatures.shaderStorageBufferArrayDynamicIndexing == VK_TRUE) :
            true;
        meetsRequirements &= (requiredFeatures.shaderStorageImageArrayDynamicIndexing == VK_TRUE) ?
            (availableFeatures.shaderStorageImageArrayDynamicIndexing == VK_TRUE) :
            true;
        meetsRequirements &=
            (requiredFeatures.shaderClipDistance == VK_TRUE) ? (availableFeatures.shaderClipDistance == VK_TRUE) : true;
        meetsRequirements &=
            (requiredFeatures.shaderCullDistance == VK_TRUE) ? (availableFeatures.shaderCullDistance == VK_TRUE) : true;
        meetsRequirements &= (requiredFeatures.shaderFloat64 == VK_TRUE) ? (availableFeatures.shaderFloat64 == VK_TRUE) : true;
        meetsRequirements &= (requiredFeatures.shaderInt64 == VK_TRUE) ? (availableFeatures.shaderInt64 == VK_TRUE) : true;
        meetsRequirements &= (requiredFeatures.shaderInt16 == VK_TRUE) ? (availableFeatures.shaderInt16 == VK_TRUE) : true;
        meetsRequirements &=
            (requiredFeatures.shaderResourceResidency == VK_TRUE) ? (availableFeatures.shaderResourceResidency == VK_TRUE) : true;
        meetsRequirements &=
            (requiredFeatures.shaderResourceMinLod == VK_TRUE) ? (availableFeatures.shaderResourceMinLod == VK_TRUE) : true;
        meetsRequirements &= (requiredFeatures.sparseBinding == VK_TRUE) ? (availableFeatures.sparseBinding == VK_TRUE) : true;
        meetsRequirements &=
            (requiredFeatures.sparseResidencyBuffer == VK_TRUE) ? (availableFeatures.sparseResidencyBuffer == VK_TRUE) : true;
        meetsRequirements &=
            (requiredFeatures.sparseResidencyImage2D == VK_TRUE) ? (availableFeatures.sparseResidencyImage2D == VK_TRUE) : true;
        meetsRequirements &=
            (requiredFeatures.sparseResidencyImage3D == VK_TRUE) ? (availableFeatures.sparseResidencyImage3D == VK_TRUE) : true;
        meetsRequirements &=
            (requiredFeatures.sparseResidency2Samples == VK_TRUE) ? (availableFeatures.sparseResidency2Samples == VK_TRUE) : true;
        meetsRequirements &=
            (requiredFeatures.sparseResidency4Samples == VK_TRUE) ? (availableFeatures.sparseResidency4Samples == VK_TRUE) : true;
        meetsRequirements &=
            (requiredFeatures.sparseResidency8Samples == VK_TRUE) ? (availableFeatures.sparseResidency8Samples == VK_TRUE) : true;
        meetsRequirements &= (requiredFeatures.sparseResidency16Samples == VK_TRUE) ?
            (availableFeatures.sparseResidency16Samples == VK_TRUE) :
            true;
        meetsRequirements &=
            (requiredFeatures.sparseResidencyAliased == VK_TRUE) ? (availableFeatures.sparseResidencyAliased == VK_TRUE) : true;
        meetsRequirements &=
            (requiredFeatures.variableMultisampleRate == VK_TRUE) ? (availableFeatures.variableMultisampleRate == VK_TRUE) : true;
        meetsRequirements &=
            (requiredFeatures.inheritedQueries == VK_TRUE) ? (availableFeatures.inheritedQueries == VK_TRUE) : true;

        return meetsRequirements;
    }

    static std::pair<vk::AccessFlags, vk::PipelineStageFlags> access_and_stage_from_layout(vk::ImageLayout layout) noexcept
    {
        switch (layout) {
        case vk::ImageLayout::eUndefined:
            return {vk::AccessFlags(), vk::PipelineStageFlagBits::eTopOfPipe};

        // GPU Texture Maps
        case vk::ImageLayout::eTransferDstOptimal:
            return {vk::AccessFlagBits::eTransferWrite, vk::PipelineStageFlagBits::eTransfer};

        case vk::ImageLayout::eShaderReadOnlyOptimal:
            return {vk::AccessFlagBits::eShaderRead, vk::PipelineStageFlagBits::eFragmentShader};

        // CPU Staging texture maps
        case vk::ImageLayout::eGeneral:
            return {vk::AccessFlagBits::eHostWrite, vk::PipelineStageFlagBits::eHost};

        case vk::ImageLayout::eTransferSrcOptimal:
            return {vk::AccessFlagBits::eTransferRead, vk::PipelineStageFlagBits::eTransfer};

        // If we are explicitly transferring an image for ePresentSrcKHR, then we are doing this
        // because we want to reuse the swapchain images in subsequent rendering. Make sure it
        // is ready for the fragment shader.
        case vk::ImageLayout::ePresentSrcKHR:
            return {
                vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite,
                vk::PipelineStageFlagBits::eColorAttachmentOutput};

        default:
            hi_no_default();
        }
    }

    void initialize_queues(std::vector<vk::DeviceQueueCreateInfo> const& device_queue_create_infos) noexcept
    {
        auto const queue_family_properties = physicalIntrinsic.getQueueFamilyProperties();

        _queues.clear();
        queue_family_indices.clear();
        for (auto const& device_queue_create_info : device_queue_create_infos) {
            auto const queue_family_index = device_queue_create_info.queueFamilyIndex;
            auto const& queue_family_property = queue_family_properties[queue_family_index];
            auto const queue_flags = queue_family_property.queueFlags;

            if (queue_family_property.minImageTransferGranularity.width != 1 or
                queue_family_property.minImageTransferGranularity.height != 1) {
                hi_log_fatal(
                    "Queue family {} has a non-1 image transfer granularity. This may cause issues with image transfers.",
                    queue_family_index);
            }

            for (uint32_t queue_index = 0; queue_index != device_queue_create_info.queueCount; ++queue_index) {
                auto queue = intrinsic.getQueue(queue_family_index, queue_index);
                auto command_pool = intrinsic.createCommandPool(
                    {vk::CommandPoolCreateFlagBits::eTransient | vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
                     queue_family_index});
                
                _queues.emplace_back(queue_family_index, queue_index, queue_flags, std::move(queue), std::move(command_pool));
            }
        }
    }

    void initialize_device();

    void initialize_quad_index_buffer()
    {
        hi_axiom(gfx_system_mutex.recurse_lock_count());

        using vertex_index_type = uint16_t;
        constexpr ssize_t maximum_number_of_vertices = 1 << (sizeof(vertex_index_type) * CHAR_BIT);
        constexpr ssize_t maximum_number_of_quads = maximum_number_of_vertices / 4;
        constexpr ssize_t maximum_number_of_triangles = maximum_number_of_quads * 2;
        constexpr ssize_t maximum_number_of_indices = maximum_number_of_triangles * 3;

        // Create vertex index buffer
        {
            vk::BufferCreateInfo const bufferCreateInfo = {
                vk::BufferCreateFlags(),
                sizeof(vertex_index_type) * maximum_number_of_indices,
                vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eTransferDst,
                vk::SharingMode::eExclusive};
            VmaAllocationCreateInfo allocationCreateInfo = {};
            allocationCreateInfo.flags = VMA_ALLOCATION_CREATE_USER_DATA_COPY_STRING_BIT;
            allocationCreateInfo.pUserData = const_cast<char*>("vertex index buffer");
            allocationCreateInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
            std::tie(quadIndexBuffer, quadIndexBufferAllocation) = createBuffer(bufferCreateInfo, allocationCreateInfo);
            setDebugUtilsObjectNameEXT(quadIndexBuffer, "vertex index buffer");
        }

        // Fill in the vertex index buffer, using a staging buffer, then copying.
        {
            // Create staging vertex index buffer.
            vk::BufferCreateInfo const bufferCreateInfo = {
                vk::BufferCreateFlags(),
                sizeof(vertex_index_type) * maximum_number_of_indices,
                vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eTransferSrc,
                vk::SharingMode::eExclusive};
            VmaAllocationCreateInfo allocationCreateInfo = {};
            allocationCreateInfo.flags = VMA_ALLOCATION_CREATE_USER_DATA_COPY_STRING_BIT;
            allocationCreateInfo.pUserData = const_cast<char*>("staging vertex index buffer");
            allocationCreateInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;
            auto const [stagingvertexIndexBuffer, stagingvertexIndexBufferAllocation] =
                createBuffer(bufferCreateInfo, allocationCreateInfo);
            setDebugUtilsObjectNameEXT(stagingvertexIndexBuffer, "staging vertex index buffer");

            // Initialize indices.
            auto const stagingvertexIndexBufferData = mapMemory<vertex_index_type>(stagingvertexIndexBufferAllocation);
            for (std::size_t i = 0; i < maximum_number_of_indices; i++) {
                auto const vertexInRectangle = i % 6;
                auto const rectangleNr = i / 6;
                auto const rectangleBase = rectangleNr * 4;

                switch (vertexInRectangle) {
                case 0:
                    stagingvertexIndexBufferData[i] = narrow_cast<vertex_index_type>(rectangleBase + 0);
                    break;
                case 1:
                    stagingvertexIndexBufferData[i] = narrow_cast<vertex_index_type>(rectangleBase + 1);
                    break;
                case 2:
                    stagingvertexIndexBufferData[i] = narrow_cast<vertex_index_type>(rectangleBase + 2);
                    break;
                case 3:
                    stagingvertexIndexBufferData[i] = narrow_cast<vertex_index_type>(rectangleBase + 2);
                    break;
                case 4:
                    stagingvertexIndexBufferData[i] = narrow_cast<vertex_index_type>(rectangleBase + 1);
                    break;
                case 5:
                    stagingvertexIndexBufferData[i] = narrow_cast<vertex_index_type>(rectangleBase + 3);
                    break;
                default:
                    hi_no_default();
                }
            }
            flushAllocation(stagingvertexIndexBufferAllocation, 0, VK_WHOLE_SIZE);
            unmapMemory(stagingvertexIndexBufferAllocation);

            // Copy indices to vertex index buffer.
            auto& queue = get_graphics_queue();
            auto commands = allocateCommandBuffers({queue.command_pool, vk::CommandBufferLevel::ePrimary, 1}).at(0);

            commands.begin({vk::CommandBufferUsageFlagBits::eOneTimeSubmit});
            cmdBeginDebugUtilsLabelEXT(commands, "copy vertex index buffer");
            commands.copyBuffer(
                stagingvertexIndexBuffer, quadIndexBuffer, {{0, 0, sizeof(vertex_index_type) * maximum_number_of_indices}});
            cmdEndDebugUtilsLabelEXT(commands);
            commands.end();

            std::vector<vk::CommandBuffer> const commandBuffersToSubmit = {commands};
            std::vector<vk::SubmitInfo> const submitInfo = {
                {0,
                 nullptr,
                 nullptr,
                 narrow_cast<uint32_t>(commandBuffersToSubmit.size()),
                 commandBuffersToSubmit.data(),
                 0,
                 nullptr}};
            queue.queue.submit(submitInfo, vk::Fence());
            queue.queue.waitIdle();

            freeCommandBuffers(queue.command_pool, {commands});
            destroyBuffer(stagingvertexIndexBuffer, stagingvertexIndexBufferAllocation);
        }
    }

    void destroy_quad_index_buffer()
    {
        hi_axiom(gfx_system_mutex.recurse_lock_count());
        destroyBuffer(quadIndexBuffer, quadIndexBufferAllocation);
    }
};

} // namespace hi::inline v1
