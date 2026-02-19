#include "vulkan_utils/fence.hpp"
#include "vulkan_utils/device.hpp"

#include <stdexcept>

namespace vkutils {

Fence::Fence(const Device& device, bool signaled) :
    device_{ device }
{
    const VkFenceCreateInfo create_info = {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .flags = signaled ? VK_FENCE_CREATE_SIGNALED_BIT : 0u
    };
    VkResult result = vkCreateFence(device, &create_info, nullptr, &handle_);
    if (result != VK_SUCCESS) {
        throw std::runtime_error{ "Failed to create Vulkan fence." };
    }
}

Fence::Fence(Fence &&other) noexcept :
    device_{ other.device_ },
    handle_{ other.handle_ }
{
    other.handle_ = VK_NULL_HANDLE;        
}

Fence::~Fence() {
    vkDestroyFence(device_, handle_, nullptr);
}

void Fence::reset() const {
    VkResult result = vkResetFences(device_, 1, &handle_);
    if (result != VK_SUCCESS) {
        throw std::runtime_error{ "Failed to reset a fence." };
    }
}

void Fence::wait() const {
    VkResult result = vkWaitForFences(device_, 1, &handle_, VK_TRUE, std::numeric_limits<uint64_t>::max());
    if (result != VK_SUCCESS) {
        throw std::runtime_error{ "Failed to wait for a fence." };
    }
}

}
