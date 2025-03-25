// Copyright Take Vos 2022.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "../geometry/geometry.hpp"
#include "../macros.hpp"
#include <vulkan/vulkan.hpp>
#include <vma/vk_mem_alloc.h>
#include <vector>
#include <cstdint>

hi_export_module(hikogui.GFX : gfx_surface_delegate);

hi_export namespace hi::inline v1 {

/** A delegate for drawing on a window below the HikoGUI user interface.
 *
 * This delegate is used to handle drawing on the window outside the HikoGUI user interface.
 * This means you can draw into the swap-chain before HikoGUI layers the user interface on top of it.
 *
 */
class gfx_surface_delegate {
public:
    virtual ~gfx_surface_delegate() = default;
    constexpr gfx_surface_delegate() noexcept = default;
    gfx_surface_delegate(gfx_surface_delegate const&) = delete;
    gfx_surface_delegate(gfx_surface_delegate&&) = delete;
    gfx_surface_delegate& operator=(gfx_surface_delegate const&) = delete;
    gfx_surface_delegate& operator=(gfx_surface_delegate&&) = delete;

    /** The swap-chain is going to be teared-down.
     *
     * This function is called just before the swap-chain is being teared down.
     *
     * This requires the destruction of any references to the swap-chain's image views, including
     * the frame-buffers created during `swapchain_build()`.
     */
    virtual void teardown_for_swapchain_lost() noexcept {}

    /** The vulkan device is going to be teared-down.
     */
    virtual void teardown_for_device_lost() noexcept {}

    /** The vulkan device has been initialized.
     *
     * This function is called when either the device has just been build, or when the widget
     * is added to a window with the device already existing.
     *
     * @param allocator The vulkan-memory-allocator used for reserving memory by HikoGUI.
     * @param instance The vulkan instance used by HikoGUI.
     * @param device The vulkan device used by HikoGUI.
     * @param graphics_queue The graphics queue for rendering on the swap-chain.
     * @param graphics_queue_family_index The family-index of the @a graphics_queue.
     */
    virtual void build_for_new_device(
        VmaAllocator allocator,
        vk::Instance instance,
        vk::Device device,
        vk::Queue graphics_queue,
        uint32_t graphics_queue_family_index) noexcept
    {
    }

    /** The swap-chain has been build.
     *
     * This function is called when either the swap-chain has just been build, or when the delegate
     * is added to a window with an already existing swap-chain.
     *
     * The swap-chain will also be build during resizing of the window. So this needs to be rather fast.
     *
     * @param views The list of swap-chain image views.
     * @param size The size of the images in the swap-chain.
     * @param format The pixel format and color space of the images in the swap-chain.
     */
    virtual void
    build_for_new_swapchain(std::vector<vk::ImageView> const& views, vk::Extent2D size, vk::SurfaceFormatKHR format) noexcept
    {
    }

    /** Draw using vulkan API.
     *
     * HikoGUI reuses previously drawn swap-chain images to reduce the amount of drawing; therefor:
     *  - Set the `initialLayout` of the attachment description to `VK_IMAGE_LAYOUT_PRESENT_SRC_KHR`.
     *  - Set the `renderArea` of the render-pass to @ render_area.
     *  - Ensure with a scissor that no drawing is done outside the @ render_area.
     *
     * See [VkRenderPassBeginInfo](https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkRenderPassBeginInfo.html)
     *
     * @param swapchain_index The index of the image-view of the swap-chain to draw into.
     * @param start The semaphore used to signal when the swapchain-image is ready to be drawn.
     * @param finish The semaphore used to signal when finishing drawing into the swapchain-image.
     * @param render_area The area of the window that is being drawn.
     */
    virtual void draw(uint32_t swapchain_index, vk::Semaphore start, vk::Semaphore finish, vk::Rect2D render_area) const noexcept {}
};

} // namespace hi::inline v1
