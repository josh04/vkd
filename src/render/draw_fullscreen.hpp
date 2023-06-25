#pragma once
        
#include <memory>
#include <vector>
#include <array>
#include <glm/glm.hpp>
#include "vulkan.hpp"

#include "sampler.hpp"
#include "engine_node.hpp"
#include "compute/image_node.hpp"
#include "command_buffer.hpp"

#include "ocio/ocio_functional.hpp"

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

        void pre_init();
        void init() override;
        
        bool update(ExecutionType type) override;
        void commands(VkCommandBuffer buf, uint32_t width, uint32_t height) override;
        void execute(ExecutionType type, Stream& stream) override;

        void allocate(VkCommandBuffer buf) override;
        void deallocate() override;

        auto get_image() const { return _image; }

        auto input_node() const { return _image_node; }

        const auto& offset_w_h() const { return _offset_w_h; }

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
        int _desc_set_to_use = 0;
        std::array<std::shared_ptr<DescriptorSet>, 3> _desc_sets;
        std::shared_ptr<DescriptorSet> _desc_set;

        std::shared_ptr<GraphicsPipeline> _pipeline = nullptr;
        
        std::shared_ptr<Kernel> _ocio_kernel = nullptr;
        std::map<int, std::shared_ptr<Image>> _ocio_images;

        std::shared_ptr<Image> _input_image = nullptr;
        std::shared_ptr<Image> _image = nullptr;

        CommandBufferPtr _command_buffer = nullptr;
        //ScopedSamplerPtr _sampler = nullptr;

        int32_t _current_working_space_index = -1;
        int32_t _current_display_space_index = -1;

        
        glm::ivec4 _offset_w_h;

        OcioParamsPtr _ocio_params = nullptr;
    };

}
