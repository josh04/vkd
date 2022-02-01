#pragma once
        
#include <memory>

#include "engine_node.hpp"
#include "fence.hpp"


namespace vkd {
    class ImageNode;
    class ImageDownloader;
    class ExrOutput : public EngineNode {
    public:
        ExrOutput();
        ~ExrOutput();
        ExrOutput(ExrOutput&&) = delete;
        ExrOutput(const ExrOutput&) = delete;

        static int32_t input_count() { return 1; }
        static int32_t output_count() { return 0; }
        static bool input_valid(int32_t input, const std::set<std::string>& tags) { return true; }
        static std::set<std::string> tags() { return {"exr"}; }

        void inputs(const std::vector<std::shared_ptr<EngineNode>>& in) override {
            if (in.size() < 1) {
                throw GraphException("Input required");
            }
            auto conv = std::dynamic_pointer_cast<ImageNode>(in[0]);
            if (!conv) {
                throw GraphException("Invalid input");
            }
            _input_node = conv;
        }
        std::shared_ptr<EngineNode> clone() const override { return std::make_shared<ExrOutput>(); }
        
        void init() override;
        void post_init() override;
        bool update(ExecutionType type) override;
        void commands(VkCommandBuffer buf, uint32_t width, uint32_t height) override {}
        void execute(ExecutionType type, const SemaphorePtr& wait_semaphore, Fence * fence) override;

        const SemaphorePtr& wait_prerender() const override { return _compute_complete; }
    private:
        std::string _filename() const;

        int32_t _width = 1, _height = 1;

        uint32_t _frame_count = 0;

        std::unique_ptr<ImageDownloader> _downloader = nullptr;

        std::shared_ptr<ParameterInterface> _path_param = nullptr;

        std::shared_ptr<ImageNode> _input_node = nullptr;

        FencePtr _fence = nullptr;

        VkCommandBuffer _compute_command_buffer;
        SemaphorePtr _compute_complete;
    };
}