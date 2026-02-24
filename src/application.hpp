#pragma once
#include "utils/non_copyable.hpp"
#include "vlk/vlk.hpp"

#include <memory>

struct SDL_Window;

class Application final :
    NonCopyable {
public:
    Application();

    // TODO(Kostu): temp?
    const vlk::Instance& get_instance() const { return vk_instance_; }
    const vlk::Surface& get_surface() const { return vk_surface_; }
    uint32_t get_queue_family() const { return vk_queue_family_index; }
    const vlk::Device& get_device() const { return vk_device; }
    const vlk::Queue& get_queue() const { return vk_queue; }
private:
    vlk::PhysicalDevice choose_physical_device_and_queue_family();
    vlk::Device create_device();

    std::unique_ptr<SDL_Window, void(*)(SDL_Window*)> window_;
    vlk::Instance vk_instance_;
    vlk::Surface vk_surface_;
    uint32_t vk_queue_family_index;
    vlk::Device vk_device;
    vlk::Queue vk_queue;
};
