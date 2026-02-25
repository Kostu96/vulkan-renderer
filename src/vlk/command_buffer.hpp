#pragma once
#include "utils/non_copyable.hpp"

#include <volk/volk.h>

namespace vlk {

class Buffer;

class CommandBuffer final :
    NonCopyable {
public:
    void begin(VkCommandBufferUsageFlags flags = {}) const;
    void end() const;

    void begin_rendering(VkClearValue clear_color,
                         VkImageView image_view,
                         VkExtent2D render_extent) const noexcept;

    void copy_buffer(const Buffer& src_buffer, const Buffer& dst_buffer) const noexcept;

    const VkCommandBuffer* ptr() const noexcept { return &handle_; }

    operator VkCommandBuffer() const noexcept { return handle_; }
private:
    friend class CommandPool;

    explicit CommandBuffer(VkCommandBuffer handle) noexcept :
        handle_{ handle } {}

    VkCommandBuffer handle_ = VK_NULL_HANDLE;
};

}
