/*
 * Vulkan Example - Basic indexed triangle rendering
 *
 * Note:
 *	This is a "pedal to the metal" example to show off how to get Vulkan up and displaying something
 *	Contrary to the other examples, this one won't make use of helper functions or initializers
 *	Except in a few cases (swap chain setup e.g.)
 *
 * Copyright (C) 2016-2017 by Sascha Willems - www.saschawillems.de
 *
 * This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
 *
 * Some of the code was modified so that it can be used to draw inside HikoGUI.
 * Copyright (C) 2022 by Take Vos
 */

#include "triangle.hpp"
#include "hikogui/hikogui.hpp"
#include <format>
#include <vector>
#include <exception>
#include <cassert>
#include <iostream>

#define VK_CHECK_RESULT(f) \
    do { \
        VkResult res = (f); \
        if (res != VK_SUCCESS) { \
            std::cerr << std::format("Vulkan error {}", std::to_underlying(res)) << std::endl; \
            std::terminate(); \
        } \
    } while (false)

[[nodiscard]] constexpr bool operator==(VkRect2D const& lhs, VkRect2D const& rhs) noexcept
{
    return lhs.offset.x == rhs.offset.x and lhs.offset.y == rhs.offset.y and lhs.extent.width == rhs.extent.width and
        lhs.extent.height == rhs.extent.height;
}

[[nodiscard]] constexpr VkRect2D operator&(VkRect2D const& lhs, VkRect2D const& rhs) noexcept
{
    auto left = std::max(lhs.offset.x, rhs.offset.x);
    auto right = std::min(lhs.offset.x + lhs.extent.width, rhs.offset.x + rhs.extent.width);
    auto top = std::max(lhs.offset.y, rhs.offset.y);
    auto bottom = std::min(lhs.offset.y + lhs.extent.height, rhs.offset.y + rhs.extent.height);

    auto width = hi::narrow_cast<int32_t>(right) > left ? right - left : uint32_t{0};
    auto height = hi::narrow_cast<int32_t>(bottom) > top ? bottom - top : uint32_t{0};
    return {VkOffset2D{left, top}, VkExtent2D{width, height}};
}

TriangleExample::TriangleExample(VmaAllocator allocator, VkDevice device, VkQueue queue, uint32_t queueFamilyIndex) :
    allocator(allocator), device(device), queue(queue), queueFamilyIndex(queueFamilyIndex)
{
    createCommandPool();
    createVertexBuffer();
    createUniformBuffer();
    createDescriptorPool();
    createDescriptorSetLayout();
    createDescriptorSet();
}

TriangleExample::~TriangleExample()
{
    if (hasSwapchain) {
        teardownForLostSwapchain();
    }

    destroyDescriptorSet();
    destroyDescriptorSetLayout();
    destroyDescriptorPool();
    destroyUniformBuffer();
    destroyVertexBuffer();
    destroyCommandPool();
}

void TriangleExample::createCommandPool()
{
    VkCommandPoolCreateInfo cmdPoolInfo = {};
    cmdPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    cmdPoolInfo.queueFamilyIndex = queueFamilyIndex;
    cmdPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    VK_CHECK_RESULT(vkCreateCommandPool(device, &cmdPoolInfo, nullptr, &cmdPool));
}

void TriangleExample::destroyCommandPool()
{
    vkDestroyCommandPool(device, cmdPool, nullptr);
}

void TriangleExample::createVertexBuffer()
{
    // Setup vertices
    std::vector<Vertex> vertexData = {
        {{1.0f, 1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}},
        {{-1.0f, 1.0f, 0.0f}, {0.0f, 1.0f, 0.0f}},
        {{0.0f, -1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}}};
    uint32_t vertexDataSize = hi::narrow_cast<uint32_t>(vertexData.size()) * sizeof(Vertex);

    // Setup indices
    std::vector<uint32_t> vertexIndexData = {0, 1, 2};
    vertexIndexCount = hi::narrow_cast<uint32_t>(vertexIndexData.size());
    uint32_t vertexIndexDataSize = vertexIndexCount * sizeof(uint32_t);

    // Create the Vertex buffer inside the GPU
    VkBufferCreateInfo vertexBufferCreateInfo = {};
    vertexBufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    vertexBufferCreateInfo.size = vertexDataSize;
    vertexBufferCreateInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;

    VmaAllocationCreateInfo vertexBufferAllocationInfo = {};
    vertexBufferAllocationInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
    vertexBufferAllocationInfo.usage = VMA_MEMORY_USAGE_AUTO;

    VK_CHECK_RESULT(vmaCreateBuffer(
        allocator, &vertexBufferCreateInfo, &vertexBufferAllocationInfo, &vertexBuffer, &vertexBufferAllocation, nullptr));

    // Copy vertex data to a buffer visible to the host
    {
        void *data = nullptr;
        VK_CHECK_RESULT(vmaMapMemory(allocator, vertexBufferAllocation, &data));
        memcpy(data, vertexData.data(), vertexDataSize);
        vmaUnmapMemory(allocator, vertexBufferAllocation);
    }

    // Create the Vertex-index buffer inside the GPU
    VkBufferCreateInfo vertexIndexBufferCreateInfo = {};
    vertexIndexBufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    vertexIndexBufferCreateInfo.size = vertexIndexDataSize;
    vertexIndexBufferCreateInfo.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;

    VmaAllocationCreateInfo vertexIndexBufferAllocationInfo = {};
    vertexIndexBufferAllocationInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
    vertexIndexBufferAllocationInfo.usage = VMA_MEMORY_USAGE_AUTO;

    VK_CHECK_RESULT(vmaCreateBuffer(
        allocator,
        &vertexIndexBufferCreateInfo,
        &vertexIndexBufferAllocationInfo,
        &vertexIndexBuffer,
        &vertexIndexBufferAllocation,
        nullptr));

    // Copy index data to a buffer visible to the host
    {
        void *data = nullptr;
        VK_CHECK_RESULT(vmaMapMemory(allocator, vertexIndexBufferAllocation, &data));
        memcpy(data, vertexIndexData.data(), vertexIndexDataSize);
        vmaUnmapMemory(allocator, vertexIndexBufferAllocation);
    }
}

void TriangleExample::createUniformBuffer()
{
    // Vertex shader uniform buffer block
    VkBufferCreateInfo uniformBufferCreateInfo = {};
    uniformBufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    uniformBufferCreateInfo.size = sizeof(Uniform);
    uniformBufferCreateInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;

    VmaAllocationCreateInfo uniformBufferAllocationInfo = {};
    uniformBufferAllocationInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
    uniformBufferAllocationInfo.usage = VMA_MEMORY_USAGE_AUTO;

    VK_CHECK_RESULT(vmaCreateBuffer(
        allocator, &uniformBufferCreateInfo, &uniformBufferAllocationInfo, &uniformBuffer, &uniformBufferAllocation, nullptr));

    // Store information in the uniform's descriptor that is used by the descriptor set.
    uniformBufferInfo.buffer = uniformBuffer;
    uniformBufferInfo.offset = 0;
    uniformBufferInfo.range = sizeof(Uniform);
}

void TriangleExample::destroyUniformBuffer()
{
    vmaDestroyBuffer(allocator, uniformBuffer, uniformBufferAllocation);
}

void TriangleExample::destroyVertexBuffer()
{
    vmaDestroyBuffer(allocator, vertexIndexBuffer, vertexIndexBufferAllocation);
    vmaDestroyBuffer(allocator, vertexBuffer, vertexBufferAllocation);
}

void TriangleExample::createDescriptorPool()
{
    // We need to tell the API the number of max. requested descriptors per type
    VkDescriptorPoolSize typeCounts[1];
    // This example only uses one descriptor type (uniform buffer) and only requests one descriptor of this type
    typeCounts[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    typeCounts[0].descriptorCount = 1;
    // For additional types you need to add new entries in the type count list
    // E.g. for two combined image samplers :
    // typeCounts[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    // typeCounts[1].descriptorCount = 2;

    // Create the global descriptor pool
    // All descriptors used in this example are allocated from this pool
    VkDescriptorPoolCreateInfo descriptorPoolInfo = {};
    descriptorPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    descriptorPoolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    descriptorPoolInfo.pNext = nullptr;
    descriptorPoolInfo.poolSizeCount = 1;
    descriptorPoolInfo.pPoolSizes = typeCounts;
    // Set the max. number of descriptor sets that can be requested from this pool (requesting beyond this limit will result
    // in an error)
    descriptorPoolInfo.maxSets = 1;

    VK_CHECK_RESULT(vkCreateDescriptorPool(device, &descriptorPoolInfo, nullptr, &descriptorPool));
}

void TriangleExample::destroyDescriptorPool()
{
    vkDestroyDescriptorPool(device, descriptorPool, nullptr);
}

void TriangleExample::createDescriptorSetLayout()
{
    // Setup layout of descriptors used in this example
    // Basically connects the different shader stages to descriptors for binding uniform buffers, image samplers, etc.
    // So every shader binding should map to one descriptor set layout binding

    // Binding 0: Uniform buffer (Vertex shader)
    VkDescriptorSetLayoutBinding layoutBinding = {};
    layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    layoutBinding.descriptorCount = 1;
    layoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    layoutBinding.pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutCreateInfo descriptorLayout = {};
    descriptorLayout.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    descriptorLayout.pNext = nullptr;
    descriptorLayout.bindingCount = 1;
    descriptorLayout.pBindings = &layoutBinding;

    VK_CHECK_RESULT(vkCreateDescriptorSetLayout(device, &descriptorLayout, nullptr, &descriptorSetLayout));

    // Create the pipeline layout that is used to generate the rendering pipelines that are based on this descriptor set
    // layout In a more complex scenario you would have different pipeline layouts for different descriptor set layouts that
    // could be reused
    VkPipelineLayoutCreateInfo pPipelineLayoutCreateInfo = {};
    pPipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pPipelineLayoutCreateInfo.pNext = nullptr;
    pPipelineLayoutCreateInfo.setLayoutCount = 1;
    pPipelineLayoutCreateInfo.pSetLayouts = &descriptorSetLayout;

    VK_CHECK_RESULT(vkCreatePipelineLayout(device, &pPipelineLayoutCreateInfo, nullptr, &pipelineLayout));
}

void TriangleExample::destroyDescriptorSetLayout()
{
    vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
    vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
}

void TriangleExample::createDescriptorSet()
{
    // Allocate a new descriptor set from the global descriptor pool
    VkDescriptorSetAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descriptorPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &descriptorSetLayout;

    VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &allocInfo, &descriptorSet));

    // Update the descriptor set determining the shader binding points
    // For every binding point used in a shader there needs to be one
    // descriptor set matching that binding point

    VkWriteDescriptorSet writeDescriptorSet = {};

    // Binding 0 : Uniform buffer
    writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet.dstSet = descriptorSet;
    writeDescriptorSet.descriptorCount = 1;
    writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    writeDescriptorSet.pBufferInfo = &uniformBufferInfo;
    // Binds this uniform buffer to binding point 0
    writeDescriptorSet.dstBinding = 0;

    vkUpdateDescriptorSets(device, 1, &writeDescriptorSet, 0, nullptr);
}

void TriangleExample::destroyDescriptorSet()
{
    vkFreeDescriptorSets(device, descriptorPool, 1, &descriptorSet);
}

void TriangleExample::buildForNewSwapchain(std::vector<VkImageView> const& imageViews, VkExtent2D imageSize, VkFormat imageFormat)
{
    assert(not hasSwapchain);

    auto colorImageFormat = imageFormat;
    auto depthImageFormat = VK_FORMAT_D24_UNORM_S8_UINT;

    createRenderPass(colorImageFormat, depthImageFormat);
    createDepthStencilImage(imageSize, depthImageFormat);
    createFrameBuffers(imageViews, imageSize);
    createCommandBuffers();
    createFences();
    createPipeline();

    hasSwapchain = true;
    previousRenderArea = {};
    previousViewPort = {};
}

void TriangleExample::teardownForLostSwapchain()
{
    assert(hasSwapchain);

    for (auto& fence : queueCompleteFences) {
        VK_CHECK_RESULT(vkWaitForFences(device, 1, &fence, VK_TRUE, UINT64_MAX));
    }

    hasSwapchain = false;

    destroyPipeline();
    destroyFences();
    destroyCommandBuffers();
    destroyFrameBuffers();
    destroyDepthStencilImage();
    destroyRenderPass();
}

void TriangleExample::createRenderPass(VkFormat colorFormat, VkFormat depthFormat)
{
    // This example will use a single render pass with one subpass

    // Descriptors for the attachments used by this renderpass
    std::array<VkAttachmentDescription, 2> attachments = {};

    // Color attachment
    // In HikoGUI we reuse the previous drawn swap-chain image, therefor:
    // - initialLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
    //
    // Although the loadOp is VK_ATTACHMENT_LOAD_OP_CLEAR, it only clears the renderArea/scissor rectangle.
    // The initialLayout makes sure that the previous image is reused.
    attachments[0].format = colorFormat; // Use the color format selected by the swapchain
    attachments[0].samples = VK_SAMPLE_COUNT_1_BIT; // We don't use multi sampling in this example
    attachments[0].loadOp =
        VK_ATTACHMENT_LOAD_OP_CLEAR; // Clear this attachment at the start of the render pass.
    attachments[0].storeOp =
        VK_ATTACHMENT_STORE_OP_STORE; // Keep its contents after the render pass is finished (for displaying it)
    attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE; // We don't use stencil, so don't care for load
    attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE; // Same for store
    attachments[0].initialLayout =
        VK_IMAGE_LAYOUT_PRESENT_SRC_KHR; // Reuse the previous draw image, so the layout is already in present mode.
    attachments[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR; // Layout to which the attachment is transitioned when the
                                                                  // render pass is finished As we want to present the color
                                                                  // buffer to the swapchain, we transition to PRESENT_KHR
    // Depth attachment
    attachments[1].format = depthFormat; // A proper depth format is selected in the example base
    attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
    attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR; // Clear depth at start of first subpass
    attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE; // We don't need depth after render pass has finished
                                                               // (DONT_CARE may result in better performance)
    attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE; // No stencil
    attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE; // No Stencil
    attachments[1].initialLayout =
        VK_IMAGE_LAYOUT_UNDEFINED; // Layout at render pass start. Initial doesn't matter, so we use undefined
    attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL; // Transition to depth/stencil attachment

    // Setup attachment references
    VkAttachmentReference colorReference = {};
    colorReference.attachment = 0; // Attachment 0 is color
    colorReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL; // Attachment layout used as color during the subpass

    VkAttachmentReference depthReference = {};
    depthReference.attachment = 1; // Attachment 1 is color
    depthReference.layout =
        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL; // Attachment used as depth/stencil used during the subpass

    // Setup a single subpass reference
    VkSubpassDescription subpassDescription = {};
    subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpassDescription.colorAttachmentCount = 1; // Subpass uses one color attachment
    subpassDescription.pColorAttachments = &colorReference; // Reference to the color attachment in slot 0
    subpassDescription.pDepthStencilAttachment = &depthReference; // Reference to the depth attachment in slot 1
    subpassDescription.inputAttachmentCount = 0; // Input attachments can be used to sample from contents of a previous subpass
    subpassDescription.pInputAttachments = nullptr; // (Input attachments not used by this example)
    subpassDescription.preserveAttachmentCount =
        0; // Preserved attachments can be used to loop (and preserve) attachments through subpasses
    subpassDescription.pPreserveAttachments = nullptr; // (Preserve attachments not used by this example)
    subpassDescription.pResolveAttachments =
        nullptr; // Resolve attachments are resolved at the end of a sub pass and can be used for e.g. multi sampling

    // Setup subpass dependencies
    // These will add the implicit attachment layout transitions specified by the attachment descriptions
    // The actual usage layout is preserved through the layout specified in the attachment reference
    // Each subpass dependency will introduce a memory and execution dependency between the source and dest subpass described
    // by srcStageMask, dstStageMask, srcAccessMask, dstAccessMask (and dependencyFlags is set) Note: VK_SUBPASS_EXTERNAL is a
    // special constant that refers to all commands executed outside of the actual renderpass)
    std::array<VkSubpassDependency, 2> dependencies;

    // First dependency at the start of the renderpass
    // Does the transition from final to initial layout
    dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL; // Producer of the dependency
    dependencies[0].dstSubpass = 0; // Consumer is our single subpass that will wait for the execution dependency
    dependencies[0].srcStageMask =
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT; // Match our pWaitDstStageMask when we vkQueueSubmit
    dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT; // is a loadOp stage for color attachments
    dependencies[0].srcAccessMask = 0; // semaphore wait already does memory dependency for us
    dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT; // is a loadOp CLEAR access mask for color attachments
    dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    // Second dependency at the end the renderpass
    // Does the transition from the initial to the final layout
    // Technically this is the same as the implicit subpass dependency, but we are gonna state it explicitly here
    dependencies[1].srcSubpass = 0; // Producer of the dependency is our single subpass
    dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL; // Consumer are all commands outside of the renderpass
    dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT; // is a storeOp stage for color attachments
    dependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT; // Do not block any subsequent work
    dependencies[1].srcAccessMask =
        VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT; // is a storeOp `STORE` access mask for color attachments
    dependencies[1].dstAccessMask = 0;
    dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    // Create the actual renderpass
    VkRenderPassCreateInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount =
        hi::narrow_cast<uint32_t>(attachments.size()); // Number of attachments used by this render pass
    renderPassInfo.pAttachments = attachments.data(); // Descriptions of the attachments used by the render pass
    renderPassInfo.subpassCount = 1; // We only use one subpass in this example
    renderPassInfo.pSubpasses = &subpassDescription; // Description of that subpass
    renderPassInfo.dependencyCount = hi::narrow_cast<uint32_t>(dependencies.size()); // Number of subpass dependencies
    renderPassInfo.pDependencies = dependencies.data(); // Subpass dependencies used by the render pass

    VK_CHECK_RESULT(vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass));
}

void TriangleExample::destroyRenderPass()
{
    vkDestroyRenderPass(device, renderPass, nullptr);
}

void TriangleExample::createDepthStencilImage(VkExtent2D imageSize, VkFormat format)
{
    VkImageCreateInfo depthImageCreateInfo = {};
    depthImageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    depthImageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
    depthImageCreateInfo.format = format;
    depthImageCreateInfo.extent.width = imageSize.width;
    depthImageCreateInfo.extent.height = imageSize.height;
    depthImageCreateInfo.extent.depth = 1;
    depthImageCreateInfo.mipLevels = 1;
    depthImageCreateInfo.arrayLayers = 1;
    depthImageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    depthImageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    depthImageCreateInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    depthImageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    depthImageCreateInfo.queueFamilyIndexCount = 0;
    depthImageCreateInfo.pQueueFamilyIndices = nullptr;
    depthImageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    VmaAllocationCreateInfo depthAllocationCreateInfo = {};
    depthAllocationCreateInfo.usage = VMA_MEMORY_USAGE_AUTO;
    VK_CHECK_RESULT(vmaCreateImage(
        allocator, &depthImageCreateInfo, &depthAllocationCreateInfo, &depthImage, &depthImageAllocation, nullptr));

    VkImageViewCreateInfo depthImageViewCreateInfo = {};
    depthImageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    depthImageViewCreateInfo.image = depthImage;
    depthImageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    depthImageViewCreateInfo.format = format;
    depthImageViewCreateInfo.components = VkComponentMapping{};
    depthImageViewCreateInfo.subresourceRange = VkImageSubresourceRange{VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 1};

    VK_CHECK_RESULT(vkCreateImageView(device, &depthImageViewCreateInfo, nullptr, &depthImageView));
}

void TriangleExample::destroyDepthStencilImage()
{
    vkDestroyImageView(device, depthImageView, nullptr);
    vmaDestroyImage(allocator, depthImage, depthImageAllocation);
}

// Create a frame buffer for each swap chain image
// Note: Override of virtual function in the base class and called from within TriangleExampleBase::prepare
void TriangleExample::createFrameBuffers(std::vector<VkImageView> const& swapChainImageViews, VkExtent2D imageSize)
{
    // Create a frame buffer for every image in the swapchain
    assert(frameBuffers.size() <= swapChainImageViews.size());
    frameBuffers.resize(swapChainImageViews.size());
    for (size_t i = 0; i < frameBuffers.size(); i++) {
        std::array<VkImageView, 2> attachments;
        attachments[0] = swapChainImageViews[i]; // Color attachment is the view of the swapchain image
        attachments[1] = depthImageView; // Depth/Stencil attachment is the same for all frame buffers

        VkFramebufferCreateInfo frameBufferCreateInfo = {};
        frameBufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        // All frame buffers use the same renderpass setup
        frameBufferCreateInfo.renderPass = renderPass;
        frameBufferCreateInfo.attachmentCount = hi::narrow_cast<uint32_t>(attachments.size());
        frameBufferCreateInfo.pAttachments = attachments.data();
        frameBufferCreateInfo.width = imageSize.width;
        frameBufferCreateInfo.height = imageSize.height;
        frameBufferCreateInfo.layers = 1;
        // Create the framebuffer
        VK_CHECK_RESULT(vkCreateFramebuffer(device, &frameBufferCreateInfo, nullptr, &frameBuffers[i]));
    }
}

void TriangleExample::destroyFrameBuffers()
{
    for (auto const& frameBuffer : frameBuffers) {
        vkDestroyFramebuffer(device, frameBuffer, nullptr);
    }
}

void TriangleExample::createCommandBuffers()
{
    // Create one command buffer for each swap chain image and reuse for rendering
    drawCmdBuffers.resize(frameBuffers.size());

    auto cmdBufAllocateInfo = VkCommandBufferAllocateInfo{};
    cmdBufAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmdBufAllocateInfo.commandPool = cmdPool;
    cmdBufAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmdBufAllocateInfo.commandBufferCount = hi::narrow_cast<uint32_t>(drawCmdBuffers.size());

    VK_CHECK_RESULT(vkAllocateCommandBuffers(device, &cmdBufAllocateInfo, drawCmdBuffers.data()));
}

void TriangleExample::destroyCommandBuffers()
{
    vkFreeCommandBuffers(device, cmdPool, hi::narrow_cast<uint32_t>(drawCmdBuffers.size()), drawCmdBuffers.data());
}

void TriangleExample::createFences()
{
    // Fences (Used to check draw command buffer completion)
    VkFenceCreateInfo fenceCreateInfo = {};
    fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    // Create in signaled state so we don't wait on first render of each command buffer
    fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    queueCompleteFences.resize(drawCmdBuffers.size());
    for (auto& fence : queueCompleteFences) {
        VK_CHECK_RESULT(vkCreateFence(device, &fenceCreateInfo, nullptr, &fence));
    }
}

void TriangleExample::destroyFences()
{
    for (auto fence : queueCompleteFences) {
        vkDestroyFence(device, fence, nullptr);
    }
}

void TriangleExample::createPipeline()
{
    // Create the graphics pipeline used in this example
    // Vulkan uses the concept of rendering pipelines to encapsulate fixed states, replacing OpenGL's complex state machine
    // A pipeline is then stored and hashed on the GPU making pipeline changes very fast
    // Note: There are still a few dynamic states that are not directly part of the pipeline (but the info that they are used
    // is)

    VkGraphicsPipelineCreateInfo pipelineCreateInfo = {};
    pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    // The layout used for this pipeline (can be shared among multiple pipelines using the same layout)
    pipelineCreateInfo.layout = pipelineLayout;
    // Renderpass this pipeline is attached to
    pipelineCreateInfo.renderPass = renderPass;

    // Construct the different states making up the pipeline

    // Input assembly state describes how primitives are assembled
    // This pipeline will assemble vertex data as a triangle lists (though we only use one triangle)
    VkPipelineInputAssemblyStateCreateInfo inputAssemblyState = {};
    inputAssemblyState.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssemblyState.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    // Rasterization state
    VkPipelineRasterizationStateCreateInfo rasterizationState = {};
    rasterizationState.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizationState.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizationState.cullMode = VK_CULL_MODE_NONE;
    rasterizationState.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizationState.depthClampEnable = VK_FALSE;
    rasterizationState.rasterizerDiscardEnable = VK_FALSE;
    rasterizationState.depthBiasEnable = VK_FALSE;
    rasterizationState.lineWidth = 1.0f;

    // Color blend state describes how blend factors are calculated (if used)
    // We need one blend attachment state per color attachment (even if blending is not used)
    VkPipelineColorBlendAttachmentState blendAttachmentState[1] = {};
    blendAttachmentState[0].colorWriteMask = 0xf;
    blendAttachmentState[0].blendEnable = VK_FALSE;
    VkPipelineColorBlendStateCreateInfo colorBlendState = {};
    colorBlendState.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlendState.attachmentCount = 1;
    colorBlendState.pAttachments = blendAttachmentState;

    // Viewport state sets the number of viewports and scissor used in this pipeline
    // Note: This is actually overridden by the dynamic states (see below)
    VkPipelineViewportStateCreateInfo viewportState = {};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    // Enable dynamic states
    // Most states are baked into the pipeline, but there are still a few dynamic states that can be changed within a command
    // buffer To be able to change these we need do specify which dynamic states will be changed using this pipeline. Their
    // actual states are set later on in the command buffer. For this example we will set the viewport and scissor using
    // dynamic states
    std::vector<VkDynamicState> dynamicStateEnables;
    dynamicStateEnables.push_back(VK_DYNAMIC_STATE_VIEWPORT);
    dynamicStateEnables.push_back(VK_DYNAMIC_STATE_SCISSOR);
    VkPipelineDynamicStateCreateInfo dynamicState = {};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.pDynamicStates = dynamicStateEnables.data();
    dynamicState.dynamicStateCount = hi::narrow_cast<uint32_t>(dynamicStateEnables.size());

    // Depth and stencil state containing depth and stencil compare and test operations
    // We only use depth tests and want depth tests and writes to be enabled and compare with less or equal
    VkPipelineDepthStencilStateCreateInfo depthStencilState = {};
    depthStencilState.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencilState.depthTestEnable = VK_TRUE;
    depthStencilState.depthWriteEnable = VK_TRUE;
    depthStencilState.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
    depthStencilState.depthBoundsTestEnable = VK_FALSE;
    depthStencilState.back.failOp = VK_STENCIL_OP_KEEP;
    depthStencilState.back.passOp = VK_STENCIL_OP_KEEP;
    depthStencilState.back.compareOp = VK_COMPARE_OP_ALWAYS;
    depthStencilState.stencilTestEnable = VK_FALSE;
    depthStencilState.front = depthStencilState.back;

    // Multi sampling state
    // This example does not make use of multi sampling (for anti-aliasing), the state must still be set and passed to the
    // pipeline
    VkPipelineMultisampleStateCreateInfo multisampleState = {};
    multisampleState.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampleState.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisampleState.pSampleMask = nullptr;

    // Vertex input descriptions
    // Specifies the vertex input parameters for a pipeline

    // Vertex input binding
    // This example uses a single vertex input binding at binding point 0 (see vkCmdBindVertexBuffers)
    VkVertexInputBindingDescription vertexInputBinding = {};
    vertexInputBinding.binding = 0;
    vertexInputBinding.stride = sizeof(Vertex);
    vertexInputBinding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    // Input attribute bindings describe shader attribute locations and memory layouts
    std::array<VkVertexInputAttributeDescription, 2> vertexInputAttributes;
    // These match the following shader layout (see triangle.vert):
    //	layout (location = 0) in vec3 inPos;
    //	layout (location = 1) in vec3 inColor;
    // Attribute location 0: Position
    vertexInputAttributes[0].binding = 0;
    vertexInputAttributes[0].location = 0;
    // Position attribute is three 32 bit signed (SFLOAT) floats (R32 G32 B32)
    vertexInputAttributes[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    vertexInputAttributes[0].offset = offsetof(Vertex, position);
    // Attribute location 1: Color
    vertexInputAttributes[1].binding = 0;
    vertexInputAttributes[1].location = 1;
    // Color attribute is three 32 bit signed (SFLOAT) floats (R32 G32 B32)
    vertexInputAttributes[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    vertexInputAttributes[1].offset = offsetof(Vertex, color);

    // Vertex input state used for pipeline creation
    VkPipelineVertexInputStateCreateInfo vertexInputState = {};
    vertexInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputState.vertexBindingDescriptionCount = 1;
    vertexInputState.pVertexBindingDescriptions = &vertexInputBinding;
    vertexInputState.vertexAttributeDescriptionCount = 2;
    vertexInputState.pVertexAttributeDescriptions = vertexInputAttributes.data();

    // Shaders
    std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages{};

    // Vertex shader
    shaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    // Set pipeline stage for this shader
    shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    // Load binary SPIR-V shader
    shaderStages[0].module = loadSPIRVShader(hi::URL{"resource:triangle.vert.spv"});
    // Main entry point for the shader
    shaderStages[0].pName = "main";
    assert(shaderStages[0].module != VK_NULL_HANDLE);

    // Fragment shader
    shaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    // Set pipeline stage for this shader
    shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    // Load binary SPIR-V shader
    shaderStages[1].module = loadSPIRVShader(hi::URL{"resource:triangle.frag.spv"});
    // Main entry point for the shader
    shaderStages[1].pName = "main";
    assert(shaderStages[1].module != VK_NULL_HANDLE);

    // Set pipeline shader stage info
    pipelineCreateInfo.stageCount = hi::narrow_cast<uint32_t>(shaderStages.size());
    pipelineCreateInfo.pStages = shaderStages.data();

    // Assign the pipeline states to the pipeline creation info structure
    pipelineCreateInfo.pVertexInputState = &vertexInputState;
    pipelineCreateInfo.pInputAssemblyState = &inputAssemblyState;
    pipelineCreateInfo.pRasterizationState = &rasterizationState;
    pipelineCreateInfo.pColorBlendState = &colorBlendState;
    pipelineCreateInfo.pMultisampleState = &multisampleState;
    pipelineCreateInfo.pViewportState = &viewportState;
    pipelineCreateInfo.pDepthStencilState = &depthStencilState;
    pipelineCreateInfo.pDynamicState = &dynamicState;

    // Create rendering pipeline using the specified states
    VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &pipeline));

    // Shader modules are no longer needed once the graphics pipeline has been created
    vkDestroyShaderModule(device, shaderStages[0].module, nullptr);
    vkDestroyShaderModule(device, shaderStages[1].module, nullptr);
}

void TriangleExample::destroyPipeline()
{
    vkDestroyPipeline(device, pipeline, nullptr);
}

// Get a new command buffer from the command pool
// If begin is true, the command buffer is also started so we can start adding commands
VkCommandBuffer TriangleExample::getCommandBuffer(bool begin)
{
    VkCommandBuffer cmdBuffer;

    VkCommandBufferAllocateInfo cmdBufAllocateInfo = {};
    cmdBufAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmdBufAllocateInfo.commandPool = cmdPool;
    cmdBufAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmdBufAllocateInfo.commandBufferCount = 1;

    VK_CHECK_RESULT(vkAllocateCommandBuffers(device, &cmdBufAllocateInfo, &cmdBuffer));

    // If requested, also start the new command buffer
    if (begin) {
        auto cmdBufInfo = VkCommandBufferBeginInfo{};
        cmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

        VK_CHECK_RESULT(vkBeginCommandBuffer(cmdBuffer, &cmdBufInfo));
    }

    return cmdBuffer;
}

// End the command buffer and submit it to the queue
// Uses a fence to ensure command buffer has finished executing before deleting it
void TriangleExample::flushCommandBuffer(VkCommandBuffer commandBuffer)
{
    assert(commandBuffer != VK_NULL_HANDLE);

    VK_CHECK_RESULT(vkEndCommandBuffer(commandBuffer));

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    // Create fence to ensure that the command buffer has finished executing
    VkFenceCreateInfo fenceCreateInfo = {};
    fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceCreateInfo.flags = 0;
    VkFence fence;
    VK_CHECK_RESULT(vkCreateFence(device, &fenceCreateInfo, nullptr, &fence));

    // Submit to the queue
    VK_CHECK_RESULT(vkQueueSubmit(queue, 1, &submitInfo, fence));
    // Wait for the fence to signal that command buffer has finished executing
    VK_CHECK_RESULT(vkWaitForFences(device, 1, &fence, VK_TRUE, 100'000'000'000));

    vkDestroyFence(device, fence, nullptr);
    vkFreeCommandBuffers(device, cmdPool, 1, &commandBuffer);
}

void TriangleExample::buildCommandBuffers(VkRect2D renderArea, VkRect2D viewPort)
{
    VkCommandBufferBeginInfo cmdBufInfo = {};
    cmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    cmdBufInfo.pNext = nullptr;

    // Set clear values for all framebuffer attachments with loadOp set to clear
    // We use two attachments (color and depth) that are cleared at the start of the subpass and as such we need to set clear
    // values for both
    VkClearValue clearValues[2];
    clearValues[0].color = {{0.0f, 0.0f, 0.2f, 1.0f}};
    clearValues[1].depthStencil = {1.0f, 0};

    VkRenderPassBeginInfo renderPassBeginInfo = {};
    renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassBeginInfo.pNext = nullptr;
    renderPassBeginInfo.renderPass = renderPass;
    renderPassBeginInfo.renderArea = renderArea;
    renderPassBeginInfo.clearValueCount = 2;
    renderPassBeginInfo.pClearValues = clearValues;

    for (int32_t i = 0; i < drawCmdBuffers.size(); ++i) {
        // Set target frame buffer
        renderPassBeginInfo.framebuffer = frameBuffers[i];

        VK_CHECK_RESULT(vkBeginCommandBuffer(drawCmdBuffers[i], &cmdBufInfo));

        // Start the first sub pass specified in our default render pass setup by the base class
        // This will clear the color and depth attachment
        vkCmdBeginRenderPass(drawCmdBuffers[i], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

        // Update dynamic viewport state
        VkViewport viewport = {};
        viewport.x = hi::narrow_cast<float>(viewPort.offset.x);
        viewport.y = hi::narrow_cast<float>(viewPort.offset.y);
        viewport.height = hi::narrow_cast<float>(viewPort.extent.height);
        viewport.width = hi::narrow_cast<float>(viewPort.extent.width);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(drawCmdBuffers[i], 0, 1, &viewport);

        // We are not allowed to draw outside of the renderArea, nor outside of the viewPort
        VkRect2D scissor = renderArea & viewPort;
        vkCmdSetScissor(drawCmdBuffers[i], 0, 1, &scissor);

        // Bind descriptor sets describing shader binding points
        vkCmdBindDescriptorSets(
            drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);

        // Bind the rendering pipeline
        // The pipeline (state object) contains all states of the rendering pipeline, binding it will set all the states
        // specified at pipeline creation time
        vkCmdBindPipeline(drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

        // Bind triangle vertex buffer (contains position and colors)
        VkDeviceSize offsets[1] = {0};
        vkCmdBindVertexBuffers(drawCmdBuffers[i], 0, 1, &vertexBuffer, offsets);

        // Bind triangle index buffer
        vkCmdBindIndexBuffer(drawCmdBuffers[i], vertexIndexBuffer, 0, VK_INDEX_TYPE_UINT32);

        // Draw indexed triangle
        vkCmdDrawIndexed(drawCmdBuffers[i], vertexIndexCount, 1, 0, 0, 1);

        vkCmdEndRenderPass(drawCmdBuffers[i]);

        // Ending the render pass will add an implicit barrier transitioning the frame buffer color attachment to
        // VK_IMAGE_LAYOUT_PRESENT_SRC_KHR for presenting it to the windowing system

        VK_CHECK_RESULT(vkEndCommandBuffer(drawCmdBuffers[i]));
    }
}

void TriangleExample::draw(uint32_t currentBuffer, VkSemaphore presentCompleteSemaphore, VkSemaphore renderCompleteSemaphore)
{
    // Use a fence to wait until the command buffer has finished execution before using it again
    VK_CHECK_RESULT(vkWaitForFences(device, 1, &queueCompleteFences[currentBuffer], VK_TRUE, UINT64_MAX));
    VK_CHECK_RESULT(vkResetFences(device, 1, &queueCompleteFences[currentBuffer]));

    // Pipeline stage at which the queue submission will wait (via pWaitSemaphores)
    VkPipelineStageFlags waitStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    // The submit info structure specifies a command buffer queue submission batch
    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.pWaitDstStageMask =
        &waitStageMask; // Pointer to the list of pipeline stages that the semaphore waits will occur at
    submitInfo.waitSemaphoreCount = 1; // One wait semaphore
    submitInfo.signalSemaphoreCount = 1; // One signal semaphore
    submitInfo.pCommandBuffers = &drawCmdBuffers[currentBuffer]; // Command buffers(s) to execute in this batch (submission)
    submitInfo.commandBufferCount = 1; // One command buffer

    // SRS - on other platforms use original bare code with local semaphores/fences for illustrative purposes
    submitInfo.pWaitSemaphores =
        &presentCompleteSemaphore; // Semaphore(s) to wait upon before the submitted command buffer starts executing
    submitInfo.pSignalSemaphores = &renderCompleteSemaphore; // Semaphore(s) to be signaled when command buffers have completed

    // Submit to the graphics queue passing a wait fence
    VK_CHECK_RESULT(vkQueueSubmit(queue, 1, &submitInfo, queueCompleteFences[currentBuffer]));
}

// Vulkan loads its shaders from an immediate binary representation called SPIR-V
// Shaders are compiled offline from e.g. GLSL using the reference glslang compiler
// This function loads such a shader from a binary file and returns a shader module structure
VkShaderModule TriangleExample::loadSPIRVShader(std::filesystem::path filename)
{
    auto view = hi::file_view(filename);
    auto span = as_span<uint32_t const>(view);

    // Create a new shader module that will be used for pipeline creation
    VkShaderModuleCreateInfo moduleCreateInfo{};
    moduleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    moduleCreateInfo.codeSize = hi::narrow_cast<uint32_t>(span.size() * sizeof(uint32_t));
    moduleCreateInfo.pCode = span.data();

    VkShaderModule shaderModule;
    VK_CHECK_RESULT(vkCreateShaderModule(device, &moduleCreateInfo, NULL, &shaderModule));

    return shaderModule;
}

void TriangleExample::updateUniformBuffers(Uniform const& uniform)
{
    // Map uniform buffer and update it
    void *data;

    VK_CHECK_RESULT(vmaMapMemory(allocator, uniformBufferAllocation, &data));
    memcpy(data, &uniform, sizeof(Uniform));
    vmaUnmapMemory(allocator, uniformBufferAllocation);
}

void TriangleExample::render(
    uint32_t currentBuffer,
    VkSemaphore presentCompleteSemaphore,
    VkSemaphore renderCompleteSemaphore,
    VkRect2D renderArea,
    VkRect2D viewPort)
{
    assert(hasSwapchain);

    if (previousViewPort != viewPort) {
        // Setup a default look-at camera
        auto viewPortSize =
            hi::extent2{hi::narrow_cast<float>(viewPort.extent.width), hi::narrow_cast<float>(viewPort.extent.height)};

        auto projection = hi::perspective{hi::to_radian(60.0f), viewPortSize, 1.0f, 256.0f};
        auto view = hi::lookat{hi::point3{0.0f, 0.0f, -3.5f}, hi::point3{}};
        auto model = hi::matrix3{};

        auto projection_m = static_cast<hi::matrix3>(projection);
        auto view_m = static_cast<hi::matrix3>(view);
        auto model_m = static_cast<hi::matrix3>(model);

        // Values not set here are initialized in the base class constructor
        // Pass matrices to the shaders
        // clang-format off
        auto uniform = Uniform{
            reflect<'x', 'y', 'z'>(projection_m),
            reflect<'x', 'y', 'z'>(model_m),
            reflect<'x', 'y', 'Z'>(view_m)};
        // clang-format on
        updateUniformBuffers(uniform);
    }

    if (previousRenderArea != renderArea or previousViewPort != viewPort) {
        buildCommandBuffers(renderArea, viewPort);
    }

    draw(currentBuffer, presentCompleteSemaphore, renderCompleteSemaphore);

    previousRenderArea = renderArea;
    previousViewPort = viewPort;
}
