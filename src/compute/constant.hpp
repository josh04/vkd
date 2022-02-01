#pragma once
        
#include <memory>
#include "engine_node.hpp"
#include "image_node.hpp"
#include "glm/glm.hpp"

namespace vkd {
    class Constant : public EngineNode, public ImageNode {
    public:
        Constant() = default;
        ~Constant() = default;
        Constant(Constant&&) = delete;
        Constant(const Constant&) = delete;

        DECLARE_NODE(0, 1, "constant")

        void inputs(const std::vector<std::shared_ptr<EngineNode>>& in) override {
        }

        std::shared_ptr<EngineNode> clone() const override { return std::make_shared<Constant>(); }

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

        std::shared_ptr<Kernel> _constant = nullptr;
        std::shared_ptr<Image> _image = nullptr;

        VkCommandBuffer _compute_command_buffer;
        SemaphorePtr _compute_complete;

        glm::uvec2 _size;
        std::shared_ptr<ParameterInterface> _size_param = nullptr;
    };
}