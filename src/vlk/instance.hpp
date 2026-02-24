#pragma once
#include "utils/non_copyable.hpp"

#include <volk/volk.h>

#include <vector>

namespace vlk {

class PhysicalDevice;

class Instance final :
    NonCopyable {
public:
    Instance(const VkApplicationInfo& app_info,
             const std::vector<const char*>& extensions,
             const std::vector<const char*>& layers);

    ~Instance();

    std::vector<PhysicalDevice> get_physical_devices() const;

    operator VkInstance() const noexcept { return handle_; }

    static std::vector<VkExtensionProperties> get_extension_properties();

    static std::vector<VkLayerProperties> get_layer_properties();
private:
    VkInstance handle_ = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT dbg_messenger_ = VK_NULL_HANDLE;
};

}
