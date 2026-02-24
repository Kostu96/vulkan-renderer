#include "vlk/device.hpp"
#include "vlk/physical_device.hpp"

#include <stdexcept>

namespace vlk {

Device::Device(PhysicalDevice physical_device,
               std::span<VkDeviceQueueCreateInfo> queues,
               std::span<const char* const> extensions,
               const void* next) :
    physical_device_{ physical_device }
{
    const VkDeviceCreateInfo create_info = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext = next,
        .queueCreateInfoCount = static_cast<uint32_t>(queues.size()),
        .pQueueCreateInfos = queues.data(),
        .enabledExtensionCount = static_cast<uint32_t>(extensions.size()),
        .ppEnabledExtensionNames = extensions.data()
    };
    VkResult result = vkCreateDevice(physical_device, &create_info, nullptr, &handle_);
    if (result != VK_SUCCESS) {
        throw std::runtime_error{ "Failed to create Vulkan device." };
    }

    volkLoadDevice(handle_);
}

Device::~Device() {
    vkDestroyDevice(handle_, nullptr);
}

Queue Device::get_queue(uint32_t family_index, uint32_t index) const noexcept {
    const VkDeviceQueueInfo2 info = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_INFO_2,
        .queueFamilyIndex = family_index,
        .queueIndex = index
    };
    VkQueue vk_handle;
    vkGetDeviceQueue2(handle_, &info, &vk_handle);

    return Queue{ vk_handle };
}

void Device::wait_idle() const {
    VkResult result = vkDeviceWaitIdle(handle_);
    if (result != VK_SUCCESS) {
        throw std::runtime_error{ "Failed to idle on a device." };
    }
}

}
