#pragma once
        
#include <memory>
#include <string>

#include "vulkan.hpp"
#include "engine_node.hpp"
#include "fence.hpp"
#include "buffer_node.hpp"
#include "imgui/ImSequencer.h"
#include "ui/timeline.hpp"

struct AVStream;
struct AVCodecContext;
struct AVFormatContext;
struct AVCodec;

namespace vkd {
    class Kernel;
    class StagingBuffer;
    class StorageBuffer;
    struct Frame;
    class ImageDownloader;
    class ImageNode;
    class OcioNode;

    class FfmpegOutput : public EngineNode {
    public:
        FfmpegOutput();
        ~FfmpegOutput();
        FfmpegOutput(FfmpegOutput&&) = delete;
        FfmpegOutput(const FfmpegOutput&) = delete;

        enum class avcodec_codec {
            x264,
            x265,
            vpx,
            prores
        };

        static int32_t input_count() { return 1; }
        static int32_t output_count() { return 0; }
        static bool input_valid(int32_t input, const std::set<std::string>& tags) { return true; }
        static std::set<std::string> tags() { return {"ffmpeg_output"}; }

        void inputs(const std::vector<std::shared_ptr<EngineNode>>& in) override {
            if (in.size() < 1) {
                throw GraphException("Input required");
            }
            auto conv = std::dynamic_pointer_cast<ImageNode>(in[0]);
            if (!conv) {
                throw GraphException("Invalid input");
            }
            _buffer_node = conv;
        }
        std::shared_ptr<EngineNode> clone() const override { return std::make_shared<FfmpegOutput>(); }
        
        void init() override;
        
        bool update(ExecutionType type) override;
        void commands(VkCommandBuffer buf, uint32_t width, uint32_t height) override {}
        void execute(ExecutionType type, Stream& stream) override;

        void finish() override;

        
        void allocate(VkCommandBuffer buf) override;
        void deallocate() override;

    private:
        int32_t _width = 1, _height = 1;

        int32_t _fps = 25;

        uint32_t _frame_count = 0;

        AVFormatContext * _format_context = nullptr;
        AVCodecContext * _codec_context = nullptr;
        AVStream * _video_stream = nullptr;
        AVCodec * _codec = NULL;

        std::unique_ptr<ImageDownloader> _downloader = nullptr;

        std::shared_ptr<ParameterInterface> _path_param = nullptr;

        std::shared_ptr<ImageNode> _buffer_node = nullptr;

        FencePtr _fence = nullptr;

        std::unique_ptr<OcioNode> _ocio = nullptr;
    };
}