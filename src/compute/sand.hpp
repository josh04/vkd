#pragma once

#include <memory>
#include "compute_pipeline.hpp"
#include "descriptor_sets.hpp"
#include "buffer.hpp"

#include <glm/glm.hpp>

#include "engine_node.hpp"
#include "image_node.hpp"

namespace vkd {
    class Kernel;
    class Sand : public EngineNode, public ImageNode {
    public:
        Sand(int32_t width, int32_t height);
        ~Sand() = default;
        Sand(Sand&&) = delete;
        Sand(const Sand&) = delete;

        static int32_t input_count() { return 0; }
        static int32_t output_count() { return 1; }
        static bool input_valid(int32_t input, const std::set<std::string>& tags) { return true; }
        static std::set<std::string> tags() { return {"sand"}; }

        void inputs(const std::vector<std::shared_ptr<EngineNode>>& in) override {}
        std::shared_ptr<EngineNode> clone() const override { return std::make_shared<Sand>(_width, _height); }

        void init() override;
        void post_init() override;
        bool update(ExecutionType type) override;
        void commands(VkCommandBuffer buf, uint32_t width, uint32_t height) override {}
        void execute(ExecutionType type, VkSemaphore wait_semaphore, Fence * fence) override;

        //Buffer& storage_buffer() { return *_compute_storage_buffer; }
        VkSemaphore wait_prerender() const override { return _compute_complete; }
        auto compute_complete() const { return _compute_complete; }
        std::shared_ptr<Image> get_output_image() const override { return _sand_image; }
    private:

        void _submit();
        int32_t _width, _height;
        std::shared_ptr<Kernel> _sand_clear = nullptr;
        std::shared_ptr<Kernel> _sand_add_bumpers = nullptr;
        std::shared_ptr<Kernel> _sand_add = nullptr;
        std::shared_ptr<Kernel> _sand_bottom_bump = nullptr;
        std::shared_ptr<Kernel> _sand_copy_stills = nullptr;
        std::shared_ptr<Kernel> _sand_process = nullptr;
        std::shared_ptr<Kernel> _sand_to_image = nullptr;

        std::shared_ptr<StagingBuffer> _add_loc_stage = nullptr;
        uint32_t * _add_loc_map = nullptr;
        std::shared_ptr<Buffer> _sand_add_locations = nullptr;
        std::shared_ptr<Image> _sand_locations = nullptr;
        std::shared_ptr<Image> _sand_locations_sc = nullptr;
        std::shared_ptr<Image> _sand_locations_scratch = nullptr;
        std::shared_ptr<Image> _sand_image = nullptr;

        struct SandUBO {
			int wobble;
			int padding[3];
		} _ubo;

        VkCommandBuffer _compute_command_buffer;

        uint8_t * _mapped_uniform_buffer = nullptr;

        VkSemaphore _compute_complete;
    };
}