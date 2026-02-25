#pragma once
#include "utils/non_copyable.hpp"

#include <volk/volk.h>

#include <span>
#include <vector>

namespace vlk {

class Device;
class CommandBuffer;

class CommandPool final :
    NonCopyable {
public:
    CommandPool(const Device& device,
                VkCommandPoolCreateFlags flags,
                uint32_t queue_family_index);

    ~CommandPool();

    std::vector<CommandBuffer> allocate_command_buffers(uint32_t count) const;

    void free_command_buffers(std::span<const CommandBuffer> cmd_buffers) const;

    operator VkCommandPool() const noexcept { return handle_; }
private:
    const Device& device_;
    VkCommandPool handle_ = VK_NULL_HANDLE;
};

}
