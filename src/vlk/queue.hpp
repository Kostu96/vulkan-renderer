#pragma once
#include <volk/volk.h>

namespace vlk {

class CommandBuffer;
class Fence;
class Semaphore;
class Swapchain;

class Queue final {
public:
    void submit(VkCommandBuffer cmd_buffer,
                const Semaphore* wait_semaphore = nullptr,
                VkPipelineStageFlags wait_stage = {},
                const Semaphore* signal_semaphore = nullptr,
                const Fence* fence = nullptr) const;

    // Returns true when swapchain should be recreated, false otherwise.
    bool present(const Swapchain& swapchain, const Semaphore& wait_semaphore, uint32_t image_index) const;

    void wait_idle() const;

    operator VkQueue() const noexcept { return handle_; }
private:
    friend class Device;

    explicit Queue(VkQueue handle) noexcept :
        handle_{ handle } {}

    VkQueue handle_ = VK_NULL_HANDLE;
};

}
