#pragma once
        
#include <memory>
#include <vector>

#include "fence.hpp"
#include "engine_node.hpp"

namespace vkd {
    class Device;
    class Fence;
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
        bool update(ExecutionType type);
        void commands(VkCommandBuffer buf, uint32_t width, uint32_t height);
        std::vector<VkSemaphore> semaphores();
        void execute(ExecutionType type);
        void ui();
        void finish();

        void flush();

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
        FencePtr _fence;
        std::vector<std::shared_ptr<vkd::EngineNode>> _nodes;
        std::vector<std::shared_ptr<vkd::EngineNode>> _terminals;
        ShaderParamMap _params;
        Frame _frame; 
    };
}