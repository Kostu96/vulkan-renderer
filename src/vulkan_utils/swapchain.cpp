#include "vulkan_utils/swapchain.hpp"
#include "vulkan_utils/device.hpp"
#include "vulkan_utils/semaphore.hpp"
#include "vulkan_utils/vulkan_utils.hpp"

#include <algorithm>
#include <stdexcept>

namespace vkutils {

Swapchain::Swapchain(const Device& device,
                     VkSurfaceKHR surface,
                     VkSurfaceCapabilitiesKHR surface_caps,
                     VkSurfaceFormatKHR format,
                     uint32_t image_count,
                     VkExtent2D extent) :
    device_{ device }
{
    uint32_t actual_image_count = std::max(image_count, surface_caps.minImageCount);
    actual_image_count = (surface_caps.maxImageCount > 0 && actual_image_count > surface_caps.maxImageCount) ? surface_caps.maxImageCount : actual_image_count;

    auto physical_device = device.get_physical_device();
    
    auto present_modes = get_physical_device_surface_present_modes(physical_device, surface);
    VkPresentModeKHR present_mode = [&present_modes](){
        if (std::ranges::any_of(present_modes,
                                [](auto& mode){ return mode == VK_PRESENT_MODE_MAILBOX_KHR; })) {
            return VK_PRESENT_MODE_MAILBOX_KHR;
        }
        return VK_PRESENT_MODE_FIFO_KHR;
    }();

    const VkSwapchainCreateInfoKHR swapchain_create_info = {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = surface,
        .minImageCount = actual_image_count,
        .imageFormat = format.format,
        .imageColorSpace = format.colorSpace,
        .imageExtent = extent,
        .imageArrayLayers = 1,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .preTransform = surface_caps.currentTransform,
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode = present_mode,
        .clipped = VK_TRUE
    };
    VkResult result = vkCreateSwapchainKHR(device, &swapchain_create_info, nullptr, &handle_);
    if (result != VK_SUCCESS) {
        throw std::runtime_error{ "Failed to create Vulkan swapchain." };
    }

    result = vkGetSwapchainImagesKHR(device, handle_, &actual_image_count, nullptr);
    if (result != VK_SUCCESS) {
        throw std::runtime_error{ "Failed to query swapchain images count." };
    }

    images_.resize(actual_image_count);
    result = vkGetSwapchainImagesKHR(device, handle_, &actual_image_count, images_.data());
    if (result != VK_SUCCESS) {
        throw std::runtime_error{ "Failed to get swapchain images." };
    }

    VkImageViewCreateInfo image_view_create_info = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = format.format,
        .subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 }
    };
    image_views_.reserve(images_.size());
    for (auto image : images_) {
        image_view_create_info.image = image;
        VkImageView view;
        vkCreateImageView(device, &image_view_create_info, nullptr, &view);
        image_views_.push_back(view);
    }
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
    for (auto& image_view : image_views_) {
        vkDestroyImageView(device_, image_view, nullptr);
    }
    vkDestroySwapchainKHR(device_, handle_, nullptr);
}

Swapchain::NextImage Swapchain::acquire_next_image(const Semaphore& semaphore) const {
    NextImage next_image = {};
    VkResult result = vkAcquireNextImageKHR(device_, handle_, std::numeric_limits<uint64_t>::max(), semaphore, VK_NULL_HANDLE, &next_image.image_index);
    if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        throw std::runtime_error{ "Failed to acquire swapchain image." };
    }

    next_image.image = images_[next_image.image_index];
    next_image.image_view = image_views_[next_image.image_index];
    next_image.should_recreate_swapchain = result == VK_ERROR_OUT_OF_DATE_KHR;
    
    return next_image;
}

}
