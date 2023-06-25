#pragma once
        
#include <memory>
#include <vector>
#include <set>

#include "fence.hpp"
#include "engine_node.hpp"

namespace vkd {
    class Device;
    class Fence;
    class Stream;

    struct GraphPreferences {
        std::string _working_space = "";
    };

    class Graph {
    public:
        Graph(const std::shared_ptr<Device>& device) : _device(device) {}
        ~Graph() = default;
        Graph(Graph&&) = delete;
        Graph(const Graph&) = delete;

        void add(std::shared_ptr<vkd::EngineNode> node);
        void add_terminal(std::shared_ptr<vkd::EngineNode> node) { _terminals.push_back(node); }

        template<typename T>
        void set_param(const std::string& name, const T& value) {
            for (auto&& node : _nodes) {
                node->set_param(name, value);
            }
        }

        void sort();
        void init();

        enum class GraphUpdate {
            NoUpdate,
            Rebake,
            Updated
        };

        GraphUpdate update(ExecutionType type, const StreamPtr& stream);
        void commands(VkCommandBuffer buf, uint32_t width, uint32_t height);
        void execute(ExecutionType type, const StreamPtr& stream, const std::vector<std::shared_ptr<EngineNode>>& extra_nodes);
        void ui();
        void finish(Stream& stream);

        void release_command_buffer(CommandBuffer * ptr) { _command_buffers.erase(ptr); }

        const auto& graph() { return _nodes; }
        const auto& terminals() { return _terminals; }
        const auto& params() { return _params; }

        void set_frame(Frame frame) {     
            set_param("frame", _frame);
            _frame = frame;
        }
        auto frame() const { return _frame; }
    private:
        std::shared_ptr<Device> _device = nullptr;
        std::vector<std::shared_ptr<vkd::EngineNode>> _nodes;
        std::vector<std::shared_ptr<vkd::EngineNode>> _terminals;
        std::map<CommandBuffer *, CommandBufferPtr> _command_buffers;
        ShaderParamMap _params;
        Frame _frame; 
    };
}