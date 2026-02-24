#pragma once
#include "utils/non_copyable.hpp"

#include <volk/volk.h>
#define VMA_STATIC_VULKAN_FUNCTIONS 0
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 0
#include <vma/vk_mem_alloc.h>

namespace vlk {

class Device;
class Instance;

}

class VMAAllocator final :
    NonCopyable {
public:
    VMAAllocator(const vlk::Instance& instance,
                 const vlk::Device& device,
                 uint32_t api_version);

    ~VMAAllocator();

    operator VmaAllocator() const noexcept { return handle_; }
private:
    VmaAllocator handle_;
};
