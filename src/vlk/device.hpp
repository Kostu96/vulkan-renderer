#pragma once
#include "utils/non_copyable.hpp"
#include "vlk/physical_device.hpp"
#include "vlk/queue.hpp"

#include <volk/volk.h>

#include <span>

namespace vlk {

class Device final :
    NonCopyable {
public:
    Device(PhysicalDevice physical_device,
           std::span<VkDeviceQueueCreateInfo> queues,
           std::span<const char* const> extensions,
           const void* next = nullptr);

    ~Device();

    Queue get_queue(uint32_t family_index, uint32_t index = 0) const noexcept;

    void wait_idle() const;

    const PhysicalDevice& get_physical_device() const noexcept { return physical_device_; }

    operator VkDevice() const noexcept { return handle_; }
private:
    PhysicalDevice physical_device_;
    VkDevice handle_ = VK_NULL_HANDLE;
};

}
