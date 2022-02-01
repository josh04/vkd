#pragma once

#include <memory>
#include <vector>
#include <glm/glm.hpp>
#include "vulkan.hpp"

#include "engine_node.hpp"

namespace vkd {

    class Device;
    class ParticlePipeline;
    class DescriptorLayout;
    class DescriptorSet;
    class DescriptorPool;
    class Image;
    class Buffer;
    class PipelineCache;
    class ImGuiPipeline;
    class VertexBuffer;
    class IndexBuffer;

    class DrawUI : public EngineNode {
    public:
        DrawUI(size_t swapchain_count) : _swapchain_count(swapchain_count) {}
        ~DrawUI();
        DrawUI(DrawUI&&) = delete;
        DrawUI(const DrawUI&) = delete;

        void inputs(const std::vector<std::shared_ptr<EngineNode>>& in) override {}
        std::shared_ptr<EngineNode> clone() const override { return std::make_shared<DrawUI>(_swapchain_count); }

        void init() override;
        void post_init() override;
        bool update(ExecutionType type) override;
        void commands(VkCommandBuffer buf, uint32_t width, uint32_t height) override;
        void execute(ExecutionType type, const SemaphorePtr& wait_semaphore, Fence * fence) override;

        void flush() const;

        struct PushConstBlock {
            glm::vec2 scale;
            glm::vec2 translate;
        };

        enum class UiVisible {
            Off,
            FPS,
            Full,
            Max
        };

        auto get_ui_visible() { return _ui_visible; }
        void set_ui_visible(UiVisible toggle) { _ui_visible = toggle; }

        void toggle_ui() {
            _ui_visible = (UiVisible)(((int)_ui_visible + 1) % (int)UiVisible::Max);
        }
    private:
        size_t _swapchain_count = 0;
        void create_font();

        std::vector<std::shared_ptr<vkd::Image>> _font_images;
        std::shared_ptr<ImGuiPipeline> _pipeline = nullptr;


        PushConstBlock _const_block;

        VkSampler _sampler;
        std::shared_ptr<vkd::DescriptorPool> _desc_pool = nullptr;
        std::shared_ptr<vkd::DescriptorLayout> _desc_set_layout = nullptr;
        std::shared_ptr<vkd::DescriptorSet> _desc_set = nullptr;

        std::shared_ptr<VertexBuffer> _vertex_buf = nullptr;
        std::shared_ptr<IndexBuffer> _index_buf = nullptr;

        UiVisible _ui_visible = UiVisible::Full;
    };
}