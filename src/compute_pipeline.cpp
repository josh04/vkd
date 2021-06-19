#include "compute_pipeline.hpp"
#include "device.hpp"
#include "shader.hpp"
#include "vertex_input.hpp"

namespace vkd {
    void ComputePipeline::create(VkDescriptorSetLayout desc_set_layout, ComputeShader * shader_stage, std::array<int32_t, 3> local_sizes) {
        std::vector<VkSpecializationMapEntry> specialisations;
        specialisations.resize(3);

        int i = 0;
        for (auto&& specialisation : specialisations) {
            specialisation.constantID = i;
            specialisation.offset = i * sizeof(int32_t);
            specialisation.size = sizeof(int32_t);
            i++;
        }

        _local_sizes = local_sizes;
        create(desc_set_layout, shader_stage, specialisations);
    }

    void ComputePipeline::create(VkDescriptorSetLayout desc_set_layout, ComputeShader * shader_stage, std::vector<VkSpecializationMapEntry> specialisations) {

        

        _bind_point = VK_PIPELINE_BIND_POINT_COMPUTE;
        _layout->create(desc_set_layout);
        VkComputePipelineCreateInfo compute_pipeline_create_info = {};
        compute_pipeline_create_info.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
        compute_pipeline_create_info.layout = _layout->get();
        compute_pipeline_create_info.stage = shader_stage->get();

        VkSpecializationInfo specialisation_info = {};
        if (specialisations.size() > 0) {
            int32_t specialisation_size = 0;
            for (auto&& specialisation : specialisations) {
                specialisation_size += specialisation.size;
            }
            specialisation_info.mapEntryCount = specialisations.size();
            specialisation_info.pMapEntries = specialisations.data();
            specialisation_info.dataSize = specialisation_size;
            specialisation_info.pData = _local_sizes.data();
            compute_pipeline_create_info.stage.pSpecializationInfo = &specialisation_info;
        }

        compute_pipeline_create_info.flags = _pipeline_create_flags;
        VK_CHECK_RESULT(vkCreateComputePipelines(_device->logical_device(), _cache->get(), 1, &compute_pipeline_create_info, nullptr, &_pipeline));
    }
}