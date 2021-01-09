#pragma once
#include "vulkan.hpp"

#include "pipeline.hpp"

namespace vulkan {
    class ComputeShader;
    class ComputePipeline : public Pipeline {
    public:
        using Pipeline::Pipeline;
        ~ComputePipeline() = default;
        ComputePipeline(ComputePipeline&&) = delete;
        ComputePipeline(const ComputePipeline&) = delete;

		void create(VkDescriptorSetLayout desc_set_layout, ComputeShader * shader_stage, std::array<int32_t, 3> local_sizes);
		void create(VkDescriptorSetLayout desc_set_layout, ComputeShader * shader_stage, std::vector<VkSpecializationMapEntry> specialisations = {});
    private:
        std::array<int32_t, 3> _local_sizes;
    };
}