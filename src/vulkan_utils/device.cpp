#include "vulkan_utils/device.hpp"

#include <stdexcept>

namespace vkutils {

Device::Device(VkPhysicalDevice physical_device,
               std::span<VkDeviceQueueCreateInfo> queues,
               std::span<const char*> extensions,
               std::span<const char*> layers,
               const void* next) {
    VkDeviceCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    create_info.queueCreateInfoCount = static_cast<uint32_t>(queues.size());;
    create_info.pQueueCreateInfos = queues.data();
    create_info.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    create_info.ppEnabledExtensionNames = extensions.data();
    create_info.enabledLayerCount = static_cast<uint32_t>(layers.size());
    create_info.ppEnabledLayerNames = layers.data();
    create_info.pNext = next;
    VkResult result = vkCreateDevice(physical_device, &create_info, nullptr, &handle_);
    if (result != VK_SUCCESS) {
        throw std::runtime_error{ "Failed to create Vulkan device." };
    }

    volkLoadDevice(handle_);
}

Device::~Device() {
    vkDestroyDevice(handle_, nullptr);
}

}
