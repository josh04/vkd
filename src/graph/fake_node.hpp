#pragma once
        
#include <memory>
#include <map>
#include <vector>
#include <set>
#include <string>

#include "vulkan.hpp"
#include "engine_node.hpp"

namespace vkd {
    struct Frame;
    class ParameterInterface;
    class Graph;
    class FakeNode;
    using FakeNodePtr = std::shared_ptr<FakeNode>;

    class FakeNode : public std::enable_shared_from_this<FakeNode> {
    public:
        FakeNode(int32_t ui_id, std::string node_name, std::string node_type) 
            : _node_name(std::to_string(ui_id) + "_" + node_name), _node_type(node_type), _ui_id(ui_id) {
            if (node_type == "display") {
                throw std::runtime_error("display node had fakenode created.");
            }

        }
        ~FakeNode() = default;
        FakeNode(FakeNode&&) = delete;
        FakeNode(const FakeNode&) = delete;

        auto ui_id() const { return _ui_id; }
        auto node_name() const { return _node_name; }
        auto node_type() const { return _node_type; }

        void flush();

        void add_input(FakeNodePtr node);

        void init() {}
        void ui() {}

        bool update(ExecutionType type) { return false; }
        void commands(VkCommandBuffer buf, uint32_t width, uint32_t height) {}
        void execute(ExecutionType type, Stream& stream) {}

        std::shared_ptr<EngineNode> real_node() const { return _real_node; }
        void real_node(std::shared_ptr<EngineNode> node);

        const auto& inputs() const { return _inputs; }
        const auto& outputs() const { return _outputs; }

        ShaderParamMap& params() {
            return _param_map;
        }

        const ShaderParamMap& params() const {
            return _param_map;
        }

        void set_params(const ShaderParamMap& params) {
            _param_map = params;
        }

        template<typename T>
        void set_param(const std::string& name, const T& value) {
            if (_param_map.find("_") == _param_map.end()) {
                _param_map.emplace("_", std::map<std::string, std::shared_ptr<ParameterInterface>>{});
            }

            auto&& set = _param_map.at("_");
            auto search = set.find(name);
            if (search != set.end()) {
                search->second->as<T>().set_force(value);
            } else {
                auto param = make_param<T>(node_name(), "path", 0);
                set[name] = param;
                param->template as<T>().set_default(value);
            }
        }

        void set_state(UINodeState state) { _state = state; }
        auto get_state() const { return _state; }

        void set_range(const FrameRange& range) { _range = range; }
        auto range() const { return _range; }

        const std::string& saved_as() const { return _saved_as; }
        void set_saved_as(const std::string& name) { _saved_as = name; }
    private:
        void add_output(FakeNodePtr node);
        int32_t _ui_id;
        UINodeState _state = UINodeState::normal;
        std::string _node_name;
        std::string _node_type;
        std::vector<FakeNodePtr> _inputs;
        std::vector<std::weak_ptr<FakeNode>> _outputs;
        
        std::shared_ptr<EngineNode> _real_node = nullptr;
        ShaderParamMap _param_map;
        FrameRange _range;
        std::string _saved_as;
    };

    class GraphBuilder {
    public:
        GraphBuilder() = default;
        ~GraphBuilder() = default;
        GraphBuilder(GraphBuilder&&) = delete;
        GraphBuilder(const GraphBuilder&) = delete;

        void add(FakeNodePtr node);

        std::vector<FakeNodePtr> unbaked_terminals() const;

        std::unique_ptr<Graph> bake(const std::shared_ptr<Device>& device);
    private:
        std::vector<FakeNodePtr> _nodes;

    };
}