#pragma once
#include "utils/non_copyable.hpp"

#include <volk/volk.h>

namespace vkutils {

class CommandPool;
class Device;

class CommandBuffer final :
    NonCopyable {
public:
    CommandBuffer(const Device& device, const CommandPool& cmd_pool);

    ~CommandBuffer();

    void begin(VkCommandBufferUsageFlags flags) const;
    void end() const;

    VkCommandBuffer* ptr() noexcept { return &handle_; }

    operator VkCommandBuffer() const noexcept { return handle_; }
private:
    const Device& device_;
    const CommandPool& cmd_pool_;
    VkCommandBuffer handle_ = VK_NULL_HANDLE;
};

}
