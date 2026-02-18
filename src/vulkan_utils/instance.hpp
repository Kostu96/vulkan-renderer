#pragma once
#include "utils/non_copyable.hpp"

#include <volk/volk.h>

#include <span>

namespace vkutils {

class Instance final :
    NonCopyable {
public:
    Instance(const VkApplicationInfo& app_info,
             std::span<const char*> extensions,
             std::span<const char*> layers);

    ~Instance();

    VkInstance get_handle() { return handle_; }
private:
    VkInstance handle_ = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT dbg_messenger_ = VK_NULL_HANDLE;
};

}
