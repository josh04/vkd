#include "graph.hpp"
#include "vulkan.hpp"
#include "device.hpp"
#include "fence.hpp"
#include "command_buffer.hpp"
#include "memory/memory_pool.hpp"

#include "TaskScheduler.h"

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

        ParameterCache::reset_changed();
        return do_update;
    }

    void Graph::commands(VkCommandBuffer buf, uint32_t width, uint32_t height) {
        for (auto&& node : _nodes) {
            if (node->range_contains(frame())) {
                node->commands(buf, width, height);
            }
        }
    }

    std::vector<SemaphorePtr> Graph::semaphores() {
        std::vector<SemaphorePtr> sems;
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

        std::map<EngineNode *, int> output_counts;


        if (_nodes_to_run.size()) {
            flush();
            std::vector<std::unique_ptr<enki::TaskSet>> dealloc_tasks;
            std::vector<CommandBufferPtr> cmd_buffers;
            for (auto&& node : _nodes_to_run) {
                auto buf = CommandBuffer::make(_device);
                {
                    auto scope = buf->record();
                    node->allocate(buf->get());
                }
                buf->submit();

                if (node != *_nodes_to_run.rbegin()) {
                    node->execute(type, buf->signal());
                } else {
                    node->execute(type, buf->signal(), _fence.get());
                }

                cmd_buffers.emplace_back(std::move(buf));
                
                for (auto&& input : node->graph_inputs()) {
                    output_counts[input.get()]++;
                    if (output_counts[input.get()] >= input->output_count()) {
                        
                        auto task = std::make_unique<enki::TaskSet>(1, [this, input](enki::TaskSetPartition range, uint32_t threadnum) mutable {
                            try {
                                auto fence = Fence::create(_device, false);
                                fence->submit();
                                fence->wait();
                                fence->reset();

                                input->deallocate();
                            } catch (...) {
                                std::cout << "Unknown error in deallocation task." << std::endl;
                            }
                        });

                        ts().AddTaskSetToPipe(task.get());
                        dealloc_tasks.emplace_back(std::move(task));
                    }
                }
            }
            for (auto&& task : dealloc_tasks) {
                ts().WaitforTask(task.get());
            }
            
            auto fence = Fence::create(_device, false);
            fence->submit();
            fence->wait();
            fence->reset();
        }

        for (auto&& node : _nodes_to_run) {
            node->post_execute(type);
        }
        constexpr size_t trim_limit = 6ULL * 1024ULL * 1024ULL * 1024ULL;
        _device->pool().trim(trim_limit);
    }

    void Graph::finish() {
        flush();
        for (auto&& node : _nodes) {
            if (node->range_contains(frame())) {
                node->finish();
            }
        }
        submit_compute_buffer(*_device, VK_NULL_HANDLE, VK_NULL_HANDLE, VK_NULL_HANDLE, _fence.get());
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