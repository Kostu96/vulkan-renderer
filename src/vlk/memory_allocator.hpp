#pragma once
#include "utils/non_copyable.hpp"

#include "vlk/vma.hpp"

namespace vlk {

class Device;
class Instance;

class MemoryAllocator final :
    NonCopyable {
public:
    MemoryAllocator(const Instance& instance,
                    const Device& device,
                    uint32_t api_version);

    ~MemoryAllocator();

    operator VmaAllocator() const noexcept { return handle_; }
private:
    VmaAllocator handle_;
};

}
