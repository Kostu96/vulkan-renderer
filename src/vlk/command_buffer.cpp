#include "vlk/command_buffer.hpp"

#include "vlk/buffer.hpp"
#include "vlk/command_pool.hpp"
#include "vlk/device.hpp"

#include <stdexcept>

namespace vlk {

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

void CommandBuffer::begin_rendering(VkClearValue clear_color,
                                    VkImageView image_view,
                                    VkExtent2D render_extent) const noexcept {
    const VkRenderingAttachmentInfo attachment_info = {
        .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
        .imageView = image_view,
        .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .clearValue = clear_color
    };

    const VkRenderingInfo rendering_info = {
        .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
        .renderArea = {.offset = { 0, 0 }, .extent = render_extent },
        .layerCount = 1,
        .colorAttachmentCount = 1,
        .pColorAttachments = &attachment_info
    };
    vkCmdBeginRendering(handle_, &rendering_info);
}

void CommandBuffer::copy_buffer(const Buffer& src_buffer, const Buffer& dst_buffer) const noexcept {
    VkBufferCopy region{ 0, 0, dst_buffer.get_size()};
    vkCmdCopyBuffer(handle_, src_buffer, dst_buffer, 1, &region);
}

}
