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

struct AVStream;
struct AVCodecContext;
struct AVFormatContext;

namespace vkd {
    class Kernel;
    class StagingBuffer;
    class StorageBuffer;
    struct Frame;
    class OcioNode;

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
        
        bool update(ExecutionType type) override;
        void commands(VkCommandBuffer buf, uint32_t width, uint32_t height) override {}
        void execute(ExecutionType type, Stream& stream) override;

        void allocate(VkCommandBuffer buf) override;
        void deallocate() override;
        
        std::shared_ptr<Image> get_output_image() const override { return _uploader ? _uploader->get_gpu() : nullptr;; }
        float get_output_ratio() const override { return _width / (float)_height; }

        std::optional<BlockEditParams> block_edit_params() const override { return _block; }
    private:
        //std::string _path = "test.mp4";

        int32_t _width = 1, _height = 1;

        std::unique_ptr<ImageUploader> _uploader = nullptr;

        

        AVFormatContext * _format_context = nullptr;
        bool _decode_next_frame = true;
        AVCodecContext * _codec_context = nullptr;
        AVStream * _video_stream = nullptr;

        std::shared_ptr<ParameterInterface> _path_param = nullptr;
        std::shared_ptr<ParameterInterface> _frame_param = nullptr;
        
        BlockEditParams _block;

        int64_t _frame_count = 0; // may not be available.

        int64_t _target_pts = 0;

        bool _eof = false;

        bool _force_scrub = true;
        int64_t _current_frame = -1;

        size_t _buffer_size = 0;
        bool _blanked = false;

        std::unique_ptr<OcioNode> _ocio = nullptr;
    };
}