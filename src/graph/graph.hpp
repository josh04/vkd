#pragma once
        
#include <memory>

namespace vulkan {
    class Graph {
    public:
        Graph() = default;
        ~Graph() = default;
        Graph(Graph&&) = delete;
        Graph(const Graph&) = delete;


        const auto& graph() { return _built_nodes; }
    private:

        std::vector<std::shared_ptr<vulkan::EngineNode>> _built_nodes;
    };
}