#pragma once
        
#include <memory>
#include "single_kernel.hpp"
#include "glm/glm.hpp"

namespace vkd {
    class Saturation : public SingleKernel {
    public:
        Saturation() = default;
        ~Saturation() = default;
        Saturation(Saturation&&) = delete;
        Saturation(const Saturation&) = delete;

        static std::set<std::string> tags() { return {"saturation"}; }

        std::shared_ptr<EngineNode> clone() const override { return std::make_shared<Saturation>(); }

        void kernel_init() override;
        void kernel_params() override;
        void kernel_update() override {}
        
    private:
        std::shared_ptr<ParameterInterface> _sat_param = nullptr;
    private:

    };
}