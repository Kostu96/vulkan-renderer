#include "vulkan_utils/fence.hpp"
#include "vulkan_utils/device.hpp"

#include <stdexcept>

namespace vkutils {

Fence::Fence(const Device& device, bool signaled) :
    device_{ device }
{
    VkFenceCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    if (signaled) {
        create_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    }
    VkResult result = vkCreateFence(device.get_handle(), &create_info, nullptr, &handle_);
    if (result != VK_SUCCESS) {
        throw std::runtime_error{ "Failed to create Vulkan fence." };
    }
}

Fence::Fence(Fence &&other) :
    device_{ other.device_ },
    handle_{ other.handle_ }
{
    other.handle_ = VK_NULL_HANDLE;        
}

Fence::~Fence() {
    vkDestroyFence(device_.get_handle(), handle_, nullptr);
}

}
