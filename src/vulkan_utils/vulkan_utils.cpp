#include "vulkan_utils.hpp"

#include <stdexcept>

namespace vkutils {

VkPhysicalDeviceProperties get_physical_device_properties(VkPhysicalDevice physical_device) {
    VkPhysicalDeviceProperties2 props = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2
    };
    vkGetPhysicalDeviceProperties2(physical_device, &props);

    return props.properties;
}

VkPhysicalDeviceFeatures get_physical_device_features(VkPhysicalDevice physical_device) {
    VkPhysicalDeviceFeatures2 feats = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2
    };
    vkGetPhysicalDeviceFeatures2(physical_device, &feats);

    return feats.features;
}

VkPhysicalDeviceMemoryProperties get_physical_device_memory_properties(VkPhysicalDevice physical_device) {
    VkPhysicalDeviceMemoryProperties2 props = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PROPERTIES_2
    };
    vkGetPhysicalDeviceMemoryProperties2(physical_device, &props);

    return props.memoryProperties;
}

std::vector<VkExtensionProperties> get_physical_device_extension_properties(VkPhysicalDevice physical_device) {
    uint32_t count = 0;
    VkResult result = vkEnumerateDeviceExtensionProperties(physical_device, nullptr, &count, nullptr);
    if (result != VK_SUCCESS) {
        throw std::runtime_error{ "Failed to query physical device extensions count." };
    }

    std::vector<VkExtensionProperties> extensions{ count };
    result = vkEnumerateDeviceExtensionProperties(physical_device, nullptr, &count, extensions.data());
    if (result != VK_SUCCESS) {
        throw std::runtime_error{ "Failed to enumerate physical device extensions." };
    }

    return extensions;
}

std::vector<VkQueueFamilyProperties> get_physical_queue_family_properties(VkPhysicalDevice physical_device) {
    uint32_t count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties2(physical_device, &count, nullptr);

    std::vector<VkQueueFamilyProperties2> props2{ count };
    for (auto& prop : props2) {
        prop.sType = VK_STRUCTURE_TYPE_QUEUE_FAMILY_PROPERTIES_2;
        prop.pNext = nullptr;
    }
    vkGetPhysicalDeviceQueueFamilyProperties2(physical_device, &count, props2.data());

    std::vector<VkQueueFamilyProperties> props;
    props.reserve(props2.size());
    for (auto& prop : props2) {
        props.push_back(prop.queueFamilyProperties);
    }

    return props;
}

VkSurfaceCapabilitiesKHR get_physical_device_surface_capabilities(VkPhysicalDevice physical_device,
                                                                  VkSurfaceKHR surface) {
    VkSurfaceCapabilitiesKHR caps = {};
    VkResult result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device, surface, &caps);
    if (result != VK_SUCCESS) {
        throw std::runtime_error{ "Failed to get physical device surface capabilities." };
    }

    return caps;
}

std::vector<VkSurfaceFormatKHR> get_physical_device_surface_formats(VkPhysicalDevice physical_device,
                                                                    VkSurfaceKHR surface) {
    uint32_t count = 0;
    VkResult result = vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &count, nullptr);
    if (result != VK_SUCCESS) {
        throw std::runtime_error{ "Failed to query physical device surface formats count." };
    }
    
    std::vector<VkSurfaceFormatKHR> formats{ count };
    result = vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &count, formats.data());
    if (result != VK_SUCCESS) {
        throw std::runtime_error{ "Failed to get physical device surface formats." };
    }

    return formats;
}

std::vector<VkPresentModeKHR> get_physical_device_surface_present_modes(VkPhysicalDevice physical_device,
                                                                        VkSurfaceKHR surface) {
    uint32_t count = 0;
    VkResult result = vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &count, nullptr);
    if (result != VK_SUCCESS) {
        throw std::runtime_error{ "Failed to query physical device surface present modes count." };
    }
    
    std::vector<VkPresentModeKHR> present_modes{ count };
    result = vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &count, present_modes.data());
    if (result != VK_SUCCESS) {
        throw std::runtime_error{ "Failed to get physical device surface present modes." };
    }

    return present_modes;
}

}
