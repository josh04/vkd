#pragma once

#include <string>
#include <memory>

#define REGISTER_NODE(A, B, C, ...) namespace{ EngineNodeRegister reg_##C{A, B, C::input_count(), C::output_count(), std::make_shared<C>(__VA_ARGS__)}; }

namespace vkd {
    class EngineNodeRegister {
    public:
        EngineNodeRegister() = delete;

        EngineNodeRegister(std::string name, std::string display_name, int32_t inputs, int32_t outputs, std::shared_ptr<EngineNode> clone);
        ~EngineNodeRegister() = default;
        EngineNodeRegister(EngineNodeRegister&&) {}
        EngineNodeRegister(const EngineNodeRegister&) {}
    };
}