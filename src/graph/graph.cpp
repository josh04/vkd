#include "graph.hpp"
#include "vulkan.hpp"
#include "device.hpp"
#include "fence.hpp"
#include "command_buffer.hpp"

namespace vkd {

    void Graph::add(std::shared_ptr<vkd::EngineNode> node) {
        _nodes.push_back(node);
    }

    void Graph::sort() {
        auto vec = std::move(_nodes);
        _nodes.clear();
        std::vector<std::shared_ptr<vkd::EngineNode>> back;

        std::deque<std::shared_ptr<vkd::EngineNode>> queue{_terminals.begin(), _terminals.end()};

        while (queue.size()) {
            auto term = queue.front();
            queue.pop_front();
            back.push_back(term);
            auto i = term->graph_inputs();
            for (auto&& in : i) {
                queue.push_back(in);
            }
        }

        _nodes = std::vector<std::shared_ptr<vkd::EngineNode>>(back.rbegin(), back.rend());
    }

    void Graph::init() {
        if (!_device) {
            throw std::runtime_error("Graph had no device.");
        }
        _fence = Fence::create(_device, true);

        for (auto&& node : _nodes) {
            try {
                node->init();
            } catch (GraphException& e) {
                node->set_state(UINodeState::error);
                throw;
            }
        }
        for (auto&& rit = _nodes.rbegin(); rit != _nodes.rend(); rit++) {
            try {
                (*rit)->post_init();
            } catch (GraphException& e) {
                (*rit)->set_state(UINodeState::error);
                throw;
            }
        }
    }
    
    bool Graph::update(ExecutionType type) {
        _fence->wait();
        bool do_update = false;
        for (auto&& node : _nodes) {
            try {
                if (node->range_contains(frame())) {
                    do_update = node->update(type) || do_update;
                }
            } catch (UpdateException& e) {
                node->set_state(UINodeState::error);
                break;
            } catch (GraphException& e) {
                node->set_state(UINodeState::error);
                break;
            }
        }
        return do_update;
    }

    void Graph::commands(VkCommandBuffer buf, uint32_t width, uint32_t height) {
        for (auto&& node : _nodes) {
            if (node->range_contains(frame())) {
                node->commands(buf, width, height);
            }
        }
    }

    std::vector<VkSemaphore> Graph::semaphores() {
        std::vector<VkSemaphore> sems;
        for (auto&& node : _nodes) {
            if (node->range_contains(frame())) {
                sems.push_back(node->wait_prerender());
            }
        }
        return sems;
    }

    void Graph::execute(ExecutionType type) {
        
        std::vector<vkd::EngineNode *> _nodes_to_run;
        _nodes_to_run.reserve(_nodes.size() / 2);
        for (auto&& node : _nodes) {
            if (node->range_contains(frame())) {
                _nodes_to_run.push_back(node.get());
            }
        }

        if (_nodes_to_run.size()) {
            flush();
            for (auto&& node : _nodes_to_run) {
                if (node != *_nodes_to_run.rbegin()) {
                    node->execute(type, VK_NULL_HANDLE);
                } else {
                    node->execute(type, VK_NULL_HANDLE, _fence.get());
                }
            }
        }
    }

    void Graph::finish() {
        flush();
        for (auto&& node : _nodes) {
            if (node->range_contains(frame())) {
                node->finish();
            }
        }
        submit_compute_buffer(_device->compute_queue(), VK_NULL_HANDLE, VK_NULL_HANDLE, VK_NULL_HANDLE, _fence.get());
    }

    void Graph::flush() {
        _fence->wait();
        _fence->reset();
    }

    void Graph::ui() {
        for (auto&& node : _nodes) {
            node->ui();
        }
    }
}