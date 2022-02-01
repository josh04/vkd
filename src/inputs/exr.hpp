#pragma once
        
#include <memory>
#include <string>
#include <algorithm>

#include "vulkan.hpp"
#include "engine_node.hpp"
#include "compute/image_node.hpp"
#include "imgui/ImSequencer.h"
#include "ui/timeline.hpp"
#include "blockedit.hpp"

#include "image_uploader.hpp"

namespace vkd {
    class Kernel;
    class StagingBuffer;
    class StorageBuffer;
    struct Frame;

    class Exr : public EngineNode, public ImageNode {
    public:
        Exr();
        ~Exr();
        Exr(Exr&&) = delete;
        Exr(const Exr&) = delete;

        static int32_t input_count() { return 0; }
        static int32_t output_count() { return 1; }
        static bool input_valid(int32_t input, const std::set<std::string>& tags) { return true; }
        static std::set<std::string> tags() { return {"exr"}; }

        void inputs(const std::vector<std::shared_ptr<EngineNode>>& in) override {}
        std::shared_ptr<EngineNode> clone() const override { return std::make_shared<Exr>(); }

        void post_setup() override;

        void init() override;
        void post_init() override;
        bool update(ExecutionType type) override;
        void commands(VkCommandBuffer buf, uint32_t width, uint32_t height) override {}
        void execute(ExecutionType type, const SemaphorePtr& wait_semaphore, Fence * fence) override;

        const SemaphorePtr& wait_prerender() const override { return _compute_complete; }
        auto compute_complete() const { return _compute_complete; }
        std::shared_ptr<Image> get_output_image() const override { return _uploader ? _uploader->get_gpu() : nullptr; }
        float get_output_ratio() const override { return _width / (float)_height; }
    private:
        int32_t _width = 1, _height = 1;
        
        VkCommandBuffer _compute_command_buffer;
        SemaphorePtr _compute_complete;

        bool _decode_next_frame = true;
        std::shared_ptr<ParameterInterface> _single_frame = nullptr;

        std::shared_ptr<ParameterInterface> _path_param = nullptr;
        std::shared_ptr<ParameterInterface> _frame_param = nullptr;

        std::unique_ptr<ImageUploader> _uploader = nullptr;

        BlockEditParams _block;

        int64_t _frame_count = 0; // may not be available.


        int64_t _current_frame = -1;

        Fence * _last_fence = nullptr;

        bool _blanked = false;
    };
}