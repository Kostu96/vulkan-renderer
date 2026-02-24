#include "application.hpp"

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

#include <algorithm>
#include <format>
#include <ranges>
#include <span>
#include <stdexcept>
#include <vector>

namespace {

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

}

Application::Application() :
    window_{ SDL_CreateWindow(app_info.pApplicationName,
                              1440, 900,
                              SDL_WINDOW_VULKAN | SDL_WINDOW_HIGH_PIXEL_DENSITY | SDL_WINDOW_RESIZABLE),
             SDL_DestroyWindow },
    vk_instance_(app_info, get_required_instance_extensions(), get_required_layers()),
    vk_surface_{ window_.get(), vk_instance_ },
    vk_device{ create_device() },
    vk_queue{ vk_device.get_queue(vk_queue_family_index) }
{
    if (!window_) {
        throw std::runtime_error(std::format("Window creation failed: {}", SDL_GetError()));
    }
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
                vk_queue_family_index = static_cast<uint32_t>(idx);
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

    return { physical_device, std::span{ &queue_create_info, 1 }, std::span {required_device_extensions }, &vlk11_features };
}
