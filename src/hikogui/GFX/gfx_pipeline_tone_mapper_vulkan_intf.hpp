// Copyright Take Vos 2020-2021.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "gfx_pipeline_vulkan_intf.hpp"
#include "../container/container.hpp"
#include "../geometry/geometry.hpp"
#include "../image/image.hpp"
#include "../macros.hpp"
#include <vma/vk_mem_alloc.h>
#include <vulkan/vulkan.hpp>
#include <span>

hi_export_module(hikogui.GFX : gfx_pipeline_tone_mapper_intf);

hi_export namespace hi { inline namespace v1 {

/*! Pipeline for rendering simple flat shaded quads.
 */
class gfx_pipeline_tone_mapper : public gfx_pipeline {
public:
struct push_constants {
    float saturation = 1.0;

    static std::vector<vk::PushConstantRange> pushConstantRanges()
    {
        return {{vk::ShaderStageFlagBits::eFragment, 0, sizeof(push_constants)}};
    }
};

struct device_shared final {
    gfx_device const &device;

    vk::ShaderModule vertexShaderModule;
    vk::ShaderModule fragmentShaderModule;
    std::vector<vk::PipelineShaderStageCreateInfo> shaderStages;

    device_shared(gfx_device const &device);
    ~device_shared();

    device_shared(device_shared const &) = delete;
    device_shared &operator=(device_shared const &) = delete;
    device_shared(device_shared &&) = delete;
    device_shared &operator=(device_shared &&) = delete;

    /*! Deallocate vulkan resources.
     * This is called in the destructor of gfx_device, therefor we can not use our gfx_device from this point on.
     */
    void destroy(gfx_device const *vulkanDevice);

    void drawInCommandBuffer(vk::CommandBuffer const &commandBuffer);

private:
    void buildShaders();
    void teardownShaders(gfx_device const*vulkanDevice);
};

    ~gfx_pipeline_tone_mapper() = default;
    gfx_pipeline_tone_mapper(const gfx_pipeline_tone_mapper&) = delete;
    gfx_pipeline_tone_mapper& operator=(const gfx_pipeline_tone_mapper&) = delete;
    gfx_pipeline_tone_mapper(gfx_pipeline_tone_mapper&&) = delete;
    gfx_pipeline_tone_mapper& operator=(gfx_pipeline_tone_mapper&&) = delete;

    gfx_pipeline_tone_mapper(gfx_surface *surface) : gfx_pipeline(surface) {}

    void draw_in_command_buffer(vk::CommandBuffer commandBuffer, draw_context const& context) override;

protected:
    push_constants _push_constants;

    [[nodiscard]] std::vector<vk::PipelineShaderStageCreateInfo> createShaderStages() const override;
    [[nodiscard]] std::vector<vk::DescriptorSetLayoutBinding> createDescriptorSetLayoutBindings() const override;
    [[nodiscard]] std::vector<vk::WriteDescriptorSet> createWriteDescriptorSet() const override;
    [[nodiscard]] size_t getDescriptorSetVersion() const override;
    [[nodiscard]] std::vector<vk::PushConstantRange> createPushConstantRanges() const override;
    [[nodiscard]] vk::PipelineDepthStencilStateCreateInfo getPipelineDepthStencilStateCreateInfo() const override;
};

}} // namespace hi::inline v1::gfx_pipeline_tone_mapper
