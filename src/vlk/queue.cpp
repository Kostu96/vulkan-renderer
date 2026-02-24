#include "vlk/queue.hpp"
#include "vlk/command_buffer.hpp"
#include "vlk/fence.hpp"
#include "vlk/semaphore.hpp"
#include "vlk/swapchain.hpp"

#include <stdexcept>

namespace vlk {

void Queue::submit(VkCommandBuffer cmd_buffer,
                   const Semaphore* wait_semaphore,
                   VkPipelineStageFlags wait_stage,
                   const Semaphore* signal_semaphore,
                   const Fence* fence) const {
    const VkSubmitInfo info = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .waitSemaphoreCount = wait_semaphore != nullptr ? 1u : 0u,
        .pWaitSemaphores = wait_semaphore != nullptr ? wait_semaphore->ptr() : nullptr,
        .pWaitDstStageMask = wait_semaphore != nullptr ? &wait_stage : nullptr,
        .commandBufferCount = 1,
        .pCommandBuffers = &cmd_buffer,
        .signalSemaphoreCount = signal_semaphore != nullptr ? 1u : 0u,
        .pSignalSemaphores = signal_semaphore != nullptr ? signal_semaphore->ptr() : nullptr,
    };
    VkResult result = vkQueueSubmit(handle_, 1, &info, fence != nullptr ? *fence : VK_NULL_HANDLE);
    if (result != VK_SUCCESS) {
        throw std::runtime_error("Failed to submit to a queue.");
    }
}

bool Queue::present(const Swapchain& swapchain, const Semaphore& wait_semaphore, uint32_t image_index) const {
    const VkPresentInfoKHR info = {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = wait_semaphore.ptr(),
        .swapchainCount = 1,
        .pSwapchains = swapchain.ptr(),
        .pImageIndices = &image_index,
    };
    VkResult result = vkQueuePresentKHR(handle_, &info);
    if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR && result != VK_ERROR_OUT_OF_DATE_KHR) {
        throw std::runtime_error("Failed to present to a queue.");
    }

    return (result == VK_SUBOPTIMAL_KHR || result == VK_ERROR_OUT_OF_DATE_KHR);
}

void Queue::wait_idle() const {
    VkResult result = vkQueueWaitIdle(handle_);
    if (result != VK_SUCCESS) {
        throw std::runtime_error("Failed to idle on a queue.");
    }
}

}
