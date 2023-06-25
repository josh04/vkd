#pragma once
        
#include <memory>
#include <string>
#include <algorithm>

#include "vulkan.hpp"
#include "engine_node.hpp"
#include "compute/image_node.hpp"
#include "imgui/ImSequencer.h"
#include "ui/timeline.hpp"

#include "inputs/image_uploader.hpp"
#include "outputs/image_downloader.hpp"

struct AVStream;
struct AVCodecContext;
struct AVFormatContext;

namespace vkd {
    class Kernel;
    class StagingBuffer;
    class StorageBuffer;
    struct Frame;
    class OcioNode;

    class FfmpegLoop : public EngineNode, public ImageNode {
    public:
        FfmpegLoop();
        ~FfmpegLoop();
        FfmpegLoop(FfmpegLoop&&) = delete;
        FfmpegLoop(const FfmpegLoop&) = delete;

        static int32_t input_count() { return 1; }
        static int32_t output_count() { return 1; }
        static bool input_valid(int32_t input, const std::set<std::string>& tags) { return true; }
        static std::set<std::string> tags() { return {"ffmpeg_loop"}; }

        void inputs(const std::vector<std::shared_ptr<EngineNode>>& in) override {
            if (in.size() < 1) {
                throw GraphException("Input required");
            }
            auto conv = std::dynamic_pointer_cast<ImageNode>(in[0]);
            if (!conv) {
                throw GraphException("Invalid input");
            }
            _image_node = conv;
        }

        std::shared_ptr<EngineNode> clone() const override { return std::make_shared<FfmpegLoop>(); }

        void scrub(const Frame& frame);
        
        void post_setup() override;

        void init() override;
        
        bool update(ExecutionType type) override;
        void commands(VkCommandBuffer buf, uint32_t width, uint32_t height) override {}
        void execute(ExecutionType type, Stream& stream) override;

        void allocate(VkCommandBuffer buf) override;
        void deallocate() override;
        
        std::shared_ptr<Image> get_output_image() const override { return _uploader ? _uploader->get_gpu() : nullptr; }
        float get_output_ratio() const override { return _size.x / (float)_size.y; }
    private:
        std::shared_ptr<ImageNode> _image_node = nullptr;
        std::shared_ptr<Image> _dl_image = nullptr;
        glm::uvec2 _size;

        std::unique_ptr<ImageDownloader> _downloader = nullptr;
        std::unique_ptr<ImageUploader> _uploader = nullptr;

        AVCodecContext * _enc_codec_context = nullptr;
        AVCodecContext * _dec_codec_context = nullptr;

        std::unique_ptr<OcioNode> _ocio_in = nullptr;
        std::unique_ptr<OcioNode> _ocio_out = nullptr;

        std::shared_ptr<ParameterInterface> _blank_offset = nullptr;
        std::shared_ptr<ParameterInterface> _blank_size = nullptr;

        std::shared_ptr<ParameterInterface> _frame_param = nullptr;
        std::shared_ptr<ParameterInterface> _unloop_param = nullptr;
    };
}