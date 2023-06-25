#include "graph.hpp"
#include "vulkan.hpp"
#include "device.hpp"
#include "fence.hpp"
#include "stream.hpp"
#include "command_buffer.hpp"
#include "memory/memory_pool.hpp"
#include "fake_node.hpp"

#include "host_scheduler.hpp"

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

        for (auto&& node : _nodes) {
            try {
                node->init();
            } catch (GraphException& e) {
                console << "Error in graph init: " << e.what() << std::endl;
                node->set_state(UINodeState::error);
                throw;
            }
        }
        /* for (auto&& rit = _nodes.rbegin(); rit != _nodes.rend(); rit++) {
            try {
                (*rit)->post_init();
            } catch (GraphException& e) {
                console << "Error in graph post_init: " << e.what() << std::endl;
                (*rit)->set_state(UINodeState::error);
                throw;
            }
        } */
    }
    
    Graph::GraphUpdate Graph::update(ExecutionType type, const StreamPtr& stream) {
        //stream.flush();

        GraphUpdate do_update = GraphUpdate::NoUpdate;
        for (auto&& node : _nodes) {
            try {
                if (node->range_contains(frame())) {
                    if (node->update(type)) {
                        do_update = GraphUpdate::Updated;
                    }
                }
                node->set_state(UINodeState::normal);
            } catch (UpdateException& e) {
                console << "UpdateException in graph update: " << e.what() << std::endl;
                node->set_state(UINodeState::unconfigured);
                break;
            } catch (GraphException& e) {
                console << "Error in graph update: " << e.what() << std::endl;
                node->set_state(UINodeState::error);
                break;
            } catch (RebakeException& e) {
                console << "Graph asked to rebake: " << e.what() << std::endl;
                do_update = GraphUpdate::Rebake;
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

    void Graph::execute(ExecutionType type, const StreamPtr& stream, const std::vector<std::shared_ptr<EngineNode>>& extra_nodes) {
        
        std::vector<vkd::EngineNode *> _nodes_to_run;
        _nodes_to_run.reserve(_nodes.size() / 2);
        for (auto&& node : _nodes) {
            if (node->range_contains(frame())) {
                _nodes_to_run.push_back(node.get());
            }
        }

        for (auto&& node : extra_nodes) {
            if (node && node->range_contains(frame())) {
                _nodes_to_run.push_back(node.get());
            }
        }

        std::map<EngineNode *, int> output_counts;

        if (_nodes_to_run.size()) {
            stream->flush();
            //std::vector<CommandBufferPtr> cmd_buffers;
            for (auto&& node : _nodes_to_run) {
                auto buf = CommandBuffer::make(_device);
                {
                    auto scope = buf->record();
                    node->allocate(buf->get());
                }
                stream->submit(buf);

                try {
                    node->execute(type, *stream);
                } catch (ImageException& e) {
                    console << "Node execution failed at " << (node->fake_node() ? node->fake_node()->node_name() : "unknown node") << ": " << e.what() << std::endl;
                }

                _command_buffers.emplace(buf.get(), std::move(buf));
                
                for (auto&& input : node->graph_inputs()) {
                    output_counts[input.get()]++;
                    if (output_counts[input.get()] >= input->output_count()) {
                        
                        auto task = std::make_unique<enki::TaskSet>(1, [this, input, stream, buf = buf.get()](enki::TaskSetPartition range, uint32_t threadnum) mutable {
                            try {
                                
                                if (stream) {
                                    auto val = stream->semaphore().increment();
                                    stream->semaphore().wait(val - 1);
                                    // important to uncomment to resume deallocing things
                                    if (input) {
                                        input->deallocate();
                                    }
                                    stream->semaphore().signal(val);
                                    stream->flush();
                                }
                                if (buf) {
                                    release_command_buffer(buf);
                                }

                            } catch (...) {
                                console << "Unknown error in deallocation task." << std::endl;
                            }
                        });

                        ts().add(std::move(task));
                    }
                }
            }
            //for (auto&& task : dealloc_tasks) {
            //    ts().WaitforTask(task.get());
            //}
            // look into moving tasks
            stream->flush();
        }

        for (auto&& node : _nodes_to_run) {
            node->post_execute(type);
        }
        constexpr size_t trim_limit = 6ULL * 1024ULL * 1024ULL * 1024ULL;
        _device->pool().trim(trim_limit);
    }

    void Graph::finish(Stream& stream) {
        stream.flush();
        for (auto&& node : _nodes) {
            if (node->range_contains(frame())) {
                node->finish();
            }
        }
    }

    void Graph::ui() {
        for (auto&& node : _nodes) {
            node->ui();
        }
    }
}