#include <vector>
#include <memory>
#include <set>
#include <mutex>
#include "vulkan.hpp"
#include "engine_node.hpp"
#include "graph/fake_node.hpp"
#include "compute/kernel.hpp"

namespace {
    std::mutex _mutex;
}

namespace vkd {
    std::atomic_int64_t EngineNode::_next_hash = 0;

    EngineNodeRegister::EngineNodeRegister(std::string name, std::string display_name, int32_t inputs, int32_t outputs, std::shared_ptr<EngineNode> clone) {
        EngineNode::register_node_type(name, display_name, inputs, outputs, clone);
    }

    void EngineNode::register_node_type(std::string name, std::string display_name, int32_t inputs, int32_t outputs, std::shared_ptr<EngineNode> clone) {
        std::lock_guard<std::mutex> lock(_mutex);
        _NodeData.emplace(name, NodeData{name, display_name, clone, inputs, outputs});
    }

    void EngineNode::register_params(Kernel& k) {
        _params[k.name()] = k.public_params();
    }
    //const std::map<std::string, NodeData> EngineNode::node_type_map() { return NodeData; }
    std::map<std::string, EngineNode::NodeData> EngineNode::_NodeData;

    void EngineNode::update_params(const ShaderParamMap& params) {
        for (auto&& param_map : params) {
            auto search_map = _params.find(param_map.first);
            if (search_map != _params.end()) {
                for (auto&& param : param_map.second) {
                    auto search_param = search_map->second.find(param.first);
                    if (search_param != search_map->second.end()) {
                        search_param->second->set_from(param.second);
                    }
                }
            }
        }
    }
    
    void EngineNode::set_state(UINodeState state) {
        auto ptr = _fake_node.lock();
        if (ptr) {
            ptr->set_state(state);
        }
    }
}