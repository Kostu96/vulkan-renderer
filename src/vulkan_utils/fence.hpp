#pragma once
#include "utils/non_copyable.hpp"

#include <volk/volk.h>

namespace vkutils {

class Device;

class Fence final :
    NonCopyable {
public:
    explicit Fence(const Device& device, bool signaled = false);
    
    Fence(Fence&& other) noexcept;

    ~Fence();

    void reset() const;

    void wait() const;

    VkFence* ptr() noexcept { return &handle_; }

    operator VkFence() const noexcept { return handle_; }
private:
    const Device& device_;
    VkFence handle_ = VK_NULL_HANDLE;
};

}
