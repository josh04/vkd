#pragma once
        
#include <memory>
#include <vector>

#include "engine_node.hpp"

namespace vkd {
    class Device;
    class Graph {
    public:
        Graph() = default;
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

        void init();
        bool update();
        void commands(VkCommandBuffer buf, uint32_t width, uint32_t height);
        std::vector<VkSemaphore> semaphores();
        void execute();
        void ui();

        void flush();

        const auto& graph() { return _nodes; }
        const auto& terminals() { return _terminals; }
        const auto& params() { return _params; }
    private:
        VkFence _fence;
        std::vector<std::shared_ptr<vkd::EngineNode>> _nodes;
        std::vector<std::shared_ptr<vkd::EngineNode>> _terminals;
        ShaderParamMap _params;
    };
}