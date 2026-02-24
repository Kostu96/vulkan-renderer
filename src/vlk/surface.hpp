#pragma once
#include "utils/non_copyable.hpp"

#include <volk/volk.h>

struct SDL_Window;

namespace vlk {

class Instance;

class Surface final :
    NonCopyable {
public:
    Surface(SDL_Window* window, const Instance& instance);

    Surface(Surface&& other) noexcept;

    ~Surface();

    VkExtent2D get_extent(const VkSurfaceCapabilitiesKHR& surface_caps) const;

    operator VkSurfaceKHR() const noexcept { return handle_; }
private:
    SDL_Window* window_ = nullptr;
    const Instance& instance_;
    VkSurfaceKHR handle_ = VK_NULL_HANDLE;
};

}
