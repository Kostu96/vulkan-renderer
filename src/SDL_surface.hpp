#pragma once
#include "utils/non_copyable.hpp"

#include <volk/volk.h>

struct SDL_Window;

namespace vkutils {

class Instance;

}

class SDLSurface final :
    NonCopyable {
public:
    SDLSurface(SDL_Window* window, const vkutils::Instance& instance);
    
    SDLSurface(SDLSurface&& other) noexcept;

    ~SDLSurface();

    VkExtent2D get_extent(const VkSurfaceCapabilitiesKHR& surface_caps) const;

    operator VkSurfaceKHR() const noexcept { return handle_; }
private:
    SDL_Window* window_ = nullptr;
    const vkutils::Instance& instance_;
    VkSurfaceKHR handle_ = VK_NULL_HANDLE;
};
