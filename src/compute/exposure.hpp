#pragma once
        
#include <memory>
#include "engine_node.hpp"
#include "image_node.hpp"
#include "glm/glm.hpp"

namespace vkd {
    class Exposure : public EngineNode, public ImageNode {
    public:
        Exposure() = default;
        ~Exposure() = default;
        Exposure(Exposure&&) = delete;
        Exposure(const Exposure&) = delete;

        static int32_t input_count() { return 1; }
        static int32_t output_count() { return 1; }
        static bool input_valid(int32_t input, const std::set<std::string>& tags) { return true; }
        static std::set<std::string> tags() { return {"exposure"}; }

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
        std::shared_ptr<EngineNode> clone() const override { return std::make_shared<Exposure>(); }

        void init() override;
        bool update() override;
        void commands(VkCommandBuffer buf, uint32_t width, uint32_t height) override {}
        void execute(VkSemaphore wait_semaphore, VkFence fence) override;

        VkSemaphore wait_prerender() const override { return _compute_complete; }
        auto compute_complete() const { return _compute_complete; }
        std::shared_ptr<Image> get_output_image() const override { return _image; }
        float get_output_ratio() const override { return _size[0] / (float)_size[1]; }
    private:

        void _submit();
        
        std::shared_ptr<ImageNode> _image_node = nullptr;
        std::shared_ptr<Kernel> _exposure = nullptr;
        std::shared_ptr<Image> _image = nullptr;

        VkCommandBuffer _compute_command_buffer;
        VkSemaphore _compute_complete;

        glm::uvec2 _size;
    private:

    };
}