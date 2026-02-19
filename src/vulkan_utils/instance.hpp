#pragma once
#include "utils/non_copyable.hpp"

#include <volk/volk.h>

#include <span>
#include <vector>

namespace vkutils {

class Instance final :
    NonCopyable {
public:
    Instance(const VkApplicationInfo& app_info,
             std::span<const char*> extensions,
             std::span<const char*> layers);

    ~Instance();

    std::vector<VkPhysicalDevice> get_physical_devices() const;

    operator VkInstance() const noexcept { return handle_; }

    static std::vector<VkExtensionProperties> get_extension_properties();

    static std::vector<VkLayerProperties> get_layer_properties();
private:
    VkInstance handle_ = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT dbg_messenger_ = VK_NULL_HANDLE;
};

}
