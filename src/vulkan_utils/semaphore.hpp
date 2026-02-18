#pragma once
#include "utils/non_copyable.hpp"

#include <volk/volk.h>

#include <span>

namespace vkutils {

class Device;

class Semaphore final :
    NonCopyable {
public:
    explicit Semaphore(const Device& device);
    
    Semaphore(Semaphore&& other);

    ~Semaphore();

    VkSemaphore get_handle() const { return handle_; }

    VkSemaphore* operator&() { return std::addressof(handle_); }
private:
    const Device& device_;
    VkSemaphore handle_ = VK_NULL_HANDLE;
};

}
