#pragma once
        
#include <memory>
#include <map>
#include <vector>
#include <string>

#include "vulkan.hpp"
#include "engine_node.hpp"

namespace vkd {
    struct Frame;
    class ParameterInterface;
    class Graph;
    class FakeNode;
    using FakeNodePtr = std::shared_ptr<FakeNode>;

    class FakeNode : public std::enable_shared_from_this<FakeNode> {
    public:
        FakeNode(int32_t ui_id, std::string node_name, std::string node_type) 
            : _node_name(std::to_string(ui_id) + "_" + node_name), _node_type(node_type), _ui_id(ui_id) {}
        ~FakeNode() = default;
        FakeNode(FakeNode&&) = delete;
        FakeNode(const FakeNode&) = delete;

        auto ui_id() const { return _ui_id; }
        auto node_name() const { return _node_name; }
        auto node_type() const { return _node_type; }

        void flush();

        void add_input(FakeNodePtr node);

        void init() {}
        void ui() {}

        VkSemaphore wait_prerender() const { return VK_NULL_HANDLE; }
        bool update() { return false; }
        void commands(VkCommandBuffer buf, uint32_t width, uint32_t height) {}
        void execute(VkSemaphore wait_semaphore, VkFence fence) {}

        std::shared_ptr<EngineNode> real_node() const { return _real_node; }
        void real_node(std::shared_ptr<EngineNode> node);

        const auto& inputs() const { return _inputs; }
        const auto& outputs() const { return _outputs; }

        const ShaderParamMap& params() const { 
            if (_real_node) {
                return _real_node->params();
            }
            static const ShaderParamMap empty;
            return empty;
        }

    private:
        void add_output(FakeNodePtr node);
        int32_t _ui_id;
        std::string _node_name;
        std::string _node_type;
        std::vector<FakeNodePtr> _inputs;
        std::vector<FakeNodePtr> _outputs;
        
        std::shared_ptr<EngineNode> _real_node = nullptr;
    };

    class GraphBuilder {
    public:
        GraphBuilder() = default;
        ~GraphBuilder() = default;
        GraphBuilder(GraphBuilder&&) = delete;
        GraphBuilder(const GraphBuilder&) = delete;

        void add(FakeNodePtr node);

        std::unique_ptr<Graph> bake(const Frame& frame);
    private:
        std::vector<FakeNodePtr> _nodes;

    };
}