#pragma once
        
#include <memory>
#include <vector>
#include <glm/glm.hpp>
#include "vulkan.hpp"

#include "engine_node.hpp"
#include "compute/image_node.hpp"


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

    class DrawFullscreen : public EngineNode {
    public:
        DrawFullscreen() = default;
        ~DrawFullscreen() = default;
        DrawFullscreen(DrawFullscreen&&) = delete;
        DrawFullscreen(const DrawFullscreen&) = delete;

        static int32_t input_count() { return 1; }
        static int32_t output_count() { return 1; }
        static bool input_valid(int32_t input, const std::set<std::string>& tags) { return true; }
        static std::set<std::string> tags() { return {"draw"}; }

        void inputs(const std::vector<std::shared_ptr<EngineNode>>& in) override {
            if (in.size() < 1) {
                throw std::runtime_error("Input required");
            }
            auto conv = std::dynamic_pointer_cast<ImageNode>(in[0]);
            if (!conv) {
                throw std::runtime_error("Invalid input");
            }
            _image_node = conv;
        }
        std::shared_ptr<EngineNode> clone() const override { return std::make_shared<DrawFullscreen>(); }

        void init() override;
        void post_init() override;
        bool update(ExecutionType type) override { return false; }
        void commands(VkCommandBuffer buf, uint32_t width, uint32_t height) override;
        void execute(ExecutionType type, VkSemaphore wait_semaphore, Fence * fence) override;

        struct FSVertex {
            glm::vec2 position;
            glm::vec2 texcoord;
        };
    private:
        std::shared_ptr<ImageNode> _image_node = nullptr;

        std::shared_ptr<DescriptorPool> _desc_pool = nullptr;

        std::shared_ptr<VertexBuffer> _vertex_buffer = nullptr;
        std::shared_ptr<IndexBuffer> _index_buffer = nullptr;

        std::shared_ptr<DescriptorLayout> _desc_set_layout = nullptr;
        std::shared_ptr<DescriptorSet> _desc_set = nullptr;

        std::shared_ptr<GraphicsPipeline> _pipeline = nullptr;
        
        VkSampler _sampler;

    };

}
