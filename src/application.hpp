#pragma once
#include "utils/non_copyable.hpp"
#include "vulkan_utils/vlk.hpp"

#include <memory>

struct SDL_Window;

class Application final :
    NonCopyable {
public:
    Application();

    // TODO(Kostu): temp?
    SDL_Window* get_window() { return window_.get(); }
    const vlk::Instance& get_instance() const { return vk_instance_; }
private:
    std::unique_ptr<SDL_Window, void(*)(SDL_Window*)> window_;

    vlk::Instance vk_instance_;
};
