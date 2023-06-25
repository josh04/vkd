#pragma once

#include <future>
#include "engine_node.hpp"
#include "immediate_exr.hpp"
#include "compute/image_node.hpp"

namespace vkd {
    class ImageDownloader;
    class AsyncImageOutput : public EngineNode {
    public:
        AsyncImageOutput(ImmediateFormat format) : _format(format) {}
        ~AsyncImageOutput() = default;
        AsyncImageOutput(AsyncImageOutput&&) = delete;
        AsyncImageOutput(const AsyncImageOutput&) = delete;

        static int32_t input_count() { return 1; }
        static int32_t output_count() { return 0; }
        static bool input_valid(int32_t input, const std::set<std::string>& tags) { return true; }
        static std::set<std::string> tags() { return {"async_image_output"}; }

        void inputs(const std::vector<std::shared_ptr<EngineNode>>& in) override {
            if (in.size() < 1) {
                throw GraphException("Input required");
            }
            auto conv = std::dynamic_pointer_cast<ImageNode>(in[0]);
            if (!conv) {
                throw GraphException("Invalid input");
            }
            _input_node = conv;
            _input_node_e = in[0];
        }
        std::shared_ptr<EngineNode> clone() const override { return std::make_shared<AsyncImageOutput>(ImmediateFormat::EXR); }
        
        void init() override;
        bool update(ExecutionType type) override;
        void commands(VkCommandBuffer buf, uint32_t width, uint32_t height) override;
        void execute(ExecutionType type, Stream& stream) override;

        std::future<bool> get_future() { return _promise.get_future(); }

    private:
        ImmediateFormat _format = ImmediateFormat::EXR;
        int32_t _width = 1, _height = 1;
        std::shared_ptr<EngineNode> _input_node_e = nullptr;
        std::shared_ptr<ImageNode> _input_node = nullptr;

        std::promise<bool> _promise;

    };
}