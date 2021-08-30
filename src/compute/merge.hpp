#pragma once
        
#include <memory>

#include "engine_node.hpp"
#include "compute/image_node.hpp"

#include "command_buffer.hpp"

namespace vkd {
    class Merge : public EngineNode, public ImageNode {
    public:
        Merge() = default;
        ~Merge() = default;
        Merge(Merge&&) = delete;
        Merge(const Merge&) = delete;

        static int32_t input_count() { return -1; }
        static int32_t output_count() { return 1; }
        static bool input_valid(int32_t input, const std::set<std::string>& tags) { return true; }
        static std::set<std::string> tags() { return {"merge"}; }

        void inputs(const std::vector<std::shared_ptr<EngineNode>>& in) override {
            if (in.size() < 1) {
                throw GraphException("Input required");
            }
            for (auto&& i : in) {
                auto conv = std::dynamic_pointer_cast<ImageNode>(i);
                if (!conv) {
                    throw GraphException("Invalid input");
                }
                _inputs.push_back(conv);
            }
        }

        std::shared_ptr<EngineNode> clone() const override { return std::make_shared<Merge>(); }

        void init() override;
        void post_init() override;
        bool update(ExecutionType type) override;
        void commands(VkCommandBuffer buf, uint32_t width, uint32_t height) override {}
        void execute(ExecutionType type, VkSemaphore wait_semaphore, Fence * fence) override;

        VkSemaphore wait_prerender() const override { return _compute_complete; }
        auto compute_complete() const { return _compute_complete; }
        std::shared_ptr<Image> get_output_image() const override { return _image; }
        float get_output_ratio() const override { return _size[0] / (float)_size[1]; }
    private:
        std::vector<std::shared_ptr<ImageNode>> _inputs;
        std::shared_ptr<Kernel> _merge = nullptr;
        std::shared_ptr<Kernel> _blank = nullptr;
        std::shared_ptr<Image> _image = nullptr;

        CommandBufferPtr _command_buffer = nullptr;
        VkSemaphore _compute_complete;

        glm::uvec2 _size;
        bool _first_run = true;
    };
}