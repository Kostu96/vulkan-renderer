#include "vulkan_utils/command_pool.hpp"
#include "vulkan_utils/device.hpp"
#include "vulkan_utils/command_buffer.hpp"

#include <stdexcept>

namespace vlk {

CommandPool::CommandPool(const Device& device,
                         VkCommandPoolCreateFlags flags,
                         uint32_t queue_family_index) :
    device_{ device }
{
    const VkCommandPoolCreateInfo create_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = flags,
        .queueFamilyIndex = queue_family_index
    };
    VkResult result = vkCreateCommandPool(device, &create_info, nullptr, &handle_);
    if (result != VK_SUCCESS) {
        throw std::runtime_error{ "Failed to create Vulkan command pool." };
    }
}

CommandPool::~CommandPool() {
    vkDestroyCommandPool(device_, handle_, nullptr);
}

std::unique_ptr<CommandBuffer> CommandPool::allocate_command_buffer() const {
    return std::make_unique<CommandBuffer>(device_, *this);
}

}
