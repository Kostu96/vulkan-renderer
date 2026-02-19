#pragma once
#include <volk/volk.h>

#include <vector>

namespace vkutils {

VkPhysicalDeviceProperties2 get_physical_device_properties(VkPhysicalDevice physical_device);

VkPhysicalDeviceFeatures2 get_physical_device_features(VkPhysicalDevice physical_device);

std::vector<VkExtensionProperties> get_physical_device_extension_properties(VkPhysicalDevice physical_device);

std::vector<VkQueueFamilyProperties2> get_physical_queue_family_properties(VkPhysicalDevice physical_device);

VkSurfaceCapabilitiesKHR get_physical_device_surface_capabilities(VkPhysicalDevice physical_device,
                                                                  VkSurfaceKHR surface);

std::vector<VkSurfaceFormatKHR> get_physical_device_surface_formats(VkPhysicalDevice physical_device,
                                                                    VkSurfaceKHR surface);

std::vector<VkPresentModeKHR> get_physical_device_surface_present_modes(VkPhysicalDevice physical_device,
                                                                        VkSurfaceKHR surface);

}
