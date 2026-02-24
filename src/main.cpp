#include "application.hpp"
#include "vma_allocator.hpp"
#include "vma_buffer.hpp"
#include "vlk/vlk.hpp"

#define VMA_STATIC_VULKAN_FUNCTIONS 0
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 0
#include <vma/vk_mem_alloc.h>

#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3/SDL_vulkan.h>

#include <glm/glm.hpp>

#include <algorithm>
#include <array>
#include <fstream>
#include <iostream>
#include <print>
#include <ranges>
#include <stdexcept>

constexpr int FRAMES_IN_FLIGHT = 2;

VkSurfaceFormatKHR choose_swap_surface_format(std::span<const VkSurfaceFormatKHR> formats) {
    for (auto& format : formats) {
        if (format.format == VK_FORMAT_B8G8R8_SRGB && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return format;
        }
    }

    return formats[0];
}

std::vector<uint32_t> read_file(const char* filename) {
    std::ifstream file{ filename, std::ios::binary | std::ios::ate };
    if (!file.is_open()) {
        throw std::runtime_error{ "Failed to open file." };
    }

    size_t file_size = static_cast<size_t>(file.tellg());
    std::vector<uint32_t> buffer(file_size / sizeof(uint32_t));
    file.seekg(0, std::ios::beg);
    file.read(reinterpret_cast<char*>(buffer.data()), file_size);
    file.close();

    return buffer;
}

void transition_image_layout(VkCommandBuffer cmd_buffer,
                             VkImage image,
                             VkImageLayout old_layout,
                             VkImageLayout new_layout,
                             VkAccessFlags2 src_access,
                             VkAccessFlags2 dst_access,
                             VkPipelineStageFlags2 src_stage,
                             VkPipelineStageFlags2 dst_stage) {
    VkImageMemoryBarrier2 barrier = {};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
    barrier.srcStageMask = src_stage;
    barrier.srcAccessMask = src_access;
    barrier.dstStageMask = dst_stage;
    barrier.dstAccessMask = dst_access;
    barrier.oldLayout = old_layout;
    barrier.newLayout = new_layout;
    barrier.image = image;
    barrier.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
    VkDependencyInfo deps_info = {};
    deps_info.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    deps_info.imageMemoryBarrierCount = 1;
    deps_info.pImageMemoryBarriers = &barrier;
    vkCmdPipelineBarrier2(cmd_buffer, &deps_info);
}

struct Vertex {
    glm::vec2 position;
    glm::vec3 color;
};

const std::array<Vertex, 4> vertices = {
    Vertex{ { -0.5f, -0.5f }, { 1.0f, 0.0f, 0.0f } },
    Vertex{ {  0.5f, -0.5f }, { 0.0f, 1.0f, 0.0f } },
    Vertex{ {  0.5f,  0.5f }, { 0.0f, 0.0f, 1.0f } },
    Vertex{ { -0.5f,  0.5f }, { 1.0f, 1.0f, 0.0f } }
};

const std::array<uint16_t, 6> indices = {
    0, 1, 2, 2, 3, 0
};

std::unique_ptr<VMAAllocator> vma_allocator;
std::unique_ptr<vlk::Swapchain> vk_swapchain;
std::unique_ptr<vlk::Pipeline> vk_pipeline;
std::unique_ptr<VMABuffer> vma_vertex_buffer; // TODO(Kostu): use one buffer for vertex and index data
std::unique_ptr<VMABuffer> vma_index_buffer;
std::unique_ptr<vlk::CommandPool> vk_cmd_pool;
std::array<VkCommandBuffer, FRAMES_IN_FLIGHT> vk_cmd_buffers;
std::vector<vlk::Semaphore> vk_present_semaphores;
std::vector<vlk::Semaphore> vk_render_semaphores;
std::vector<vlk::Fence> vk_draw_fences;
VkSurfaceCapabilitiesKHR vk_surface_caps;
VkSurfaceFormatKHR vk_surface_format;
VkExtent2D vk_extent;
uint32_t frame_index = 0;

void record_cmd_buffer(VkImage image, VkImageView image_view) {
    VkCommandBuffer cmd_buffer = vk_cmd_buffers[frame_index];

    const VkCommandBufferBeginInfo cmd_buffer_begin_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO
    };
    vkBeginCommandBuffer(cmd_buffer, &cmd_buffer_begin_info);

    transition_image_layout(cmd_buffer,
                            image,
                            VK_IMAGE_LAYOUT_UNDEFINED,
                            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                            {},
                            VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
                            VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
                            VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT);

    VkClearValue clear_color;
    clear_color.color = { 0.0f, 0.0f, 0.0f, 1.0f };
    const VkRenderingAttachmentInfo attachment_info = {
        .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
        .imageView = image_view,
        .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .clearValue = clear_color
    };

    VkRenderingInfo rendering_info = {};
    rendering_info.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
    rendering_info.renderArea = { .offset = { 0, 0 }, .extent = vk_extent };
    rendering_info.layerCount = 1;
    rendering_info.colorAttachmentCount = 1;
    rendering_info.pColorAttachments = &attachment_info;
    vkCmdBeginRendering(cmd_buffer, &rendering_info);

    vkCmdBindPipeline(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *vk_pipeline);
    
    VkViewport viewport = { 0.0f, 0.0f, static_cast<float>(vk_extent.width), static_cast<float>(vk_extent.height), 0.0f, 1.0f };
    vkCmdSetViewport(cmd_buffer, 0, 1, &viewport);
    VkRect2D scissor = { { 0, 0 }, vk_extent };
    vkCmdSetScissor(cmd_buffer, 0, 1, &scissor);

    const VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(cmd_buffer, 0, 1, vma_vertex_buffer->ptr(), &offset);

    vkCmdBindIndexBuffer(cmd_buffer, *vma_index_buffer, 0, VK_INDEX_TYPE_UINT16);

    vkCmdDrawIndexed(cmd_buffer, indices.size(), 1, 0, 0, 0);

    vkCmdEndRendering(cmd_buffer);
    
    transition_image_layout(cmd_buffer,
                            image,
                            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                            VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                            VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
                            {},
                            VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
                            VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT);

    vkEndCommandBuffer(cmd_buffer);
}

void recreate_swapchain(const vlk::Device& device, const vlk::Surface& surface) {
    device.wait_idle();
    vk_extent = surface.get_extent(vk_surface_caps);
    vk_swapchain = std::make_unique<vlk::Swapchain>(device, surface, vk_surface_caps, vk_surface_format, 3u, vk_extent);
}

uint32_t find_memory_type(const vlk::Device device, uint32_t type_filter, VkMemoryPropertyFlags properties) {
    auto memory_properties = device.get_physical_device().get_memory_properties();
    for (uint32_t i = 0; i < memory_properties.memoryTypeCount; i++) {
        if ((type_filter & (1 << i)) && (memory_properties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }

    throw std::runtime_error("Failed to find suitable memory type!");
    return 0;
}

SDL_AppResult SDL_AppInit(void** appstate, int argc, char* argv[]) {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        std::println(std::cerr, "SDL_Init failed: {}.", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    VkResult result = volkInitialize();
    if (result != VK_SUCCESS) {
        std::println(std::cerr, "Failed to load Vulkan library.");
        return SDL_APP_FAILURE;
    }

    Application* app = new Application;
    *appstate = app;

    vma_allocator = std::make_unique<VMAAllocator>(app->get_instance(), app->get_device(), VK_API_VERSION_1_4);

    auto physical_device = app->get_device().get_physical_device();

    vk_surface_caps = physical_device.get_surface_capabilities(app->get_surface());
    auto surface_formats = physical_device.get_surface_formats(app->get_surface());

    vk_surface_format = choose_swap_surface_format(surface_formats);

    recreate_swapchain(app->get_device(), app->get_surface());

    auto shader_code = read_file("shaders/simple.spv");
    if (shader_code[0] != 0x07230203) {
        std::println(std::cerr, "Not valid SPIR-V");
        return SDL_APP_FAILURE;
    }
    
    VkShaderModuleCreateInfo shader_module_create_info = {};
    shader_module_create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shader_module_create_info.codeSize = shader_code.size() * sizeof(uint32_t);
    shader_module_create_info.pCode = shader_code.data();
    VkShaderModule vk_shader_module;
    vkCreateShaderModule(app->get_device(), &shader_module_create_info, nullptr, &vk_shader_module);

    std::vector<VkPipelineShaderStageCreateInfo> stages(2);
    stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[0].module = vk_shader_module;
    stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    stages[0].pName = "vert_main";
    stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[1].module = vk_shader_module;
    stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    stages[1].pName = "frag_main";

    const VkVertexInputBindingDescription vertex_binding_description = {
        .binding = 0,
        .stride = sizeof(Vertex),
        .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
    };

    std::array<VkVertexInputAttributeDescription, 2> vertex_attribute_descriptions = {
        VkVertexInputAttributeDescription{
            .location = 0,
            .binding = 0,
            .format = VK_FORMAT_R32G32_SFLOAT,
            .offset = offsetof(Vertex, position)
        },
        VkVertexInputAttributeDescription{
            .location = 1,
            .binding = 0,
            .format = VK_FORMAT_R32G32B32_SFLOAT,
            .offset = offsetof(Vertex, color)
        }
    };

    vk_pipeline = std::make_unique<vlk::Pipeline>(app->get_device(), stages, vk_surface_format.format, vertex_binding_description, vertex_attribute_descriptions);

    vkDestroyShaderModule(app->get_device(), vk_shader_module, nullptr);

    auto staging_cmd_pool = std::make_unique<vlk::CommandPool>(app->get_device(), VK_COMMAND_POOL_CREATE_TRANSIENT_BIT, app->get_queue_family());

    {
        const VkDeviceSize vertex_buffer_size = vertices.size() * sizeof(Vertex);

        VMABuffer vertex_staging_buffer{ *vma_allocator,
                                         vertex_buffer_size,
                                         VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                         VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT };

        vertex_staging_buffer.copy_memory_to_allocation(vertices.data(), vertex_buffer_size);

        vma_vertex_buffer = std::make_unique<VMABuffer>(*vma_allocator,
                                                        vertex_buffer_size,
                                                        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                                        0);

        const VkDeviceSize index_buffer_size = indices.size() * sizeof(uint16_t);

        VMABuffer index_staging_buffer{ *vma_allocator,
                                         index_buffer_size,
                                         VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                         VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT };

        index_staging_buffer.copy_memory_to_allocation(indices.data(), index_buffer_size);

        vma_index_buffer = std::make_unique<VMABuffer>(*vma_allocator,
                                                       index_buffer_size,
                                                       VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                                       0);

        auto staging_cmd_buffer = staging_cmd_pool->allocate_command_buffer();
        staging_cmd_buffer->begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
        VkBufferCopy vertex_region{ 0, 0, vertex_buffer_size };
        vkCmdCopyBuffer(*staging_cmd_buffer, vertex_staging_buffer, *vma_vertex_buffer, 1, &vertex_region);
        VkBufferCopy index_region{ 0, 0, index_buffer_size };
        vkCmdCopyBuffer(*staging_cmd_buffer, index_staging_buffer, *vma_index_buffer, 1, &index_region);
        staging_cmd_buffer->end();
        
        app->get_queue().submit(*staging_cmd_buffer);
        app->get_queue().wait_idle();
    }

    vk_cmd_pool = std::make_unique<vlk::CommandPool>(app->get_device(), VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT, app->get_queue_family());

    VkCommandBufferAllocateInfo cmd_buffer_alloc_info = {};
    cmd_buffer_alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmd_buffer_alloc_info.commandPool = *vk_cmd_pool;
    cmd_buffer_alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmd_buffer_alloc_info.commandBufferCount = FRAMES_IN_FLIGHT;
    vkAllocateCommandBuffers(app->get_device(), &cmd_buffer_alloc_info, vk_cmd_buffers.data());

    for (uint32_t i = 0; i < vk_swapchain->get_image_count(); ++i) {
        vk_render_semaphores.emplace_back(app->get_device());
    }
    for (int i = 0; i < FRAMES_IN_FLIGHT; ++i) {
        vk_present_semaphores.emplace_back(app->get_device());
        vk_draw_fences.emplace_back(app->get_device(), true);
    }

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void* appstate, SDL_Event* event) {
    switch (event->type) {
    case SDL_EVENT_QUIT:
    case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
        return SDL_APP_SUCCESS;
    }

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void* appstate) {
    Application* app = static_cast<Application*>(appstate);

    vk_draw_fences[frame_index].wait();

    auto next_image = vk_swapchain->acquire_next_image(vk_present_semaphores[frame_index]);
    if (next_image.should_recreate_swapchain) {
        recreate_swapchain(app->get_device(), app->get_surface());
        return SDL_APP_CONTINUE;
    }

    record_cmd_buffer(next_image.image, next_image.image_view);

    vk_draw_fences[frame_index].reset();

    app->get_queue().submit(vk_cmd_buffers[frame_index],
                            &vk_present_semaphores[frame_index],
                            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                            &vk_render_semaphores[next_image.image_index],
                            &vk_draw_fences[frame_index]);

    if (app->get_queue().present(*vk_swapchain, vk_render_semaphores[next_image.image_index], next_image.image_index)) {
        recreate_swapchain(app->get_device(), app->get_surface());
    }

    frame_index = (frame_index + 1) % FRAMES_IN_FLIGHT;

    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void* appstate, SDL_AppResult result) {
    Application* app = static_cast<Application*>(appstate);

    app->get_device().wait_idle();

    vk_render_semaphores.clear();
    vk_present_semaphores.clear();
    vk_draw_fences.clear();
    vkFreeCommandBuffers(app->get_device(), *vk_cmd_pool, static_cast<uint32_t>(vk_cmd_buffers.size()), vk_cmd_buffers.data());
    vk_cmd_pool.reset();
    vma_index_buffer.reset();
    vma_vertex_buffer.reset();
    vk_pipeline.reset();
    vk_swapchain.reset();
    vma_allocator.reset();

    delete app;
}
