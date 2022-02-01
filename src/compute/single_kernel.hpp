#pragma once
        
#include <memory>
#include "engine_node.hpp"
#include "image_node.hpp"
#include "image.hpp"
#include "glm/glm.hpp"
#include "command_buffer.hpp"

namespace vkd {
    class SingleKernel : public EngineNode, public ImageNode {
    public:
        SingleKernel() = default;
        ~SingleKernel() = default;
        SingleKernel(SingleKernel&&) = delete;
        SingleKernel(const SingleKernel&) = delete;

        static int32_t input_count() { return 1; }
        static int32_t output_count() { return 1; }
        static bool input_valid(int32_t input, const std::set<std::string>& tags) { return true; }

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

        void init() override;
        virtual void kernel_init() = 0;
        virtual void kernel_params() = 0;
        void post_init() override;
        bool update(ExecutionType type) override;
        void commands(VkCommandBuffer buf, uint32_t width, uint32_t height) override {}
        void execute(ExecutionType type, const SemaphorePtr& wait_semaphore, Fence * fence) override;

        const SemaphorePtr& wait_prerender() const override { return _compute_complete; }
        std::shared_ptr<Image> get_input_image() const { return _input_image; }
        std::shared_ptr<Image> get_output_image() const override { return _output_image; }
        float get_output_ratio() const override { return output_size()[0] / (float)output_size()[1]; }

        virtual glm::ivec2 kernel_dim() const { return _input_image->dim(); }
        virtual glm::ivec2 output_size() const { return _input_image->dim(); }

        auto& command_buffer() { return *_command_buffer; }
        auto semaphore() const { return _compute_complete; }

        void allocate(VkCommandBuffer buf) override;
        void deallocate() override;
    protected:
        std::shared_ptr<Kernel> _kernel = nullptr;
        CommandBufferPtr _command_buffer;
    private:
        void _check_param_sanity();
        
        std::shared_ptr<ImageNode> _image_node = nullptr;
        std::shared_ptr<Image> _input_image = nullptr;
        std::shared_ptr<Image> _output_image = nullptr;

        SemaphorePtr _compute_complete;
        
        std::shared_ptr<ParameterInterface> _sat_param = nullptr;
        bool _first_run = true;

    };
}