#include "vlk/memory_allocator.hpp"
#include "vlk/device.hpp"
#include "vlk/instance.hpp"
#include "vlk/physical_device.hpp"

#include <stdexcept>

namespace vlk {

MemoryAllocator::MemoryAllocator(const Instance& instance,
                                 const Device& device,
                                 uint32_t api_version) {
    VmaAllocatorCreateInfo create_info = {
        .physicalDevice = device.get_physical_device(),
        .device = device,
        .instance = instance,
        .vulkanApiVersion = api_version
    };
    VmaVulkanFunctions vulkan_functions;
    VkResult result = vmaImportVulkanFunctionsFromVolk(&create_info, &vulkan_functions);
    if (result != VK_SUCCESS) {
        throw std::runtime_error{ "Failed to import vulkan functions." };
    }

    create_info.pVulkanFunctions = &vulkan_functions;
    result = vmaCreateAllocator(&create_info, &handle_);
    if (result != VK_SUCCESS) {
        throw std::runtime_error{ "Failed to create memory allocator." };
    }
}

MemoryAllocator::~MemoryAllocator() {
    vmaDestroyAllocator(handle_);
}

}
