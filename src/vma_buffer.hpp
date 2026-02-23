#pragma once
#include "utils/non_copyable.hpp"

#include <volk/volk.h>
#define VMA_STATIC_VULKAN_FUNCTIONS 0
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 0
#include <vma/vk_mem_alloc.h>

class VMAAllocator;

class VMABuffer final :
    NonCopyable {
public:
    VMABuffer(const VMAAllocator& allocator,
              VkDeviceSize size,
              VkBufferUsageFlags usage,
              VmaAllocationCreateFlags flags);

    ~VMABuffer();

    void copy_memory_to_allocation(const void* memory, VkDeviceSize size, VkDeviceSize offset = 0) const;

    VmaAllocation get_allocation() const noexcept { return allocation_; }

    VkBuffer* ptr() noexcept { return &buffer_; }

    operator VkBuffer() const noexcept { return buffer_; }
private:
    const VMAAllocator& allocator_;
    VkBuffer buffer_ = VK_NULL_HANDLE;
    VmaAllocation allocation_;
};
