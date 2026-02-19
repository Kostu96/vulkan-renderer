#pragma once
#include "utils/non_copyable.hpp"

#include <volk/volk.h>

#include <span>

namespace vkutils {

class Device;

class Pipeline final :
    NonCopyable {
public:
    Pipeline(const Device& device, std::span<const VkPipelineShaderStageCreateInfo> stages, VkFormat color_attachment_format);

    ~Pipeline();

    operator VkPipeline() const noexcept { return handle_; }
private:
    const Device& device_;
    VkPipeline handle_ = VK_NULL_HANDLE;
};

}
