// Copyright Take Vos 2020-2021.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "gfx_pipeline_vulkan_intf.hpp"
#include "../container/container.hpp"
#include "../geometry/geometry.hpp"
#include "../color/color.hpp"
#include "../image/image.hpp"
#include "../macros.hpp"
#include <vulkan/vulkan.hpp>
#include <vma/vk_mem_alloc.h>
#include <span>

hi_export_module(hikogui.GFX : gfx_pipeline_box_intf);

hi_export namespace hi { inline namespace v1 {

/*! Pipeline for rendering simple box shaded quads.
 */
class gfx_pipeline_box : public gfx_pipeline {
public:
    /*! A vertex defining a rectangle on a window.
     * The vertex shader will convert window pixel-coordinates to normalized projection-coordinates.
     */
    struct alignas(16) vertex {
        /** The pixel-coordinates where the origin is located relative to the bottom-left corner of the window.
         */
        sfloat_rgba32 position;

        /** The position in pixels of the clipping rectangle relative to the bottom-left corner of the window, and extent in
         * pixels.
         */
        sfloat_rgba32 clipping_rectangle;

        /** Double 2D coordinates inside the quad, used to determine the distance from the sides and corner inside the fragment
         * shader. x = Number of pixels to the right from the left edge of the quad. y = Number of pixels above the bottom edge. z
         * = Number of pixels to the left from the right edge of the quad. w = Number of pixels below the top edge.
         *
         * The rasterizer will interpolate these numbers, so that inside the fragment shader
         * the distance from a corner can be determined easily.
         */
        sfloat_rgba32 corner_coordinate;

        /** Shape of each corner, negative values are cut corners, positive values are rounded corners.
         */
        sfloat_rgba32 corner_radii;

        /** background color of the box.
         */
        sfloat_rgba16 fill_color;

        /** border color of the box.
         */
        sfloat_rgba16 line_color;

        float line_width;

        vertex(
            sfloat_rgba32 position,
            sfloat_rgba32 clipping_rectangle,
            sfloat_rgba32 corner_coordinate,
            sfloat_rgba32 corner_radii,
            sfloat_rgba16 fill_color,
            sfloat_rgba16 line_color,
            float line_width) noexcept :
            position(position),
            clipping_rectangle(clipping_rectangle),
            corner_coordinate(corner_coordinate),
            corner_radii(corner_radii),
            fill_color(fill_color),
            line_color(line_color),
            line_width(line_width)
        {
        }

        static vk::VertexInputBindingDescription inputBindingDescription()
        {
            return {0, sizeof(vertex), vk::VertexInputRate::eVertex};
        }

        static std::vector<vk::VertexInputAttributeDescription> inputAttributeDescriptions()
        {
            return {
                {0, 0, vk::Format::eR32G32B32A32Sfloat, offsetof(vertex, position)},
                {1, 0, vk::Format::eR32G32B32A32Sfloat, offsetof(vertex, clipping_rectangle)},
                {2, 0, vk::Format::eR32G32B32A32Sfloat, offsetof(vertex, corner_coordinate)},
                {3, 0, vk::Format::eR32G32B32A32Sfloat, offsetof(vertex, corner_radii)},
                {4, 0, vk::Format::eR16G16B16A16Sfloat, offsetof(vertex, fill_color)},
                {5, 0, vk::Format::eR16G16B16A16Sfloat, offsetof(vertex, line_color)},
                {6, 0, vk::Format::eR32Sfloat, offsetof(vertex, line_width)},
            };
        }
    };

    struct push_constants {
        sfloat_rg32 windowExtent = extent2{0.0, 0.0};
        sfloat_rg32 viewportScale = scale2{0.0, 0.0};

        static std::vector<vk::PushConstantRange> pushConstantRanges()
        {
            return {{vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment, 0, sizeof(push_constants)}};
        }
    };

    struct device_shared final {
        gfx_device const& device;

        vk::ShaderModule vertexShaderModule;
        vk::ShaderModule fragmentShaderModule;
        std::vector<vk::PipelineShaderStageCreateInfo> shaderStages;

        device_shared(gfx_device const& device);
        ~device_shared();

        device_shared(device_shared const&) = delete;
        device_shared& operator=(device_shared const&) = delete;
        device_shared(device_shared&&) = delete;
        device_shared& operator=(device_shared&&) = delete;

        /*! Deallocate vulkan resources.
         * This is called in the destructor of gfx_device, therefor we can not use our `device`.
         */
        void destroy(gfx_device const *vulkanDevice);

        void drawInCommandBuffer(vk::CommandBuffer const& commandBuffer);

        static void place_vertices(
            vector_span<vertex>& vertices,
            aarectangle clipping_rectangle,
            quad box,
            quad_color fill_colors,
            quad_color line_colors,
            float line_width,
            corner_radii corner_radii);

    private:
        void buildShaders();
        void teardownShaders(gfx_device const *vulkanDevice);
    };

    vector_span<vertex> vertexBufferData;

    ~gfx_pipeline_box() = default;
    gfx_pipeline_box(const gfx_pipeline_box&) = delete;
    gfx_pipeline_box& operator=(const gfx_pipeline_box&) = delete;
    gfx_pipeline_box(gfx_pipeline_box&&) = delete;
    gfx_pipeline_box& operator=(gfx_pipeline_box&&) = delete;

    gfx_pipeline_box(gfx_surface *surface) : gfx_pipeline(surface) {}

    void draw_in_command_buffer(vk::CommandBuffer commandBuffer, draw_context const& context) override;

protected:
    push_constants pushConstants;

    vk::Buffer vertexBuffer;
    VmaAllocation vertexBufferAllocation;

    [[nodiscard]] std::vector<vk::PipelineShaderStageCreateInfo> createShaderStages() const override;
    [[nodiscard]] std::vector<vk::DescriptorSetLayoutBinding> createDescriptorSetLayoutBindings() const override;
    [[nodiscard]] std::vector<vk::WriteDescriptorSet> createWriteDescriptorSet() const override;
    [[nodiscard]] size_t getDescriptorSetVersion() const override;
    [[nodiscard]] std::vector<vk::PushConstantRange> createPushConstantRanges() const override;
    [[nodiscard]] vk::VertexInputBindingDescription createVertexInputBindingDescription() const override;
    [[nodiscard]] std::vector<vk::VertexInputAttributeDescription> createVertexInputAttributeDescriptions() const override;

private:
    void build_vertex_buffers() override;
    void teardown_vertex_buffers() override;
};

}} // namespace hi::v1
