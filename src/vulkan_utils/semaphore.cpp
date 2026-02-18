#include "vulkan_utils/semaphore.hpp"
#include "vulkan_utils/device.hpp"

#include <stdexcept>

namespace vkutils {

Semaphore::Semaphore(const Device& device) :
    device_{ device }
{
    VkSemaphoreCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    VkResult result = vkCreateSemaphore(device, &create_info, nullptr, &handle_);
    if (result != VK_SUCCESS) {
        throw std::runtime_error{ "Failed to create Vulkan semaphore." };
    }
}

Semaphore::Semaphore(Semaphore &&other) noexcept :
    device_{ other.device_ },
    handle_{ other.handle_ }
{
    other.handle_ = VK_NULL_HANDLE;        
}

Semaphore::~Semaphore() {
    vkDestroySemaphore(device_, handle_, nullptr);
}

}
