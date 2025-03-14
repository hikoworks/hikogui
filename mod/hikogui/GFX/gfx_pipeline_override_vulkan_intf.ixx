// Copyright Take Vos 2022.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

module;
#include "../macros.hpp"

#include <vma/vk_mem_alloc.h>
#include <vulkan/vulkan.hpp>
#include <span>
#include <vector>

export module hikogui_GFX : gfx_pipeline_override_intf;
import : gfx_pipeline_intf;
import hikogui_color;
import hikogui_container;
import hikogui_geometry;
import hikogui_image;

export namespace hi { inline namespace v1 {

/*! Pipeline for rendering simple alpha shaded quads.
 */
class gfx_pipeline_override : public gfx_pipeline {
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

        /** The color value of the resulting pixels inside the quad.
         */
        sfloat_rgba16 color;

        /** The blend-factor value of the resulting pixels inside the quad.
         */
        sfloat_rgba16 blend_factor;

        vertex(sfloat_rgba32 position, sfloat_rgba32 clipping_rectangle, sfloat_rgba16 color, sfloat_rgba16 blend_factor) noexcept :
            position(position), clipping_rectangle(clipping_rectangle), color(color), blend_factor(blend_factor)
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
                {2, 0, vk::Format::eR16G16B16A16Sfloat, offsetof(vertex, color)},
                {3, 0, vk::Format::eR16G16B16A16Sfloat, offsetof(vertex, blend_factor)},
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

    struct device_shared {
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

        static void place_vertices(vector_span<vertex>& vertices, aarectangle clipping_rectangle, quad box, quad_color color, quad_color blend_factor);

    private:
        void buildShaders();
        void teardownShaders(gfx_device const *vulkanDevice);
    };

    vector_span<vertex> vertexBufferData;

    ~gfx_pipeline_override() = default;
    gfx_pipeline_override(const gfx_pipeline_override&) = delete;
    gfx_pipeline_override& operator=(const gfx_pipeline_override&) = delete;
    gfx_pipeline_override(gfx_pipeline_override&&) = delete;
    gfx_pipeline_override& operator=(gfx_pipeline_override&&) = delete;

    gfx_pipeline_override(gfx_surface *surface) : gfx_pipeline(surface) {}

    void draw_in_command_buffer(vk::CommandBuffer commandBuffer, draw_context const& context) override;

protected:
    push_constants pushConstants;

    vk::Buffer vertexBuffer;
    VmaAllocation vertexBufferAllocation;

    [[nodiscard]] std::vector<vk::PipelineColorBlendAttachmentState> getPipelineColorBlendAttachmentStates() const override;
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
