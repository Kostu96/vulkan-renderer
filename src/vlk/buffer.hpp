#pragma once
#include "utils/non_copyable.hpp"

#include "vlk/vma.hpp"

namespace vlk {

class MemoryAllocator;

class Buffer final :
    NonCopyable {
public:
    Buffer(const vlk::MemoryAllocator& allocator,
           VkDeviceSize size,
           VkBufferUsageFlags usage,
           VmaAllocationCreateFlags flags);

    ~Buffer();

    void copy_memory_to_allocation(const void* memory, VkDeviceSize size, VkDeviceSize offset = 0) const;

    VkDeviceSize get_size() const noexcept { return size_; }

    VkBuffer* ptr() noexcept { return &buffer_; }

    operator VkBuffer() const noexcept { return buffer_; }
private:
    const vlk::MemoryAllocator& allocator_;
    VkBuffer buffer_ = VK_NULL_HANDLE;
    VmaAllocation allocation_;
    VkDeviceSize size_ = 0;
};

}
