#pragma once

#include <vector>
#include <optional>
#include <memory>
#include <set>
#include <map>
#include "vulkan.hpp"
#include "graph_exception.hpp"

#include "engine_node_register.hpp"
#include "parameter.hpp"
#include "semaphore.hpp"
#include "stream.hpp"
#include "command_buffer.hpp"
#include "inputs/blockedit.hpp"

#define DECLARE_NODE(INPUTS, OUTPUTS, TAG) \
        static int32_t input_count() { return INPUTS; } \
        static int32_t output_count() { return OUTPUTS; } \
        static bool input_valid(int32_t input, const std::set<std::string>& tags) { return true; } \
        static std::set<std::string> tags() { return {TAG}; }


namespace vkd {

    class ParameterInterface;

    enum class UINodeState {
        normal,
        unconfigured,
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

    class Timeline;
    class ParameterInterface;
    class Kernel;
    
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
        void register_non_kernel_param(const std::shared_ptr<ParameterInterface>& param);
        void update_params(const ShaderParamMap& params);

        virtual void inputs(const std::vector<std::shared_ptr<EngineNode>>& in) = 0;
        virtual std::shared_ptr<EngineNode> clone() const = 0;

        virtual void post_setup() {}

        virtual void init() = 0;
        virtual bool update(ExecutionType type) = 0;
        virtual void commands(VkCommandBuffer buf, uint32_t width, uint32_t height) = 0;
        virtual void execute(ExecutionType type, Stream& stream) = 0;
        virtual void post_execute(ExecutionType type) {}
        virtual void ui() {};
        virtual void finish() {};

        virtual bool working() const { return false; }

        void set_device(std::shared_ptr<Device> device) { _device = device; }
        void set_renderpass(std::shared_ptr<Renderpass> renderpass) { _renderpass = renderpass; }
        void set_pipeline_cache(std::shared_ptr<PipelineCache> pipeline_cache) { _pipeline_cache = pipeline_cache; }
        void set_param_hash_name(const std::string& param_hash_name) { _param_hash_name = param_hash_name; }
        const auto& param_hash_name() const { return _param_hash_name; }

        struct NodeData {
            std::string type;
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
        auto fake_node() const { return _fake_node.lock(); }

        void update_params() const; 

        void set_state(UINodeState state);

        std::string hash() const { return std::to_string(_hash); }

        const auto& graph_inputs() const { return _inputs; }
        void graph_inputs(std::vector<std::shared_ptr<EngineNode>> input) {
            inputs(input);
            _inputs = input; 
        }

        const auto& device() const { return _device; }

        auto output_count() const { return _output_count; }
        void output_count(size_t c) { _output_count = c; }

        virtual void allocate(VkCommandBuffer buf) {}
        virtual void deallocate() {}

        virtual std::optional<BlockEditParams> block_edit_params() const { return std::nullopt; }
    protected:
        std::shared_ptr<Device> _device = nullptr;
        std::shared_ptr<Renderpass> _renderpass = nullptr;
        std::shared_ptr<PipelineCache> _pipeline_cache = nullptr;
        ShaderParamMap _params;

        std::vector<std::shared_ptr<EngineNode>> _inputs;

        CommandBuffer& command_buffer();

    private:
        static std::map<std::string, NodeData> _NodeData;
        std::string _param_hash_name;

        FrameRange _range;
        mutable bool _last_contains = true;
        mutable Frame _last_frame;
        std::weak_ptr<FakeNode> _fake_node;

        size_t _output_count = 0;

        const int64_t _hash = _next_hash++;
        static std::atomic_int64_t _next_hash;

        std::optional<CommandBufferPtr> _compute_command_buffer;
    };
}

#include "make_param.hpp"