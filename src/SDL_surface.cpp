#include "SDL_surface.hpp"
#include "vulkan_utils/instance.hpp"

#include <SDL3/SDL_vulkan.h>

#include <algorithm>
#include <limits>
#include <stdexcept>

SDLSurface::SDLSurface(SDL_Window* window, const vkutils::Instance& instance) :
    window_{ window },
    instance_{ instance }
{
    if (!SDL_Vulkan_CreateSurface(window, instance, nullptr, &handle_)) {
        throw std::runtime_error{ "Failed to create Vulkan surface." };
    }
}

SDLSurface::SDLSurface(SDLSurface &&other) noexcept :
    window_{ other.window_ },
    instance_{ other.instance_ },
    handle_{ other.handle_ }
{
    other.handle_ = VK_NULL_HANDLE;        
}

SDLSurface::~SDLSurface() {
    SDL_Vulkan_DestroySurface(instance_, handle_, nullptr);
}

VkExtent2D SDLSurface::get_extent(const VkSurfaceCapabilitiesKHR& surface_caps) const {
    if (surface_caps.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        return surface_caps.currentExtent;
    }

    int width, height;
    SDL_GetWindowSizeInPixels(window_, &width, &height);

    return {
        std::clamp<uint32_t>(width, surface_caps.minImageExtent.width, surface_caps.maxImageExtent.width),
        std::clamp<uint32_t>(height, surface_caps.minImageExtent.height, surface_caps.maxImageExtent.height)
    };
}
