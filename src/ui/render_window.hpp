#pragma once
        
#include <memory>
#include <chrono>
#include <array>

#include "graph/fake_node.hpp"

namespace vkd {
    class Timeline;
    class Graph;
    class Performance;
    class FakeNode;
    class RenderWindow {
    public:
        RenderWindow(Timeline& timeline, Performance& perf) : _timeline(timeline), _performance(perf) {
            memset(_path, 0, 1024);
        }
        ~RenderWindow() = default;
        RenderWindow(RenderWindow&&) = delete;
        RenderWindow(const RenderWindow&) = delete;

        void attach_renderer(GraphBuilder& graph_builder, const std::vector<FakeNodePtr>& terms);

        void draw(bool& execute_graph);
        void execute(Graph& graph);

        void open(bool set) { _open = set; }
        bool rendering() const { return _running_render; }
    private:
        void _execute(Graph& graph);
        Timeline& _timeline;
        Performance& _performance;

        bool _open = false;
        
        bool _running_render = false;
        int _running_render_current = 0;
        int _running_render_target = 100;

        std::chrono::microseconds _render_time = std::chrono::microseconds(0);

        glm::ivec2 _range = {0, 100};

        char _path[1024];

    };
}