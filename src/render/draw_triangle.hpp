#pragma once
#include <vector>
#include <glm/glm.hpp>
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
    class GraphicsPipeline;
    class VertexBuffer;
    class IndexBuffer;
    class UniformBuffer;
    class DescriptorPool;

    class DrawTriangle : public EngineNode {
    public:
        DrawTriangle(float aspect
                     ) : _aspect(aspect) {}
        ~DrawTriangle() = default;
        DrawTriangle(DrawTriangle&&) = delete;
        DrawTriangle(const DrawTriangle&) = delete;

        static int32_t input_count() { return 0; }
        static int32_t output_count() { return 1; }
        static bool input_valid(int32_t input, const std::set<std::string>& tags) { return true; }
        std::shared_ptr<EngineNode> clone() const override { return std::make_shared<DrawTriangle>(_aspect); }
        static std::set<std::string> tags() { return {"draw"}; }

        void inputs(const std::vector<std::shared_ptr<EngineNode>>& in) override {}

        void init() override;
        
        bool update(ExecutionType type) override { return false; }
        void commands(VkCommandBuffer buf, uint32_t width, uint32_t height) override;
        void execute(ExecutionType type, Stream& stream) override;
    private:
        static EngineNodeRegister reg;

        std::shared_ptr<DescriptorPool> _desc_pool = nullptr;

        std::shared_ptr<VertexBuffer> _vertex_buffer = nullptr;
        std::shared_ptr<IndexBuffer> _index_buffer = nullptr;
        std::shared_ptr<UniformBuffer> _uniform_buffer = nullptr;

		glm::mat4 projection_mat;
		glm::mat4 model_mat;
		glm::mat4 view_mat;

        std::shared_ptr<DescriptorLayout> _desc_set_layout = nullptr;
        std::shared_ptr<DescriptorSet> _desc_set = nullptr;

        std::shared_ptr<GraphicsPipeline> _pipeline = nullptr;

        float _aspect = 0.0f;
    };
}