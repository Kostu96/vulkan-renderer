#pragma once
#include "utils/non_copyable.hpp"

#include <volk/volk.h>

#include <span>

namespace vkutils {

class Device final :
    NonCopyable {
public:
    Device(VkPhysicalDevice physical_device,
           std::span<VkDeviceQueueCreateInfo> queues,
           std::span<const char*> extensions,
           const void* next = nullptr);

    ~Device();

    void wait_idle() const;

    VkPhysicalDevice get_physical_device() const { return physical_device_; }

    operator VkDevice() const noexcept { return handle_; }
private:
    VkPhysicalDevice physical_device_ = VK_NULL_HANDLE;
    VkDevice handle_ = VK_NULL_HANDLE;
};

}
