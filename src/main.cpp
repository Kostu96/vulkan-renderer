#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3/SDL_vulkan.h>
#include <volk/volk.h>

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

SDL_Window* sdl_window = nullptr;
VkInstance vk_instance = VK_NULL_HANDLE;
VkDebugUtilsMessengerEXT vk_debug_messenger = VK_NULL_HANDLE;
VkSurfaceKHR vk_surface = VK_NULL_HANDLE;
VkPhysicalDevice vk_physical_device = VK_NULL_HANDLE;
uint32_t vk_queue_family_index = 0;
VkDevice vk_device = VK_NULL_HANDLE;
VkQueue vk_queue = VK_NULL_HANDLE;

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

    uint32_t instance_extension_props_count = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &instance_extension_props_count, nullptr);
    std::vector<VkExtensionProperties> instance_extension_props{ instance_extension_props_count };
    vkEnumerateInstanceExtensionProperties(nullptr, &instance_extension_props_count, instance_extension_props.data());

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

    uint32_t instance_layer_props_count = 0;
    vkEnumerateInstanceLayerProperties(&instance_layer_props_count, nullptr);
    std::vector<VkLayerProperties> instance_layer_props{ instance_layer_props_count };
    vkEnumerateInstanceLayerProperties(&instance_layer_props_count, instance_layer_props.data());

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
    // if (enable_validation_layers) {
    //     instance_create_info.pNext = &dbg_msg_create_info;
    // }
    vkCreateInstance(&instance_create_info, nullptr, &vk_instance);
    
    volkLoadInstanceOnly(vk_instance);

    if (enable_validation_layers) {
        VkDebugUtilsMessengerCreateInfoEXT dbg_msg_create_info = {};
        dbg_msg_create_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        dbg_msg_create_info.messageSeverity =
            //VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        dbg_msg_create_info.messageType =
            VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        dbg_msg_create_info.pfnUserCallback = vk_debug_callback;
        vkCreateDebugUtilsMessengerEXT(vk_instance, &dbg_msg_create_info, nullptr, &vk_debug_messenger);
    }

    if (!SDL_Vulkan_CreateSurface(sdl_window, vk_instance, nullptr, &vk_surface)) {
        std::println(std::cerr, "Failed to create Vulkan surface");
        return SDL_APP_FAILURE;
    }

    uint32_t physical_devices_count = 0;
    vkEnumeratePhysicalDevices(vk_instance, &physical_devices_count, nullptr);
    std::vector<VkPhysicalDevice> physical_devices{ physical_devices_count };
    vkEnumeratePhysicalDevices(vk_instance, &physical_devices_count, physical_devices.data());

    std::vector<const char*> required_device_extensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };

    for (auto& physical_device : physical_devices) {
        // TODO(Kostu): Device scoring
        VkPhysicalDeviceProperties2 physical_device_props = {};
        physical_device_props.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
        vkGetPhysicalDeviceProperties2(physical_device, &physical_device_props);

        VkPhysicalDeviceFeatures2 physical_device_feats = {};
        physical_device_feats.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
        vkGetPhysicalDeviceFeatures2(physical_device, &physical_device_feats);

        uint32_t physical_device_extensions_count = 0;
        vkEnumerateDeviceExtensionProperties(physical_device, nullptr, &physical_device_extensions_count, nullptr);
        std::vector<VkExtensionProperties> physical_device_extensions{ physical_device_extensions_count };
        vkEnumerateDeviceExtensionProperties(physical_device, nullptr, &physical_device_extensions_count, physical_device_extensions.data());

        uint32_t queue_families_count = 0;
        vkGetPhysicalDeviceQueueFamilyProperties2(physical_device, &queue_families_count, nullptr);
        std::vector<VkQueueFamilyProperties2> queue_families{ queue_families_count };
        for (auto& queue_family : queue_families) {
            queue_family.sType = VK_STRUCTURE_TYPE_QUEUE_FAMILY_PROPERTIES_2;
            queue_family.pNext = nullptr;
        }
        vkGetPhysicalDeviceQueueFamilyProperties2(physical_device, &queue_families_count, queue_families.data());

        bool is_suitable = (physical_device_props.properties.apiVersion >= VK_API_VERSION_1_3);

        bool has_graphics_capable_queue = false;
        for (auto [idx, queue_family] : std::views::enumerate(queue_families)) {
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
    vkDestroyDevice(vk_device, nullptr);
    SDL_Vulkan_DestroySurface(vk_instance, vk_surface, nullptr);
    if (enable_validation_layers) {
        vkDestroyDebugUtilsMessengerEXT(vk_instance, vk_debug_messenger, nullptr);
    }
    vkDestroyInstance(vk_instance, nullptr);

    SDL_DestroyWindow(sdl_window);
    SDL_Quit();
}
