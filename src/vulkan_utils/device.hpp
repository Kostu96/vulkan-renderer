#pragma once
#include "utils/non_copyable.hpp"

#include <volk/volk.h>

#include <span>

namespace vkutils {

class Device :
    NonCopyable {
public:
    Device(VkPhysicalDevice physical_device,
           std::span<VkDeviceQueueCreateInfo> queues,
           std::span<const char*> extensions,
           std::span<const char*> layers,
           const void* next = nullptr);

    ~Device();

    VkDevice get_handle() const { return handle_; }
private:
    VkDevice handle_ = VK_NULL_HANDLE;
};

}
