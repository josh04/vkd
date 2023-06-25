#pragma once

#include <mutex>
#include <vector>
#include <deque>
#include <optional>
#include "engine_node.hpp"

namespace vkd {
    class GraphRequests {
    public:
        GraphRequests() = default;
        ~GraphRequests() = default;
        GraphRequests(GraphRequests&&) = delete;
        GraphRequests(const GraphRequests&) = delete;

        struct Request {
            std::vector<std::shared_ptr<EngineNode>> extra_nodes;
            ExecutionType type;
        };

        static GraphRequests& Get();
        static void Shutdown();

        void add(const Request& request);
        void add_ui_run_with(std::shared_ptr<EngineNode> terminal);
        std::optional<Request> take();
    private:
        static std::mutex _singleton_mutex;
        static std::unique_ptr<GraphRequests> _singleton; 
        std::mutex _request_mutex;
        std::deque<Request> _execution_requests;
    };
}