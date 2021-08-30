#pragma once
        
#include <memory>

#include "engine_node.hpp"
#include "image_node.hpp"

namespace vkd {
    class Image;
    class Kernel;
    class ColourSquares : public EngineNode, public ImageNode {
    public:
        ColourSquares(int32_t width, int32_t height);
        ~ColourSquares() = default;
        ColourSquares(ColourSquares&&) = delete;
        ColourSquares(const ColourSquares&) = delete;

        static int32_t input_count() { return 0; }
        static int32_t output_count() { return 1; }
        static bool input_valid(int32_t input, const std::set<std::string>& tags) { return true; }
        static std::set<std::string> tags() { return {"sand"}; }

        void inputs(const std::vector<std::shared_ptr<EngineNode>>& in) override {}
        std::shared_ptr<EngineNode> clone() const override { return std::make_shared<ColourSquares>(_width, _height); }

        void init() override;
        void post_init() override;
        bool update(ExecutionType type) override;
        void commands(VkCommandBuffer buf, uint32_t width, uint32_t height) override {}
        void execute(ExecutionType type, VkSemaphore wait_semaphore, Fence * fence) override;

        //Buffer& storage_buffer() { return *_compute_storage_buffer; }
        VkSemaphore wait_prerender() const override { return _compute_complete; }
        auto compute_complete() const { return _compute_complete; }
        std::shared_ptr<Image> get_output_image() const override { return _image; }
    private:
        int32_t _width, _height;

        std::shared_ptr<Kernel> _square = nullptr;
        std::shared_ptr<Image> _image = nullptr;

        VkCommandBuffer _compute_command_buffer;
        VkSemaphore _compute_complete;
    };
}