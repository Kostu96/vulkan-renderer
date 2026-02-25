#include "vlk/shader_module.hpp"
#include "vlk/device.hpp"

#include <format>
#include <fstream>
#include <stdexcept>

namespace {

std::vector<uint32_t> read_file(const char* filename) {
    std::ifstream file{ filename, std::ios::binary | std::ios::ate };
    if (!file.is_open()) {
        throw std::runtime_error(std::format("Failed to open file: {}", filename));
    }

    size_t file_size = static_cast<size_t>(file.tellg());
    std::vector<uint32_t> buffer(file_size / sizeof(uint32_t));
    file.seekg(0, std::ios::beg);
    file.read(reinterpret_cast<char*>(buffer.data()), file_size);
    file.close();

    return buffer;
}

}

namespace vlk {

ShaderModule::ShaderModule(const Device& device, const char* filename) :
    device_{ device }
{
    auto shader_code = read_file(filename);
    if (shader_code[0] != 0x07230203) {
        throw std::runtime_error(std::format("Not a valid SPIR-V - {}", filename));
    }

    const VkShaderModuleCreateInfo create_info = {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = shader_code.size() * sizeof(uint32_t),
        .pCode = shader_code.data()
    };
    VkResult result = vkCreateShaderModule(device, &create_info, nullptr, &handle_);
    if (result != VK_SUCCESS) {
        throw std::runtime_error(std::format("Failed to create Vulkan shader module from: {}", filename));
    }
}

ShaderModule::~ShaderModule() {
    vkDestroyShaderModule(device_, handle_, nullptr);
}

}
