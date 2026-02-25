#include "vlk/command_pool.hpp"
#include "vlk/device.hpp"
#include "vlk/command_buffer.hpp"

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

std::vector<CommandBuffer> CommandPool::allocate_command_buffers(uint32_t count) const {
    std::vector<VkCommandBuffer> vk_handles(count);
    const VkCommandBufferAllocateInfo alloc_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = handle_,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = count
    };
    VkResult result = vkAllocateCommandBuffers(device_, &alloc_info, vk_handles.data());
    if (result != VK_SUCCESS) {
        throw std::runtime_error{ "Failed to allocate Vulkan command buffers." };
    }

    std::vector<CommandBuffer> cmd_buffers;
    cmd_buffers.reserve(vk_handles.size());
    for (VkCommandBuffer handle : vk_handles) {
        cmd_buffers.push_back(CommandBuffer{ handle });
    }

    return cmd_buffers;
}

void CommandPool::free_command_buffers(std::span<const CommandBuffer> cmd_buffers) const {
    std::vector<VkCommandBuffer> vk_handles;
    vk_handles.reserve(cmd_buffers.size());
    for (VkCommandBuffer cmd_buffer : cmd_buffers) {
        vk_handles.push_back(cmd_buffer);
    }

    vkFreeCommandBuffers(device_, handle_, static_cast<uint32_t>(vk_handles.size()), vk_handles.data());
}

}
