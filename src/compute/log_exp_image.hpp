#pragma once
        
#include <memory>
#include "single_kernel.hpp"
#include "glm/glm.hpp"

namespace vkd {
    class Exp : public SingleKernel {
    public:
        Exp() = default;
        ~Exp() = default;
        Exp(Exp&&) = delete;
        Exp(const Exp&) = delete;

        static std::set<std::string> tags() { return {"exp"}; }

        std::shared_ptr<EngineNode> clone() const override { return std::make_shared<Exp>(); }

        void kernel_init() override;
        void kernel_params() override;
        void kernel_update() override {}

    private:

    };
    class Log : public SingleKernel {
    public:
        Log() = default;
        ~Log() = default;
        Log(Log&&) = delete;
        Log(const Log&) = delete;

        static std::set<std::string> tags() { return {"log"}; }

        std::shared_ptr<EngineNode> clone() const override { return std::make_shared<Log>(); }

        void kernel_init() override;
        void kernel_params() override;
        void kernel_update() override {}

    private:

    };
}