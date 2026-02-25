#pragma once
#include "utils/non_copyable.hpp"
#include "vlk/vlk.hpp"

#include <memory>

struct SDL_Window;

class Application final :
    NonCopyable {
public:
    Application();

    ~Application();

    void update();
private:
    vlk::PhysicalDevice choose_physical_device_and_queue_family();
    vlk::Device create_device();
    VkSurfaceFormatKHR choose_swapchain_surface_format();
    vlk::Swapchain create_swapchain();
    vlk::Pipeline create_pipeline();
    void record_cmd_buffer(const vlk::CommandBuffer& cmd_buffer, VkImage image, VkImageView image_view);

    std::unique_ptr<SDL_Window, void(*)(SDL_Window*)> window_;
    vlk::Instance vk_instance_;
    vlk::Surface vk_surface_;
    uint32_t vk_queue_family_index_;
    vlk::Device vk_device_;
    vlk::Queue vk_queue_;
    vlk::CommandPool vk_staging_cmd_pool_;
    vlk::CommandPool vk_cmd_pool_;
    vlk::MemoryAllocator vk_memory_allocator_;
    VkSurfaceCapabilitiesKHR vk_surface_caps_;
    VkSurfaceFormatKHR vk_surface_format_;
    VkExtent2D vk_frame_extent_;
    vlk::Swapchain vk_swapchain_;
    vlk::Pipeline vk_pipeline_;
    vlk::Buffer vk_vertex_buffer_; // TODO(Kostu): use one buffer for vertex and index data
    vlk::Buffer vk_index_buffer_;
    std::vector<vlk::CommandBuffer> vk_cmd_buffers_;
    std::vector<vlk::Fence> vk_draw_fences_;
    std::vector<vlk::Semaphore> vk_present_semaphores_;
    std::vector<vlk::Semaphore> vk_render_semaphores_;
};
