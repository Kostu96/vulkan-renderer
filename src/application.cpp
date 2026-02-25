#include "application.hpp"

#include <glm/glm.hpp>
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

#include <algorithm>
#include <format>
#include <print>
#include <ranges>
#include <span>
#include <stdexcept>
#include <vector>

namespace {

constexpr uint32_t NUM_FRAMES_IN_FLIGHT = 2;

std::vector<const char*> get_required_instance_extensions() {
    uint32_t sdl_vk_extensions_count = 0;
    auto sdl_vk_extensions = SDL_Vulkan_GetInstanceExtensions(&sdl_vk_extensions_count);

    std::vector<const char*> extensions{ sdl_vk_extensions_count };
    for (uint32_t i = 0; i < sdl_vk_extensions_count; ++i) {
        extensions[i] = sdl_vk_extensions[i];
    }

#ifndef NDEBUG
    extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif

    return extensions;
}

std::vector<const char*> get_required_layers() {
    std::vector<const char*> layers;
#ifndef NDEBUG
    layers.push_back("VK_LAYER_KHRONOS_validation");
#endif

    return layers;
}

const VkApplicationInfo app_info = {
    .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
    .pApplicationName = "Vulkan Renderer",
    .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
    .pEngineName = "No Engine",
    .engineVersion = VK_MAKE_VERSION(1, 0, 0),
    .apiVersion = VK_API_VERSION_1_4
};

const std::vector<const char*> required_device_extensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

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

}

Application::Application() :
    window_{ SDL_CreateWindow(app_info.pApplicationName,
                              1440, 900,
                              SDL_WINDOW_VULKAN | SDL_WINDOW_HIGH_PIXEL_DENSITY | SDL_WINDOW_RESIZABLE),
             SDL_DestroyWindow },
    vk_instance_(app_info, get_required_instance_extensions(), get_required_layers()),
    vk_surface_{ window_.get(), vk_instance_ },
    vk_device_{ create_device() },
    vk_queue_{ vk_device_.get_queue(vk_queue_family_index_) },
    vk_staging_cmd_pool_{ vk_device_, VK_COMMAND_POOL_CREATE_TRANSIENT_BIT, vk_queue_family_index_ },
    vk_cmd_pool_{ vk_device_, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT, vk_queue_family_index_ },
    vk_memory_allocator_{ vk_instance_, vk_device_, app_info.apiVersion },
    vk_surface_caps_{ vk_device_.get_physical_device().get_surface_capabilities(vk_surface_) },
    vk_surface_format_{ choose_swapchain_surface_format() },
    vk_swapchain_{ create_swapchain() },
    vk_pipeline_{ create_pipeline() },
    vk_vertex_buffer_{ vk_memory_allocator_, vertices.size() * sizeof(Vertex),
                       VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, 0 },
    vk_index_buffer_{ vk_memory_allocator_, indices.size() * sizeof(uint16_t),
                      VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, 0 },
    vk_cmd_buffers_{ vk_cmd_pool_.allocate_command_buffers(NUM_FRAMES_IN_FLIGHT) }
{
    // TODO(Kostu): this check happens too late, need to wrap SDL_Window for this
    if (!window_) {
        throw std::runtime_error(std::format("Window creation failed: {}", SDL_GetError()));
    }

    for (uint32_t i = 0; i < vk_swapchain_.get_image_count(); ++i) {
        vk_render_semaphores_.emplace_back(vk_device_);
    }
    for (uint32_t i = 0; i < NUM_FRAMES_IN_FLIGHT; ++i) {
        vk_present_semaphores_.emplace_back(vk_device_);
        vk_draw_fences_.emplace_back(vk_device_, true);
    }

    VkDeviceSize vertex_buffer_size = vk_vertex_buffer_.get_size();
    vlk::Buffer vertex_staging_buffer{ vk_memory_allocator_,
                                       vertex_buffer_size,
                                       VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                       VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT };
    vertex_staging_buffer.copy_memory_to_allocation(vertices.data(), vertex_buffer_size);

    VkDeviceSize index_buffer_size = vk_index_buffer_.get_size();
    vlk::Buffer index_staging_buffer{ vk_memory_allocator_,
                                      index_buffer_size,
                                      VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                      VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT };
    index_staging_buffer.copy_memory_to_allocation(indices.data(), index_buffer_size);

    auto staging_cmd_buffers = vk_staging_cmd_pool_.allocate_command_buffers(1);

    auto& staging_cmd_buffer = staging_cmd_buffers[0];
    staging_cmd_buffer.begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
    staging_cmd_buffer.copy_buffer(vertex_staging_buffer, vk_vertex_buffer_);
    staging_cmd_buffer.copy_buffer(index_staging_buffer, vk_index_buffer_);
    staging_cmd_buffer.end();

    vk_queue_.submit(staging_cmd_buffer);
    vk_queue_.wait_idle();

    vk_staging_cmd_pool_.free_command_buffers(staging_cmd_buffers);
}

Application::~Application() {
    vk_device_.wait_idle();
}

void Application::update() {
    static uint32_t frame_index = 0;

    vk_draw_fences_[frame_index].wait();

    auto next_image = vk_swapchain_.acquire_next_image(vk_present_semaphores_[frame_index]);
    if (next_image.should_recreate_swapchain) {
        vk_swapchain_ = create_swapchain();
        return;
    }

    const auto& current_cmd_buffer = vk_cmd_buffers_[frame_index];
    record_cmd_buffer(current_cmd_buffer, next_image.image, next_image.image_view);

    vk_draw_fences_[frame_index].reset();

    vk_queue_.submit(current_cmd_buffer,
                     &vk_present_semaphores_[frame_index],
                     VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                     &vk_render_semaphores_[next_image.image_index],
                     &vk_draw_fences_[frame_index]);

    if (vk_queue_.present(vk_swapchain_,
                          vk_render_semaphores_[next_image.image_index],
                          next_image.image_index)) {
        vk_swapchain_ = create_swapchain();
    }

    frame_index = (frame_index + 1) % NUM_FRAMES_IN_FLIGHT;
}

vlk::PhysicalDevice Application::choose_physical_device_and_queue_family() {
    auto physical_devices = vk_instance_.get_physical_devices();
    for (auto& physical_device : physical_devices) {
        // TODO(Kostu): Device scoring
        auto physical_device_props = physical_device.get_properties();
        auto physical_device_feats = physical_device.get_features();
        auto physical_device_extensions = physical_device.get_extension_properties();
        auto queue_family_props = physical_device.get_queue_family_properties();

        bool is_suitable = (physical_device_props.apiVersion >= VK_API_VERSION_1_3);

        bool has_graphics_capable_queue = false;
        for (auto [idx, queue_family] : std::views::enumerate(queue_family_props)) {
            if ((queue_family.queueFlags & VK_QUEUE_GRAPHICS_BIT) &&
                (queue_family.queueFlags & VK_QUEUE_TRANSFER_BIT) &&
                SDL_Vulkan_GetPresentationSupport(vk_instance_, physical_device, idx) &&
                physical_device.get_surface_support(idx, vk_surface_))
            {
                has_graphics_capable_queue = true;
                vk_queue_family_index_ = static_cast<uint32_t>(idx);
                break;
            }
        }
        is_suitable = is_suitable && has_graphics_capable_queue;

        bool found = true;
        for (auto& extension : required_device_extensions) {
            auto it = std::ranges::find_if(physical_device_extensions, [extension](auto& ext) {return strcmp(ext.extensionName, extension) == 0; });
            found = found && it != physical_device_extensions.end();
        }
        is_suitable = is_suitable && found;

        if (is_suitable) {
            return physical_device;
        }
    }

    throw std::runtime_error("Failed to pick Vulkan physical device.");
}

vlk::Device Application::create_device() {
    auto physical_device = choose_physical_device_and_queue_family();

    float queue_priority = 1.0f;
    VkDeviceQueueCreateInfo queue_create_info = {};
    queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_create_info.queueFamilyIndex = vk_queue_family_index_;
    queue_create_info.queueCount = 1;
    queue_create_info.pQueuePriorities = &queue_priority;

    VkPhysicalDeviceVulkan13Features vlk13_features = {};
    vlk13_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
    vlk13_features.dynamicRendering = VK_TRUE;
    vlk13_features.synchronization2 = VK_TRUE;

    VkPhysicalDeviceVulkan11Features vlk11_features = {};
    vlk11_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;
    vlk11_features.shaderDrawParameters = VK_TRUE;
    vlk11_features.pNext = &vlk13_features;

    return { physical_device, std::span{ &queue_create_info, 1 }, std::span {required_device_extensions }, &vlk11_features };
}

VkSurfaceFormatKHR Application::choose_swapchain_surface_format() {
    auto formats = vk_device_.get_physical_device().get_surface_formats(vk_surface_);
    for (auto& format : formats) {
        if (format.format == VK_FORMAT_B8G8R8_SRGB && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return format;
        }
    }

    return formats[0];
}

vlk::Swapchain Application::create_swapchain() {
    vk_device_.wait_idle();

    constexpr uint32_t image_count = 3;
    vk_frame_extent_ = vk_surface_.get_extent(vk_surface_caps_);
    
    return { vk_device_, vk_surface_, vk_surface_caps_, vk_surface_format_, image_count, vk_frame_extent_ };
}

vlk::Pipeline Application::create_pipeline() {
    vlk::ShaderModule shader_module{ vk_device_, "shaders/simple.spv" };

    std::vector<VkPipelineShaderStageCreateInfo> stages(2);
    stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[0].module = shader_module;
    stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    stages[0].pName = "vert_main";
    stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[1].module = shader_module;
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

    return { vk_device_,
             stages,
             vk_surface_format_.format,
             vertex_binding_description,
             vertex_attribute_descriptions };
}

void Application::record_cmd_buffer(const vlk::CommandBuffer& cmd_buffer, VkImage image, VkImageView image_view) {
    cmd_buffer.begin();

    transition_image_layout(cmd_buffer,
                            image,
                            VK_IMAGE_LAYOUT_UNDEFINED,
                            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                            {},
                            VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
                            VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
                            VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT);

    cmd_buffer.begin_rendering({ 0.0f, 0.0f, 0.0f, 1.0f }, image_view, vk_frame_extent_);

    vkCmdBindPipeline(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vk_pipeline_);

    VkViewport viewport = { 0.0f, 0.0f, static_cast<float>(vk_frame_extent_.width), static_cast<float>(vk_frame_extent_.height), 0.0f, 1.0f };
    vkCmdSetViewport(cmd_buffer, 0, 1, &viewport);
    VkRect2D scissor = { { 0, 0 }, vk_frame_extent_ };
    vkCmdSetScissor(cmd_buffer, 0, 1, &scissor);

    const VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(cmd_buffer, 0, 1, vk_vertex_buffer_.ptr(), &offset);

    vkCmdBindIndexBuffer(cmd_buffer, vk_index_buffer_, 0, VK_INDEX_TYPE_UINT16);

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

    cmd_buffer.end();
}
