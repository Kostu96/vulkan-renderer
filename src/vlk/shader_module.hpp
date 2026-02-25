#pragma once
#include "utils/non_copyable.hpp"

#include <volk/volk.h>

namespace vlk {

class Device;

class ShaderModule final :
    NonCopyable {
public:
    ShaderModule(const Device& device, const char* filename);

    ~ShaderModule();

    operator VkShaderModule() const noexcept { return handle_; }
private:
    const Device& device_;
    VkShaderModule handle_ = VK_NULL_HANDLE;
};

}
