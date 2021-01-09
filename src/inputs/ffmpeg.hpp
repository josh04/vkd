#pragma once
        
#include <memory>
#include <string>

#include "vulkan.hpp"
#include "engine_node.hpp"
#include "compute/image_node.hpp"
#include "imgui/ImSequencer.h"
#include "ui/timeline.hpp"

struct AVStream;
struct AVCodecContext;
struct AVFormatContext;

namespace vulkan {
    class Kernel;
    class StagingBuffer;
/*
    struct SequencerImpl : public ImSequencer::SequenceInterface {
        int GetFrameMin() const override { return 0; }
        int GetFrameMax() const override { return frame_count; }
        int GetItemCount() const override { return 1; }

        void Get(int index, int** start, int** end, int* type, unsigned int* color) override {
            if (start) {
                *start = &p1;
            }
            if (end) {
                *end = &p2;
            }

            if (color) {
                *color = 0x999999FF;
            }

        }

        int p1 = 0;
        int p2 = 100;
        
        int frame_count = 0;
        int current_frame = 0;
        bool expanded = true;
        int selected_entry = 0;
        int first_frame = 0;

    };
    */

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

        void scrub(int64_t i);
        
        void init() override;
        bool update() override;
        void commands(VkCommandBuffer buf, uint32_t width, uint32_t height) override {}
        void execute(VkSemaphore wait_semaphore) override;

        VkSemaphore wait_prerender() const override { return _compute_complete; }
        auto compute_complete() const { return _compute_complete; }
        std::shared_ptr<Image> get_output_image() const override { return _image; }
        float get_output_ratio() const override { return _width / (float)_height; }
    private:
        //std::string _path = "test.mp4";

        int32_t _width = 1, _height = 1;
        uint32_t _frame_count = 0;

        std::shared_ptr<StagingBuffer> _buffer = nullptr;
        uint32_t * _buffer_ptr = nullptr;
        std::shared_ptr<Image> _image = nullptr;

        std::shared_ptr<Kernel> _yuv420 = nullptr;

        VkCommandBuffer _compute_command_buffer;
        VkSemaphore _compute_complete;

        AVFormatContext * _format_context = nullptr;
        bool _next_frame = true;
        AVCodecContext * _codec_context = nullptr;
        AVStream * _video_stream = nullptr;

        std::shared_ptr<ParameterInterface> _path_param = nullptr;

        std::shared_ptr<SequencerLine> _local_timeline = nullptr;

        int64_t _target_pts = 0;

        bool _eof = false;
    };
}