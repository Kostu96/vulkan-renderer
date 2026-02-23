#include "vma_buffer.hpp"
#include "vma_allocator.hpp"

#include <stdexcept>

VMABuffer::VMABuffer(const VMAAllocator& allocator,
                     VkDeviceSize size,
                     VkBufferUsageFlags usage,
                     VmaAllocationCreateFlags flags) :
    allocator_{ allocator }
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
        throw std::runtime_error{ "Failed to create VMA buffer." };
    }
}

VMABuffer::~VMABuffer() {
    vmaDestroyBuffer(allocator_, buffer_, allocation_);
}

void VMABuffer::copy_memory_to_allocation(const void* memory, VkDeviceSize size, VkDeviceSize offset) const {
    vmaCopyMemoryToAllocation(allocator_, memory, allocation_, offset, size);
}
