#pragma once
#include "utils/non_copyable.hpp"

#include <volk/volk.h>

namespace vkutils {

class Device;

class Pipeline final :
    NonCopyable {
public:
    Pipeline(const Device& device);

    ~Pipeline();

    operator VkPipeline() const noexcept { return handle_; }
private:
    const Device& device_;
    VkPipeline handle_ = VK_NULL_HANDLE;
};

}
