#pragma once
#include "utils/non_copyable.hpp"

#include <volk/volk.h>

#include <span>

namespace vkutils {

class Device;

class Fence final :
    NonCopyable {
public:
    explicit Fence(const Device& device, bool signaled = false);
    
    Fence(Fence&& other);

    ~Fence();

    VkFence get_handle() const { return handle_; }

    VkFence* operator&() { return std::addressof(handle_); }
private:
    const Device& device_;
    VkFence handle_ = VK_NULL_HANDLE;
};

}
