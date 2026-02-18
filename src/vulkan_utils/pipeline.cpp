#include "vulkan_utils/pipeline.hpp"
#include "vulkan_utils/device.hpp"

#include <stdexcept>

namespace vkutils {

Pipeline::Pipeline(const Device& device) :
    device_{ device }
{
    VkGraphicsPipelineCreateInfo pipeline_create_info = {};
    pipeline_create_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipeline_create_info.stageCount = static_cast<uint32_t>(stages.size());
    pipeline_create_info.pStages = stages.data();
    pipeline_create_info.pVertexInputState = &vertex_input_create_info;
    pipeline_create_info.pInputAssemblyState = &input_assembly_create_info;
    pipeline_create_info.pViewportState = &viewport_create_info;
    pipeline_create_info.pRasterizationState = &rasterization_create_info;
    pipeline_create_info.pMultisampleState = &multisample_create_info;
    pipeline_create_info.pDynamicState = &dynamic_state_create_info;
    pipeline_create_info.pColorBlendState = &color_blend_create_info;
    pipeline_create_info.layout = layout;
    pipeline_create_info.pNext = &rendering_create_info;
    VkResult result = vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipeline_create_info, nullptr, &handle_);
    if (result != VK_SUCCESS) {
        throw std::runtime_error{ "Failed to create Vulkan pipeline." };
    }
}

Pipeline::~Pipeline() {
    vkDestroyPipeline(device_, handle_, nullptr);
}

}
