#pragma once
#include <volk/volk.h>

#include <span>

namespace vkutils {

class Device {
public:
    Device(VkPhysicalDevice physical_device,
           std::span<VkDeviceQueueCreateInfo> queues,
           std::span<const char*> extensions,
           std::span<const char*> layers,
           const void* next = nullptr);

    ~Device();

    VkDevice get_handle() { return handle_; }
private:
    VkDevice handle_ = VK_NULL_HANDLE;
};

}
