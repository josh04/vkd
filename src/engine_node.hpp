#pragma once

#include <vector>
#include <memory>
#include <set>
#include <map>
#include "vulkan.hpp"
#include "parameter.hpp"

#define REGISTER_NODE(A, B, C, ...) namespace{ EngineNodeRegister reg{A, B, C::input_count(), C::output_count(), std::make_shared<C>(__VA_ARGS__)}; }

namespace vkd {
    class EngineNodeRegister {
    public:
        EngineNodeRegister() = delete;

        EngineNodeRegister(std::string name, std::string display_name, int32_t inputs, int32_t outputs, std::shared_ptr<EngineNode> clone);
        ~EngineNodeRegister() = default;
        EngineNodeRegister(EngineNodeRegister&&) {}
        EngineNodeRegister(const EngineNodeRegister&) {}
    };
    class Timeline;
    class ParameterInterface;
    class Kernel;
    
    using ShaderParamMap = std::map<std::string, std::map<std::string, std::shared_ptr<ParameterInterface>>>;

    class EngineNode {
    public:
        EngineNode() = default;
        virtual ~EngineNode() = default;
        EngineNode(EngineNode&&) = delete;
        EngineNode(const EngineNode&) = delete;

        virtual const ShaderParamMap& params() const { return _params; }

        template<typename T>
        void set_param(const std::string& name, const T& value) {
            if (_params.find("_") == _params.end()) {
                return;
            }

            auto&& set = _params.at("_");
            auto search = set.find(name);
            if (search != set.end()) {
                search->second->as<T>().set(value);
            }
        }

        void register_params(Kernel& k);

        virtual void inputs(const std::vector<std::shared_ptr<EngineNode>>& in) = 0;
        virtual std::shared_ptr<EngineNode> clone() const = 0;
        virtual VkSemaphore wait_prerender() const { return VK_NULL_HANDLE; }

        virtual void init() = 0;
        virtual bool update() = 0;
        virtual void commands(VkCommandBuffer buf, uint32_t width, uint32_t height) = 0;
        virtual void execute(VkSemaphore wait_semaphore, VkFence fence) = 0;
        virtual void execute(VkSemaphore wait_semaphore) { execute(wait_semaphore, VK_NULL_HANDLE); };
        virtual void ui() {};

        void set_device(std::shared_ptr<Device> device) { _device = device; }
        void set_renderpass(std::shared_ptr<Renderpass> renderpass) { _renderpass = renderpass; }
        void set_pipeline_cache(std::shared_ptr<PipelineCache> pipeline_cache) { _pipeline_cache = pipeline_cache; }
        void set_timeline(std::shared_ptr<Timeline> timeline) { _timeline = timeline; }
        void set_param_hash_name(const std::string& param_hash_name) { _param_hash_name = param_hash_name; }
        const auto& param_hash_name() const { return _param_hash_name; }

        struct NodeData {
            std::string name;
            std::string display_name;
            std::shared_ptr<EngineNode> clone;
            int32_t inputs;
            int32_t outputs;
        };

        static void register_node_type(std::string name, std::string display_name, int32_t inputs, int32_t outputs, std::shared_ptr<EngineNode> clone);
        static const std::map<std::string, NodeData>& node_type_map() { return _NodeData; }
    protected:
        std::shared_ptr<Device> _device = nullptr;
        std::shared_ptr<Renderpass> _renderpass = nullptr;
        std::shared_ptr<PipelineCache> _pipeline_cache = nullptr;
        std::shared_ptr<Timeline> _timeline = nullptr;
        ShaderParamMap _params;

    private:
        static std::map<std::string, NodeData> _NodeData;
        std::string _param_hash_name;
    };
}