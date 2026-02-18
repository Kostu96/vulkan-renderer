#pragma once
#include "utils/non_copyable.hpp"

#include <volk/volk.h>

namespace vkutils {

class Device;

class Semaphore final :
    NonCopyable {
public:
    explicit Semaphore(const Device& device);
    
    Semaphore(Semaphore&& other) noexcept;

    ~Semaphore();

    VkSemaphore* ptr() noexcept { return &handle_; }

    operator VkSemaphore() const noexcept { return handle_; }
private:
    const Device& device_;
    VkSemaphore handle_ = VK_NULL_HANDLE;
};

}
