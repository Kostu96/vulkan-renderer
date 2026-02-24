#pragma once
#include "utils/non_copyable.hpp"

#include <volk/volk.h>

#include <memory>

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

    std::unique_ptr<CommandBuffer> allocate_command_buffer() const;

    operator VkCommandPool() const noexcept { return handle_; }
private:
    const Device& device_;
    VkCommandPool handle_ = VK_NULL_HANDLE;
};

}
