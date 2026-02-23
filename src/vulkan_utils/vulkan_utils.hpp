#pragma once
#include "vulkan_utils/command_buffer.hpp"
#include "vulkan_utils/command_pool.hpp"
#include "vulkan_utils/device.hpp"
#include "vulkan_utils/fence.hpp"
#include "vulkan_utils/instance.hpp"
#include "vulkan_utils/pipeline.hpp"
#include "vulkan_utils/semaphore.hpp"
#include "vulkan_utils/swapchain.hpp"

#include <vector>

namespace vkutils {

VkPhysicalDeviceProperties get_physical_device_properties(VkPhysicalDevice physical_device);

VkPhysicalDeviceFeatures get_physical_device_features(VkPhysicalDevice physical_device);

VkPhysicalDeviceMemoryProperties get_physical_device_memory_properties(VkPhysicalDevice physical_device);

std::vector<VkExtensionProperties> get_physical_device_extension_properties(VkPhysicalDevice physical_device);

std::vector<VkQueueFamilyProperties> get_physical_queue_family_properties(VkPhysicalDevice physical_device);

VkSurfaceCapabilitiesKHR get_physical_device_surface_capabilities(VkPhysicalDevice physical_device,
                                                                  VkSurfaceKHR surface);

std::vector<VkSurfaceFormatKHR> get_physical_device_surface_formats(VkPhysicalDevice physical_device,
                                                                    VkSurfaceKHR surface);

std::vector<VkPresentModeKHR> get_physical_device_surface_present_modes(VkPhysicalDevice physical_device,
                                                                        VkSurfaceKHR surface);

}
