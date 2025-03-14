// Copyright Take Vos 2019-2022.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

module;
#include "../macros.hpp"

#include <array>
#include <vector>
#include <vulkan/vulkan.hpp>

export module hikogui_GFX : gfx_pipeline_impl;
import : gfx_device_impl;
import : gfx_pipeline_intf;
import : gfx_surface_intf;
import hikogui_telemetry;

export namespace hi { inline namespace v1 {

[[nodiscard]] gfx_device *gfx_pipeline::device() const noexcept
{
    hi_axiom_not_null(surface);
    return surface->device();
}

void gfx_pipeline::draw_in_command_buffer(vk::CommandBuffer commandBuffer, draw_context const& context)
{
    commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, intrinsic);

    if (descriptorSet) {
        if (descriptorSetVersion < getDescriptorSetVersion()) {
            descriptorSetVersion = getDescriptorSetVersion();

            hi_axiom_not_null(device());
            device()->updateDescriptorSets(createWriteDescriptorSet(), {});
        }

        commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout, 0, {descriptorSet}, {});
    }
}

void gfx_pipeline::build_descriptor_sets()
{
    hilet descriptorSetLayoutBindings = createDescriptorSetLayoutBindings();

    if (ssize(descriptorSetLayoutBindings) == 0) {
        // Make sure that there is no descriptor set.
        descriptorSet = nullptr;
        return;
    }

    hilet descriptorSetLayoutCreateInfo = vk::DescriptorSetLayoutCreateInfo{
        vk::DescriptorSetLayoutCreateFlags(),
        narrow_cast<uint32_t>(descriptorSetLayoutBindings.size()),
        descriptorSetLayoutBindings.data()};

    hi_axiom_not_null(device());
    descriptorSetLayout = device()->createDescriptorSetLayout(descriptorSetLayoutCreateInfo);

    hilet descriptorPoolSizes =
        transform<std::vector<vk::DescriptorPoolSize>>(descriptorSetLayoutBindings, [](auto x) -> vk::DescriptorPoolSize {
            return {x.descriptorType, narrow_cast<uint32_t>(x.descriptorCount)};
        });

    descriptorPool = device()->createDescriptorPool(
        {vk::DescriptorPoolCreateFlags(),
         1, // maxSets
         narrow_cast<uint32_t>(descriptorPoolSizes.size()),
         descriptorPoolSizes.data()});

    hilet descriptorSetLayouts = std::array{descriptorSetLayout};

    hilet descriptorSets = device()->allocateDescriptorSets(
        {descriptorPool, narrow_cast<uint32_t>(descriptorSetLayouts.size()), descriptorSetLayouts.data()});

    descriptorSet = descriptorSets.at(0);
    descriptorSetVersion = 0;
}

void gfx_pipeline::teardown_descriptor_sets()
{
    if (!descriptorSet) {
        return;
    }

    hi_axiom_not_null(device());
    device()->destroy(descriptorPool);
    device()->destroy(descriptorSetLayout);
    descriptorSet = nullptr;
}

vk::PipelineDepthStencilStateCreateInfo gfx_pipeline::getPipelineDepthStencilStateCreateInfo() const
{
    // Reverse-z depth configuration
    return {
        vk::PipelineDepthStencilStateCreateFlags(),
        VK_TRUE, // depthTestEnable;
        VK_TRUE, // depthWriteEnable;
        vk::CompareOp::eGreaterOrEqual, // depthCompareOp
        VK_FALSE, // depthBoundsTestEnable
        VK_FALSE, // stencilTestEnable,
        vk::StencilOpState(), // front
        vk::StencilOpState(), // back
        1.0f, // minDepthBounds
        0.0f, // maxDepthBounds
    };
}

/* pre-multiplied alpha blending.
 */
std::vector<vk::PipelineColorBlendAttachmentState> gfx_pipeline::getPipelineColorBlendAttachmentStates() const
{
    return {
        {VK_TRUE, // blendEnable
         vk::BlendFactor::eOne, // srcColorBlendFactor
         vk::BlendFactor::eOneMinusSrcAlpha, // dstColorBlendFactor
         vk::BlendOp::eAdd, // colorBlendOp
         vk::BlendFactor::eOne, // srcAlphaBlendFactor
         vk::BlendFactor::eOneMinusSrcAlpha, // dstAlphaBlendFactor
         vk::BlendOp::eAdd, // alphaBlendOp
         vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB |
             vk::ColorComponentFlagBits::eA}};
}

void gfx_pipeline::build_pipeline(vk::RenderPass renderPass, uint32_t renderSubpass, vk::Extent2D _extent)
{
    hi_log_info("buildPipeline previous size ({}, {})", extent.width, extent.height);
    extent = _extent;

    const auto pushConstantRanges = createPushConstantRanges();
    const auto vertexInputBindingDescription = createVertexInputBindingDescription();
    const auto vertexInputAttributeDescriptions = createVertexInputAttributeDescriptions();
    const auto shaderStages = createShaderStages();

    std::vector<vk::DescriptorSetLayout> descriptorSetLayouts;
    if (descriptorSet) {
        descriptorSetLayouts.push_back(descriptorSetLayout);
    }

    hi_axiom_not_null(device());
    pipelineLayout = device()->createPipelineLayout(
        {vk::PipelineLayoutCreateFlags(),
         narrow_cast<uint32_t>(descriptorSetLayouts.size()),
         descriptorSetLayouts.data(),
         narrow_cast<uint32_t>(pushConstantRanges.size()),
         pushConstantRanges.data()});

    const vk::PipelineVertexInputStateCreateInfo pipelineVertexInputStateCreateInfo = {
        vk::PipelineVertexInputStateCreateFlags(),
        1,
        &vertexInputBindingDescription,
        narrow_cast<uint32_t>(vertexInputAttributeDescriptions.size()),
        vertexInputAttributeDescriptions.data()};

    const vk::PipelineInputAssemblyStateCreateInfo pipelineInputAssemblyStateCreateInfo = {
        vk::PipelineInputAssemblyStateCreateFlags(), vk::PrimitiveTopology::eTriangleList, VK_FALSE};

    const std::array<vk::Viewport, 1> viewports = {vk::Viewport{
        0.0f,
        0.0f,
        narrow_cast<float>(extent.width),
        narrow_cast<float>(extent.height),
        // Reverse-z, with float buffer this will give a linear depth buffer.
        1.0f,
        0.0f}};

    hilet scissor = vk::Rect2D{vk::Offset2D{0, 0}, extent};

    hilet scissors = std::array{scissor};

    const vk::PipelineViewportStateCreateInfo pipelineViewportStateCreateInfo = {
        vk::PipelineViewportStateCreateFlags(),
        narrow_cast<uint32_t>(viewports.size()),
        viewports.data(),
        narrow_cast<uint32_t>(scissors.size()),
        scissors.data()};

    const vk::PipelineRasterizationStateCreateInfo pipelineRasterizationStateCreateInfo = {
        vk::PipelineRasterizationStateCreateFlags(),
        VK_FALSE, // depthClampEnable
        VK_FALSE, // rasterizerDiscardEnable
        vk::PolygonMode::eFill,
        vk::CullModeFlagBits::eBack,
        vk::FrontFace::eCounterClockwise,
        VK_FALSE, // depthBiasEnable
        0.0, // depthBiasConstantFactor
        0.0, // depthBiasClamp
        0.0, // depthBiasSlopeFactor
        1.0 // lineWidth
    };

    const vk::PipelineMultisampleStateCreateInfo pipelineMultisampleStateCreateInfo = {
        vk::PipelineMultisampleStateCreateFlags(),
        vk::SampleCountFlagBits::e1,
        VK_FALSE, // sampleShadingEnable
        1.0f, // minSampleShading
        nullptr, // sampleMask
        VK_FALSE, // alphaToCoverageEnable
        VK_FALSE // alphaToOneEnable
    };

    hilet pipelineDepthStencilStateCreateInfo = getPipelineDepthStencilStateCreateInfo();

    /* Pre-multiplied alpha blending.
     */
    hilet pipelineColorBlendAttachmentStates = getPipelineColorBlendAttachmentStates();

    const vk::PipelineColorBlendStateCreateInfo pipelineColorBlendStateCreateInfo = {
        vk::PipelineColorBlendStateCreateFlags(),
        VK_FALSE, // logicOpenable
        vk::LogicOp::eCopy,
        narrow_cast<uint32_t>(pipelineColorBlendAttachmentStates.size()),
        pipelineColorBlendAttachmentStates.data()};

    hilet dynamicStates = std::array{vk::DynamicState::eScissor};

    hilet pipelineDynamicStateInfo = vk::PipelineDynamicStateCreateInfo{
        vk::PipelineDynamicStateCreateFlags(), narrow_cast<uint32_t>(dynamicStates.size()), dynamicStates.data()};

    const vk::GraphicsPipelineCreateInfo graphicsPipelineCreateInfo = {
        vk::PipelineCreateFlags(),
        narrow_cast<uint32_t>(shaderStages.size()),
        shaderStages.data(),
        &pipelineVertexInputStateCreateInfo,
        &pipelineInputAssemblyStateCreateInfo,
        nullptr, // tesselationStateCreateInfo
        &pipelineViewportStateCreateInfo,
        &pipelineRasterizationStateCreateInfo,
        &pipelineMultisampleStateCreateInfo,
        &pipelineDepthStencilStateCreateInfo,
        &pipelineColorBlendStateCreateInfo,
        &pipelineDynamicStateInfo,
        pipelineLayout,
        renderPass,
        renderSubpass, // subpass
        vk::Pipeline(), // basePipelineHandle
        -1 // basePipelineIndex
    };

    hi_axiom_not_null(device());
    intrinsic = device()->createGraphicsPipeline(vk::PipelineCache(), graphicsPipelineCreateInfo);
    hi_log_info("/buildPipeline new size ({}, {})", extent.width, extent.height);
}

void gfx_pipeline::teardown_pipeline()
{
    hi_axiom_not_null(device());
    device()->destroy(intrinsic);
    device()->destroy(pipelineLayout);
}

void gfx_pipeline::build_for_new_device()
{
    build_vertex_buffers();
}

void gfx_pipeline::teardown_for_device_lost()
{
    teardown_vertex_buffers();
}

void gfx_pipeline::build_for_new_swapchain(vk::RenderPass renderPass, uint32_t renderSubpass, vk::Extent2D _extent)
{
    // Input attachments described by the descriptor set will change when a
    // new swap chain is created.
    build_descriptor_sets();
    build_pipeline(renderPass, renderSubpass, _extent);
}

void gfx_pipeline::teardown_for_swapchain_lost()
{
    teardown_pipeline();
    teardown_descriptor_sets();
}

}} // namespace hi::inline v1
