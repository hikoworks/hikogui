// Copyright Take Vos 2019-2022.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

module;
#include "../macros.hpp"

#include <vulkan/vulkan.hpp>

export module hikogui_GFX : gfx_pipeline_image_impl;
import : gfx_device_impl;
import : gfx_pipeline_image_intf;

export namespace hi { inline namespace v1 {

void gfx_pipeline_image::draw_in_command_buffer(vk::CommandBuffer commandBuffer, draw_context const& context)
{
    gfx_pipeline::draw_in_command_buffer(commandBuffer, context);

    hi_axiom_not_null(device());
    device()->flushAllocation(vertexBufferAllocation, 0, vertexBufferData.size() * sizeof(vertex));
    device()->image_pipeline->prepare_atlas_for_rendering();

    std::vector<vk::Buffer> tmpvertexBuffers = {vertexBuffer};
    std::vector<vk::DeviceSize> tmpOffsets = {0};
    hi_assert(tmpvertexBuffers.size() == tmpOffsets.size());

    device()->image_pipeline->draw_in_command_buffer(commandBuffer);

    commandBuffer.bindVertexBuffers(0, tmpvertexBuffers, tmpOffsets);

    pushConstants.windowExtent = extent2{narrow_cast<float>(extent.width), narrow_cast<float>(extent.height)};
    pushConstants.viewportScale = sfloat_rg32{2.0f / extent.width, 2.0f / extent.height};
    pushConstants.atlasExtent = sfloat_rg32{device_shared::atlas_image_axis_size, device_shared::atlas_image_axis_size};
    pushConstants.atlasScale =
        sfloat_rg32{1.0f / device_shared::atlas_image_axis_size, 1.0f / device_shared::atlas_image_axis_size};
    commandBuffer.pushConstants(
        pipelineLayout,
        vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
        0,
        sizeof(push_constants),
        &pushConstants);

    hilet numberOfRectangles = vertexBufferData.size() / 4;
    hilet numberOfTriangles = numberOfRectangles * 2;
    device()->cmdBeginDebugUtilsLabelEXT(commandBuffer, "draw images");
    commandBuffer.drawIndexed(narrow_cast<uint32_t>(numberOfTriangles * 3), 1, 0, 0, 0);
    device()->cmdEndDebugUtilsLabelEXT(commandBuffer);
}

std::vector<vk::PipelineShaderStageCreateInfo> gfx_pipeline_image::createShaderStages() const
{
    hi_axiom_not_null(device());
    return device()->image_pipeline->shader_stages;
}

std::vector<vk::DescriptorSetLayoutBinding> gfx_pipeline_image::createDescriptorSetLayoutBindings() const
{
    return {
        {0, // binding
         vk::DescriptorType::eSampler,
         1, // descriptorCount
         vk::ShaderStageFlagBits::eFragment},
        {1, // binding
         vk::DescriptorType::eSampledImage,
         narrow_cast<uint32_t>(device_shared::atlas_maximum_num_images), // descriptorCount
         vk::ShaderStageFlagBits::eFragment}};
}

std::vector<vk::WriteDescriptorSet> gfx_pipeline_image::createWriteDescriptorSet() const
{
    hi_axiom_not_null(device());
    hilet& sharedImagePipeline = device()->image_pipeline;

    return {
        {
            descriptorSet,
            0, // destBinding
            0, // arrayElement
            1, // descriptorCount
            vk::DescriptorType::eSampler,
            &sharedImagePipeline->atlas_sampler_descriptor_image_info,
            nullptr, // bufferInfo
            nullptr // texelBufferView
        },
        {
            descriptorSet,
            1, // destBinding
            0, // arrayElement
            narrow_cast<uint32_t>(sharedImagePipeline->atlas_descriptor_image_infos.size()), // descriptorCount
            vk::DescriptorType::eSampledImage,
            sharedImagePipeline->atlas_descriptor_image_infos.data(),
            nullptr, // bufferInfo
            nullptr // texelBufferView
        }};
}

size_t gfx_pipeline_image::getDescriptorSetVersion() const
{
    hi_axiom_not_null(device());
    return device()->image_pipeline->atlas_textures.size();
}

std::vector<vk::PushConstantRange> gfx_pipeline_image::createPushConstantRanges() const
{
    return push_constants::pushConstantRanges();
}

vk::VertexInputBindingDescription gfx_pipeline_image::createVertexInputBindingDescription() const
{
    return vertex::inputBindingDescription();
}

std::vector<vk::VertexInputAttributeDescription> gfx_pipeline_image::createVertexInputAttributeDescriptions() const
{
    return vertex::inputAttributeDescriptions();
}

void gfx_pipeline_image::build_vertex_buffers()
{
    using vertexIndexType = uint16_t;
    constexpr ssize_t numberOfVertices = 1 << (sizeof(vertexIndexType) * CHAR_BIT);

    vk::BufferCreateInfo const bufferCreateInfo = {
        vk::BufferCreateFlags(),
        sizeof(vertex) * numberOfVertices,
        vk::BufferUsageFlagBits::eVertexBuffer,
        vk::SharingMode::eExclusive};
    VmaAllocationCreateInfo allocationCreateInfo = {};
    allocationCreateInfo.flags = VMA_ALLOCATION_CREATE_USER_DATA_COPY_STRING_BIT;
    allocationCreateInfo.pUserData = const_cast<char *>("image-pipeline vertex buffer");
    allocationCreateInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;

    hi_axiom_not_null(device());
    std::tie(vertexBuffer, vertexBufferAllocation) = device()->createBuffer(bufferCreateInfo, allocationCreateInfo);
    device()->setDebugUtilsObjectNameEXT(vertexBuffer, "image-pipeline vertex buffer");
    vertexBufferData = device()->mapMemory<vertex>(vertexBufferAllocation);
}

void gfx_pipeline_image::teardown_vertex_buffers()
{
    hi_axiom_not_null(device());
    device()->unmapMemory(vertexBufferAllocation);
    device()->destroyBuffer(vertexBuffer, vertexBufferAllocation);
}

void
gfx_pipeline_image::texture_map::transitionLayout(const gfx_device& device, vk::Format format, vk::ImageLayout nextLayout)
{
    if (layout != nextLayout) {
        device.transition_layout(image, format, layout, nextLayout);
        layout = nextLayout;
    }
}

gfx_pipeline_image::paged_image::paged_image(gfx_surface const *surface, std::size_t width, std::size_t height) noexcept :
    device(nullptr), width(width), height(height), pages()
{
    if (surface == nullptr) {
        // During initialization of a widget, the window may not have a surface yet.
        // As it needs to determine the size of the surface based on the size of the containing widgets.
        // Return an empty image.
        return;
    }

    // Like before the surface may not be assigned to a device either.
    // In that case also return an empty image.
    hilet lock = std::scoped_lock(gfx_system_mutex);
    if ((this->device = surface->device()) != nullptr) {
        hilet[num_columns, num_rows] = size_in_int_pages();
        this->pages = device->image_pipeline->allocate_pages(num_columns * num_rows);
    }
}

gfx_pipeline_image::paged_image::paged_image(gfx_surface const *surface, pixmap_span<sfloat_rgba16 const> image) noexcept :
    paged_image(surface, narrow_cast<std::size_t>(image.width()), narrow_cast<std::size_t>(image.height()))
{
    if (this->device) {
        hilet lock = std::scoped_lock(gfx_system_mutex);
        this->upload(image);
    }
}

gfx_pipeline_image::paged_image::paged_image(gfx_surface const *surface, png const& image) noexcept :
    paged_image(surface, narrow_cast<std::size_t>(image.width()), narrow_cast<std::size_t>(image.height()))
{
    if (this->device) {
        hilet lock = std::scoped_lock(gfx_system_mutex);
        this->upload(image);
    }
}

gfx_pipeline_image::paged_image::paged_image(paged_image&& other) noexcept :
    state(other.state.exchange(state_type::uninitialized)),
    device(std::exchange(other.device, nullptr)),
    width(other.width),
    height(other.height),
    pages(std::move(other.pages))
{
}

gfx_pipeline_image::paged_image& gfx_pipeline_image::paged_image::operator=(paged_image&& other) noexcept
{
    hi_return_on_self_assignment(other);

    // If the old image had pages, free them.
    if (device) {
        device->image_pipeline->free_pages(pages);
    }

    state = other.state.exchange(state_type::uninitialized);
    device = std::exchange(other.device, nullptr);
    width = other.width;
    height = other.height;
    pages = std::move(other.pages);
    return *this;
}

gfx_pipeline_image::paged_image::~paged_image()
{
    if (device) {
        device->image_pipeline->free_pages(pages);
    }
}

void gfx_pipeline_image::paged_image::upload(png const& image) noexcept
{
    hi_assert(image.width() == width and image.height() == height);

    if (device) {
        hilet lock = std::scoped_lock(gfx_system_mutex);

        state = state_type::drawing;

        auto staging_image = device->image_pipeline->get_staging_pixmap(image.width(), image.height());
        image.decode_image(staging_image);
        device->image_pipeline->update_atlas_with_staging_pixmap(*this);

        state = state_type::uploaded;
    }
}

void gfx_pipeline_image::paged_image::upload(pixmap_span<sfloat_rgba16 const> image) noexcept
{
    hi_assert(image.width() == width and image.height() == height);

    if (device) {
        hilet lock = std::scoped_lock(gfx_system_mutex);

        state = state_type::drawing;

        auto staging_image = device->image_pipeline->get_staging_pixmap(image.width(), image.height());
        copy(image, staging_image);
        device->image_pipeline->update_atlas_with_staging_pixmap(*this);

        state = state_type::uploaded;
    }
}

gfx_pipeline_image::device_shared::device_shared(gfx_device const& device) : device(device)
{
    build_shaders();
    build_atlas();
}

gfx_pipeline_image::device_shared::~device_shared() {}

void gfx_pipeline_image::device_shared::destroy(gfx_device const *old_device)
{
    hi_assert_not_null(old_device);
    teardown_shaders(old_device);
    teardown_atlas(old_device);
}

std::vector<std::size_t> gfx_pipeline_image::device_shared::allocate_pages(std::size_t num_pages) noexcept
{
    while (num_pages > _atlas_free_pages.size()) {
        add_atlas_image();
    }

    auto r = std::vector<std::size_t>();
    for (int i = 0; i < num_pages; i++) {
        hilet page = _atlas_free_pages.back();
        r.push_back(page);
        _atlas_free_pages.pop_back();
    }
    return r;
}

void gfx_pipeline_image::device_shared::free_pages(std::vector<std::size_t> const& pages) noexcept
{
    _atlas_free_pages.insert(_atlas_free_pages.end(), pages.begin(), pages.end());
}

hi::pixmap_span<sfloat_rgba16> gfx_pipeline_image::device_shared::get_staging_pixmap()
{
    staging_texture.transitionLayout(device, vk::Format::eR16G16B16A16Sfloat, vk::ImageLayout::eGeneral);

    return staging_texture.pixmap.subimage(1, 1, staging_image_width - 2, staging_image_height - 2);
}

/** Get the coordinate in the atlas from a page index.
 *
 * @param page number in the atlas
 * @return x, y pixel coordinate in an atlasTexture and z the atlasTextureIndex. Inside the border.
 */
[[nodiscard]] point3 get_atlas_position(std::size_t page) noexcept
{
    // The amount of pixels per page, that is the page plus two borders.
    constexpr auto page_stride = gfx_pipeline_image::paged_image::page_size + 2;

    hilet image_nr = page / gfx_pipeline_image::device_shared::atlas_num_pages_per_image;
    hilet image_page = page % gfx_pipeline_image::device_shared::atlas_num_pages_per_image;

    return point3{
        narrow_cast<float>((image_page % gfx_pipeline_image::device_shared::atlas_num_pages_per_axis) * page_stride + 1),
        narrow_cast<float>((image_page / gfx_pipeline_image::device_shared::atlas_num_pages_per_axis) * page_stride + 1),
        narrow_cast<float>(image_nr)};
}

/** Get the position in the staging texture map to copy from.
 *
 * @param image The image
 * @param page_index The index of the page of the image.
 * @return The position into the staging map.
 */
point2 get_staging_position(const gfx_pipeline_image::paged_image& image, std::size_t page_index)
{
    hilet width_in_pages = (image.width + gfx_pipeline_image::paged_image::page_size - 1) / gfx_pipeline_image::paged_image::page_size;

    return point2{
        narrow_cast<float>((page_index % width_in_pages) * gfx_pipeline_image::paged_image::page_size + 1),
        narrow_cast<float>((page_index / width_in_pages) * gfx_pipeline_image::paged_image::page_size + 1)};
}

void gfx_pipeline_image::device_shared::make_staging_border_transparent(aarectangle border_rectangle) noexcept
{
    hilet width = ceil_cast<std::size_t>(border_rectangle.width());
    hilet height = ceil_cast<std::size_t>(border_rectangle.height());
    hilet bottom = floor_cast<std::size_t>(border_rectangle.bottom());
    hilet top = ceil_cast<std::size_t>(border_rectangle.top());
    hilet left = floor_cast<std::size_t>(border_rectangle.left());
    hilet right = ceil_cast<std::size_t>(border_rectangle.right());

    hi_assert(bottom == 0);
    hi_assert(left == 0);
    hi_assert(top >= 2);
    hi_assert(right >= 2);

    // Add a border below and above the image.
    auto border_bottom_row = staging_texture.pixmap[bottom];
    auto border_top_row = staging_texture.pixmap[top - 1];
    auto image_bottom_row = staging_texture.pixmap[bottom + 1];
    auto image_top_row = staging_texture.pixmap[top - 2];
    for (auto x = 0_uz; x != width; ++x) {
        border_bottom_row[x] = make_transparent(image_bottom_row[x]);
        border_top_row[x] = make_transparent(image_top_row[x]);
    }

    // Add a border to the left and right of the image.
    for (auto y = 0_uz; y != height; ++y) {
        auto row = staging_texture.pixmap[y];
        row[left] = make_transparent(row[left + 1]);
        row[right - 2] = make_transparent(row[right - 1]);
    }
}

void gfx_pipeline_image::device_shared::clear_staging_between_border_and_upload(
    aarectangle border_rectangle,
    aarectangle upload_rectangle) noexcept
{
    hi_assert(border_rectangle.left() == 0.0f and border_rectangle.bottom() == 0.0f);
    hi_assert(upload_rectangle.left() == 0.0f and upload_rectangle.bottom() == 0.0f);

    hilet border_top = floor_cast<std::size_t>(border_rectangle.top());
    hilet border_right = floor_cast<std::size_t>(border_rectangle.right());
    hilet upload_top = floor_cast<std::size_t>(upload_rectangle.top());
    hilet upload_right = floor_cast<std::size_t>(upload_rectangle.right());
    hi_assert(border_right <= upload_right);
    hi_assert(border_top <= upload_top);

    // Clear the area to the right of the border.
    for (auto y = 0_uz; y != border_top; ++y) {
        auto row = staging_texture.pixmap[y];
        for (auto x = border_right; x != upload_right; ++x) {
            row[x] = sfloat_rgba16{};
        }
    }

    // Clear the area above the border.
    for (auto y = border_top; y != upload_top; ++y) {
        auto row = staging_texture.pixmap[y];
        for (auto x = 0_uz; x != upload_right; ++x) {
            row[x] = sfloat_rgba16{};
        }
    }
}

void gfx_pipeline_image::device_shared::prepare_staging_for_upload(paged_image const& image) noexcept
{
    hilet image_rectangle = aarectangle{point2{1.0f, 1.0f}, image.size()};
    hilet border_rectangle = image_rectangle + 1;
    hilet upload_width = ceil(image.width, paged_image::page_size) + 2;
    hilet upload_height = ceil(image.height, paged_image::page_size) + 2;
    hilet upload_rectangle = aarectangle{extent2{narrow_cast<float>(upload_width), narrow_cast<float>(upload_height)}};

    make_staging_border_transparent(border_rectangle);
    clear_staging_between_border_and_upload(border_rectangle, upload_rectangle);

    // Flush the given image, everything that may be uploaded.
    static_assert(std::is_same_v<decltype(staging_texture.pixmap)::value_type, sfloat_rgba16>);
    device.flushAllocation(staging_texture.allocation, 0, upload_height * staging_texture.pixmap.stride() * 8);
    staging_texture.transitionLayout(device, vk::Format::eR16G16B16A16Sfloat, vk::ImageLayout::eTransferSrcOptimal);
}

void gfx_pipeline_image::device_shared::update_atlas_with_staging_pixmap(paged_image const& image) noexcept
{
    prepare_staging_for_upload(image);

    std::array<std::vector<vk::ImageCopy>, atlas_maximum_num_images> regions_to_copy_per_atlas_texture;
    for (std::size_t index = 0; index < size(image.pages); index++) {
        hilet page = image.pages.at(index);

        hilet src_position = get_staging_position(image, index);
        hilet dst_position = get_atlas_position(page);

        // Copy including a 1 pixel border.
        constexpr auto width = narrow_cast<int32_t>(paged_image::page_size + 2);
        constexpr auto height = narrow_cast<int32_t>(paged_image::page_size + 2);
        hilet src_x = floor_cast<int32_t>(src_position.x() - 1.0f);
        hilet src_y = floor_cast<int32_t>(src_position.y() - 1.0f);
        hilet dst_x = floor_cast<int32_t>(dst_position.x() - 1.0f);
        hilet dst_y = floor_cast<int32_t>(dst_position.y() - 1.0f);
        hilet dst_z = floor_cast<std::size_t>(dst_position.z());

        auto& regionsToCopy = regions_to_copy_per_atlas_texture.at(dst_z);
        regionsToCopy.emplace_back(
            vk::ImageSubresourceLayers{vk::ImageAspectFlagBits::eColor, 0, 0, 1},
            vk::Offset3D{src_x, src_y, 0},
            vk::ImageSubresourceLayers{vk::ImageAspectFlagBits::eColor, 0, 0, 1},
            vk::Offset3D{dst_x, dst_y, 0},
            vk::Extent3D{width, height, 1});
    }

    for (std::size_t atlas_texture_index = 0; atlas_texture_index < size(atlas_textures); atlas_texture_index++) {
        hilet& regions_to_copy = regions_to_copy_per_atlas_texture.at(atlas_texture_index);
        if (regions_to_copy.empty()) {
            continue;
        }

        auto& atlas_texture = atlas_textures.at(atlas_texture_index);
        atlas_texture.transitionLayout(device, vk::Format::eR16G16B16A16Sfloat, vk::ImageLayout::eTransferDstOptimal);

        device.copyImage(
            staging_texture.image,
            vk::ImageLayout::eTransferSrcOptimal,
            atlas_texture.image,
            vk::ImageLayout::eTransferDstOptimal,
            regions_to_copy);
    }
}

void gfx_pipeline_image::device_shared::prepare_atlas_for_rendering()
{
    for (auto& atlas_texture : atlas_textures) {
        atlas_texture.transitionLayout(device, vk::Format::eR16G16B16A16Sfloat, vk::ImageLayout::eShaderReadOnlyOptimal);
    }
}

void gfx_pipeline_image::device_shared::draw_in_command_buffer(vk::CommandBuffer const& commandBuffer)
{
    commandBuffer.bindIndexBuffer(device.quadIndexBuffer, 0, vk::IndexType::eUint16);
}

void gfx_pipeline_image::device_shared::build_shaders()
{
    vertex_shader_module = device.loadShader(URL("resource:image_vulkan.vert.spv"));
    fragment_shader_module = device.loadShader(URL("resource:image_vulkan.frag.spv"));

    shader_stages = {
        {vk::PipelineShaderStageCreateFlags(), vk::ShaderStageFlagBits::eVertex, vertex_shader_module, "main"},
        {vk::PipelineShaderStageCreateFlags(), vk::ShaderStageFlagBits::eFragment, fragment_shader_module, "main"}};
}

void gfx_pipeline_image::device_shared::teardown_shaders(gfx_device const *vulkanDevice)
{
    hi_assert_not_null(vulkanDevice);
    vulkanDevice->destroy(vertex_shader_module);
    vulkanDevice->destroy(fragment_shader_module);
}

void gfx_pipeline_image::device_shared::add_atlas_image()
{
    hilet current_image_index = size(atlas_textures);

    // Create atlas image
    vk::ImageCreateInfo const imageCreateInfo = {
        vk::ImageCreateFlags(),
        vk::ImageType::e2D,
        vk::Format::eR16G16B16A16Sfloat,
        vk::Extent3D(atlas_image_axis_size, atlas_image_axis_size, 1),
        1, // mipLevels
        1, // arrayLayers
        vk::SampleCountFlagBits::e1,
        vk::ImageTiling::eOptimal,
        vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled,
        vk::SharingMode::eExclusive,
        0,
        nullptr,
        vk::ImageLayout::eUndefined};
    VmaAllocationCreateInfo allocationCreateInfo = {};
    auto allocation_name = std::format("image-pipeline atlas image {}", current_image_index);
    allocationCreateInfo.flags = VMA_ALLOCATION_CREATE_USER_DATA_COPY_STRING_BIT;
    allocationCreateInfo.pUserData = const_cast<char *>(allocation_name.c_str());
    allocationCreateInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

    hilet[atlasImage, atlasImageAllocation] = device.createImage(imageCreateInfo, allocationCreateInfo);
    device.setDebugUtilsObjectNameEXT(atlasImage, allocation_name.c_str());

    hilet atlasImageView = device.createImageView(
        {vk::ImageViewCreateFlags(),
         atlasImage,
         vk::ImageViewType::e2D,
         imageCreateInfo.format,
         vk::ComponentMapping(),
         {
             vk::ImageAspectFlagBits::eColor,
             0, // baseMipLevel
             1, // levelCount
             0, // baseArrayLayer
             1 // layerCount
         }});

    atlas_textures.push_back({atlasImage, atlasImageAllocation, atlasImageView});

    // Add pages for this image to free list.
    hilet page_offset = current_image_index * atlas_num_pages_per_image;
    for (int i = 0; i < atlas_num_pages_per_image; i++) {
        _atlas_free_pages.push_back({page_offset + i});
    }

    // Build image descriptor info.
    for (std::size_t i = 0; i < size(atlas_descriptor_image_infos); i++) {
        // Point the descriptors to each imageView,
        // repeat the first imageView if there are not enough.
        atlas_descriptor_image_infos.at(i) = {
            vk::Sampler(),
            i < atlas_textures.size() ? atlas_textures.at(i).view : atlas_textures.at(0).view,
            vk::ImageLayout::eShaderReadOnlyOptimal};
    }
}

void gfx_pipeline_image::device_shared::build_atlas()
{
    // Create staging image
    vk::ImageCreateInfo const imageCreateInfo = {
        vk::ImageCreateFlags(),
        vk::ImageType::e2D,
        vk::Format::eR16G16B16A16Sfloat,
        vk::Extent3D(staging_image_width, staging_image_height, 1),
        1, // mipLevels
        1, // arrayLayers
        vk::SampleCountFlagBits::e1,
        vk::ImageTiling::eLinear,
        vk::ImageUsageFlagBits::eTransferSrc,
        vk::SharingMode::eExclusive,
        0,
        nullptr,
        vk::ImageLayout::ePreinitialized};
    VmaAllocationCreateInfo allocationCreateInfo = {};
    allocationCreateInfo.flags = VMA_ALLOCATION_CREATE_USER_DATA_COPY_STRING_BIT;
    allocationCreateInfo.pUserData = const_cast<char *>("image-pipeline staging image");
    allocationCreateInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
    hilet[image, allocation] = device.createImage(imageCreateInfo, allocationCreateInfo);
    device.setDebugUtilsObjectNameEXT(image, "image-pipeline staging image");
    hilet data = device.mapMemory<sfloat_rgba16>(allocation);

    staging_texture = {
        image,
        allocation,
        vk::ImageView(),
        hi::pixmap_span<sfloat_rgba16>{data.data(), imageCreateInfo.extent.width, imageCreateInfo.extent.height}};

    vk::SamplerCreateInfo const samplerCreateInfo = {
        vk::SamplerCreateFlags(),
        vk::Filter::eLinear, // magFilter
        vk::Filter::eLinear, // minFilter
        vk::SamplerMipmapMode::eNearest, // mipmapMode
        vk::SamplerAddressMode::eRepeat, // addressModeU
        vk::SamplerAddressMode::eRepeat, // addressModeV
        vk::SamplerAddressMode::eRepeat, // addressModeW
        0.0, // mipLodBias
        VK_FALSE, // anisotropyEnable
        0.0, // maxAnisotropy
        VK_FALSE, // compareEnable
        vk::CompareOp::eNever,
        0.0, // minLod
        0.0, // maxLod
        vk::BorderColor::eFloatTransparentBlack,
        VK_FALSE // unnormalizedCoordinates
    };
    atlas_sampler = device.createSampler(samplerCreateInfo);

    atlas_sampler_descriptor_image_info = {atlas_sampler, vk::ImageView(), vk::ImageLayout::eUndefined};

    // There needs to be at least one atlas image, so the array of samplers can point to
    // the single image.
    add_atlas_image();
}

void gfx_pipeline_image::device_shared::teardown_atlas(gfx_device const *old_device)
{
    hi_assert_not_null(old_device);
    old_device->destroy(atlas_sampler);

    for (const auto& atlas_texture : atlas_textures) {
        old_device->destroy(atlas_texture.view);
        old_device->destroyImage(atlas_texture.image, atlas_texture.allocation);
    }
    atlas_textures.clear();

    old_device->unmapMemory(staging_texture.allocation);
    old_device->destroyImage(staging_texture.image, staging_texture.allocation);
}

void gfx_pipeline_image::device_shared::place_vertices(
    vector_span<vertex>& vertices,
    aarectangle const& clipping_rectangle,
    quad const& box,
    paged_image const& image) noexcept
{
    hi_assert(image.state == paged_image::state_type::uploaded);

    constexpr auto page_size2 =
        f32x4{i32x4{narrow_cast<int32_t>(paged_image::page_size), narrow_cast<int32_t>(paged_image::page_size)}};

    hilet image_size = image.size();
    hilet size_in_float_pages = f32x4{image.size_in_float_pages()};
    hilet size_in_int_pages = i32x4{ceil(size_in_float_pages)};
    hilet num_columns = narrow_cast<std::size_t>(size_in_int_pages.x());
    hilet num_rows = narrow_cast<std::size_t>(size_in_int_pages.y());

    hilet page_to_quad_ratio = rcp(size_in_float_pages);
    hilet page_to_quad_ratio_x = scale3{page_to_quad_ratio.xxx1()};
    hilet page_to_quad_ratio_y = scale3{page_to_quad_ratio.yyy1()};
    hilet left_increment = page_to_quad_ratio_y * box.left();
    hilet right_increment = page_to_quad_ratio_y * box.right();

    auto left_bottom = box.p0;
    auto right_bottom = box.p1;
    auto bottom_increment = page_to_quad_ratio_x * (right_bottom - left_bottom);
    auto it = image.pages.begin();
    for (std::size_t page_index = 0, row_nr = 0; row_nr != num_rows; ++row_nr) {
        hilet left_top = left_bottom + left_increment;
        hilet right_top = right_bottom + right_increment;
        hilet top_increment = page_to_quad_ratio_x * (right_top - left_top);

        auto new_p0 = left_bottom;
        auto new_p2 = left_top;
        for (std::size_t column_nr = 0; column_nr != num_columns; ++column_nr, ++page_index, ++it) {
            hilet new_p1 = new_p0 + bottom_increment;
            hilet new_p3 = new_p2 + top_increment;

            // The new quad, limited to the right-top corner of the original quad.
            hilet atlas_position = get_atlas_position(*it);

            hilet xy = f32x4{i32x4{narrow_cast<int32_t>(column_nr), narrow_cast<int32_t>(row_nr)} * paged_image::page_size};
            hilet uv_rectangle = rectangle{atlas_position, extent2{page_size2}};

            vertices.emplace_back(new_p0, clipping_rectangle, get<0>(uv_rectangle));
            vertices.emplace_back(new_p1, clipping_rectangle, get<1>(uv_rectangle));
            vertices.emplace_back(new_p2, clipping_rectangle, get<2>(uv_rectangle));
            vertices.emplace_back(new_p3, clipping_rectangle, get<3>(uv_rectangle));

            new_p0 = new_p1;
            new_p2 = new_p3;
        }

        left_bottom = left_top;
        right_bottom = right_top;
        bottom_increment = top_increment;
    }
}

}} // namespace hi::v1
