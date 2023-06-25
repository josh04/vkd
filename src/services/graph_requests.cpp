#include "graph_requests.hpp"


namespace vkd {
    std::mutex GraphRequests::_singleton_mutex;
    std::unique_ptr<GraphRequests> GraphRequests::_singleton = nullptr;

    GraphRequests& GraphRequests::Get() {
        std::scoped_lock lock(_singleton_mutex);
        if (_singleton == nullptr) { _singleton = std::make_unique<GraphRequests>(); }
        return *_singleton;
    }

    void GraphRequests::Shutdown() {
        std::scoped_lock lock(_singleton_mutex);
        if (_singleton != nullptr) { _singleton = nullptr;}
    }
        
    void GraphRequests::add(const Request& request) { 
        std::scoped_lock lock(_request_mutex);
        _execution_requests.emplace_back(request); 
    }
        
    void GraphRequests::add_ui_run_with(std::shared_ptr<EngineNode> terminal) { 
        GraphRequests::Request request;
        request.type = ExecutionType::UI;
        request.extra_nodes = {terminal};
        add(request);
    }

    std::optional<GraphRequests::Request> GraphRequests::take() {
        std::scoped_lock lock(_request_mutex);
        if (_execution_requests.empty()) { return std::nullopt; }
        auto r = std::move(_execution_requests.front());
        _execution_requests.pop_front();
        return {r};
    }
}