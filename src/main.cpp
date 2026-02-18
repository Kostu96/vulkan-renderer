#include "vulkan_utils/device.hpp"
#include "vulkan_utils/instance.hpp"
#include "vulkan_utils/semaphore.hpp"
#include "vulkan_utils/vulkan_utils.hpp"

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

#ifdef NDEBUG
    constexpr bool enable_validation_layers = false;
#else
    constexpr bool enable_validation_layers = true;
#endif

constexpr int FRAMES_IN_FLIGHT = 2;

std::vector<const char*> get_required_extensions() {
    uint32_t sdl_vk_extensions_count = 0;
    auto sdl_vk_extensions = SDL_Vulkan_GetInstanceExtensions(&sdl_vk_extensions_count);

    std::vector<const char*> extensions{ sdl_vk_extensions_count };
    for (uint32_t i = 0; i < sdl_vk_extensions_count; ++i) {
        extensions[i] = sdl_vk_extensions[i];
    }

    if (enable_validation_layers) {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    return extensions;
}

VKAPI_ATTR VkBool32 VKAPI_CALL
vk_debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
                  VkDebugUtilsMessageTypeFlagsEXT message_type,
                  const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
                  void* user_data) {
    std::println(std::cerr, "Vulkan debug callback: {}.", callback_data->pMessage);
    return VK_FALSE;
}

VkExtent2D choose_swap_extent(SDL_Window* window, const VkSurfaceCapabilitiesKHR& capabilities) {
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        return capabilities.currentExtent;
    }

    int width, height;
    SDL_GetWindowSizeInPixels(window, &width, &height);

    return {
        std::clamp<uint32_t>(width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width),
        std::clamp<uint32_t>(height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height)
    };
}

VkSurfaceFormatKHR choose_swap_surface_format(std::span<const VkSurfaceFormatKHR> formats) {
    for (auto& format : formats) {
        if (format.format == VK_FORMAT_B8G8R8_SRGB && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return format;
        }
    }

    return formats[0];
}

VkPresentModeKHR choose_swap_present_mode(std::span<const VkPresentModeKHR> present_modes) {
    for (const auto& present_mode : present_modes) {
        if (present_mode == VK_PRESENT_MODE_MAILBOX_KHR) {
            return present_mode;
        }
    }

    return VK_PRESENT_MODE_FIFO_KHR;
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
VkDebugUtilsMessengerEXT vk_debug_messenger = VK_NULL_HANDLE;
VkSurfaceKHR vk_surface = VK_NULL_HANDLE;
VkPhysicalDevice vk_physical_device = VK_NULL_HANDLE;
uint32_t vk_queue_family_index = 0;
std::unique_ptr<vkutils::Device> vk_device;
VkQueue vk_queue = VK_NULL_HANDLE;
VkSwapchainKHR vk_swapchain = VK_NULL_HANDLE;
std::vector<VkImage> vk_swapchain_images;
std::vector<VkImageView> vk_swapchain_images_views;
VkPipeline vk_pipeline;
VkCommandPool vk_cmd_pool;
std::array<VkCommandBuffer, FRAMES_IN_FLIGHT> vk_cmd_buffers;
std::vector<vkutils::Semaphore> vk_present_semaphores;
std::vector<vkutils::Semaphore> vk_render_semaphores;
std::array<VkFence, FRAMES_IN_FLIGHT> vk_draw_fences;
VkExtent2D vk_extent;
uint32_t frame_index = 0;

void record_cmd_buffer(uint32_t image_index) {
    VkCommandBuffer cmd_buffer = vk_cmd_buffers[frame_index];

    VkCommandBufferBeginInfo cmd_buffer_begin_info = {};
    cmd_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    vkBeginCommandBuffer(cmd_buffer, &cmd_buffer_begin_info);

    transition_image_layout(cmd_buffer,
                            vk_swapchain_images[image_index],
                            VK_IMAGE_LAYOUT_UNDEFINED,
                            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                            {},
                            VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
                            VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
                            VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT);

    VkClearValue clear_color;
    clear_color.color = { 0.0f, 0.0f, 0.0f, 1.0f };
    VkRenderingAttachmentInfo attachment_info = {};
    attachment_info.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    attachment_info.imageView = vk_swapchain_images_views[image_index];
    attachment_info.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    attachment_info.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachment_info.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachment_info.clearValue = clear_color;

    VkRenderingInfo rendering_info = {};
    rendering_info.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
    rendering_info.renderArea = { .offset = { 0, 0 }, .extent = vk_extent };
    rendering_info.layerCount = 1;
    rendering_info.colorAttachmentCount = 1;
    rendering_info.pColorAttachments = &attachment_info;
    vkCmdBeginRendering(cmd_buffer, &rendering_info);

    vkCmdBindPipeline(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vk_pipeline);
    
    VkViewport viewport = { 0.0f, 0.0f, static_cast<float>(vk_extent.width), static_cast<float>(vk_extent.height), 0.0f, 1.0f };
    vkCmdSetViewport(cmd_buffer, 0, 1, &viewport);
    VkRect2D scissor = { { 0, 0 }, vk_extent };
    vkCmdSetScissor(cmd_buffer, 0, 1, &scissor);

    vkCmdDraw(cmd_buffer, 3, 1, 0, 0);

    vkCmdEndRendering(cmd_buffer);
    
    transition_image_layout(cmd_buffer,
                            vk_swapchain_images[image_index],
                            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                            VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                            VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
                            {},
                            VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
                            VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT);

    vkEndCommandBuffer(cmd_buffer);
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
    auto instance_extension_props = vkutils::get_instance_extension_properties();
    for (auto& extension : required_extensions) {
        if (std::ranges::none_of(instance_extension_props,
                                 [extension](auto& extension_prop) {
                                 return strcmp(extension_prop.extensionName, extension) == 0; })) {
            std::println(std::cerr, "Required SDL Vulkan extension not supported: {}.", extension);
            return SDL_APP_FAILURE;
        }
    }

    std::vector<const char*> required_layers;
    if (enable_validation_layers) {
        required_layers.push_back("VK_LAYER_KHRONOS_validation");
    }

    auto instance_layer_props = vkutils::get_instance_layer_properties();

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

    if (enable_validation_layers) {
        vk_debug_messenger = vkutils::create_debug_messenger(vk_instance->get_handle(), vk_debug_callback);
    }

    if (!SDL_Vulkan_CreateSurface(sdl_window, vk_instance->get_handle(), nullptr, &vk_surface)) {
        std::println(std::cerr, "Failed to create Vulkan surface");
        return SDL_APP_FAILURE;
    }

    std::vector<const char*> required_device_extensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };
    auto physical_devices = vkutils::get_physical_devices(vk_instance->get_handle());
    for (auto& physical_device : physical_devices) {
        // TODO(Kostu): Device scoring
        auto physical_device_props = vkutils::get_physical_device_properties(physical_device);
        auto physical_device_feats = vkutils::get_physical_device_features(physical_device);
        auto physical_device_extensions = vkutils::get_physical_device_extension_properties(physical_device);
        auto queue_family_props = vkutils::get_physical_queue_family_properties(physical_device);

        bool is_suitable = (physical_device_props.properties.apiVersion >= VK_API_VERSION_1_3);

        bool has_graphics_capable_queue = false;
        for (auto [idx, queue_family] : std::views::enumerate(queue_family_props)) {
            bool present_supported = SDL_Vulkan_GetPresentationSupport(vk_instance->get_handle(), physical_device, idx);
            if ((queue_family.queueFamilyProperties.queueFlags & VK_QUEUE_GRAPHICS_BIT) && present_supported) {
                has_graphics_capable_queue = true;
                vk_queue_family_index = static_cast<uint32_t>(idx);
                break;
            }
        }
        is_suitable = is_suitable && has_graphics_capable_queue;

        bool found = true;
        for (auto& extension : required_device_extensions) {
            auto it = std::ranges::find_if(physical_device_extensions, [extension](auto& ext) {return strcmp(ext.extensionName, extension) == 0;});
            found = found && it != physical_device_extensions.end();
        }
        is_suitable = is_suitable && found;

        if (is_suitable) {
            vk_physical_device = physical_device;
            break;
        }
    }

    if (vk_physical_device == VK_NULL_HANDLE) {
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

    vk_device = std::make_unique<vkutils::Device>(vk_physical_device, std::span{ &queue_create_info, 1 }, required_device_extensions, required_layers, &vlk11_features);

    VkDeviceQueueInfo2 queue_info = {};
    queue_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_INFO_2;
    queue_info.queueFamilyIndex = vk_queue_family_index;
    queue_info.queueIndex = 0;
    vkGetDeviceQueue2(vk_device->get_handle(), &queue_info, &vk_queue);
    
    if (vk_queue == VK_NULL_HANDLE) {
        std::println(std::cerr, "Failed to retrieve Vulkan queue.");
        return SDL_APP_FAILURE;
    }

    auto surface_caps = vkutils::get_physical_device_surface_capabilities(vk_physical_device, vk_surface);
    auto surface_formats = vkutils::get_physical_device_surface_formats(vk_physical_device, vk_surface);
    auto surface_present_modes = vkutils::get_physical_device_surface_present_modes(vk_physical_device, vk_surface);

    vk_extent = choose_swap_extent(sdl_window, surface_caps);
    auto format = choose_swap_surface_format(surface_formats);
    auto present_mode = choose_swap_present_mode(surface_present_modes);

    auto swap_image_count = std::max( 3u, surface_caps.minImageCount );
    swap_image_count = (surface_caps.maxImageCount > 0 && swap_image_count > surface_caps.maxImageCount) ? surface_caps.maxImageCount : swap_image_count;

    VkSwapchainCreateInfoKHR swapchain_create_info = {};
    swapchain_create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchain_create_info.surface = vk_surface;
    swapchain_create_info.minImageCount = swap_image_count;
    swapchain_create_info.imageFormat = format.format;
    swapchain_create_info.imageColorSpace = format.colorSpace;
    swapchain_create_info.imageExtent = vk_extent;
    swapchain_create_info.imageArrayLayers = 1;
    swapchain_create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    swapchain_create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    swapchain_create_info.preTransform = surface_caps.currentTransform;
    swapchain_create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchain_create_info.presentMode = present_mode;
    swapchain_create_info.clipped = VK_TRUE;
    swapchain_create_info.oldSwapchain = VK_NULL_HANDLE;
    vkCreateSwapchainKHR(vk_device->get_handle(), &swapchain_create_info, nullptr, &vk_swapchain);

    uint32_t images_count = 0;
    vkGetSwapchainImagesKHR(vk_device->get_handle(), vk_swapchain, &images_count, nullptr);
    vk_swapchain_images.resize(images_count);
    vkGetSwapchainImagesKHR(vk_device->get_handle(), vk_swapchain, &images_count, vk_swapchain_images.data());

    VkImageViewCreateInfo image_view_create_info = {};
    image_view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    image_view_create_info.format = format.format;
    image_view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    image_view_create_info.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
    vk_swapchain_images_views.reserve(vk_swapchain_images.size());
    for (auto image : vk_swapchain_images) {
        image_view_create_info.image = image;
        VkImageView view;
        vkCreateImageView(vk_device->get_handle(), &image_view_create_info, nullptr, &view);
        vk_swapchain_images_views.push_back(view);
    }

    auto shader_code = read_file("shaders/simple.spv");
    if (shader_code[0] != 0x07230203) {
        throw std::runtime_error("Not valid SPIR-V");
    }
    
    VkShaderModuleCreateInfo shader_module_create_info = {};
    shader_module_create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shader_module_create_info.codeSize = shader_code.size() * sizeof(uint32_t);
    shader_module_create_info.pCode = shader_code.data();
    VkShaderModule vk_shader_module;
    vkCreateShaderModule(vk_device->get_handle(), &shader_module_create_info, nullptr, &vk_shader_module);

    std::vector<VkPipelineShaderStageCreateInfo> stages(2);
    stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[0].module = vk_shader_module;
    stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    stages[0].pName = "vert_main";
    stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[1].module = vk_shader_module;
    stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    stages[1].pName = "frag_main";

    std::vector<VkDynamicState> dynamic_states = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };
    VkPipelineDynamicStateCreateInfo dynamic_state_create_info = {};
    dynamic_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamic_state_create_info.dynamicStateCount = static_cast<uint32_t>(dynamic_states.size());
    dynamic_state_create_info.pDynamicStates = dynamic_states.data();

    VkPipelineVertexInputStateCreateInfo vertex_input_create_info = {};
    vertex_input_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

    VkPipelineInputAssemblyStateCreateInfo input_assembly_create_info = {};
    input_assembly_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    input_assembly_create_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    VkPipelineViewportStateCreateInfo viewport_create_info = {};
    viewport_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport_create_info.viewportCount = 1;
    viewport_create_info.scissorCount = 1;

    VkPipelineRasterizationStateCreateInfo rasterization_create_info = {};
    rasterization_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterization_create_info.depthClampEnable = VK_FALSE;
    rasterization_create_info.rasterizerDiscardEnable = VK_FALSE;
    rasterization_create_info.polygonMode = VK_POLYGON_MODE_FILL;
    rasterization_create_info.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterization_create_info.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterization_create_info.depthBiasEnable = VK_FALSE;
    rasterization_create_info.depthBiasSlopeFactor = 1.0f;
    rasterization_create_info.lineWidth = 1.0f;

    VkPipelineMultisampleStateCreateInfo multisample_create_info = {};
    multisample_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisample_create_info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisample_create_info.sampleShadingEnable = VK_FALSE;

    VkPipelineColorBlendAttachmentState color_blend_attachment = {};
    color_blend_attachment.blendEnable = VK_FALSE;
    color_blend_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    VkPipelineColorBlendStateCreateInfo color_blend_create_info = {};
    color_blend_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    color_blend_create_info.logicOpEnable = VK_FALSE;
    color_blend_create_info.attachmentCount = 1;
    color_blend_create_info.pAttachments = &color_blend_attachment;

    VkPipelineLayoutCreateInfo layout_create_info = {};
    layout_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    VkPipelineLayout layout;
    vkCreatePipelineLayout(vk_device->get_handle(), &layout_create_info, nullptr, &layout);

    VkPipelineRenderingCreateInfo rendering_create_info = {};
    rendering_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
    rendering_create_info.colorAttachmentCount = 1;
    rendering_create_info.pColorAttachmentFormats = &format.format;

    VkGraphicsPipelineCreateInfo pipeline_create_info = {};
    pipeline_create_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipeline_create_info.stageCount = static_cast<uint32_t>(stages.size());
    pipeline_create_info.pStages = stages.data();
    pipeline_create_info.pVertexInputState = &vertex_input_create_info;
    pipeline_create_info.pInputAssemblyState = &input_assembly_create_info;
    pipeline_create_info.pViewportState = &viewport_create_info;
    pipeline_create_info.pRasterizationState = &rasterization_create_info;
    pipeline_create_info.pMultisampleState = &multisample_create_info;
    pipeline_create_info.pDynamicState = &dynamic_state_create_info;
    pipeline_create_info.pColorBlendState = &color_blend_create_info;
    pipeline_create_info.layout = layout;
    pipeline_create_info.pNext = &rendering_create_info;
    vkCreateGraphicsPipelines(vk_device->get_handle(), VK_NULL_HANDLE, 1, &pipeline_create_info, nullptr, &vk_pipeline);

    vkDestroyPipelineLayout(vk_device->get_handle(), layout, nullptr);
    vkDestroyShaderModule(vk_device->get_handle(), vk_shader_module, nullptr);

    VkCommandPoolCreateInfo cmd_pool_create_info = {};
    cmd_pool_create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    cmd_pool_create_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    cmd_pool_create_info.queueFamilyIndex = vk_queue_family_index;
    vkCreateCommandPool(vk_device->get_handle(), &cmd_pool_create_info, nullptr, &vk_cmd_pool);

    VkCommandBufferAllocateInfo cmd_buffer_alloc_info = {};
    cmd_buffer_alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmd_buffer_alloc_info.commandPool = vk_cmd_pool;
    cmd_buffer_alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmd_buffer_alloc_info.commandBufferCount = FRAMES_IN_FLIGHT;
    vkAllocateCommandBuffers(vk_device->get_handle(), &cmd_buffer_alloc_info, vk_cmd_buffers.data());

    for (int i = 0; i < FRAMES_IN_FLIGHT; ++i) {
        vk_present_semaphores.emplace_back(*vk_device);
    }
    for (int i = 0; i < vk_swapchain_images.size(); ++i) {
        vk_render_semaphores.emplace_back(*vk_device);
    }

    VkFenceCreateInfo fence_create_info = {};
    fence_create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_create_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    for (auto& fence : vk_draw_fences) {
        vkCreateFence(vk_device->get_handle(), &fence_create_info, nullptr, &fence);
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
    vkWaitForFences(vk_device->get_handle(), 1, &vk_draw_fences[frame_index], VK_TRUE, std::numeric_limits<uint64_t>::max());

    uint32_t image_index = 0;
    vkAcquireNextImageKHR(vk_device->get_handle(), vk_swapchain, std::numeric_limits<uint64_t>::max(), vk_present_semaphores[frame_index].get_handle(), VK_NULL_HANDLE, &image_index);

    record_cmd_buffer(image_index);

    vkResetFences(vk_device->get_handle(), 1, &vk_draw_fences[frame_index]);

    VkPipelineStageFlags wait_destination_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    VkSubmitInfo submit_info = {};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.waitSemaphoreCount = 1;
    submit_info.pWaitSemaphores = &vk_present_semaphores[frame_index];
    submit_info.pWaitDstStageMask = &wait_destination_stage;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &vk_cmd_buffers[frame_index];
    submit_info.signalSemaphoreCount = 1;
    submit_info.pSignalSemaphores = &vk_render_semaphores[image_index];
    vkQueueSubmit(vk_queue, 1, &submit_info, vk_draw_fences[frame_index]);

    VkPresentInfoKHR present_info = {};
    present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present_info.waitSemaphoreCount = 1;
    present_info.pWaitSemaphores = &vk_render_semaphores[image_index];
    present_info.swapchainCount = 1;
    present_info.pSwapchains = &vk_swapchain;
    present_info.pImageIndices = &image_index;
    vkQueuePresentKHR(vk_queue, &present_info);

    frame_index = (frame_index + 1) % FRAMES_IN_FLIGHT;

    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void* appstate, SDL_AppResult result) {
    vkDeviceWaitIdle(vk_device->get_handle());

    for (auto& fence : vk_draw_fences) {
        vkDestroyFence(vk_device->get_handle(), fence, nullptr);
    }
    vk_render_semaphores.clear();
    vk_present_semaphores.clear();
    vkFreeCommandBuffers(vk_device->get_handle(), vk_cmd_pool, static_cast<uint32_t>(vk_cmd_buffers.size()), vk_cmd_buffers.data());
    vkDestroyCommandPool(vk_device->get_handle(), vk_cmd_pool, nullptr);
    vkDestroyPipeline(vk_device->get_handle(), vk_pipeline, nullptr);
    for (auto& image_view : vk_swapchain_images_views) {
        vkDestroyImageView(vk_device->get_handle(), image_view, nullptr);
    }
    vkDestroySwapchainKHR(vk_device->get_handle(), vk_swapchain, nullptr);
    vk_device.reset();
    SDL_Vulkan_DestroySurface(vk_instance->get_handle(), vk_surface, nullptr);
    if (enable_validation_layers) {
        vkDestroyDebugUtilsMessengerEXT(vk_instance->get_handle(), vk_debug_messenger, nullptr);
    }
    vk_instance.reset();

    SDL_DestroyWindow(sdl_window);
    SDL_Quit();
}
