#pragma once

#include <memory>
#include "compute_pipeline.hpp"
#include "descriptor_sets.hpp"
#include "buffer.hpp"

#include <glm/glm.hpp>

#include "engine_node.hpp"

namespace vkd {
    class Particles : public EngineNode {
    public:
        Particles() {}
        ~Particles() = default;
        Particles(Particles&&) = delete;
        Particles(const Particles&) = delete;

        static int32_t input_count() { return 0; }
        static int32_t output_count() { return 1; }
        static bool input_valid(int32_t input, const std::set<std::string>& tags) { return true; }
        static std::set<std::string> tags() { return {"particles"}; }

        void inputs(const std::vector<std::shared_ptr<EngineNode>>& in) override {}
        std::shared_ptr<EngineNode> clone() const override { return std::make_shared<Particles>(); }

        void init() override;
        void post_init() override;
        bool update(ExecutionType type) override { return false; }
        void commands(VkCommandBuffer buf, uint32_t width, uint32_t height) override {}
        void execute(ExecutionType type, const SemaphorePtr& wait_semaphore, Fence * fence) override;

        struct Particle {
            glm::vec2 pos;								// Particle position
            glm::vec2 vel;								// Particle velocity
            glm::vec4 gradientPos;						// Texture coordinates for the gradient ramp map
        };

        Buffer& storage_buffer() { return *_compute_storage_buffer; }
        const SemaphorePtr& wait_prerender() const override { return _compute_complete; }
        auto compute_complete() const { return _compute_complete; }
    private:
    #define PARTICLE_COUNT 256 * 128
        std::shared_ptr<ComputePipeline> _compute_pipeline = nullptr;

        std::shared_ptr<DescriptorLayout> _compute_desc_set_layout = nullptr;
        std::shared_ptr<DescriptorSet> _compute_desc_set = nullptr;
        std::shared_ptr<DescriptorPool> _desc_pool = nullptr;

        std::shared_ptr<UniformBuffer> _compute_uniform_buffer = nullptr;
        std::shared_ptr<Buffer> _compute_storage_buffer = nullptr;

        struct ComputeUBO {							// Compute shader uniform block object
			float deltaT;							//		Frame delta time
			float destX;							//		x position of the attractor
			float destY;							//		y position of the attractor
			int32_t particleCount = PARTICLE_COUNT;
		} ubo;

        // SSBO particle declaration
        

        VkCommandBuffer _compute_command_buffer;

        uint8_t * _mapped_uniform_buffer = nullptr;

        SemaphorePtr _compute_complete;
    };
}