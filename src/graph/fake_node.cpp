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

    std::unique_ptr<Graph> GraphBuilder::bake(const Frame& frame) {
        auto graph = std::make_unique<Graph>();

        for (auto&& fake_node : _nodes) {
            auto real_node = vkd::make(fake_node->node_type(), fake_node->node_name());
            
            if (real_node) {
                fake_node->real_node(real_node);
                std::vector<std::shared_ptr<vkd::EngineNode>> inputs;
                for (auto&& input : fake_node->inputs()) {
                    if (input->real_node()) {
                        inputs.push_back(input->real_node());
                    }
                }
                real_node->inputs(inputs);

                graph->add(real_node);

                if (fake_node->outputs().size() == 0) {
                    graph->add_terminal(real_node);
                    /*
                    auto conv = std::dynamic_pointer_cast<ImageNode>(real_node);
                    if (conv != nullptr) {
                        auto draw_node = vkd::make("draw", fake_node->node_name() + "_draw");
                        draw_node->inputs({real_node});
                        graph->add(draw_node);
                    }
                    */
                }

            }
        }

        graph->init();

        return graph;
    }
}