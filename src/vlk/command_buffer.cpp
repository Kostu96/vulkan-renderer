#include "vlk/command_buffer.hpp"
#include "vlk/command_pool.hpp"
#include "vlk/device.hpp"

#include <stdexcept>

namespace vlk {

CommandBuffer::CommandBuffer(const Device& device, const CommandPool& cmd_pool) :
    device_{ device },
    cmd_pool_{ cmd_pool }
{
    const VkCommandBufferAllocateInfo staging_cmd_buffer_alloc_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = cmd_pool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1
    };
    VkResult result = vkAllocateCommandBuffers(device, &staging_cmd_buffer_alloc_info, &handle_);
    if (result != VK_SUCCESS) {
        throw std::runtime_error{ "Failed to allocate Vulkan command buffer." };
    }
}

CommandBuffer::~CommandBuffer() {
    vkFreeCommandBuffers(device_, cmd_pool_, 1, &handle_);
}

void CommandBuffer::begin(VkCommandBufferUsageFlags flags) const {
    const VkCommandBufferBeginInfo info = {
           .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
           .flags = flags
    };
    VkResult result = vkBeginCommandBuffer(handle_, &info);
    if (result != VK_SUCCESS) {
        throw std::runtime_error{ "Failed to begin Vulkan command buffer." };
    }
}

void CommandBuffer::end() const {
    VkResult result = vkEndCommandBuffer(handle_);
    if (result != VK_SUCCESS) {
        throw std::runtime_error{ "Failed to end Vulkan command buffer." };
    }
}

}
