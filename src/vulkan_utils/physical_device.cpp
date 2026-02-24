#include "vulkan_utils/physical_device.hpp"

#include <stdexcept>

namespace vlk {

VkPhysicalDeviceProperties PhysicalDevice::get_properties() const noexcept {
    VkPhysicalDeviceProperties2 props = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2
    };
    vkGetPhysicalDeviceProperties2(handle_, &props);

    return props.properties;
}

VkPhysicalDeviceFeatures PhysicalDevice::get_features() const noexcept {
    VkPhysicalDeviceFeatures2 feats = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2
    };
    vkGetPhysicalDeviceFeatures2(handle_, &feats);

    return feats.features;
}

VkPhysicalDeviceMemoryProperties PhysicalDevice::get_memory_properties() const noexcept {
    VkPhysicalDeviceMemoryProperties2 props = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PROPERTIES_2
    };
    vkGetPhysicalDeviceMemoryProperties2(handle_, &props);

    return props.memoryProperties;
}

std::vector<VkExtensionProperties> PhysicalDevice::get_extension_properties() const {
    uint32_t count = 0;
    VkResult result = vkEnumerateDeviceExtensionProperties(handle_, nullptr, &count, nullptr);
    if (result != VK_SUCCESS) {
        throw std::runtime_error{ "Failed to query physical device extensions count." };
    }

    std::vector<VkExtensionProperties> extensions{ count };
    result = vkEnumerateDeviceExtensionProperties(handle_, nullptr, &count, extensions.data());
    if (result != VK_SUCCESS) {
        throw std::runtime_error{ "Failed to enumerate physical device extensions." };
    }

    return extensions;
}

std::vector<VkQueueFamilyProperties> PhysicalDevice::get_queue_family_properties() const {
    uint32_t count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties2(handle_, &count, nullptr);

    std::vector<VkQueueFamilyProperties2> props2{ count };
    for (auto& prop : props2) {
        prop.sType = VK_STRUCTURE_TYPE_QUEUE_FAMILY_PROPERTIES_2;
        prop.pNext = nullptr;
    }
    vkGetPhysicalDeviceQueueFamilyProperties2(handle_, &count, props2.data());

    std::vector<VkQueueFamilyProperties> props;
    props.reserve(props2.size());
    for (auto& prop : props2) {
        props.push_back(prop.queueFamilyProperties);
    }

    return props;
}

VkSurfaceCapabilitiesKHR PhysicalDevice::get_surface_capabilities(VkSurfaceKHR surface) const {
    VkSurfaceCapabilitiesKHR caps = {};
    VkResult result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(handle_, surface, &caps);
    if (result != VK_SUCCESS) {
        throw std::runtime_error{ "Failed to get physical device surface capabilities." };
    }

    return caps;
}

std::vector<VkSurfaceFormatKHR> PhysicalDevice::get_surface_formats(VkSurfaceKHR surface) const {
    uint32_t count = 0;
    VkResult result = vkGetPhysicalDeviceSurfaceFormatsKHR(handle_, surface, &count, nullptr);
    if (result != VK_SUCCESS) {
        throw std::runtime_error{ "Failed to query physical device surface formats count." };
    }

    std::vector<VkSurfaceFormatKHR> formats{ count };
    result = vkGetPhysicalDeviceSurfaceFormatsKHR(handle_, surface, &count, formats.data());
    if (result != VK_SUCCESS) {
        throw std::runtime_error{ "Failed to get physical device surface formats." };
    }

    return formats;
}

std::vector<VkPresentModeKHR> PhysicalDevice::get_surface_present_modes(VkSurfaceKHR surface) const {
    uint32_t count = 0;
    VkResult result = vkGetPhysicalDeviceSurfacePresentModesKHR(handle_, surface, &count, nullptr);
    if (result != VK_SUCCESS) {
        throw std::runtime_error{ "Failed to query physical device surface present modes count." };
    }

    std::vector<VkPresentModeKHR> present_modes{ count };
    result = vkGetPhysicalDeviceSurfacePresentModesKHR(handle_, surface, &count, present_modes.data());
    if (result != VK_SUCCESS) {
        throw std::runtime_error{ "Failed to get physical device surface present modes." };
    }

    return present_modes;
}

bool PhysicalDevice::get_surface_support(uint32_t queue_family_index, VkSurfaceKHR surface) const {
    VkBool32 support;
    VkResult result = vkGetPhysicalDeviceSurfaceSupportKHR(handle_, queue_family_index, surface, &support);
    if (result != VK_SUCCESS) {
        throw std::runtime_error{ "Failed to get physical device surface support." };
    }

    return support == VK_TRUE;
}

}
