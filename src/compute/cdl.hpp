#pragma once
        
#include <memory>
#include "single_kernel.hpp"
#include "glm/glm.hpp"

#include "ocio/ocio_functional.hpp"

namespace vkd {
    class CDL : public SingleKernel {
    public:
        CDL() = default;
        ~CDL() = default;
        CDL(CDL&&) = delete;
        CDL(const CDL&) = delete;

        static std::set<std::string> tags() { return {"cdl"}; }

        std::shared_ptr<EngineNode> clone() const override { return std::make_shared<CDL>(); }

        void kernel_init() override;
        void kernel_params() override;
        void kernel_pre_update(ExecutionType type) override;
        void kernel_update() override;

        void execute(ExecutionType type, Stream& stream) override;
        void post_execute(ExecutionType type) override;

        void allocate(VkCommandBuffer buf) override;
        void deallocate() override;
    private:

        std::shared_ptr<ParameterInterface> _slope_picker = nullptr;
        std::shared_ptr<ParameterInterface> _slop_param = nullptr;
        
        std::shared_ptr<ParameterInterface> _ocio_in_space = nullptr;
        std::map<int, std::shared_ptr<Image>> _ocio_images;
        OcioParamsPtr _ocio_params = nullptr;

        std::shared_ptr<Image> _intermediate_image = nullptr;
        std::shared_ptr<Kernel> _ocio_in_transform = nullptr;
        std::shared_ptr<Kernel> _ocio_out_transform = nullptr;

    };
}