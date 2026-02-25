#include "vlk/buffer.hpp"
#include "vlk/memory_allocator.hpp"

#include <stdexcept>

namespace vlk {

Buffer::Buffer(const vlk::MemoryAllocator& allocator,
               VkDeviceSize size,
               VkBufferUsageFlags usage,
               VmaAllocationCreateFlags flags) :
    allocator_{ allocator },
    size_{ size }
{
    const VkBufferCreateInfo buffer_create_info = {
            .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .size = size,
            .usage = usage
    };
    const VmaAllocationCreateInfo alloc_create_info = {
        .flags = flags,
        .usage = VMA_MEMORY_USAGE_AUTO
    };
    VkResult result = vmaCreateBuffer(allocator, &buffer_create_info, &alloc_create_info, &buffer_, &allocation_, nullptr);
    if (result != VK_SUCCESS) {
        throw std::runtime_error{ "Failed to create buffer." };
    }
}

Buffer::~Buffer() {
    vmaDestroyBuffer(allocator_, buffer_, allocation_);
}

void Buffer::copy_memory_to_allocation(const void* memory,
                                       VkDeviceSize size,
                                       VkDeviceSize offset) const {
    VkResult result = vmaCopyMemoryToAllocation(allocator_, memory, allocation_, offset, size);
    if (result != VK_SUCCESS) {
        throw std::runtime_error{ "Failed to copy memory to allocation." };
    }
}

}
