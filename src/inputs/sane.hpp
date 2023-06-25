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
#include "host_scheduler.hpp"

#include "hash.hpp"

#include "sane/sane_formats.hpp"
#include "ocio/ocio_functional.hpp"

namespace vkd {
    class Kernel;
    class StagingBuffer;
    class StorageBuffer;
    struct Frame;

    class Sane : public EngineNode, public ImageNode {
    public:
        Sane();
        ~Sane();
        Sane(Sane&&) = delete;
        Sane(const Sane&) = delete;

        static int32_t input_count() { return 0; }
        static int32_t output_count() { return 1; }
        static bool input_valid(int32_t input, const std::set<std::string>& tags) { return true; }
        static std::set<std::string> tags() { return {"sane"}; }

        void inputs(const std::vector<std::shared_ptr<EngineNode>>& in) override {}
        std::shared_ptr<EngineNode> clone() const override { return std::make_shared<Sane>(); }

        void post_setup() override;

        void init() override;
        
        bool update(ExecutionType type) override;
        void commands(VkCommandBuffer buf, uint32_t width, uint32_t height) override {}
        void execute(ExecutionType type, Stream& stream) override;

        std::shared_ptr<Image> get_output_image() const override { return _uploader ? _uploader->get_gpu() : nullptr; }
        float get_output_ratio() const override { return _format.width / (float)_format.height; }

        void allocate(VkCommandBuffer buf) override;
        void deallocate() override;

        Hash hash() { return _scan_device ? Hash{param_hash_name(), _scan_device->as<int>().get(), _format.width, _format.height, _format.format} : Hash{}; }

        bool working() const override;
    private:
        void queue_new_scan();
        void sane_scan();
        void clear_dynamic_params();

        void read_options();

        UploadFormat _format;

        void recreate_uploader();

        std::shared_ptr<ParameterInterface> _scan_device = nullptr;
        std::shared_ptr<ParameterInterface> _new_scan = nullptr;

        std::shared_ptr<ParameterInterface> _format_width = nullptr;
        std::shared_ptr<ParameterInterface> _format_height = nullptr;
        std::shared_ptr<ParameterInterface> _format_format = nullptr;

        std::map<int32_t, ParamPtr> _dynamic_params;

        std::unique_ptr<ImageUploader> _uploader = nullptr;

        BlockEditParams _block;

        enki::TaskSet * _process_task = nullptr;

        std::atomic_bool _scan_complete = false;

        std::unique_ptr<OcioNode> _ocio = nullptr;
    };
}