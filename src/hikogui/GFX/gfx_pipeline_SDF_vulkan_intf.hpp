// Copyright Take Vos 2020-2021.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "gfx_pipeline_vulkan_intf.hpp"
#include "../container/container.hpp"
#include "../geometry/geometry.hpp"
#include "../image/image.hpp"
#include "../font/font.hpp"
#include "../macros.hpp"
#include <vulkan/vulkan.hpp>
#include <vma/vk_mem_alloc.h>
#include <span>

hi_export_module(hikogui.GFX : gfx_pipeline_SDF_intf);

hi_export namespace hi {
inline namespace v1 {

/*! Pipeline for rendering backings of widgets.
 * Maintains texture map atlases and sharing for all views.
 */
class gfx_pipeline_SDF : public gfx_pipeline {
public:
    /*! A vertex defining a rectangle on a window.
     * The vertex shader will convert window pixel-coordinates to normalized projection-coordinates.
     */
    struct vertex {
        //! The pixel-coordinates where the origin is located relative to the bottom-left corner of the window.
        sfloat_rgb32 position;

        //! Clipping rectangle. (x,y)=bottom-left, (z,w)=top-right
        sfloat_rgba32 clippingRectangle;

        //! The x, y (relative to bottom-left) coordinate inside the texture-atlas, z is used as an index in the texture-atlas
        //! array
        sfloat_rgb32 textureCoord;

        //! The color of the glyph.
        sfloat_rgba16 color;

        vertex(point3 position, aarectangle clippingRectangle, point3 textureCoord, hi::color color) noexcept :
            position(position), clippingRectangle(clippingRectangle), textureCoord(textureCoord), color(color)
        {
        }

        static vk::VertexInputBindingDescription inputBindingDescription()
        {
            return {0, sizeof(vertex), vk::VertexInputRate::eVertex};
        }

        static std::vector<vk::VertexInputAttributeDescription> inputAttributeDescriptions()
        {
            return {
                {0, 0, vk::Format::eR32G32B32Sfloat, offsetof(vertex, position)},
                {1, 0, vk::Format::eR32G32B32A32Sfloat, offsetof(vertex, clippingRectangle)},
                {2, 0, vk::Format::eR32G32B32Sfloat, offsetof(vertex, textureCoord)},
                {3, 0, vk::Format::eR16G16B16A16Sfloat, offsetof(vertex, color)}};
        }
    };

    struct push_constants {
        sfloat_rg32 window_extent = extent2{0.0, 0.0};
        sfloat_rg32 viewport_scale = scale2{0.0, 0.0};
        sfloat_rg32 red_subpixel_offset = vector2{0.0, 0.0};
        sfloat_rg32 blue_subpixel_offset = vector2{0.0, 0.0};
        VkBool32 has_subpixels = false;

        static std::vector<vk::PushConstantRange> pushConstantRanges()
        {
            return {{vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment, 0, sizeof(push_constants)}};
        }
    };

    struct specialization_constants {
        float sdf_r8maxDistance;
        float atlasImageWidth;

        [[nodiscard]] vk::SpecializationInfo specializationInfo(std::vector<vk::SpecializationMapEntry>& entries) const noexcept
        {
            return {narrow_cast<uint32_t>(ssize(entries)), entries.data(), sizeof(specialization_constants), this};
        }

        [[nodiscard]] static std::vector<vk::SpecializationMapEntry> specializationConstantMapEntries() noexcept
        {
            return {
                {0, offsetof(specialization_constants, sdf_r8maxDistance), sizeof(sdf_r8maxDistance)},
                {1, offsetof(specialization_constants, atlasImageWidth), sizeof(atlasImageWidth)},
            };
        }
    };

    struct transfer_buffer {
        vk::Buffer buffer;
        VmaAllocation allocation = {};
        std::span<std::byte> mapping = {};

        /** Get the offset in bytes to a staging image in the buffer.
         * 
         * @param staging_image_nr The index of the staging image.
         * @return The offset in bytes.
         */
        [[nodiscard]] size_t offset(size_t staging_image_nr) const noexcept
        {
            return staging_image_nr * device_shared::staging_image_size;
        }

        /** Get a pixmap to a staging image in the buffer.
         * 
         * @param staging_image_nr The index of the staging image.
         * @return A pixmap to the staging image.
         */
        [[nodiscard]] pixmap_span<sdf_r8> pixmap(size_t staging_image_nr) noexcept
        {
            return {
                reinterpret_cast<sdf_r8*>(mapping.data() + offset(staging_image_nr)),
                device_shared::staging_image_width,
                device_shared::staging_image_height};
        }

        /** Get a subimage of a staging image in the buffer.
         * 
         * @param staging_image_nr The index of the staging image.
         * @param width The width of the subimage.
         * @param height The height of the subimage.
         * @return A pixmap to the subimage.
         */
        [[nodiscard]] pixmap_span<sdf_r8> pixmap(size_t staging_image_nr, size_t width, size_t height) noexcept
        {
            return pixmap(staging_image_nr).subimage(0, 0, width, height);
        }
    };

    struct texture_map {
        vk::Image image;
        VmaAllocation allocation = {};
        vk::ImageView view;
        hi::pixmap_span<sdf_r8> pixmap;
        vk::ImageLayout layout = vk::ImageLayout::eUndefined;

        void transitionLayout(const gfx_device& device, vk::Format format, vk::ImageLayout nextLayout);
    };

    struct device_shared {
        // Studies in China have shown that literate individuals know and use between 3,000 and 4,000 characters.
        // Handle up to 7 * 7 * 128 == 6321 characters with a 16 x 1024 x 1024, 16 x 1 MByte
        //
        // For latin characters we can store about 7 * 12 == 84 characters in a single image, which is enough
        // for the full alpha numeric range that an application will use.

        constexpr static size_t atlasImageWidth = 256; // 7-12 characters, of 34 pixels wide.
        constexpr static size_t atlasImageHeight = 256; // 7 characters, of 34 pixels height.
        static_assert(atlasImageWidth == atlasImageHeight, "needed for fwidth(textureCoord)");
        constexpr static size_t atlasMaximumNrImages = 128; // 128 * 49 characters.

        constexpr static size_t staging_image_width = 64; // One 'em' is 28 pixels, with edges 34 pixels.
        constexpr static size_t staging_image_height = 64;
        constexpr static size_t staging_image_size = staging_image_width * staging_image_height * sizeof(sdf_r8);
        constexpr static size_t num_staging_images = 16;

        constexpr static float atlasTextureCoordinateMultiplier = 1.0f / atlasImageWidth;
        constexpr static float drawfontSize = 28.0f;
        constexpr static float drawBorder = sdf_r8::max_distance;
        constexpr static float scaledDrawBorder = drawBorder / drawfontSize;

        gfx_device const& device;

        vk::ShaderModule vertexShaderModule;
        vk::ShaderModule fragmentShaderModule;

        specialization_constants specializationConstants;
        std::vector<vk::SpecializationMapEntry> fragmentShaderSpecializationMapEntries;
        vk::SpecializationInfo fragmentShaderSpecializationInfo;
        std::vector<vk::PipelineShaderStageCreateInfo> shaderStages;

        transfer_buffer staging_buffer;
        resource_pool<size_t> staging_pool;
        future_pool<void> staging_futures;
        
        std::vector<texture_map> atlasTextures;

        std::array<vk::DescriptorImageInfo, atlasMaximumNrImages> atlasDescriptorImageInfos;
        vk::Sampler atlasSampler;
        vk::DescriptorImageInfo atlasSamplerDescriptorImageInfo;

        point3 atlas_allocation_position = {};
        /// During allocation on a row, we keep track of the tallest glyph.
        int atlasAllocationMaxHeight = 0;

        device_shared(gfx_device const& device);
        ~device_shared();

        device_shared(device_shared const&) = delete;
        device_shared& operator=(device_shared const&) = delete;
        device_shared(device_shared&&) = delete;
        device_shared& operator=(device_shared&&) = delete;

        /*! Deallocate vulkan resources.
         * This is called in the destructor of gfx_device, therefor we can not use our gfx_device from this point on.
         */
        void destroy(gfx_device const* vulkanDevice);

        /** Allocate an glyph in the atlas.
         * This may allocate an atlas texture, up to atlasMaximumNrImages.
         */
        [[nodiscard]] glyph_atlas_info allocate_rect(extent2 draw_extent, scale2 draw_scale) noexcept;

        void drawInCommandBuffer(vk::CommandBuffer const& commandBuffer);

        /** Once drawing in the staging pixmap is completed, you can upload it to the atlas.
         * This will transition the stating texture to 'source' and the atlas to 'destination'.
         *
         * @param location The location in the atlas to upload the pixmap to.
         * @param staging_image_nr The index of the staging image to upload.
         */
        void upload_staging_pixmap_to_atlas(glyph_atlas_info const& location, size_t staging_image_nr);

        /** This will transition the staging texture to 'general' for writing by the CPU.
         */
        void prepareStagingPixmapForDrawing();

        /** This will transition the atlas to 'shader-read'.
         */
        void prepare_atlas_for_rendering();

        /** Place vertices for a single glyph.
         *
         * @param vertices The list of vertices to add to.
         * @param clipping_rectangle The rectangle to clip the glyph.
         * @param box The rectangle of the glyph in window coordinates. The box's size must be the size
         *            of the glyph's bounding box times @a glyph_size.
         * @param glyphs The font-id, composed-glyphs to render
         * @param colors The color of each corner of the glyph.
         * @return True is atlas was updated.
         */
        bool place_vertices(
            vector_span<vertex>& vertices,
            aarectangle const& clipping_rectangle,
            quad const& box,
            hi::font_id font,
            glyph_id glyph,
            quad_color colors) noexcept;

    private:
        void buildShaders();
        void teardownShaders(gfx_device const* vulkanDevice);
        void addAtlasImage();
        void buildAtlas();
        void teardownAtlas(gfx_device const* vulkanDevice);
        void add_glyph_to_atlas(hi::font_id font, glyph_id glyph, glyph_atlas_info& info) noexcept;

        /**
         * @return The Atlas rectangle and true if a new glyph was added to the atlas.
         */
        hi_force_inline std::pair<glyph_atlas_info const*, bool> get_glyph_from_atlas(hi::font_id font, glyph_id glyph) noexcept
        {
            auto const& font_ref = hi::get_font(font);
            auto& info = font_ref.atlas_info(glyph);

            if (info) [[likely]] {
                return {&info, false};

            } else {
                add_glyph_to_atlas(font, glyph, info);
                return {&info, true};
            }
        }
    };

    vector_span<vertex> vertexBufferData;

    ~gfx_pipeline_SDF() = default;
    gfx_pipeline_SDF(const gfx_pipeline_SDF&) = delete;
    gfx_pipeline_SDF& operator=(const gfx_pipeline_SDF&) = delete;
    gfx_pipeline_SDF(gfx_pipeline_SDF&&) = delete;
    gfx_pipeline_SDF& operator=(gfx_pipeline_SDF&&) = delete;

    gfx_pipeline_SDF(gfx_surface* surface) : gfx_pipeline(surface) {}

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
    [[nodiscard]] std::vector<vk::PipelineColorBlendAttachmentState> getPipelineColorBlendAttachmentStates() const override;

private:
    void build_vertex_buffers() override;
    void teardown_vertex_buffers() override;
};

} // namespace v1
} // namespace hi
