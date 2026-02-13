#pragma once
#include <volk/volk.h>

#include <span>

namespace vkutils {

class Instance {
public:
    Instance(const VkApplicationInfo& app_info,
             std::span<const char*> extensions,
             std::span<const char*> layers);

    ~Instance();

    VkInstance get_handle() { return handle_; }
private:
    VkInstance handle_ = VK_NULL_HANDLE;
};

}
