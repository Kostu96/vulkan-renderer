#include "vulkan_utils/instance.hpp"
#include "vulkan_utils/vulkan_utils.hpp"

#include <algorithm>
#include <iostream>
#include <print>
#include <stdexcept>
#include <string.h>

namespace {

VKAPI_ATTR VkBool32 VKAPI_CALL
dbg_callback(VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
             VkDebugUtilsMessageTypeFlagsEXT message_type,
             const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
             void* user_data) {
    std::println(std::cerr, "Vulkan debug callback: {}.", callback_data->pMessage);
    return VK_FALSE;
}

}

namespace vkutils {

Instance::Instance(const VkApplicationInfo& app_info,
                   std::span<const char*> extensions,
                   std::span<const char*> layers) {
    const VkInstanceCreateInfo create_info = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &app_info,
        .enabledLayerCount = static_cast<uint32_t>(layers.size()),
        .ppEnabledLayerNames = layers.data(),
        .enabledExtensionCount = static_cast<uint32_t>(extensions.size()),
        .ppEnabledExtensionNames = extensions.data()
    };
    VkResult result = vkCreateInstance(&create_info, nullptr, &handle_);
    if (result != VK_SUCCESS) {
        throw std::runtime_error{ "Failed to create Vulkan instance." };
    }
    
    volkLoadInstanceOnly(handle_);

    if (std::ranges::any_of(extensions,
                            [](auto& extension) { return strcmp(extension, VK_EXT_DEBUG_UTILS_EXTENSION_NAME) == 0; })) {
        const VkDebugUtilsMessengerCreateInfoEXT dbg_messenger_create_info = {
            .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
            .messageSeverity =
                VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
            .messageType =
                VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
            .pfnUserCallback = dbg_callback
        };
        VkResult result = vkCreateDebugUtilsMessengerEXT(handle_, &dbg_messenger_create_info, nullptr, &dbg_messenger_);
        if (result != VK_SUCCESS) {
            throw std::runtime_error{ "Failed to create debug messenger." };
        }
    }
}

Instance::~Instance() {
    vkDestroyDebugUtilsMessengerEXT(handle_, dbg_messenger_, nullptr);
    vkDestroyInstance(handle_, nullptr);
}

std::vector<VkPhysicalDevice> Instance::get_physical_devices() const {
    uint32_t count = 0;
    VkResult result = vkEnumeratePhysicalDevices(handle_, &count, nullptr);
    if (result != VK_SUCCESS) {
        throw std::runtime_error{ "Failed to query physical devices count." };
    }

    std::vector<VkPhysicalDevice> physical_devices{ count };
    result = vkEnumeratePhysicalDevices(handle_, &count, physical_devices.data());
    if (result != VK_SUCCESS) {
        throw std::runtime_error{ "Failed to enumerate physical devices." };
    }

    return physical_devices;
}

std::vector<VkExtensionProperties> Instance::get_extension_properties() {
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

std::vector<VkLayerProperties> Instance::get_layer_properties() {
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

}
