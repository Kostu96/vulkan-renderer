#include "vulkan_utils/instance.hpp"
#include "vulkan_utils/vulkan_utils.hpp"

#include <algorithm>
#include <iostream>
#include <print>
#include <stdexcept>
#include <string.h>

namespace {

VKAPI_ATTR VkBool32 VKAPI_CALL
dbg_callback(VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
             VkDebugUtilsMessageTypeFlagsEXT message_type,
             const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
             void* user_data) {
    std::println(std::cerr, "Vulkan debug callback: {}.", callback_data->pMessage);
    return VK_FALSE;
}

}

namespace vkutils {

Instance::Instance(const VkApplicationInfo& app_info,
                   std::span<const char*> extensions,
                   std::span<const char*> layers) {
    VkInstanceCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    create_info.pApplicationInfo = &app_info;
    create_info.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    create_info.ppEnabledExtensionNames = extensions.data();
    create_info.enabledLayerCount = static_cast<uint32_t>(layers.size());
    create_info.ppEnabledLayerNames = layers.data();
    VkResult result = vkCreateInstance(&create_info, nullptr, &handle_);
    if (result != VK_SUCCESS) {
        throw std::runtime_error{ "Failed to create Vulkan instance." };
    }
    
    volkLoadInstanceOnly(handle_);

    if (std::ranges::any_of(extensions,
                            [](auto& extension) { return strcmp(extension, VK_EXT_DEBUG_UTILS_EXTENSION_NAME) == 0; })) {
        dbg_messenger_ = vkutils::create_debug_messenger(handle_, dbg_callback);
    }
}

Instance::~Instance() {
    vkDestroyDebugUtilsMessengerEXT(handle_, dbg_messenger_, nullptr);
    vkDestroyInstance(handle_, nullptr);
}

}
