#include "vulkan_utils/instance.hpp"

#include <stdexcept>

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
}

Instance::~Instance() {
    vkDestroyInstance(handle_, nullptr);
}

}
