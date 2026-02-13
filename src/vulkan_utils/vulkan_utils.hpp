#pragma once
#include <volk/volk.h>

#include <vector>

namespace vkutils {

std::vector<VkExtensionProperties> get_instance_extension_properties();

std::vector<VkLayerProperties> get_instance_layer_properties();

VkDebugUtilsMessengerEXT create_debug_messenger(VkInstance instance,
                                                PFN_vkDebugUtilsMessengerCallbackEXT callback);

std::vector<VkPhysicalDevice> get_physical_devices(VkInstance instance);

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
