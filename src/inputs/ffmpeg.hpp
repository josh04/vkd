#pragma once
        
#include <memory>
#include <string>
#include <algorithm>

#include "vulkan.hpp"
#include "engine_node.hpp"
#include "compute/image_node.hpp"
#include "imgui/ImSequencer.h"
#include "ui/timeline.hpp"

struct AVStream;
struct AVCodecContext;
struct AVFormatContext;

namespace vkd {
    class Kernel;
    class StagingBuffer;
    class StorageBuffer;
    struct Frame;

    class Ffmpeg : public EngineNode, public ImageNode {
    public:
        Ffmpeg();
        ~Ffmpeg();
        Ffmpeg(Ffmpeg&&) = delete;
        Ffmpeg(const Ffmpeg&) = delete;

        static int32_t input_count() { return 0; }
        static int32_t output_count() { return 1; }
        static bool input_valid(int32_t input, const std::set<std::string>& tags) { return true; }
        static std::set<std::string> tags() { return {"ffmpeg"}; }

        void inputs(const std::vector<std::shared_ptr<EngineNode>>& in) override {}
        std::shared_ptr<EngineNode> clone() const override { return std::make_shared<Ffmpeg>(); }

        void scrub(const Frame& frame);
        
        void post_setup() override;

        void init() override;
        void post_init() override;
        bool update(ExecutionType type) override;
        void commands(VkCommandBuffer buf, uint32_t width, uint32_t height) override {}
        void execute(ExecutionType type, VkSemaphore wait_semaphore, Fence * fence) override;

        VkSemaphore wait_prerender() const override { return _compute_complete; }
        auto compute_complete() const { return _compute_complete; }
        std::shared_ptr<Image> get_output_image() const override { return _image; }
        float get_output_ratio() const override { return _width / (float)_height; }
    private:
        //std::string _path = "test.mp4";

        int32_t _width = 1, _height = 1;

        std::shared_ptr<StagingBuffer> _staging_buffer = nullptr;
        uint32_t * _staging_buffer_ptr = nullptr;
        std::shared_ptr<StorageBuffer> _gpu_buffer = nullptr;
        std::shared_ptr<Image> _image = nullptr;

        std::shared_ptr<Kernel> _yuv420 = nullptr;

        VkCommandBuffer _compute_command_buffer;
        VkSemaphore _compute_complete;

        AVFormatContext * _format_context = nullptr;
        bool _decode_next_frame = true;
        AVCodecContext * _codec_context = nullptr;
        AVStream * _video_stream = nullptr;

        std::shared_ptr<ParameterInterface> _path_param = nullptr;
        std::shared_ptr<ParameterInterface> _frame_param = nullptr;
        
        struct BlockEditParams {
            BlockEditParams() {}
            BlockEditParams(ShaderParamMap& map, std::string hash);

            Frame translate(const Frame& frame, bool clamp) {
                auto fin_frame = frame;
                Frame crs = content_relative_start->as<Frame>().get();
                int64_t tot = total_frame_count->as<int>().get();

                if (clamp) {
                    fin_frame.index = std::clamp(frame.index + crs.index, (int64_t)0, tot);
                } else {
                    fin_frame.index = std::max(frame.index + crs.index, (int64_t)0);
                }
                return fin_frame;
            }
            std::shared_ptr<ParameterInterface> total_frame_count = nullptr;
            std::shared_ptr<ParameterInterface> frame_start_block = nullptr;
            std::shared_ptr<ParameterInterface> frame_end_block = nullptr;
            std::shared_ptr<ParameterInterface> content_relative_start = nullptr;
        };

        BlockEditParams _block;

        int64_t _frame_count = 0; // may not be available.

        int64_t _target_pts = 0;

        bool _eof = false;

        bool _force_scrub = true;
        int64_t _current_frame = -1;

        Fence * _last_fence = nullptr;

        size_t _buffer_size = 0;
        bool _blanked = false;
    };
}