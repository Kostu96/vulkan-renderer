#include "vulkan_utils.hpp"

#include <stdexcept>

namespace vkutils {

std::vector<VkExtensionProperties> get_instance_extension_properties() {
    uint32_t count = 0;
    VkResult result = vkEnumerateInstanceExtensionProperties(nullptr, &count, nullptr);
    if (result != VK_SUCCESS) {
        throw std::runtime_error{ "Failed to query instance extention properties count." };
    }

    std::vector<VkExtensionProperties> props{ count };
    result = vkEnumerateInstanceExtensionProperties(nullptr, &count, props.data());
    if (result != VK_SUCCESS) {
        throw std::runtime_error{ "Failed to enumerate instance extention properties." };
    }

    return props;
}

std::vector<VkLayerProperties> get_instance_layer_properties() {
    uint32_t count = 0;
    VkResult result = vkEnumerateInstanceLayerProperties(&count, nullptr);
    if (result != VK_SUCCESS) {
        throw std::runtime_error{ "Failed to query instance layer properties count." };
    }

    std::vector<VkLayerProperties> props{ count };
    result = vkEnumerateInstanceLayerProperties(&count, props.data());
    if (result != VK_SUCCESS) {
        throw std::runtime_error{ "Failed to enumerate instance layer properties." };
    }

    return props;
}

VkDebugUtilsMessengerEXT create_debug_messenger(VkInstance instance,
                                                PFN_vkDebugUtilsMessengerCallbackEXT callback) {
    VkDebugUtilsMessengerEXT messenger;
    VkDebugUtilsMessengerCreateInfoEXT info = {};
    info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    info.messageSeverity =
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    info.messageType =
        VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    info.pfnUserCallback = callback;
    VkResult result = vkCreateDebugUtilsMessengerEXT(instance, &info, nullptr, &messenger);
    if (result != VK_SUCCESS) {
        throw std::runtime_error{ "Failed to create debug messenger." };
    }

    return messenger;
}

std::vector<VkPhysicalDevice> get_physical_devices(VkInstance instance) {
    uint32_t count = 0;
    VkResult result = vkEnumeratePhysicalDevices(instance, &count, nullptr);
    if (result != VK_SUCCESS) {
        throw std::runtime_error{ "Failed to query physical devices count." };
    }

    std::vector<VkPhysicalDevice> physical_devices{ count };
    result = vkEnumeratePhysicalDevices(instance, &count, physical_devices.data());
    if (result != VK_SUCCESS) {
        throw std::runtime_error{ "Failed to enumerate physical devices." };
    }

    return physical_devices;
}

VkPhysicalDeviceProperties2 get_physical_device_properties(VkPhysicalDevice physical_device) {
    VkPhysicalDeviceProperties2 props = {};
    props.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
    vkGetPhysicalDeviceProperties2(physical_device, &props);

    return props;
}

VkPhysicalDeviceFeatures2 get_physical_device_features(VkPhysicalDevice physical_device) {
    VkPhysicalDeviceFeatures2 feats = {};
    feats.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    vkGetPhysicalDeviceFeatures2(physical_device, &feats);

    return feats;
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

std::vector<VkQueueFamilyProperties2> get_physical_queue_family_properties(VkPhysicalDevice physical_device) {
    uint32_t count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties2(physical_device, &count, nullptr);

    std::vector<VkQueueFamilyProperties2> props{ count };
    for (auto& prop : props) {
        prop.sType = VK_STRUCTURE_TYPE_QUEUE_FAMILY_PROPERTIES_2;
        prop.pNext = nullptr;
    }
    vkGetPhysicalDeviceQueueFamilyProperties2(physical_device, &count, props.data());

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
