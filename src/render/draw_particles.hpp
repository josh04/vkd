#pragma once

#include <memory> 
#include "vulkan.hpp"

#include "engine_node.hpp"

namespace vkd {
    
    class Device;
    class ParticlePipeline;
    class DescriptorLayout;
    class DescriptorSet;
    class DescriptorPool;
    class Image;
    class Buffer;
    class PipelineCache;

    class Particles;

    class DrawParticles : public EngineNode {
    public:
        DrawParticles() = default;
        ~DrawParticles() = default;
        DrawParticles(DrawParticles&&) = delete;
        DrawParticles(const DrawParticles&) = delete;

        static int32_t input_count() { return 1; }
        static int32_t output_count() { return 1; }
        static bool input_valid(int32_t input, const std::set<std::string>& tags) { return (tags.find("particles") != tags.end()); }
        static std::set<std::string> tags() { return {"draw"}; }

        void inputs(const std::vector<std::shared_ptr<EngineNode>>& in) override;
        std::shared_ptr<EngineNode> clone() const override { return std::make_shared<DrawParticles>(); }

        void init() override;
        bool update() override { return false; }
        void commands(VkCommandBuffer buf, uint32_t width, uint32_t height) override;
        void execute(VkSemaphore wait_semaphore, VkFence fence) override;

    private:
        std::shared_ptr<DescriptorPool> _desc_pool = nullptr;
        
        std::shared_ptr<ParticlePipeline> _particle_pipeline = nullptr;

        std::shared_ptr<DescriptorLayout> _particle_desc_layout = nullptr;
        std::shared_ptr<DescriptorSet> _particle_desc_set = nullptr;

        std::shared_ptr<Image> _particle_image_1 = nullptr;
        std::shared_ptr<Image> _particle_image_2 = nullptr;
        VkSampler _particle_sampler;

        std::shared_ptr<Particles> _compute_particles = nullptr;
    };
}