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

VKAPI_ATTR VkBool32 VKAPI_CALL
vk_debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
                  VkDebugUtilsMessageTypeFlagsEXT message_type,
                  const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
                  void* user_data) {
    std::println(std::cerr, "Vulkan debug callback: {}", callback_data->pMessage);
    return VK_FALSE;
}

SDL_Window* sdl_window;
VkInstance vk_instance;
VkDebugUtilsMessengerEXT vk_debug_messenger;

SDL_AppResult SDL_AppInit(void** appstate, int argc, char* argv[]) {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        std::println(std::cerr, "SDL_Init failed: {}", SDL_GetError());
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

    VkApplicationInfo application_info = {};
    application_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    application_info.pApplicationName = "Vulkan Renderer";
    application_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    application_info.pEngineName = "No Engine";
    application_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    application_info.apiVersion = VK_API_VERSION_1_4;

    uint32_t sdl_vk_extensions_count = 0;
    auto sdl_vk_extensions = SDL_Vulkan_GetInstanceExtensions(&sdl_vk_extensions_count);

    uint32_t instance_extension_props_count = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &instance_extension_props_count, nullptr);
    std::vector<VkExtensionProperties> instance_extension_props{ instance_extension_props_count };
    vkEnumerateInstanceExtensionProperties(nullptr, &instance_extension_props_count, instance_extension_props.data());

    for (uint32_t i = 0; i < sdl_vk_extensions_count; ++i) {
        if (std::ranges::none_of(instance_extension_props,
                                 [sdl_vk_extension = sdl_vk_extensions[i]](auto& extension_prop) {
                                     return strcmp(extension_prop.extensionName, sdl_vk_extension) == 0; })) {
            std::println(std::cerr, "Required SDL Vulkan extension not supported: {}", sdl_vk_extensions[i]);
            return SDL_APP_FAILURE;
        }
    }

    std::vector<char const*> required_layers;

#ifdef NDEBUG
    constexpr bool enable_validation_layers = false;
#else
    constexpr bool enable_validation_layers = true;
    required_layers.push_back("VK_LAYER_KHRONOS_validation");
#endif

    uint32_t instance_layer_props_count = 0;
    vkEnumerateInstanceLayerProperties(&instance_layer_props_count, nullptr);
    std::vector<VkLayerProperties> instance_layer_props{ instance_layer_props_count };
    vkEnumerateInstanceLayerProperties(&instance_layer_props_count, instance_layer_props.data());

    if (std::ranges::any_of(required_layers,
                            [&instance_layer_props](auto& required_layer) {
            return std::ranges::none_of(instance_layer_props,
                                        [required_layer](auto& layer) {
                return strcmp(layer.layerName, required_layer) == 0; }); })) {
        std::println(std::cerr, "One or more required layers are not supported");
        return SDL_APP_FAILURE;
    }

    VkDebugUtilsMessengerCreateInfoEXT dbg_msg_create_info = {};
    dbg_msg_create_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    dbg_msg_create_info.messageSeverity =
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    dbg_msg_create_info.messageType =
        VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    dbg_msg_create_info.pfnUserCallback = vk_debug_callback;

    VkInstanceCreateInfo instance_create_info = {};
    instance_create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    //instance_create_info.pNext = &dbg_msg_create_info;
    instance_create_info.pApplicationInfo = &application_info;
    instance_create_info.enabledExtensionCount = sdl_vk_extensions_count;
    instance_create_info.ppEnabledExtensionNames = sdl_vk_extensions;
    instance_create_info.enabledLayerCount = static_cast<uint32_t>(required_layers.size());
    instance_create_info.ppEnabledLayerNames = required_layers.data();
    vkCreateInstance(&instance_create_info, nullptr, &vk_instance);

    volkLoadInstance(vk_instance);

    vkCreateDebugUtilsMessengerEXT(vk_instance, &dbg_msg_create_info, nullptr, &vk_debug_messenger);

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
    vkDestroyDebugUtilsMessengerEXT(vk_instance, vk_debug_messenger, nullptr);
    vkDestroyInstance(vk_instance, nullptr);

    SDL_DestroyWindow(sdl_window);
    SDL_Quit();
}
