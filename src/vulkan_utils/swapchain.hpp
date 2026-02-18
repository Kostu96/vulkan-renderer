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
              VkSurfaceCapabilitiesKHR surface_caps,
              VkSurfaceFormatKHR format,
              uint32_t image_count,
              VkExtent2D extent);
    
    Swapchain(Swapchain&& other) noexcept;

    ~Swapchain();

    VkSwapchainKHR* ptr() noexcept { return &handle_; }

    uint32_t get_image_count() const noexcept { return static_cast<uint32_t>(images_.size()); }

    VkImage get_image(uint32_t index) const noexcept { return images_[index]; }

    VkImageView get_image_view(uint32_t index) const noexcept { return image_views_[index]; }

    operator VkSwapchainKHR() const noexcept { return handle_; }
private:
    const Device& device_;
    VkSwapchainKHR handle_ = VK_NULL_HANDLE;
    std::vector<VkImage> images_;
    std::vector<VkImageView> image_views_;
};

}
