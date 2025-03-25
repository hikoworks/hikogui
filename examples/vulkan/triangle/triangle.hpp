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

#include "hikogui/hikogui.hpp"
#include <vulkan/vulkan.h>
#include <vma/vk_mem_alloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <fstream>
#include <vector>
#include <exception>
#include <filesystem>

// This example is a copy of https://github.com/SaschaWillems/Vulkan/blob/master/examples/triangle/triangle_impl.cpp
// It is modified so:
//  - It does not use any other utility files.
//  - It is reordered so that the swap-chain can be replaced on window resize.
//  - Uses the vulkan-memory-allocator.
//  - Uses an externally provided vulkan-instance, vulkan-device, vulkan-queue and swap-chains.
//  - Uses an externally provided view-port and render-area.
class TriangleExample {
public:
    TriangleExample(VmaAllocator allocator, VkDevice device, VkQueue queue, uint32_t queueFamilyIndex);
    ~TriangleExample();

    void buildForNewSwapchain(std::vector<VkImageView> const& imageViews, VkExtent2D imageSize, VkFormat imageFormat);
    void teardownForLostSwapchain();

    void render(
        uint32_t currentBuffer,
        VkSemaphore presentCompleteSemaphore,
        VkSemaphore renderCompleteSemaphore,
        VkRect2D renderArea,
        VkRect2D viewPort);

private:
    // Vertex layout used in this example
    struct Vertex {
        float position[3];
        float color[3];
    };

    // For simplicity we use the same uniform block layout as in the shader:
    //
    //	layout(set = 0, binding = 0) uniform UBO
    //	{
    //		mat4 projectionMatrix;
    //		mat4 modelMatrix;
    //		mat4 viewMatrix;
    //	} ubo;
    //
    // This way we can just memcopy the ubo data to the ubo
    // Note: You should use data types that align with the GPU in order to avoid manual padding (vec4, mat4)
    struct Uniform {
        hi::sfloat_rgba32x4 projectionMatrix;
        hi::sfloat_rgba32x4 modelMatrix;
        hi::sfloat_rgba32x4 viewMatrix;
    };

    bool hasSwapchain = false;
    VkRect2D previousViewPort = {};
    VkRect2D previousRenderArea = {};

    VmaAllocator allocator;

    // The vulkan device to use for drawing.
    VkDevice device;

    // The graphic draw queue.
    VkQueue queue;
    uint32_t queueFamilyIndex;

    VkCommandPool cmdPool;
    std::vector<VkCommandBuffer> drawCmdBuffers;

    VmaAllocation depthImageAllocation;
    VkImage depthImage;
    VkImageView depthImageView;

    std::vector<VkFramebuffer> frameBuffers;

    VkRenderPass renderPass;

    // The pipeline layout is used by a pipeline to access the descriptor sets
    // It defines interface (without binding any actual data) between the shader stages used by the pipeline and the shader
    // resources A pipeline layout can be shared among multiple pipelines as long as their interfaces match
    VkPipelineLayout pipelineLayout;

    // Pipelines (often called "pipeline state objects") are used to bake all states that affect a pipeline
    // While in OpenGL every state can be changed at (almost) any time, Vulkan requires to layout the graphics (and compute)
    // pipeline states upfront So for each combination of non-dynamic pipeline states you need a new pipeline (there are a few
    // exceptions to this not discussed here) Even though this adds a new dimension of planning ahead, it's a great opportunity
    // for performance optimizations by the driver
    VkPipeline pipeline;

    VkDescriptorPool descriptorPool;

    // The descriptor set layout describes the shader binding layout (without actually referencing descriptor)
    // Like the pipeline layout it's pretty much a blueprint and can be used with different descriptor sets as long as their
    // layout matches
    VkDescriptorSetLayout descriptorSetLayout;

    // The descriptor set stores the resources bound to the binding points in a shader
    // It connects the binding points of the different shaders with the buffers and images used for those bindings
    VkDescriptorSet descriptorSet;

    // Fences
    // Used to check the completion of queue operations (e.g. command buffer execution)
    std::vector<VkFence> queueCompleteFences;

    VmaAllocation vertexBufferAllocation;
    VkBuffer vertexBuffer;

    VmaAllocation vertexIndexBufferAllocation;
    VkBuffer vertexIndexBuffer;
    uint32_t vertexIndexCount;

    VmaAllocation uniformBufferAllocation;
    VkBuffer uniformBuffer;
    VkDescriptorBufferInfo uniformBufferInfo;

    // The following functions are used in-order when TriangleExample is constructed.
    void createCommandPool();
    void destroyCommandPool();
    void createVertexBuffer();
    void destroyVertexBuffer();
    void createUniformBuffer();
    void destroyUniformBuffer();
    void createDescriptorPool();
    void destroyDescriptorPool();
    void createDescriptorSetLayout();
    void destroyDescriptorSetLayout();
    void createDescriptorSet();
    void destroyDescriptorSet();

    // The following functions are used in-order when a new swap-chain is created.
    void createRenderPass(VkFormat colorFormat, VkFormat depthFormat);
    void destroyRenderPass();
    void createDepthStencilImage(VkExtent2D imageSize, VkFormat format);
    void destroyDepthStencilImage();
    void createFrameBuffers(std::vector<VkImageView> const& imageViews, VkExtent2D imageSize);
    void destroyFrameBuffers();
    void createCommandBuffers();
    void destroyCommandBuffers();
    void createFences();
    void destroyFences();
    void createPipeline();
    void destroyPipeline();

    // The following functions are used during render().

    // Get a new command buffer from the command pool
    // If begin is true, the command buffer is also started so we can start adding commands
    VkCommandBuffer getCommandBuffer(bool begin);

    // End the command buffer and submit it to the queue
    // Uses a fence to ensure command buffer has finished executing before deleting it
    void flushCommandBuffer(VkCommandBuffer commandBuffer);

    // Build separate command buffers for every framebuffer image
    // Unlike in OpenGL all rendering commands are recorded once into command buffers that are then resubmitted to the queue
    // This allows to generate work upfront and from multiple threads, one of the biggest advantages of Vulkan
    void buildCommandBuffers(VkRect2D renderArea, VkRect2D viewPort);

    // Vulkan loads its shaders from an immediate binary representation called SPIR-V
    // Shaders are compiled offline from e.g. GLSL using the reference glslang compiler
    // This function loads such a shader from a binary file and returns a shader module structure
    VkShaderModule loadSPIRVShader(std::filesystem::path filename);

    void updateUniformBuffers(Uniform const& uniform);

    void draw(uint32_t currentBuffer, VkSemaphore presentCompleteSemaphore, VkSemaphore renderCompleteSemaphore) const;
};
