#include "vulkan_utils.hpp"

#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3/SDL_vulkan.h>

#include <algorithm>
#include <iostream>
#include <print>
#include <ranges>
#include <vector>

#ifdef NDEBUG
    constexpr bool enable_validation_layers = false;
#else
    constexpr bool enable_validation_layers = true;
#endif

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

SDL_Window* sdl_window = nullptr;
VkInstance vk_instance = VK_NULL_HANDLE;
VkDebugUtilsMessengerEXT vk_debug_messenger = VK_NULL_HANDLE;
VkSurfaceKHR vk_surface = VK_NULL_HANDLE;
VkPhysicalDevice vk_physical_device = VK_NULL_HANDLE;
uint32_t vk_queue_family_index = 0;
VkDevice vk_device = VK_NULL_HANDLE;
VkQueue vk_queue = VK_NULL_HANDLE;
VkSwapchainKHR vk_swapchain = VK_NULL_HANDLE;
std::vector<VkImage> vk_swapchain_images;
std::vector<VkImageView> vk_swapchain_images_views;

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

    VkResult vk_result;

    vk_result = volkInitialize();
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

    std::vector<char const*> required_layers;
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
    
    VkApplicationInfo application_info = {};
    application_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    application_info.pApplicationName = "Vulkan Renderer";
    application_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    application_info.pEngineName = "No Engine";
    application_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    application_info.apiVersion = VK_API_VERSION_1_4;

    VkInstanceCreateInfo instance_create_info = {};
    instance_create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instance_create_info.pApplicationInfo = &application_info;
    instance_create_info.enabledExtensionCount = static_cast<uint32_t>(required_extensions.size());
    instance_create_info.ppEnabledExtensionNames = required_extensions.data();
    instance_create_info.enabledLayerCount = static_cast<uint32_t>(required_layers.size());
    instance_create_info.ppEnabledLayerNames = required_layers.data();
    vkCreateInstance(&instance_create_info, nullptr, &vk_instance);
    
    volkLoadInstanceOnly(vk_instance);

    if (enable_validation_layers) {
        vk_debug_messenger = vkutils::create_debug_messenger(vk_instance, vk_debug_callback);
    }

    if (!SDL_Vulkan_CreateSurface(sdl_window, vk_instance, nullptr, &vk_surface)) {
        std::println(std::cerr, "Failed to create Vulkan surface");
        return SDL_APP_FAILURE;
    }

    std::vector<const char*> required_device_extensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };
    auto physical_devices = vkutils::get_physical_devices(vk_instance);
    for (auto& physical_device : physical_devices) {
        // TODO(Kostu): Device scoring
        auto physical_device_props = vkutils::get_physical_device_properties(physical_device);
        auto physical_device_feats = vkutils::get_physical_device_features(physical_device);
        auto physical_device_extensions = vkutils::get_physical_device_extension_properties(physical_device);
        auto queue_family_props = vkutils::get_physical_queue_family_properties(physical_device);

        bool is_suitable = (physical_device_props.properties.apiVersion >= VK_API_VERSION_1_3);

        bool has_graphics_capable_queue = false;
        for (auto [idx, queue_family] : std::views::enumerate(queue_family_props)) {
            bool present_supported = SDL_Vulkan_GetPresentationSupport(vk_instance, physical_device, idx);
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

    VkDeviceCreateInfo device_create_info = {};
    device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    device_create_info.queueCreateInfoCount = 1;
    device_create_info.pQueueCreateInfos = &queue_create_info;
    device_create_info.enabledExtensionCount = static_cast<uint32_t>(required_device_extensions.size());
    device_create_info.ppEnabledExtensionNames = required_device_extensions.data();
    device_create_info.enabledLayerCount = static_cast<uint32_t>(required_layers.size());
    device_create_info.ppEnabledLayerNames = required_layers.data();
    vkCreateDevice(vk_physical_device, &device_create_info, nullptr, &vk_device);

    if (vk_device == VK_NULL_HANDLE) {
        std::println(std::cerr, "Failed to create Vulkan device.");
        return SDL_APP_FAILURE;
    }

    volkLoadDevice(vk_device);

    VkDeviceQueueInfo2 queue_info = {};
    queue_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_INFO_2;
    queue_info.queueFamilyIndex = vk_queue_family_index;
    queue_info.queueIndex = 0;
    vkGetDeviceQueue2(vk_device, &queue_info, &vk_queue);

    if (vk_queue == VK_NULL_HANDLE) {
        std::println(std::cerr, "Failed to retrieve Vulkan queue.");
        return SDL_APP_FAILURE;
    }

    auto surface_caps = vkutils::get_physical_device_surface_capabilities(vk_physical_device, vk_surface);
    auto surface_formats = vkutils::get_physical_device_surface_formats(vk_physical_device, vk_surface);
    auto surface_present_modes = vkutils::get_physical_device_surface_present_modes(vk_physical_device, vk_surface);

    auto extent = choose_swap_extent(sdl_window, surface_caps);
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
    swapchain_create_info.imageExtent = extent;
    swapchain_create_info.imageArrayLayers = 1;
    swapchain_create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    swapchain_create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    swapchain_create_info.preTransform = surface_caps.currentTransform;
    swapchain_create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchain_create_info.presentMode = present_mode;
    swapchain_create_info.clipped = VK_TRUE;
    swapchain_create_info.oldSwapchain = VK_NULL_HANDLE;
    vkCreateSwapchainKHR(vk_device, &swapchain_create_info, nullptr, &vk_swapchain);

    uint32_t images_count = 0;
    vkGetSwapchainImagesKHR(vk_device, vk_swapchain, &images_count, nullptr);
    vk_swapchain_images.resize(images_count);
    vkGetSwapchainImagesKHR(vk_device, vk_swapchain, &images_count, vk_swapchain_images.data());

    VkImageViewCreateInfo image_view_create_info = {};
    image_view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    image_view_create_info.format = format.format;
    image_view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    image_view_create_info.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
    vk_swapchain_images_views.reserve(vk_swapchain_images.size());
    for (auto image : vk_swapchain_images) {
        image_view_create_info.image = image;
        VkImageView view;
        vkCreateImageView(vk_device, &image_view_create_info, nullptr, &view);
        vk_swapchain_images_views.push_back(view);
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
    SDL_Delay(16);
    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void* appstate, SDL_AppResult result) {
    for (auto& image_view : vk_swapchain_images_views) {
        vkDestroyImageView(vk_device, image_view, nullptr);
    }
    vkDestroySwapchainKHR(vk_device, vk_swapchain, nullptr);
    vkDestroyDevice(vk_device, nullptr);
    SDL_Vulkan_DestroySurface(vk_instance, vk_surface, nullptr);
    if (enable_validation_layers) {
        vkDestroyDebugUtilsMessengerEXT(vk_instance, vk_debug_messenger, nullptr);
    }
    vkDestroyInstance(vk_instance, nullptr);

    SDL_DestroyWindow(sdl_window);
    SDL_Quit();
}
