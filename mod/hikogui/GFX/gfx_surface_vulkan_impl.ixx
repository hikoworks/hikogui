// Copyright Take Vos 2019-2022.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

module;
#include "../macros.hpp"

#include <vector>
#include <vulkan/vulkan.hpp>

export module hikogui_GFX : gfx_surface_impl;
import : gfx_device_impl;
import : gfx_pipeline_SDF_intf;
import : gfx_pipeline_box_intf;
import : gfx_pipeline_image_intf;
import : gfx_pipeline_override_intf;
import : gfx_pipeline_tone_mapper_intf;
import : gfx_surface_delegate;
import : gfx_surface_intf;
import : gfx_system_intf;
import hikogui_telemetry;
import hikogui_utility;

export namespace hi::inline v1 {

void gfx_surface::set_device(gfx_device *new_device) noexcept
{
    hi_assert_not_null(new_device);

    hilet lock = std::scoped_lock(gfx_system_mutex);

    hi_axiom(gfx_system_mutex.recurse_lock_count());

    if (_device == new_device) {
        return;
    }

    if (_device) {
        loss = gfx_surface_loss::device_lost;
        teardown();
    }

    _device = new_device;

    _present_queue = std::addressof(_device->get_present_queue(intrinsic));
    _graphics_queue = std::addressof(_device->get_graphics_queue(intrinsic));
}

void gfx_surface::add_delegate(gfx_surface_delegate *delegate) noexcept
{
    hilet lock = std::scoped_lock(gfx_system_mutex);

    hi_assert_not_null(delegate);
    auto& delegate_info = _delegates.emplace_back(delegate, _device->createSemaphore());

    if (state >= gfx_surface_state::has_device) {
        auto& graphics_queue = _device->get_graphics_queue(intrinsic);

        delegate_info.delegate->build_for_new_device(
            _device->allocator, vulkan_instance(), _device->intrinsic, graphics_queue.queue, graphics_queue.family_queue_index);
    }
    if (state >= gfx_surface_state::has_swapchain) {
        auto image_views = std::vector<vk::ImageView>{};
        image_views.reserve(swapchain_image_infos.size());
        for (hilet& image_info : swapchain_image_infos) {
            image_views.push_back(image_info.image_view);
        }

        delegate_info.delegate->build_for_new_swapchain(image_views, swapchainImageExtent, swapchainImageFormat);
    }
}

void gfx_surface::remove_delegate(gfx_surface_delegate *delegate) noexcept
{
    hilet lock = std::scoped_lock(gfx_system_mutex);

    hi_assert_not_null(delegate);
    auto it = std::find_if(_delegates.begin(), _delegates.end(), [delegate](hilet& item) {
        return item.delegate == delegate;
    });

    if (state >= gfx_surface_state::has_swapchain) {
        it->delegate->teardown_for_swapchain_lost();
    }
    if (state >= gfx_surface_state::has_device) {
        it->delegate->teardown_for_device_lost();
    }

    _device->destroy(it->semaphore);

    _delegates.erase(it);
}

[[nodiscard]] extent2 gfx_surface::size() const noexcept
{
    return {narrow_cast<float>(swapchainImageExtent.width), narrow_cast<float>(swapchainImageExtent.height)};
}

void gfx_surface::wait_idle()
{
    hi_axiom(gfx_system_mutex.recurse_lock_count());

    hi_assert(_device);
    if (renderFinishedFence) {
        _device->waitForFences({renderFinishedFence}, VK_TRUE, std::numeric_limits<uint64_t>::max());
    }
    _device->waitIdle();
    hi_log_info("/waitIdle");
}

std::optional<uint32_t> gfx_surface::acquire_next_image_from_swapchain()
{
    hi_axiom(gfx_system_mutex.recurse_lock_count());

    // swap chain, fence & imageAvailableSemaphore must be externally synchronized.
    uint32_t frameBufferIndex = 0;
    // hi_log_debug("acquireNextImage '{}'", title);

    hilet result = _device->acquireNextImageKHR(swapchain, 0, imageAvailableSemaphore, vk::Fence(), &frameBufferIndex);
    // hi_log_debug("acquireNextImage {}", frameBufferIndex);

    switch (result) {
    case vk::Result::eSuccess:
        return {frameBufferIndex};

    case vk::Result::eSuboptimalKHR:
        // Techniqually we received an image here, but we treat it
        // as a swapchain-lost which should reset the swapchain anyway,
        // so not rendering to the image should be okay.
        hi_log_info("acquireNextImageKHR() eSuboptimalKHR");
        loss = gfx_surface_loss::swapchain_lost;
        return std::nullopt;

    case vk::Result::eNotReady:
        // Don't render, we didn't receive an image.
        // The timeout was zero, so we only expect eSuccess or eNotReady.
        // This will wait until the next vertical sync.
        return std::nullopt;

    case vk::Result::eTimeout:
        // Don't render, we didn't receive an image.
        // Since we did not set the timeout we don't expect this result.
        // This will wait until the next vertical sync.
        hi_log_info("acquireNextImageKHR() eTimeout");
        return std::nullopt;

    case vk::Result::eErrorOutOfDateKHR:
        hi_log_info("acquireNextImageKHR() eErrorOutOfDateKHR");
        loss = gfx_surface_loss::swapchain_lost;
        return std::nullopt;

    case vk::Result::eErrorSurfaceLostKHR:
        hi_log_info("acquireNextImageKHR() eErrorSurfaceLostKHR");
        loss = gfx_surface_loss::window_lost;
        return std::nullopt;

    default:
        throw gui_error(std::format("Unknown result from acquireNextImageKHR(). '{}'", to_string(result)));
    }
}

void gfx_surface::present_image_to_queue(uint32_t frameBufferIndex, vk::Semaphore semaphore)
{
    hi_axiom(gfx_system_mutex.recurse_lock_count());

    hi_assert_not_null(_device);

    std::array<vk::Semaphore, 1> const renderFinishedSemaphores = {semaphore};
    std::array<vk::SwapchainKHR, 1> const presentSwapchains = {swapchain};
    std::array<uint32_t, 1> const presentImageIndices = {frameBufferIndex};
    hi_assert(presentSwapchains.size() == presentImageIndices.size());

    try {
        // hi_log_debug("presentQueue {}", presentImageIndices.at(0));
        hilet result = _present_queue->queue.presentKHR(
            {narrow_cast<uint32_t>(renderFinishedSemaphores.size()),
             renderFinishedSemaphores.data(),
             narrow_cast<uint32_t>(presentSwapchains.size()),
             presentSwapchains.data(),
             presentImageIndices.data()});

        switch (result) {
        case vk::Result::eSuccess:
            return;

        case vk::Result::eSuboptimalKHR:
            hi_log_info("presentKHR() eSuboptimalKHR");
            loss = gfx_surface_loss::swapchain_lost;
            return;

        default:
            throw gui_error(std::format("Unknown result from presentKHR(). '{}'", to_string(result)));
        }

    } catch (vk::OutOfDateKHRError const&) {
        hi_log_info("presentKHR() eErrorOutOfDateKHR");
        loss = gfx_surface_loss::swapchain_lost;
        return;

    } catch (vk::SurfaceLostKHRError const&) {
        hi_log_info("presentKHR() eErrorSurfaceLostKHR");
        loss = gfx_surface_loss::window_lost;
        return;
    }
}

gfx_surface_loss gfx_surface::build_for_new_device() noexcept
{
    if (_device->score(intrinsic) <= 0) {
        return gfx_surface_loss::device_lost;
    }

    box_pipeline->build_for_new_device();
    image_pipeline->build_for_new_device();
    SDF_pipeline->build_for_new_device();
    override_pipeline->build_for_new_device();
    tone_mapper_pipeline->build_for_new_device();

    auto& graphics_queue = _device->get_graphics_queue(intrinsic);
    for (auto [delegate, semaphore] : _delegates) {
        hi_assert_not_null(delegate);

        delegate->build_for_new_device(
            _device->allocator, vulkan_instance(), _device->intrinsic, graphics_queue.queue, graphics_queue.family_queue_index);
    }

    return gfx_surface_loss::none;
}

gfx_surface_loss gfx_surface::build_for_new_swapchain(extent2 new_size) noexcept
{
    try {
        hilet[clamped_count, clamped_size] = get_image_count_and_size(defaultNumberOfSwapchainImages, new_size);
        if (not new_size) {
            // Minimized window, can not build a new swap chain.
            return gfx_surface_loss::swapchain_lost;
        }

        if (loss = build_swapchain(clamped_count, clamped_size); loss != gfx_surface_loss::none) {
            return loss;
        }

        hilet[clamped_count_check, clamped_size_check] = get_image_count_and_size(clamped_count, clamped_size);
        if (clamped_count_check != clamped_count or clamped_size_check != clamped_size) {
            // Window has changed during swap chain creation, it is in a inconsistent bad state.
            // This is a bug in the Vulkan specification.
            teardown_swapchain();
            return gfx_surface_loss::swapchain_lost;
        }

        build_render_passes(); // Render-pass requires the swapchain/color/depth image-format.
        build_frame_buffers(); // Framebuffer required render passes.
        build_command_buffers();
        build_semaphores();
        hi_assert_not_null(box_pipeline);
        hi_assert_not_null(image_pipeline);
        hi_assert_not_null(SDF_pipeline);
        hi_assert_not_null(override_pipeline);
        hi_assert_not_null(tone_mapper_pipeline);
        box_pipeline->build_for_new_swapchain(renderPass, 0, swapchainImageExtent);
        image_pipeline->build_for_new_swapchain(renderPass, 1, swapchainImageExtent);
        SDF_pipeline->build_for_new_swapchain(renderPass, 2, swapchainImageExtent);
        override_pipeline->build_for_new_swapchain(renderPass, 3, swapchainImageExtent);
        tone_mapper_pipeline->build_for_new_swapchain(renderPass, 4, swapchainImageExtent);

        auto image_views = std::vector<vk::ImageView>{};
        image_views.reserve(swapchain_image_infos.size());
        for (hilet& image_info : swapchain_image_infos) {
            image_views.push_back(image_info.image_view);
        }

        for (auto [delegate, semaphore] : _delegates) {
            hi_assert_not_null(delegate);
            delegate->build_for_new_swapchain(image_views, swapchainImageExtent, swapchainImageFormat);
        }

        return gfx_surface_loss::none;

    } catch (vk::SurfaceLostKHRError const&) {
        // During swapchain build we lost the surface.
        // This state will cause the swapchain to be teardown.
        return gfx_surface_loss::window_lost;
    }
}

void gfx_surface::build(extent2 new_size) noexcept
{
    hi_axiom(gfx_system_mutex.recurse_lock_count());
    hi_assert(loss == gfx_surface_loss::none);

    if (state == gfx_surface_state::has_window) {
        if (_device) {
            if (loss = build_for_new_device(); loss != gfx_surface_loss::none) {
                return;
            }
            state = gfx_surface_state::has_device;
        }
    }

    if (state == gfx_surface_state::has_device) {
        if (hilet tmp = build_for_new_swapchain(new_size); tmp == gfx_surface_loss::swapchain_lost) {
            // No new swapchain was created, state has_device is maintained.
            return;

        } else if (loss = tmp; tmp != gfx_surface_loss::none) {
            return;
        }

        state = gfx_surface_state::has_swapchain;
    }
}

void gfx_surface::teardown_for_swapchain_lost() noexcept
{
    hi_log_info("Tearing down because the window lost the swapchain.");
    wait_idle();

    for (auto [delegate, semaphore] : _delegates) {
        hi_assert_not_null(delegate);
        delegate->teardown_for_swapchain_lost();
    }

    tone_mapper_pipeline->teardown_for_swapchain_lost();
    override_pipeline->teardown_for_swapchain_lost();
    SDF_pipeline->teardown_for_swapchain_lost();
    image_pipeline->teardown_for_swapchain_lost();
    box_pipeline->teardown_for_swapchain_lost();
    teardown_semaphores();
    teardown_command_buffers();
    teardown_frame_buffers();
    teardown_render_passes();
    teardown_swapchain();
}

void gfx_surface::teardown_for_device_lost() noexcept
{
    hi_log_info("Tearing down because the window lost the vulkan device.");
    for (auto [delegate, semaphore] : _delegates) {
        hi_assert_not_null(delegate);
        delegate->teardown_for_device_lost();
    }
    tone_mapper_pipeline->teardown_for_device_lost();
    override_pipeline->teardown_for_device_lost();
    SDF_pipeline->teardown_for_device_lost();
    image_pipeline->teardown_for_device_lost();
    box_pipeline->teardown_for_device_lost();
    _device = nullptr;
}

void gfx_surface::teardown_for_window_lost() noexcept
{
    gfx_system::global().destroySurfaceKHR(intrinsic);
}

void gfx_surface::teardown() noexcept
{
    hi_axiom(gfx_system_mutex.recurse_lock_count());

    if (state == gfx_surface_state::has_swapchain and loss >= gfx_surface_loss::swapchain_lost) {
        teardown_for_swapchain_lost();
        state = gfx_surface_state::has_device;
    }

    if (state == gfx_surface_state::has_device and loss >= gfx_surface_loss::device_lost) {
        teardown_for_device_lost();
        state = gfx_surface_state::has_window;
    }

    if (state == gfx_surface_state::has_window and loss >= gfx_surface_loss::window_lost) {
        hi_log_info("Tearing down because the window doesn't exist anymore.");
        teardown_for_window_lost();
        state = gfx_surface_state::no_window;
    }
    loss = gfx_surface_loss::none;
}

void gfx_surface::update(extent2 new_size) noexcept
{
    hilet lock = std::scoped_lock(gfx_system_mutex);

    if (size() != new_size and state == gfx_surface_state::has_swapchain) {
        // On resize lose the swapchain, which will be cleaned up at teardown().
        loss = gfx_surface_loss::swapchain_lost;
    }

    // Tear down then buildup from the Vulkan objects that where invalid.
    teardown();
    build(new_size);
}

draw_context gfx_surface::render_start(aarectangle redraw_rectangle)
{
    // Extent the redraw_rectangle to the render-area-granularity to improve performance on tile based GPUs.
    redraw_rectangle = ceil(redraw_rectangle, _render_area_granularity);

    hilet lock = std::scoped_lock(gfx_system_mutex);

    auto r = draw_context{
        *_device,
        box_pipeline->vertexBufferData,
        image_pipeline->vertexBufferData,
        SDF_pipeline->vertexBufferData,
        override_pipeline->vertexBufferData};

    // Bail out when the window is not yet ready to be rendered, or if there is nothing to render.
    if (state != gfx_surface_state::has_swapchain or not redraw_rectangle) {
        return r;
    }

    hilet optional_frame_buffer_index = acquire_next_image_from_swapchain();
    if (!optional_frame_buffer_index) {
        // No image is ready to be rendered, yet, possibly because our vertical sync function
        // is not working correctly.
        return r;
    }

    // Setting the frame buffer index, also enabled the draw_context.
    r.frame_buffer_index = narrow_cast<size_t>(*optional_frame_buffer_index);

    // Record which part of the image will be redrawn on the current swapchain image.
    auto& current_image = swapchain_image_infos.at(r.frame_buffer_index);
    current_image.redraw_rectangle = redraw_rectangle;

    // Calculate the scissor rectangle, from the combined redraws of the complete swapchain.
    // We need to do this so that old redraws are also executed in the current swapchain image.
    r.scissor_rectangle =
        std::accumulate(swapchain_image_infos.cbegin(), swapchain_image_infos.cend(), aarectangle{}, [](hilet& sum, hilet& item) {
            return sum | item.redraw_rectangle;
        });

    // Wait until previous rendering has finished, before the next rendering.
    _device->waitForFences({renderFinishedFence}, VK_TRUE, std::numeric_limits<uint64_t>::max());

    // Unsignal the fence so we will not modify/destroy the command buffers during rendering.
    _device->resetFences({renderFinishedFence});

    return r;
}

void gfx_surface::render_finish(draw_context const& context)
{
    hilet lock = std::scoped_lock(gfx_system_mutex);

    auto& current_image = swapchain_image_infos.at(context.frame_buffer_index);

    // Because we use a scissor/render_area, the image from the swapchain around the scissor-area is reused.
    // Because of reuse the swapchain image must already be in the "ePresentSrcKHR" layout.
    // The swapchain creates images in undefined layout, so we need to change the layout once.
    if (not current_image.layout_is_present) {
        _device->transition_layout(
            current_image.image, swapchainImageFormat.format, vk::ImageLayout::eUndefined, vk::ImageLayout::ePresentSrcKHR);

        current_image.layout_is_present = true;
    }

    // Clamp the scissor rectangle to the size of the window.
    hilet clamped_scissor_rectangle = intersect(
        context.scissor_rectangle,
        aarectangle{0, 0, narrow_cast<float>(swapchainImageExtent.width), narrow_cast<float>(swapchainImageExtent.height)});

    hilet render_area = vk::Rect2D{
        vk::Offset2D(
            round_cast<uint32_t>(clamped_scissor_rectangle.left()),
            round_cast<uint32_t>(
                swapchainImageExtent.height - clamped_scissor_rectangle.bottom() - clamped_scissor_rectangle.height())),
        vk::Extent2D(
            round_cast<uint32_t>(clamped_scissor_rectangle.width()), round_cast<uint32_t>(clamped_scissor_rectangle.height()))};

    // Start the first delegate when the swapchain-image becomes available.
    auto start_semaphore = imageAvailableSemaphore;
    for (auto [delegate, end_semaphore] : _delegates) {
        hi_assert_not_null(delegate);

        delegate->draw(narrow_cast<uint32_t>(context.frame_buffer_index), start_semaphore, end_semaphore, render_area);
        start_semaphore = end_semaphore;
    }

    // Wait for the semaphore of the last delegate before it will write into the swapchain-image.
    fill_command_buffer(current_image, context, render_area);
    submit_command_buffer(start_semaphore);

    // Signal the fence when all rendering has finished on the graphics queue.
    // When the fence is signaled we can modify/destroy the command buffers.
    [[maybe_unused]] hilet submit_result = _graphics_queue->queue.submit(0, nullptr, renderFinishedFence);

    present_image_to_queue(narrow_cast<uint32_t>(context.frame_buffer_index), renderFinishedSemaphore);

    // Do an early tear down of invalid vulkan objects.
    teardown();
}

void gfx_surface::fill_command_buffer(
    swapchain_image_info const& current_image,
    draw_context const& context,
    vk::Rect2D render_area)
{
    hi_axiom(gfx_system_mutex.recurse_lock_count());

    auto t = trace<"fill_command_buffer">{};

    commandBuffer.reset(vk::CommandBufferResetFlagBits::eReleaseResources);
    commandBuffer.begin({vk::CommandBufferUsageFlagBits::eSimultaneousUse});

    hilet background_color_f32x4 = f32x4{1.0f, 0.0f, 0.0f, 1.0f};
    hilet background_color_array = static_cast<std::array<float, 4>>(background_color_f32x4);

    hilet colorClearValue = vk::ClearColorValue{background_color_array};
    hilet sdfClearValue = vk::ClearColorValue{std::array{0.0f, 0.0f, 0.0f, 0.0f}};
    hilet depthClearValue = vk::ClearDepthStencilValue{0.0, 0};
    hilet clearValues = std::array{
        vk::ClearValue{depthClearValue},
        vk::ClearValue{colorClearValue},
        vk::ClearValue{sdfClearValue},
        vk::ClearValue{colorClearValue}};

    // The scissor and render area makes sure that the frame buffer is not modified where we are not drawing the widgets.
    hilet scissors = std::array{render_area};
    commandBuffer.setScissor(0, scissors);

    commandBuffer.beginRenderPass(
        {renderPass, current_image.frame_buffer, render_area, narrow_cast<uint32_t>(clearValues.size()), clearValues.data()},
        vk::SubpassContents::eInline);

    box_pipeline->draw_in_command_buffer(commandBuffer, context);
    commandBuffer.nextSubpass(vk::SubpassContents::eInline);
    image_pipeline->draw_in_command_buffer(commandBuffer, context);
    commandBuffer.nextSubpass(vk::SubpassContents::eInline);
    SDF_pipeline->draw_in_command_buffer(commandBuffer, context);
    commandBuffer.nextSubpass(vk::SubpassContents::eInline);
    override_pipeline->draw_in_command_buffer(commandBuffer, context);
    commandBuffer.nextSubpass(vk::SubpassContents::eInline);
    tone_mapper_pipeline->draw_in_command_buffer(commandBuffer, context);

    commandBuffer.endRenderPass();
    commandBuffer.end();
}

void gfx_surface::submit_command_buffer(vk::Semaphore delegate_semaphore)
{
    hi_axiom(gfx_system_mutex.recurse_lock_count());

    hilet waitSemaphores = std::array{delegate_semaphore};

    hilet waitStages = std::array{vk::PipelineStageFlags{vk::PipelineStageFlagBits::eColorAttachmentOutput}};

    hi_assert(waitSemaphores.size() == waitStages.size());

    hilet signalSemaphores = std::array{renderFinishedSemaphore};
    hilet commandBuffersToSubmit = std::array{commandBuffer};

    hilet submitInfo = std::array{vk::SubmitInfo{
        narrow_cast<uint32_t>(waitSemaphores.size()),
        waitSemaphores.data(),
        waitStages.data(),
        narrow_cast<uint32_t>(commandBuffersToSubmit.size()),
        commandBuffersToSubmit.data(),
        narrow_cast<uint32_t>(signalSemaphores.size()),
        signalSemaphores.data()}};

    _graphics_queue->queue.submit(submitInfo, vk::Fence());
}

std::tuple<std::size_t, extent2> gfx_surface::get_image_count_and_size(std::size_t new_count, extent2 new_size)
{
    hi_axiom(gfx_system_mutex.recurse_lock_count());

    hilet surfaceCapabilities = _device->getSurfaceCapabilitiesKHR(intrinsic);

    hilet min_count = narrow_cast<std::size_t>(surfaceCapabilities.minImageCount);
    hilet max_count = narrow_cast<std::size_t>(surfaceCapabilities.maxImageCount ? surfaceCapabilities.maxImageCount : 3);
    hilet clamped_count = std::clamp(new_count, min_count, max_count);
    hi_log_info(
        "gfx_surface min_count={}, max_count={}, requested_count={}, count={}", min_count, max_count, new_count, clamped_count);

    // minImageExtent and maxImageExtent are always valid. currentImageExtent may be 0xffffffff.
    hilet min_size = extent2{
        narrow_cast<float>(surfaceCapabilities.minImageExtent.width),
        narrow_cast<float>(surfaceCapabilities.minImageExtent.height)};
    hilet max_size = extent2{
        narrow_cast<float>(surfaceCapabilities.maxImageExtent.width),
        narrow_cast<float>(surfaceCapabilities.maxImageExtent.height)};
    hilet clamped_size = clamp(new_size, min_size, max_size);

    hi_log_info("gfx_surface min_size={}, max_size={}, requested_size={}, size={}", min_size, max_size, new_size, clamped_size);
    return {clamped_count, clamped_size};
}

gfx_surface_loss gfx_surface::build_swapchain(std::size_t new_count, extent2 new_size)
{
    hi_axiom(gfx_system_mutex.recurse_lock_count());

    hi_log_info("Building swap chain");

    hilet sharingMode = _graphics_queue == _present_queue ? vk::SharingMode::eExclusive : vk::SharingMode::eConcurrent;

    std::array<uint32_t, 2> const sharingQueueFamilyAllIndices = {
        _graphics_queue->family_queue_index, _present_queue->family_queue_index};

    swapchainImageFormat = _device->get_surface_format(intrinsic);
    nrSwapchainImages = narrow_cast<uint32_t>(new_count);
    swapchainImageExtent = VkExtent2D{round_cast<uint32_t>(new_size.width()), round_cast<uint32_t>(new_size.height())};
    vk::SwapchainCreateInfoKHR swapchainCreateInfo{
        vk::SwapchainCreateFlagsKHR(),
        intrinsic,
        nrSwapchainImages,
        swapchainImageFormat.format,
        swapchainImageFormat.colorSpace,
        swapchainImageExtent,
        1, // imageArrayLayers
        vk::ImageUsageFlagBits::eColorAttachment,
        sharingMode,
        sharingMode == vk::SharingMode::eConcurrent ? narrow_cast<uint32_t>(sharingQueueFamilyAllIndices.size()) : 0,
        sharingMode == vk::SharingMode::eConcurrent ? sharingQueueFamilyAllIndices.data() : nullptr,
        vk::SurfaceTransformFlagBitsKHR::eIdentity,
        vk::CompositeAlphaFlagBitsKHR::eOpaque,
        _device->get_present_mode(intrinsic),
        VK_TRUE, // clipped
        nullptr};

    vk::Result const result = _device->createSwapchainKHR(&swapchainCreateInfo, nullptr, &swapchain);
    switch (result) {
    case vk::Result::eSuccess:
        break;

    case vk::Result::eErrorSurfaceLostKHR:
        return gfx_surface_loss::window_lost;

    default:
        throw gui_error(std::format("Unknown result from createSwapchainKHR(). '{}'", to_string(result)));
    }

    hi_log_info("Finished building swap chain");
    hi_log_info(" - extent=({}, {})", swapchainCreateInfo.imageExtent.width, swapchainCreateInfo.imageExtent.height);
    hi_log_info(
        " - colorSpace={}, format={}",
        vk::to_string(swapchainCreateInfo.imageColorSpace),
        vk::to_string(swapchainCreateInfo.imageFormat));
    hi_log_info(
        " - presentMode={}, imageCount={}", vk::to_string(swapchainCreateInfo.presentMode), swapchainCreateInfo.minImageCount);

    // Create depth matching the swapchain.
    vk::ImageCreateInfo const depthImageCreateInfo = {
        vk::ImageCreateFlags(),
        vk::ImageType::e2D,
        depthImageFormat,
        vk::Extent3D(swapchainCreateInfo.imageExtent.width, swapchainCreateInfo.imageExtent.height, 1),
        1, // mipLevels
        1, // arrayLayers
        vk::SampleCountFlagBits::e1,
        vk::ImageTiling::eOptimal,
        vk::ImageUsageFlagBits::eDepthStencilAttachment | _device->transientImageUsageFlags,
        vk::SharingMode::eExclusive,
        0,
        nullptr,
        vk::ImageLayout::eUndefined};

    VmaAllocationCreateInfo depthAllocationCreateInfo = {};
    depthAllocationCreateInfo.flags = VMA_ALLOCATION_CREATE_USER_DATA_COPY_STRING_BIT;
    depthAllocationCreateInfo.pUserData = const_cast<char *>("vk::Image depth attachment");
    depthAllocationCreateInfo.usage = _device->lazyMemoryUsage;
    std::tie(depthImage, depthImageAllocation) = _device->createImage(depthImageCreateInfo, depthAllocationCreateInfo);
    _device->setDebugUtilsObjectNameEXT(depthImage, "vk::Image depth attachment");

    // Create color image matching the swapchain.
    vk::ImageCreateInfo const colorImageCreateInfo = {
        vk::ImageCreateFlags(),
        vk::ImageType::e2D,
        colorImageFormat,
        vk::Extent3D(swapchainCreateInfo.imageExtent.width, swapchainCreateInfo.imageExtent.height, 1),
        1, // mipLevels
        1, // arrayLayers
        vk::SampleCountFlagBits::e1,
        vk::ImageTiling::eOptimal,
        vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eInputAttachment | _device->transientImageUsageFlags,
        vk::SharingMode::eExclusive,
        0,
        nullptr,
        vk::ImageLayout::eUndefined};

    VmaAllocationCreateInfo colorAllocationCreateInfo = {};
    colorAllocationCreateInfo.flags = VMA_ALLOCATION_CREATE_USER_DATA_COPY_STRING_BIT;
    colorAllocationCreateInfo.pUserData = const_cast<char *>("vk::Image color attachment");
    colorAllocationCreateInfo.usage = _device->lazyMemoryUsage;

    std::tie(colorImages[0], colorImageAllocations[0]) = _device->createImage(colorImageCreateInfo, colorAllocationCreateInfo);
    _device->setDebugUtilsObjectNameEXT(colorImages[0], "vk::Image color attachment");

    return gfx_surface_loss::none;
}

void gfx_surface::teardown_swapchain()
{
    hi_axiom(gfx_system_mutex.recurse_lock_count());

    _device->destroy(swapchain);
    _device->destroyImage(depthImage, depthImageAllocation);

    for (std::size_t i = 0; i != colorImages.size(); ++i) {
        _device->destroyImage(colorImages[i], colorImageAllocations[i]);
    }
}

void gfx_surface::build_frame_buffers()
{
    hi_axiom(gfx_system_mutex.recurse_lock_count());

    depthImageView = _device->createImageView(
        {vk::ImageViewCreateFlags(),
         depthImage,
         vk::ImageViewType::e2D,
         depthImageFormat,
         vk::ComponentMapping(),
         {vk::ImageAspectFlagBits::eDepth, 0, 1, 0, 1}});

    for (std::size_t i = 0; i != colorImageViews.size(); ++i) {
        colorImageViews[i] = _device->createImageView(
            {vk::ImageViewCreateFlags(),
             colorImages[i],
             vk::ImageViewType::e2D,
             colorImageFormat,
             vk::ComponentMapping(),
             {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1}});

        colorDescriptorImageInfos[i] = {vk::Sampler(), colorImageViews[i], vk::ImageLayout::eShaderReadOnlyOptimal};
    }

    auto swapchain_images = _device->getSwapchainImagesKHR(swapchain);
    for (auto image : swapchain_images) {
        auto image_view = _device->createImageView(
            {vk::ImageViewCreateFlags(),
             image,
             vk::ImageViewType::e2D,
             swapchainImageFormat.format,
             vk::ComponentMapping(),
             {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1}});

        hilet attachments = std::array{depthImageView, colorImageViews[0], image_view};

        hilet frame_buffer = _device->createFramebuffer({
            vk::FramebufferCreateFlags(),
            renderPass,
            narrow_cast<uint32_t>(attachments.size()),
            attachments.data(),
            swapchainImageExtent.width,
            swapchainImageExtent.height,
            1 // layers
        });

        swapchain_image_infos.emplace_back(
            std::move(image), std::move(image_view), std::move(frame_buffer), aarectangle{}, false);
    }

    hi_assert(swapchain_image_infos.size() == swapchain_images.size());
}

void gfx_surface::teardown_frame_buffers()
{
    hi_axiom(gfx_system_mutex.recurse_lock_count());

    for (auto& info : swapchain_image_infos) {
        _device->destroy(info.frame_buffer);
        _device->destroy(info.image_view);
    }
    swapchain_image_infos.clear();

    _device->destroy(depthImageView);
    for (std::size_t i = 0; i != colorImageViews.size(); ++i) {
        _device->destroy(colorImageViews[i]);
    }
}

/** Build render passes.
 *
 * One pass, with 4 subpasses:
 *  1. box shader: to color-attachment+depth
 *  2. image shader: to color-attachment+depth
 *  3. sdf shader: to color-attachment+depth
 *  4. alpha shader: to color-attachment+depth
 *  4. tone-mapper: color-input-attachment to swapchain-attachment.
 *
 * Rendering is done on a float-16 RGBA color-attachment.
 * In the last subpass the color-attachment is translated to the swap-chain attachment.
 */
void gfx_surface::build_render_passes()
{
    hi_axiom(gfx_system_mutex.recurse_lock_count());

    hilet attachment_descriptions = std::array{
        vk::AttachmentDescription{
            // Depth attachment
            vk::AttachmentDescriptionFlags(),
            depthImageFormat,
            vk::SampleCountFlagBits::e1,
            vk::AttachmentLoadOp::eClear,
            vk::AttachmentStoreOp::eDontCare,
            vk::AttachmentLoadOp::eDontCare, // stencilLoadOp
            vk::AttachmentStoreOp::eDontCare, // stencilStoreOp
            vk::ImageLayout::eUndefined, // initialLayout
            vk::ImageLayout::eDepthStencilAttachmentOptimal // finalLayout
        },
        vk::AttachmentDescription{
            // Color attachment
            vk::AttachmentDescriptionFlags(),
            colorImageFormat,
            vk::SampleCountFlagBits::e1,
            vk::AttachmentLoadOp::eClear,
            vk::AttachmentStoreOp::eDontCare,
            vk::AttachmentLoadOp::eDontCare, // stencilLoadOp
            vk::AttachmentStoreOp::eDontCare, // stencilStoreOp
            vk::ImageLayout::eUndefined, // initialLayout
            vk::ImageLayout::eColorAttachmentOptimal // finalLayout
        },
        vk::AttachmentDescription{
            // Swapchain attachment.
            vk::AttachmentDescriptionFlags(),
            swapchainImageFormat.format,
            vk::SampleCountFlagBits::e1,
            vk::AttachmentLoadOp::eLoad,
            vk::AttachmentStoreOp::eStore,
            vk::AttachmentLoadOp::eDontCare, // stencilLoadOp
            vk::AttachmentStoreOp::eDontCare, // stencilStoreOp
            vk::ImageLayout::ePresentSrcKHR, // initialLayout
            vk::ImageLayout::ePresentSrcKHR // finalLayout
        }};

    hilet depth_attachment_reference = vk::AttachmentReference{0, vk::ImageLayout::eDepthStencilAttachmentOptimal};
    hilet color_attachment_references = std::array{vk::AttachmentReference{1, vk::ImageLayout::eColorAttachmentOptimal}};
    hilet color_input_attachment_references = std::array{vk::AttachmentReference{1, vk::ImageLayout::eShaderReadOnlyOptimal}};
    hilet swapchain_attachment_references = std::array{vk::AttachmentReference{2, vk::ImageLayout::eColorAttachmentOptimal}};

    hilet subpass_descriptions = std::array{
        vk::SubpassDescription{
            vk::SubpassDescriptionFlags(), // Subpass 0 Box
            vk::PipelineBindPoint::eGraphics,
            0, // inputAttachmentReferencesCount
            nullptr, // inputAttachmentReferences
            narrow_cast<uint32_t>(color_attachment_references.size()),
            color_attachment_references.data(),
            nullptr, // resolveAttachments
            &depth_attachment_reference

        },
        vk::SubpassDescription{
            vk::SubpassDescriptionFlags(), // Subpass 1 Image
            vk::PipelineBindPoint::eGraphics,
            0, // inputAttachmentReferencesCount
            nullptr, // inputAttachmentReferences
            narrow_cast<uint32_t>(color_attachment_references.size()),
            color_attachment_references.data(),
            nullptr, // resolveAttachments
            &depth_attachment_reference

        },
        vk::SubpassDescription{
            vk::SubpassDescriptionFlags(), // Subpass 2 SDF
            vk::PipelineBindPoint::eGraphics,
            0,
            nullptr,
            narrow_cast<uint32_t>(color_attachment_references.size()),
            color_attachment_references.data(),
            nullptr, // resolveAttachments
            &depth_attachment_reference

        },
        vk::SubpassDescription{
            vk::SubpassDescriptionFlags(), // Subpass 3 alpha
            vk::PipelineBindPoint::eGraphics,
            0,
            nullptr,
            narrow_cast<uint32_t>(color_attachment_references.size()),
            color_attachment_references.data(),
            nullptr, // resolveAttachments
            &depth_attachment_reference

        },
        vk::SubpassDescription{
            vk::SubpassDescriptionFlags(), // Subpass 4 tone-mapper
            vk::PipelineBindPoint::eGraphics,
            narrow_cast<uint32_t>(color_input_attachment_references.size()),
            color_input_attachment_references.data(),
            narrow_cast<uint32_t>(swapchain_attachment_references.size()),
            swapchain_attachment_references.data(),
            nullptr,
            nullptr}};

    hilet subpass_dependency = std::array{
        vk::SubpassDependency{
            VK_SUBPASS_EXTERNAL,
            0,
            vk::PipelineStageFlagBits::eBottomOfPipe,
            vk::PipelineStageFlagBits::eColorAttachmentOutput,
            vk::AccessFlagBits::eMemoryRead,
            vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite,
            vk::DependencyFlagBits::eByRegion},
        // Subpass 0: Render shaded polygons to color+depth with fixed function alpha compositing
        vk::SubpassDependency{
            0,
            1,
            vk::PipelineStageFlagBits::eColorAttachmentOutput,
            vk::PipelineStageFlagBits::eColorAttachmentOutput,
            vk::AccessFlagBits::eColorAttachmentWrite,
            vk::AccessFlagBits::eColorAttachmentRead,
            vk::DependencyFlagBits::eByRegion},
        // Subpass 1: Render texture mapped polygons to color+depth with fixed function alpha compositing
        vk::SubpassDependency{
            1,
            2,
            vk::PipelineStageFlagBits::eColorAttachmentOutput,
            vk::PipelineStageFlagBits::eColorAttachmentOutput,
            vk::AccessFlagBits::eColorAttachmentWrite,
            vk::AccessFlagBits::eColorAttachmentRead,
            vk::DependencyFlagBits::eByRegion},
        // Subpass 2: Render SDF-texture mapped polygons to color+depth with fixed function alpha compositing
        vk::SubpassDependency{
            2,
            3,
            vk::PipelineStageFlagBits::eColorAttachmentOutput,
            vk::PipelineStageFlagBits::eFragmentShader,
            vk::AccessFlagBits::eColorAttachmentWrite,
            vk::AccessFlagBits::eShaderRead | vk::AccessFlagBits::eInputAttachmentRead,
            vk::DependencyFlagBits::eByRegion},
        // Subpass 3: Render alpha polygons to color+depth with alpha override
        vk::SubpassDependency{
            3,
            4,
            vk::PipelineStageFlagBits::eColorAttachmentOutput,
            vk::PipelineStageFlagBits::eFragmentShader,
            vk::AccessFlagBits::eColorAttachmentWrite,
            vk::AccessFlagBits::eShaderRead | vk::AccessFlagBits::eInputAttachmentRead,
            vk::DependencyFlagBits::eByRegion},
        // Subpass 4: Tone mapping color to swapchain.
        vk::SubpassDependency{
            4,
            VK_SUBPASS_EXTERNAL,
            vk::PipelineStageFlagBits::eColorAttachmentOutput,
            vk::PipelineStageFlagBits::eBottomOfPipe,
            vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite,
            vk::AccessFlagBits::eMemoryRead,
            vk::DependencyFlagBits::eByRegion}};

    vk::RenderPassCreateInfo const render_pass_create_info = {
        vk::RenderPassCreateFlags(),
        narrow_cast<uint32_t>(attachment_descriptions.size()), // attachmentCount
        attachment_descriptions.data(), // attachments
        narrow_cast<uint32_t>(subpass_descriptions.size()), // subpassCount
        subpass_descriptions.data(), // subpasses
        narrow_cast<uint32_t>(subpass_dependency.size()), // dependencyCount
        subpass_dependency.data() // dependencies
    };

    renderPass = _device->createRenderPass(render_pass_create_info);
    hilet granularity = _device->getRenderAreaGranularity(renderPass);
    _render_area_granularity = extent2{narrow_cast<float>(granularity.width), narrow_cast<float>(granularity.height)};
}

void gfx_surface::teardown_render_passes()
{
    hi_axiom(gfx_system_mutex.recurse_lock_count());

    _device->destroy(renderPass);
}

void gfx_surface::build_semaphores()
{
    hi_axiom(gfx_system_mutex.recurse_lock_count());

    imageAvailableSemaphore = _device->createSemaphore();
    renderFinishedSemaphore = _device->createSemaphore();

    // This fence is used to wait for the Window and its Pipelines to be idle.
    // It should therefor be signed at the start so that when no rendering has been
    // done it is still idle.
    renderFinishedFence = _device->createFence({vk::FenceCreateFlagBits::eSignaled});
}

void gfx_surface::teardown_semaphores()
{
    hi_axiom(gfx_system_mutex.recurse_lock_count());

    _device->destroy(renderFinishedSemaphore);
    _device->destroy(imageAvailableSemaphore);
    _device->destroy(renderFinishedFence);
}

void gfx_surface::build_command_buffers()
{
    hi_axiom(gfx_system_mutex.recurse_lock_count());

    hilet commandBuffers = _device->allocateCommandBuffers({_graphics_queue->command_pool, vk::CommandBufferLevel::ePrimary, 1});

    commandBuffer = commandBuffers.at(0);
}

void gfx_surface::teardown_command_buffers()
{
    hi_axiom(gfx_system_mutex.recurse_lock_count());
    hilet commandBuffers = std::vector<vk::CommandBuffer>{commandBuffer};

    _device->freeCommandBuffers(_graphics_queue->command_pool, commandBuffers);
}

[[nodiscard]] std::unique_ptr<gfx_surface> make_unique_gfx_surface(os_handle instance, void *os_window)
{
    hilet lock = std::scoped_lock(gfx_system_mutex);

    auto surface_create_info = vk::Win32SurfaceCreateInfoKHR{
        vk::Win32SurfaceCreateFlagsKHR(), reinterpret_cast<HINSTANCE>(instance), reinterpret_cast<HWND>(os_window)};

    auto vulkan_surface = vulkan_instance().createWin32SurfaceKHR(surface_create_info);

    auto surface = std::make_unique<gfx_surface>(vulkan_surface);

    // Now that we have a physical window and render surface it is time to find the gfx-device
    // for rendering on this surface.
    auto device = find_best_device(*surface);
    if (not device) {
        throw gfx_error("Could not find a vulkan-device matching this surface");
    }
    surface->set_device(device);

    return surface;
}

} // namespace hi::inline v1
