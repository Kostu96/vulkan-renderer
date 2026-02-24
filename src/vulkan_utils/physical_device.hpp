#pragma once
#include <volk/volk.h>

#include <vector>

namespace vlk {

    class PhysicalDevice final {
    public:
        VkPhysicalDeviceProperties get_properties() const noexcept;

        VkPhysicalDeviceFeatures get_features() const noexcept;

        VkPhysicalDeviceMemoryProperties get_memory_properties() const noexcept;

        std::vector<VkExtensionProperties> get_extension_properties() const;

        std::vector<VkQueueFamilyProperties> get_queue_family_properties() const;

        VkSurfaceCapabilitiesKHR get_surface_capabilities(VkSurfaceKHR surface) const;

        std::vector<VkSurfaceFormatKHR> get_surface_formats(VkSurfaceKHR surface) const;

        std::vector<VkPresentModeKHR> get_surface_present_modes(VkSurfaceKHR surface) const;

        bool get_surface_support(uint32_t queue_family_index, VkSurfaceKHR surface) const;

        operator VkPhysicalDevice() const noexcept { return handle_; }
    private:
        friend class Instance;

        explicit PhysicalDevice(VkPhysicalDevice handle) noexcept :
            handle_{ handle } {}

        VkPhysicalDevice handle_ = VK_NULL_HANDLE;
    };

}
