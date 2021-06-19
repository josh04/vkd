#include "graph.hpp"
#include "vulkan.hpp"
#include "device.hpp"
#include "fence.hpp"

namespace vkd {

    void Graph::add(std::shared_ptr<vkd::EngineNode> node) {
        _nodes.push_back(node);
    }

    void Graph::init() {
        _fence = create_fence(device().logical_device(), true);

        for (auto&& node : _nodes) {
            node->init();
        }
		//vkDestroyFence(device().logical_device(), fence, nullptr);
    }
    
    bool Graph::update() {
        bool do_update = false;
        for (auto&& node : _nodes) {
            do_update = node->update() || do_update;
        }
        return do_update;
    }

    void Graph::commands(VkCommandBuffer buf, uint32_t width, uint32_t height) {
        for (auto&& node : _nodes) {
            node->commands(buf, width, height);
        }
    }

    std::vector<VkSemaphore> Graph::semaphores() {
        std::vector<VkSemaphore> sems;
        for (auto&& node : _nodes) {
            sems.push_back(node->wait_prerender());
        }
        return sems;
    }

    void Graph::execute() {
        flush();
        for (auto&& node : _nodes) {
            if (node != *_nodes.rbegin()) {
                node->execute(VK_NULL_HANDLE);
            } else {
                node->execute(VK_NULL_HANDLE, _fence);
            }
        }
    }

    void Graph::flush() {
#define GRAPH_FENCE_TIMEOUT 500000000
		VK_CHECK_RESULT(vkWaitForFences(device().logical_device(), 1, &_fence, VK_TRUE, GRAPH_FENCE_TIMEOUT));
		VK_CHECK_RESULT(vkResetFences(device().logical_device(), 1, &_fence));
    }

    void Graph::ui() {
        for (auto&& node : _nodes) {
            node->ui();
        }
    }
}