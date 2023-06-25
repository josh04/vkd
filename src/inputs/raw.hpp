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
#include "hash.hpp"

#include "image_uploader.hpp"
#include "ocio/ocio_functional.hpp"

namespace enki {
    class TaskSet;
}

class LibRaw;

namespace vkd {
    class Kernel;
    class StagingBuffer;
    class StorageBuffer;
    class StaticHostImage;
    struct Frame;

    class Raw : public EngineNode, public ImageNode {
    public:
        Raw();
        ~Raw();
        Raw(Raw&&) = delete;
        Raw(const Raw&) = delete;

        static int32_t input_count() { return 0; }
        static int32_t output_count() { return 1; }
        static bool input_valid(int32_t input, const std::set<std::string>& tags) { return true; }
        static std::set<std::string> tags() { return {"raw"}; }

        void inputs(const std::vector<std::shared_ptr<EngineNode>>& in) override {}
        std::shared_ptr<EngineNode> clone() const override { return std::make_shared<Raw>(); }

        void post_setup() override;

        void init() override;
        std::string get_metadata(LibRaw& improc) const;
        
        bool update(ExecutionType type) override;
        void commands(VkCommandBuffer buf, uint32_t width, uint32_t height) override {}
        void execute(ExecutionType type, Stream& stream) override;

        
        std::shared_ptr<Image> get_output_image() const override { return _uploader ? _uploader->get_gpu() : nullptr; }
        float get_output_ratio() const override { return _width / (float)_height; }
        
        void allocate(VkCommandBuffer buf) override;
        void deallocate() override;

        Hash hash() { return Hash{_path_param->as<std::string>().get(), _current_dcraw_col_space}; }

        bool working() const override;
    private:
        void libraw_process();
        void load_to_uploader();
        int32_t _width = 1, _height = 1;
        
        

        std::shared_ptr<ParameterInterface> _path_param = nullptr;
        std::shared_ptr<ParameterInterface> _frame_param = nullptr;
        std::shared_ptr<ParameterInterface> _info_box = nullptr;
        std::shared_ptr<ParameterInterface> _reset = nullptr;

        std::shared_ptr<ParameterInterface> _dcraw_colour_space = nullptr;
        int32_t _current_dcraw_col_space = 0;

        std::unique_ptr<ImageUploader> _uploader = nullptr;

        BlockEditParams _block;

        int64_t _frame_count = 0; // may not be available.

        int64_t _current_frame = -1;

        bool _blanked = false;

        enki::TaskSet * _process_task = nullptr;
        std::unique_ptr<OcioNode> _ocio = nullptr;
    };
}