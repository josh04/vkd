#pragma once

#include <vector>
#include <memory>
#include <set>
#include <map>
#include "vulkan.hpp"
#include "parameter.hpp"
#include "graph_exception.hpp"

#define REGISTER_NODE(A, B, C, ...) namespace{ EngineNodeRegister reg{A, B, C::input_count(), C::output_count(), std::make_shared<C>(__VA_ARGS__)}; }

namespace vkd {

    enum class UINodeState {
        normal,
        error
    };
    
    class FakeNode;

    struct FrameInterval {
        Frame start;
        Frame end;
    };

    static bool operator<(const FrameInterval& lhs, const FrameInterval& rhs) {
        return lhs.start < rhs.start;
    }

    struct FrameRange {
        std::set<Frame> _individual_frames;
        std::set<FrameInterval> _frame_ranges;

        bool contains(Frame f) const {
            if (_individual_frames.find(f) != _individual_frames.end()) {
                return true;
            }
            for (auto&& range : _frame_ranges) {
                if (f.index >= range.start.index && f.index <= range.end.index) {
                    return true;
                }
            }
            return false;
        }
    };

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

    enum class ExecutionType {
        UI,
        Execution
    };

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
                _params.emplace("_", std::map<std::string, std::shared_ptr<ParameterInterface>>{});
            }

            auto&& set = _params.at("_");
            auto search = set.find(name);
            if (search != set.end()) {
                search->second->as<T>().set(value);
            }
        }

        void register_params(Kernel& k);
        void update_params(const ShaderParamMap& params);

        virtual void inputs(const std::vector<std::shared_ptr<EngineNode>>& in) = 0;
        virtual std::shared_ptr<EngineNode> clone() const = 0;
        virtual VkSemaphore wait_prerender() const { return VK_NULL_HANDLE; }

        virtual void post_setup() {}

        virtual void init() = 0;
        virtual void post_init() = 0;
        virtual bool update(ExecutionType type) = 0;
        virtual void commands(VkCommandBuffer buf, uint32_t width, uint32_t height) = 0;
        virtual void execute(ExecutionType type, VkSemaphore wait_semaphore, Fence * fence) = 0;
        virtual void execute(ExecutionType type, VkSemaphore wait_semaphore) { execute(type, wait_semaphore, nullptr); };
        virtual void ui() {};
        virtual void finish() {};

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

        const auto& range() const { return _range; }
        void set_range(const FrameRange& range) { _range = range; }
        bool range_contains(Frame frame) const { 
            if (frame == _last_frame) {
                return _last_contains;
            }
            _last_frame = frame; 
            _last_contains = _range.contains(frame); 
            return _last_contains; 
        }

        void fake_node(const std::shared_ptr<FakeNode>& fake_node) { _fake_node = fake_node; }
        void set_state(UINodeState state);

        std::string hash() const { return std::to_string(_hash); }

        const auto& graph_inputs() const { return _inputs; }
        void graph_inputs(std::vector<std::shared_ptr<EngineNode>> input) {
            inputs(input);
            _inputs = input; 
        }
    protected:
        std::shared_ptr<Device> _device = nullptr;
        std::shared_ptr<Renderpass> _renderpass = nullptr;
        std::shared_ptr<PipelineCache> _pipeline_cache = nullptr;
        std::shared_ptr<Timeline> _timeline = nullptr;
        ShaderParamMap _params;

        std::vector<std::shared_ptr<EngineNode>> _inputs;

    private:
        static std::map<std::string, NodeData> _NodeData;
        std::string _param_hash_name;

        FrameRange _range;
        mutable bool _last_contains = true;
        mutable Frame _last_frame;
        std::weak_ptr<FakeNode> _fake_node;

        const int64_t _hash = _next_hash++;
        static std::atomic_int64_t _next_hash;

    };
}