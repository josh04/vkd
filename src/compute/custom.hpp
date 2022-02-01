#pragma once
        
#include <memory>
#include "engine_node.hpp"
#include "image_node.hpp"
#include "glm/glm.hpp"

namespace vkd {
    class Custom : public EngineNode, public ImageNode {
    public:
        Custom() = default;
        ~Custom() = default;
        Custom(Custom&&) = delete;
        Custom(const Custom&) = delete;

        DECLARE_NODE(1, 1, "custom")

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
        std::shared_ptr<EngineNode> clone() const override { return std::make_shared<Custom>(); }

        void init() override;
        void post_init() override;
        bool update(ExecutionType type) override;
        void commands(VkCommandBuffer buf, uint32_t width, uint32_t height) override {}
        void execute(ExecutionType type, const SemaphorePtr& wait_semaphore, Fence * fence) override;

        const SemaphorePtr& wait_prerender() const override { return _compute_complete; }
        std::shared_ptr<Image> get_output_image() const override { return _image; }
        float get_output_ratio() const override { return _size[0] / (float)_size[1]; }
        
        void allocate(VkCommandBuffer buf) override;
        void deallocate() override;
    private:
        void _make_shader(const std::shared_ptr<Image>& inp, const std::shared_ptr<Image>& outp, const std::string& custom_kernel);

        std::shared_ptr<ImageNode> _image_node = nullptr;
        std::shared_ptr<Kernel> _custom = nullptr;
        std::shared_ptr<Image> _image = nullptr;

        std::shared_ptr<ParameterInterface> _custom_kernel = nullptr;
        std::shared_ptr<ParameterInterface> _recompile = nullptr;

        VkCommandBuffer _compute_command_buffer;
        SemaphorePtr _compute_complete;

        glm::uvec2 _size;

    };
}