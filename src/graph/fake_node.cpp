#include "fake_node.hpp"
#include "graph.hpp"
#include "compute/image_node.hpp"

namespace vkd {
    void FakeNode::flush() {
        _inputs.clear();
        _outputs.clear();
        //_real_node = nullptr;
    }

    void FakeNode::add_input(FakeNodePtr node) {
        _inputs.push_back(node);
        node->add_output(shared_from_this());
    }

    void FakeNode::add_output(FakeNodePtr node) {
        _outputs.push_back(node);
    }

    void FakeNode::real_node(std::shared_ptr<EngineNode> node) {
        _real_node = node;
    }
    
    void GraphBuilder::add(FakeNodePtr node) {
        _nodes.push_back(node);
    }

    std::vector<FakeNodePtr> GraphBuilder::unbaked_terminals() const {
        std::vector<FakeNodePtr> ret;
        for (auto&& node : _nodes) {
            if (node->outputs().size() == 0) {
                ret.push_back(node);
            }
        }

        return ret;
    }

    std::unique_ptr<Graph> GraphBuilder::bake(const std::shared_ptr<Device>& device) {
        auto graph = std::make_unique<Graph>(device);
        bool failed_any = false;
        try {
            for (auto&& fake_node : _nodes) {
                try {
                    fake_node->set_state(UINodeState::normal);
                    auto real_node = vkd::make(fake_node->node_type(), fake_node->node_name());
                
                    if (real_node) {
                        fake_node->real_node(real_node);
                        real_node->fake_node(fake_node);
                        real_node->set_range(fake_node->range());

                        graph->add(real_node);

                        if (fake_node->outputs().size() == 0) {
                            graph->add_terminal(real_node);
                        }
                    }

                } catch (GraphException& e) {
                    failed_any = true;
                    fake_node->set_state(UINodeState::error);
                }
            }
            
            // have to do inputs in a second pass so the real nodes all exist
            for (auto&& fake_node : _nodes) {
                try {
                    auto real_node = fake_node->real_node();
                    if (real_node) {
                        std::vector<std::shared_ptr<vkd::EngineNode>> inputs;
                        for (auto&& input : fake_node->inputs()) {
                            if (input->real_node()) {
                                inputs.push_back(input->real_node());
                            }
                        }
                        real_node->graph_inputs(inputs);
                        real_node->output_count(fake_node->outputs().size());
                        // to set init-vars
                        real_node->update_params(fake_node->params());
                    }
                    
                } catch (GraphException& e) {
                    failed_any = true;
                    fake_node->set_state(UINodeState::error);
                }
            }

            if (failed_any) {
                return nullptr;
            }

            graph->sort();
            graph->init();

        } catch (GraphException& e) {
            graph = nullptr;
        }
        
        // still do this in case the errors are resolvable through configuration
        for (auto&& fake_node : _nodes) {
            if (fake_node->real_node()) {
                // to allow kernels to register and be then set
                fake_node->real_node()->update_params(fake_node->params());
                fake_node->set_params(fake_node->real_node()->params());
            }
        }
        return graph; 
    }
}