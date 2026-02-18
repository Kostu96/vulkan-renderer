#include "vulkan_utils/swapchain.hpp"
#include "vulkan_utils/device.hpp"

#include <stdexcept>

namespace vkutils {

Swapchain::Swapchain(const Device& device,
                     VkSurfaceKHR surface,
                     VkSurfaceFormatKHR format,
                     uint32_t image_count,
                     VkExtent2D extent,
                     VkSurfaceTransformFlagBitsKHR pre_transform,
                     VkPresentModeKHR present_mode) :
    device_{ device }
{
    VkSwapchainCreateInfoKHR swapchain_create_info = {};
    swapchain_create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchain_create_info.surface = surface;
    swapchain_create_info.imageFormat = format.format;
    swapchain_create_info.imageColorSpace = format.colorSpace;
    swapchain_create_info.minImageCount = image_count;
    swapchain_create_info.imageExtent = extent;
    swapchain_create_info.imageArrayLayers = 1;
    swapchain_create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    swapchain_create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    swapchain_create_info.preTransform = pre_transform;
    swapchain_create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchain_create_info.presentMode = present_mode;
    swapchain_create_info.clipped = VK_TRUE;
    swapchain_create_info.oldSwapchain = VK_NULL_HANDLE;
    vkCreateSwapchainKHR(device, &swapchain_create_info, nullptr, &handle_);
}

Swapchain::Swapchain(Swapchain &&other) noexcept :
    device_{ other.device_ },
    handle_{ other.handle_ },
    images_{ std::move(other.images_) },
    image_views_{ std::move(other.image_views_) }
{
    other.handle_ = VK_NULL_HANDLE;        
}

Swapchain::~Swapchain() {
    vkDestroySwapchainKHR(device_, handle_, nullptr);
}

}
