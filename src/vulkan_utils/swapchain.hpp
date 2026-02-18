#pragma once
#include "utils/non_copyable.hpp"

#include <volk/volk.h>

#include <vector>

namespace vkutils {

class Device;

class Swapchain final :
    NonCopyable {
public:
    Swapchain(const Device& device,
              VkSurfaceKHR surface,
              VkSurfaceFormatKHR format,
              uint32_t image_count,
              VkExtent2D extent,
              VkSurfaceTransformFlagBitsKHR pre_transform,
              VkPresentModeKHR present_mode);
    
    Swapchain(Swapchain&& other) noexcept;

    ~Swapchain();

    VkSwapchainKHR* ptr() noexcept { return &handle_; }

    operator VkSwapchainKHR() const noexcept { return handle_; }
private:
    const Device& device_;
    VkSwapchainKHR handle_ = VK_NULL_HANDLE;
    std::vector<VkImage> images_;
    std::vector<VkImageView> image_views_;
};

}
