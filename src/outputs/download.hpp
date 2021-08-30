#pragma once
        
#include <memory>
#include "engine_node.hpp"
#include "buffer_node.hpp"
#include "compute/image_node.hpp"
#include "buffer.hpp"
#include "glm/glm.hpp"


namespace vkd {
    class Download : public EngineNode, public HostBufferNode, public HostImageNode {
    public:
        Download() = default;
        ~Download() = default;
        Download(Download&&) = delete;
        Download(const Download&) = delete;
        
        static int32_t input_count() { return 1; }
        static int32_t output_count() { return 1; }
        static bool input_valid(int32_t input, const std::set<std::string>& tags) { return true; }
        static std::set<std::string> tags() { return {"download"}; }

        void inputs(const std::vector<std::shared_ptr<EngineNode>>& in) override {
            if (in.size() < 1) {
                throw GraphException("Input required");
            }
            auto conv = std::dynamic_pointer_cast<ImageNode>(in[0]);
            if (!conv) {
                throw GraphException("Invalid input");
            }
            _image_node = conv;
        }

        std::shared_ptr<EngineNode> clone() const override { return std::make_shared<Download>(); }

        void init() override;
        void post_init() override;
        bool update(ExecutionType type) override;
        void commands(VkCommandBuffer buf, uint32_t width, uint32_t height) override {}
        void execute(ExecutionType type, VkSemaphore wait_semaphore, Fence * fence) override;

        std::shared_ptr<AutoMapStagingBuffer> get_output_buffer() const override { return _host_buffer; }
        glm::uvec2 get_image_size() const override { return _size; }

        VkSemaphore wait_prerender() const override { return _compute_complete; }
    private:

        void _submit();
        
        std::shared_ptr<ImageNode> _image_node = nullptr;
        std::shared_ptr<Kernel> _quantise_luma = nullptr;
        std::shared_ptr<Kernel> _quantise_chroma_u = nullptr;
        std::shared_ptr<Kernel> _quantise_chroma_v = nullptr;
        std::shared_ptr<Image> _image = nullptr;
        std::shared_ptr<Buffer> _gpu_buffer = nullptr;
        std::shared_ptr<AutoMapStagingBuffer> _host_buffer = nullptr;

        VkCommandBuffer _compute_command_buffer;
        VkSemaphore _compute_complete;

        glm::uvec2 _size;
        size_t _buffer_size;
    private:

    };
}