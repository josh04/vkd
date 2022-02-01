#pragma once
        
#include <memory>
#include "engine_node.hpp"
#include "image_node.hpp"
#include "glm/glm.hpp"

namespace vkd {
    class Crop : public EngineNode, public ImageNode {
    public:
        Crop() = default;
        ~Crop() = default;
        Crop(Crop&&) = delete;
        Crop(const Crop&) = delete;

        DECLARE_NODE(1, 1, "crop")

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
        std::shared_ptr<EngineNode> clone() const override { return std::make_shared<Crop>(); }

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

        std::shared_ptr<ImageNode> _image_node = nullptr;
        std::shared_ptr<Kernel> _crop = nullptr;
        std::shared_ptr<Image> _image = nullptr;

        VkCommandBuffer _compute_command_buffer;
        SemaphorePtr _compute_complete;

        glm::uvec2 _size;

        std::shared_ptr<ParameterInterface> _crop_margin = nullptr;
        std::shared_ptr<ParameterInterface> _crop_offset = nullptr;
    };
}