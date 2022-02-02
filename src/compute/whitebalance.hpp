#pragma once
        
#include <memory>
#include "engine_node.hpp"
#include "image_node.hpp"
#include "glm/glm.hpp"

namespace vkd {
    class WhiteBalance : public EngineNode, public ImageNode {
    public:
        WhiteBalance() = default;
        ~WhiteBalance() = default;
        WhiteBalance(WhiteBalance&&) = delete;
        WhiteBalance(const WhiteBalance&) = delete;

        static int32_t input_count() { return 1; }
        static int32_t output_count() { return 1; }
        static bool input_valid(int32_t input, const std::set<std::string>& tags) { return true; }
        static std::set<std::string> tags() { return {"whitebalance"}; }

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
        std::shared_ptr<EngineNode> clone() const override { return std::make_shared<WhiteBalance>(); }

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
        void _check_param_sanity();
        
        std::shared_ptr<ImageNode> _image_node = nullptr;
        std::shared_ptr<Kernel> _whitebalance = nullptr;
        std::shared_ptr<Image> _image = nullptr;

        VkCommandBuffer _compute_command_buffer;
        SemaphorePtr _compute_complete;

        glm::uvec2 _size;
        
        std::shared_ptr<ParameterInterface> _wb_param = nullptr;
    private:

    };
}