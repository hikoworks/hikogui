// Copyright Take Vos 2019, 2021.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "gfx_pipeline_vulkan_intf.hpp"
#include "../container/container.hpp"
#include "../geometry/geometry.hpp"
#include "../image/image.hpp"
#include "../codec/codec.hpp"
#include "../macros.hpp"
#include <vulkan/vulkan.hpp>
#include <vma/vk_mem_alloc.h>

hi_export_module(hikogui.GFX : gfx_pipeline_image_intf);

hi_export namespace hi { inline namespace v1 {

/*! Pipeline for rendering backings of widgets.
 * Maintains texture map atlas and sharing for all views.
 */
class gfx_pipeline_image : public gfx_pipeline {
public:
    /*! A vertex defining a rectangle on a window.
     * The vertex shader will convert window pixel-coordinates to normalized projection-coordinates.
     */
    struct alignas(16) vertex {
        //! The pixel-coordinates where the origin is located relative to the bottom-left corner of the window.
        sfloat_rgba32 position;

        //! The position in pixels of the clipping rectangle relative to the bottom-left corner of the window, and extent in
        //! pixels.
        sfloat_rgba32 clipping_rectangle;

        //! The x, y coordinate inside the texture-atlas, z is used as an index in the texture-atlas array
        sfloat_rgba32 atlas_position;

        vertex(sfloat_rgba32 position, sfloat_rgba32 clipping_rectangle, sfloat_rgba32 atlas_position) noexcept :
            position(position), clipping_rectangle(clipping_rectangle), atlas_position(atlas_position)
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
                {2, 0, vk::Format::eR32G32B32A32Sfloat, offsetof(vertex, atlas_position)},
            };
        }
    };

    struct push_constants {
        sfloat_rg32 windowExtent = extent2{0.0, 0.0};
        sfloat_rg32 viewportScale = scale2{0.0, 0.0};
        sfloat_rg32 atlasExtent = extent2{0.0, 0.0};
        sfloat_rg32 atlasScale = scale2{0.0, 0.0};

        static std::vector<vk::PushConstantRange> pushConstantRanges()
        {
            return {{vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment, 0, sizeof(push_constants)}};
        }
    };

    struct texture_map {
        vk::Image image;
        VmaAllocation allocation = {};
        vk::ImageView view;
        hi::pixmap_span<sfloat_rgba16> pixmap;
        vk::ImageLayout layout = vk::ImageLayout::eUndefined;

        void transitionLayout(const gfx_device& device, vk::Format format, vk::ImageLayout nextLayout);
    };

    /** This is a image that is uploaded into the texture atlas.
     */
    struct paged_image {
        enum class state_type { uninitialized, drawing, uploaded };

        constexpr static std::size_t page_size = 62; // 64x64 including a 1 pixel border.

        mutable std::atomic<state_type> state = state_type::uninitialized;
        gfx_device *device = nullptr;
        std::size_t width;
        std::size_t height;
        std::vector<std::size_t> pages;

        ~paged_image();
        constexpr paged_image() noexcept = default;
        paged_image(paged_image&& other) noexcept;
        paged_image& operator=(paged_image&& other) noexcept;
        paged_image(paged_image const& other) = delete;
        paged_image& operator=(paged_image const& other) = delete;

        paged_image(gfx_surface const *surface, std::size_t width, std::size_t height) noexcept;
        paged_image(gfx_surface const *surface, pixmap_span<sfloat_rgba16 const> image) noexcept;
        paged_image(gfx_surface const *surface, pixmap<sfloat_rgba16> const& image) noexcept :
            paged_image(surface, pixmap_span<sfloat_rgba16 const>{image})
        {
        }

        [[nodiscard]] constexpr explicit operator bool() const noexcept
        {
            return device != nullptr;
        }

        [[nodiscard]] constexpr extent2 size() const noexcept
        {
            return extent2{narrow_cast<float>(width), narrow_cast<float>(height)};
        }

        [[nodiscard]] constexpr std::pair<std::size_t, std::size_t> size_in_int_pages() const noexcept
        {
            auto const num_columns = (width + page_size - 1) / page_size;
            auto const num_rows = (height + page_size - 1) / page_size;
            return {num_columns, num_rows};
        }

        [[nodiscard]] constexpr extent2 size_in_float_pages() const noexcept
        {
            constexpr auto page_size_ = f32x4{narrow_cast<float>(page_size), narrow_cast<float>(page_size), 1.0f, 1.0f};
            auto size = f32x4{narrow_cast<float>(width), narrow_cast<float>(height), 0.0f, 0.0f};
            return extent2{size / page_size_};
        }

        /** Upload image to atlas.
         */
        void upload(pixmap_span<sfloat_rgba16 const> image) noexcept;
    };

    struct device_shared {
        constexpr static std::size_t atlas_num_pages_per_axis = 8;
        constexpr static std::size_t atlas_num_pages_per_image = atlas_num_pages_per_axis * atlas_num_pages_per_axis;
        constexpr static std::size_t atlas_image_axis_size = atlas_num_pages_per_axis * (paged_image::page_size + 2);
        constexpr static std::size_t atlas_maximum_num_images = 64;
        constexpr static std::size_t staging_image_width = 1024;
        constexpr static std::size_t staging_image_height = 1024;

        gfx_device const& device;

        vk::ShaderModule vertex_shader_module;
        vk::ShaderModule fragment_shader_module;
        std::vector<vk::PipelineShaderStageCreateInfo> shader_stages;

        texture_map staging_texture;
        std::vector<texture_map> atlas_textures;

        std::array<vk::DescriptorImageInfo, atlas_maximum_num_images> atlas_descriptor_image_infos;
        vk::Sampler atlas_sampler;
        vk::DescriptorImageInfo atlas_sampler_descriptor_image_info;

        device_shared(gfx_device const& device);
        ~device_shared();

        device_shared(device_shared const&) = delete;
        device_shared& operator=(device_shared const&) = delete;
        device_shared(device_shared&&) = delete;
        device_shared& operator=(device_shared&&) = delete;

        /*! Deallocate vulkan resources.
         * This is called in the destructor of gfx_device, therefor we can not use our gfx_device from this point on.
         */
        void destroy(gfx_device const *vulkanDevice);

        /** Allocate pages from the atlas.
         */
        std::vector<std::size_t> allocate_pages(std::size_t num_pages) noexcept;

        /** Deallocate pages back to the atlas.
         */
        void free_pages(std::vector<std::size_t> const& pages) noexcept;

        void draw_in_command_buffer(vk::CommandBuffer const& commandBuffer);

        /** Get the full staging pixel map excluding border.
         *
         * The returned pixel-map is offset by the page::border.
         */
        hi::pixmap_span<sfloat_rgba16> get_staging_pixmap();

        /** Prepare the atlas so that it can be used as a texture map by the shaders.
         */
        void prepare_atlas_for_rendering();

        /** Place vertices for a single image.
         *
         * @pre The image is uploaded.
         * @param vertices The list of vertices to add to.
         * @param clipping_rectangle The rectangle to clip the glyph.
         * @param box The rectangle of the image in window coordinates.
         * @param image The image to render.
         */
        void place_vertices(
            vector_span<vertex>& vertices,
            aarectangle const& clipping_rectangle,
            quad const& box,
            paged_image const& image) noexcept;

    private:
        std::vector<std::size_t> _atlas_free_pages;

        /** Get a submap of the staging pixel map to draw the image in.
         */
        hi::pixmap_span<sfloat_rgba16> get_staging_pixmap(std::size_t width, std::size_t height)
        {
            return get_staging_pixmap().subimage(0, 0, width, height);
        }

        /** Add a transparent border around the image.
         *
         * @param border_rectangle The rectangle of the border, the image-rectangle is inside this 1 pixel border.
         */
        void make_staging_border_transparent(aarectangle border_rectangle) noexcept;

        /** Clear the area between the border rectangle and upload rectangle.
         *
         * @param border_rectangle The rectangle where the border is located.
         * @param upload_rectangle The rectangle which will be uploaded to the atlas.
         */
        void clear_staging_between_border_and_upload(aarectangle border_rectangle, aarectangle upload_rectangle) noexcept;

        /** Prepare the staging image for upload.
         *
         * The following will be done.
         *  * Around the edge of the image the color is copied into the 1 pixel border.
         *    with the alpha channel set to zero.
         *  * On the right and upper edge the pixels are set to transparent-black up to
         *    a multiple of the `paged_image::page_size`.
         *  * flush the image to the GPU
         *  * transition the image for transferring to the atlas.
         */
        void prepare_staging_for_upload(paged_image const& image) noexcept;

        /** Copy the image from the staging pixel map into the atlas.
         */
        void update_atlas_with_staging_pixmap(paged_image const& image) noexcept;

        void build_shaders();
        void teardown_shaders(gfx_device const *device);
        void add_atlas_image();
        void build_atlas();
        void teardown_atlas(gfx_device const *device);

        friend paged_image;
    };

    vector_span<vertex> vertexBufferData;

    ~gfx_pipeline_image() = default;
    gfx_pipeline_image(const gfx_pipeline_image&) = delete;
    gfx_pipeline_image& operator=(const gfx_pipeline_image&) = delete;
    gfx_pipeline_image(gfx_pipeline_image&&) = delete;
    gfx_pipeline_image& operator=(gfx_pipeline_image&&) = delete;

    gfx_pipeline_image(gfx_surface *surface) : gfx_pipeline(surface) {}

    void draw_in_command_buffer(vk::CommandBuffer commandBuffer, draw_context const& context) override;

private:
    push_constants pushConstants;
    int numberOfAtlasImagesInDescriptor = 0;

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
