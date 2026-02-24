#pragma once
#include "utils/non_copyable.hpp"

#include <volk/volk.h>

#include <span>

namespace vlk {

class PhysicalDevice;

class Device final :
    NonCopyable {
public:
    Device(const PhysicalDevice& physical_device,
           std::span<VkDeviceQueueCreateInfo> queues,
           std::span<const char*> extensions,
           const void* next = nullptr);

    ~Device();

    void wait_idle() const;

    const PhysicalDevice& get_physical_device() const { return physical_device_; }

    operator VkDevice() const noexcept { return handle_; }
private:
    const PhysicalDevice& physical_device_;
    VkDevice handle_ = VK_NULL_HANDLE;
};

}
