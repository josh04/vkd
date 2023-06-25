#include "draw_particles.hpp"

#include "pipeline.hpp"
#include <glm/glm.hpp>

#include "sampler.hpp"
#include "descriptor_sets.hpp"
#include "load_ktx.hpp"
#include "shader.hpp"
#include "compute/particles.hpp"
#include "vertex_input.hpp"
#include "viewport_and_scissor.hpp"
#include "device.hpp"

namespace vkd {
    REGISTER_NODE("draw_particles", "draw particles", DrawParticles);

    class ParticlePipelineLayout : public vkd::PipelineLayout {
    public:
        using vkd::PipelineLayout::PipelineLayout;

        void constant(VkPipelineLayoutCreateInfo& info) override {

            VkPushConstantRange push_constant_range = {};
            push_constant_range.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
            push_constant_range.offset = 0;
            push_constant_range.size = sizeof(glm::vec2);

            _push_constant_ranges.push_back(push_constant_range);
        }
    };

    class ParticlePipeline : public GraphicsPipeline {
    public:
        using GraphicsPipeline::GraphicsPipeline;

        void _blend_attachments() override {
            VkPipelineColorBlendAttachmentState blend_attachment_state = {};
            blend_attachment_state.blendEnable = VK_TRUE;
            blend_attachment_state.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
            blend_attachment_state.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
            blend_attachment_state.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
            blend_attachment_state.colorBlendOp = VK_BLEND_OP_ADD;
            blend_attachment_state.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
            blend_attachment_state.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
            blend_attachment_state.alphaBlendOp = VK_BLEND_OP_ADD;

            _blend_attachment_states.push_back(blend_attachment_state);
        }
    };

    void DrawParticles::inputs(const std::vector<std::shared_ptr<EngineNode>>& in) {
        if (in.size() < 1) {
            throw GraphException("DrawParticles requires more than one input");
        }
        _compute_particles = std::dynamic_pointer_cast<Particles>(in[0]);
        if (!_compute_particles) {
            throw GraphException("DrawParticles requires passing a Particles node as inputmore than one inputmore than one input");
        }
    }

    void DrawParticles::init() {
        _particle_sampler = ScopedSampler::make(_device);
        _particle_image_1 = load_ktx(_device, "textures/particle01_rgba.ktx", VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        _particle_image_2 = load_ktx(_device, "textures/particle_gradient_rgba.ktx", VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
       
        _desc_pool = std::make_shared<DescriptorPool>(_device);
        _desc_pool->add_combined_image_sampler(2);
        _desc_pool->create(1); // one for triangles, one for particles

        _particle_desc_layout = std::make_shared<DescriptorLayout>(_device);
        _particle_desc_layout->add(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT);
        _particle_desc_layout->add(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT);
        _particle_desc_layout->create();

        _particle_desc_set = std::make_shared<DescriptorSet>(_device, _particle_desc_layout, _desc_pool);
        _particle_desc_set->add_image(_particle_image_1, _particle_sampler);
        _particle_desc_set->add_image(_particle_image_2, _particle_sampler);
        _particle_desc_set->create();

        _particle_pipeline = std::make_shared<ParticlePipeline>(_device, _pipeline_cache, _renderpass);
		auto shader_stages = std::make_unique<Shader>(_device);
		shader_stages->add(VK_SHADER_STAGE_VERTEX_BIT, "shaders/particles/particle.vert.spv", "main");
		shader_stages->add(VK_SHADER_STAGE_FRAGMENT_BIT, "shaders/particles/particle.frag.spv", "main");

		auto vertex_input = std::make_unique<VertexInput>();
		vertex_input->add_binding(0, sizeof(Particles::Particle), VK_VERTEX_INPUT_RATE_VERTEX);
		vertex_input->add_attribute(0, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Particles::Particle, pos));
		vertex_input->add_attribute(0, 1, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(Particles::Particle, gradientPos));

        auto pl = std::make_shared<ParticlePipelineLayout>(_device);
        pl->create(_particle_desc_layout->get());

        _particle_pipeline->set_topology(VK_PRIMITIVE_TOPOLOGY_POINT_LIST);
        _particle_pipeline->set_depth_test(false);
        _particle_pipeline->create(pl, std::move(shader_stages), std::move(vertex_input));
    }

    void DrawParticles::commands(VkCommandBuffer buf, uint32_t width, uint32_t height) {
        viewport_and_scissor(buf, width, height, width, height);
        _particle_pipeline->bind(buf, _particle_desc_set);

		glm::vec2 screendim = glm::vec2((float)width, (float)height);
        vkCmdPushConstants(buf, _particle_pipeline->layout()->get(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::vec2), &screendim);

        std::array<VkDeviceSize, 1> offsets = { 0 };
        auto vb = _compute_particles->storage_buffer().buffer();
        vkCmdBindVertexBuffers(buf, 0, 1, &vb, offsets.data());

        vkCmdDraw(buf, PARTICLE_COUNT, 1, 0, 0);
    }

    void DrawParticles::execute(ExecutionType type, Stream& stream) {

    }
}