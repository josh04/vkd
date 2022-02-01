#pragma once
        
#include <memory>
#include "single_kernel.hpp"
#include "glm/glm.hpp"

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

        void execute(ExecutionType type, const SemaphorePtr& wait_semaphore, Fence * fence) override;
        void post_execute(ExecutionType type);
    private:

        std::shared_ptr<ParameterInterface> _slope_picker = nullptr;
        std::shared_ptr<ParameterInterface> _slop_param = nullptr;
        FencePtr _fence = nullptr;
    };
}