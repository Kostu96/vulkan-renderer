#include "vulkan_utils/device.hpp"
#include "vulkan_utils/fence.hpp"
#include "vulkan_utils/instance.hpp"
#include "vulkan_utils/pipeline.hpp"
#include "vulkan_utils/semaphore.hpp"
#include "vulkan_utils/swapchain.hpp"
#include "vulkan_utils/vulkan_utils.hpp"
#include "SDL_surface.hpp"

#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3/SDL_vulkan.h>

#include <algorithm>
#include <array>
#include <fstream>
#include <iostream>
#include <memory>
#include <print>
#include <ranges>
#include <vector>
#include <stdexcept>

constexpr int FRAMES_IN_FLIGHT = 2;

std::vector<const char*> get_required_extensions() {
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

SDL_Window* sdl_window = nullptr;
std::unique_ptr<vkutils::Instance> vk_instance;
std::unique_ptr<SDLSurface> vk_surface;
uint32_t vk_queue_family_index = 0;
std::unique_ptr<vkutils::Device> vk_device;
VkQueue vk_queue = VK_NULL_HANDLE;
std::unique_ptr<vkutils::Swapchain> vk_swapchain;
std::unique_ptr<vkutils::Pipeline> vk_pipeline;
VkCommandPool vk_cmd_pool;
std::array<VkCommandBuffer, FRAMES_IN_FLIGHT> vk_cmd_buffers;
std::vector<vkutils::Semaphore> vk_present_semaphores;
std::vector<vkutils::Semaphore> vk_render_semaphores;
std::vector<vkutils::Fence> vk_draw_fences;
VkSurfaceCapabilitiesKHR vk_surface_caps;
VkSurfaceFormatKHR vk_surface_format;
VkExtent2D vk_extent;
uint32_t frame_index = 0;

VkPhysicalDevice choose_physical_device(std::span<const char*> required_extensions) {
    auto physical_devices = vk_instance->get_physical_devices();
    for (auto& physical_device : physical_devices) {
        // TODO(Kostu): Device scoring
        auto physical_device_props = vkutils::get_physical_device_properties(physical_device);
        auto physical_device_feats = vkutils::get_physical_device_features(physical_device);
        auto physical_device_extensions = vkutils::get_physical_device_extension_properties(physical_device);
        auto queue_family_props = vkutils::get_physical_queue_family_properties(physical_device);

        bool is_suitable = (physical_device_props.properties.apiVersion >= VK_API_VERSION_1_3);

        bool has_graphics_capable_queue = false;
        for (auto [idx, queue_family] : std::views::enumerate(queue_family_props)) {
            if ((queue_family.queueFamilyProperties.queueFlags & VK_QUEUE_GRAPHICS_BIT) &&
                SDL_Vulkan_GetPresentationSupport(*vk_instance, physical_device, idx) &&
                vk_surface->presentation_support(physical_device, idx))
            {
                has_graphics_capable_queue = true;
                vk_queue_family_index = static_cast<uint32_t>(idx);
                break;
            }
        }
        is_suitable = is_suitable && has_graphics_capable_queue;

        bool found = true;
        for (auto& extension : required_extensions) {
            auto it = std::ranges::find_if(physical_device_extensions, [extension](auto& ext) {return strcmp(ext.extensionName, extension) == 0;});
            found = found && it != physical_device_extensions.end();
        }
        is_suitable = is_suitable && found;

        if (is_suitable) {
            return physical_device;
        }
    }

    return VK_NULL_HANDLE;
}

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

    vkCmdDraw(cmd_buffer, 3, 1, 0, 0);

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

void recreate_swapchain() {
    vk_device->wait_idle();
    vk_extent = vk_surface->get_extent(vk_surface_caps);
    vk_swapchain = std::make_unique<vkutils::Swapchain>(*vk_device, *vk_surface, vk_surface_caps, vk_surface_format, 3u, vk_extent);
}

SDL_AppResult SDL_AppInit(void** appstate, int argc, char* argv[]) {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        std::println(std::cerr, "SDL_Init failed: {}.", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    sdl_window = SDL_CreateWindow("Vulkan Renderer",
        1440, 900,
        SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);

    if (sdl_window == nullptr) {
        std::println(std::cerr, "Window creation failed: {}", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    VkResult vk_result = volkInitialize();
    if (vk_result != VK_SUCCESS) {
        std::println(std::cerr, "Failed to load Vulkan library.");
        return SDL_APP_FAILURE;
    }

    std::vector<const char*> required_extensions = get_required_extensions();
    auto instance_extension_props = vkutils::Instance::get_extension_properties();
    for (auto& extension : required_extensions) {
        if (std::ranges::none_of(instance_extension_props,
                                 [extension](auto& extension_prop) {
                                 return strcmp(extension_prop.extensionName, extension) == 0; })) {
            std::println(std::cerr, "Required SDL Vulkan extension not supported: {}.", extension);
            return SDL_APP_FAILURE;
        }
    }

    std::vector<const char*> required_layers;
#ifndef NDEBUG
    required_layers.push_back("VK_LAYER_KHRONOS_validation");
#endif

    auto instance_layer_props = vkutils::Instance::get_layer_properties();

    if (std::ranges::any_of(required_layers,
                            [&instance_layer_props](auto& required_layer) {
            return std::ranges::none_of(instance_layer_props,
                                        [required_layer](auto& layer) {
                return strcmp(layer.layerName, required_layer) == 0; }); })) {
        std::println(std::cerr, "One or more required layers are not supported.");
        return SDL_APP_FAILURE;
    }
    
    VkApplicationInfo app_info = {};
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pApplicationName = "Vulkan Renderer";
    app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.pEngineName = "No Engine";
    app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.apiVersion = VK_API_VERSION_1_4;

    vk_instance = std::make_unique<vkutils::Instance>(app_info, std::span{ required_extensions }, std::span{ required_layers });
    vk_surface = std::make_unique<SDLSurface>(sdl_window, *vk_instance);

    std::vector<const char*> required_device_extensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };
    auto physical_device = choose_physical_device(required_device_extensions);
    if (physical_device == VK_NULL_HANDLE) {
        std::println(std::cerr, "Failed to pick Vulkan physical device.");
        return SDL_APP_FAILURE;
    }
    
    float queue_priority = 1.0f;
    VkDeviceQueueCreateInfo queue_create_info = {};
    queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_create_info.queueFamilyIndex = vk_queue_family_index;
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

    vk_device = std::make_unique<vkutils::Device>(physical_device, std::span{ &queue_create_info, 1 }, required_device_extensions, &vlk11_features);

    VkDeviceQueueInfo2 queue_info = {};
    queue_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_INFO_2;
    queue_info.queueFamilyIndex = vk_queue_family_index;
    queue_info.queueIndex = 0;
    vkGetDeviceQueue2(*vk_device, &queue_info, &vk_queue);
    if (vk_queue == VK_NULL_HANDLE) {
        std::println(std::cerr, "Failed to retrieve Vulkan queue.");
        return SDL_APP_FAILURE;
    }

    vk_surface_caps = vkutils::get_physical_device_surface_capabilities(physical_device, *vk_surface);
    auto surface_formats = vkutils::get_physical_device_surface_formats(physical_device, *vk_surface);

    vk_surface_format = choose_swap_surface_format(surface_formats);

    recreate_swapchain();

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
    vkCreateShaderModule(*vk_device, &shader_module_create_info, nullptr, &vk_shader_module);

    std::vector<VkPipelineShaderStageCreateInfo> stages(2);
    stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[0].module = vk_shader_module;
    stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    stages[0].pName = "vert_main";
    stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[1].module = vk_shader_module;
    stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    stages[1].pName = "frag_main";

    vk_pipeline = std::make_unique<vkutils::Pipeline>(*vk_device, stages, vk_surface_format.format);

    vkDestroyShaderModule(*vk_device, vk_shader_module, nullptr);

    VkCommandPoolCreateInfo cmd_pool_create_info = {};
    cmd_pool_create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    cmd_pool_create_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    cmd_pool_create_info.queueFamilyIndex = vk_queue_family_index;
    vkCreateCommandPool(*vk_device, &cmd_pool_create_info, nullptr, &vk_cmd_pool);

    VkCommandBufferAllocateInfo cmd_buffer_alloc_info = {};
    cmd_buffer_alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmd_buffer_alloc_info.commandPool = vk_cmd_pool;
    cmd_buffer_alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmd_buffer_alloc_info.commandBufferCount = FRAMES_IN_FLIGHT;
    vkAllocateCommandBuffers(*vk_device, &cmd_buffer_alloc_info, vk_cmd_buffers.data());

    for (uint32_t i = 0; i < vk_swapchain->get_image_count(); ++i) {
        vk_render_semaphores.emplace_back(*vk_device);
    }
    for (int i = 0; i < FRAMES_IN_FLIGHT; ++i) {
        vk_present_semaphores.emplace_back(*vk_device);
        vk_draw_fences.emplace_back(*vk_device, true);
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
    vk_draw_fences[frame_index].wait();

    uint32_t image_index = 0;
    VkResult result = vkAcquireNextImageKHR(*vk_device, *vk_swapchain, std::numeric_limits<uint64_t>::max(), vk_present_semaphores[frame_index], VK_NULL_HANDLE, &image_index);
    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        recreate_swapchain();
        return SDL_APP_CONTINUE;
    }
    if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        std::println(std::cerr, "Failed to acquire swapchain image.");
        return SDL_APP_FAILURE;
    }

    record_cmd_buffer(vk_swapchain->get_image(image_index), vk_swapchain->get_image_view(image_index));

    vk_draw_fences[frame_index].reset();

    const VkPipelineStageFlags wait_destination_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    VkSubmitInfo submit_info = {};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.waitSemaphoreCount = 1;
    submit_info.pWaitSemaphores = vk_present_semaphores[frame_index].ptr();
    submit_info.pWaitDstStageMask = &wait_destination_stage;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &vk_cmd_buffers[frame_index];
    submit_info.signalSemaphoreCount = 1;
    submit_info.pSignalSemaphores = vk_render_semaphores[image_index].ptr();
    vkQueueSubmit(vk_queue, 1, &submit_info, vk_draw_fences[frame_index]);

    VkPresentInfoKHR present_info = {};
    present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present_info.waitSemaphoreCount = 1;
    present_info.pWaitSemaphores = vk_render_semaphores[image_index].ptr();
    present_info.swapchainCount = 1;
    present_info.pSwapchains = vk_swapchain->ptr();
    present_info.pImageIndices = &image_index;
    result = vkQueuePresentKHR(vk_queue, &present_info);
    if (result == VK_SUBOPTIMAL_KHR || result == VK_ERROR_OUT_OF_DATE_KHR) {
        recreate_swapchain();
    }

    frame_index = (frame_index + 1) % FRAMES_IN_FLIGHT;

    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void* appstate, SDL_AppResult result) {
    vk_device->wait_idle();

    vk_render_semaphores.clear();
    vk_present_semaphores.clear();
    vk_draw_fences.clear();
    vkFreeCommandBuffers(*vk_device, vk_cmd_pool, static_cast<uint32_t>(vk_cmd_buffers.size()), vk_cmd_buffers.data());
    vkDestroyCommandPool(*vk_device, vk_cmd_pool, nullptr);
    vk_pipeline.reset();
    vk_swapchain.reset();
    vk_device.reset();
    vk_surface.reset();
    vk_instance.reset();

    SDL_DestroyWindow(sdl_window);
    SDL_Quit();
}
