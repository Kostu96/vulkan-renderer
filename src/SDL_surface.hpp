#pragma once
#include "utils/non_copyable.hpp"

#include <volk/volk.h>

struct SDL_Window;

namespace vlk {

class Instance;

}

class SDLSurface final :
    NonCopyable {
public:
    SDLSurface(SDL_Window* window, const vlk::Instance& instance);
    
    SDLSurface(SDLSurface&& other) noexcept;

    ~SDLSurface();

    VkExtent2D get_extent(const VkSurfaceCapabilitiesKHR& surface_caps) const;

    operator VkSurfaceKHR() const noexcept { return handle_; }
private:
    SDL_Window* window_ = nullptr;
    const vlk::Instance& instance_;
    VkSurfaceKHR handle_ = VK_NULL_HANDLE;
};
