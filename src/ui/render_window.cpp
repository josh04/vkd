#include "main_ui.hpp"
#include "render_window.hpp"

namespace vkd {
    void RenderWindow::draw(bool& execute_graph)
    {
        if (!_open) {
            return;
        }

        ImGui::SetNextWindowPos(ImVec2(40, 100), ImGuiCond_Once);
        ImGui::SetNextWindowSize(ImVec2(300, 300), ImGuiCond_Once);
        ImGui::SetNextWindowCollapsed(false, ImGuiCond_FirstUseEver);

        ImGui::Begin("render", &_open);

        ImGui::InputInt2("start/end", &_range.x);

        ImGui::InputText("path", _path, 1023);

        std::string render_button = "render";
        if (_running_render) { render_button = "render in progress"; }
        if (ImGui::Button(render_button.c_str()) && !_running_render) {
            _timeline.scrub(_range.x);
            _running_render_current = _range.x;
            _running_render_target = _range.y;
            _running_render = true;
            execute_graph = true;
        }

        ImGui::End();

    }

    void RenderWindow::execute(Graph& graph) {

        if (_running_render) {
            _execute(graph);
        }

    }

    void RenderWindow::attach_renderer(GraphBuilder& graph_builder, const std::vector<FakeNodePtr>& terms) {
        auto merge = std::make_shared<vkd::FakeNode>(-1, "vkd_output_merge", "merge");
        auto download = std::make_shared<vkd::FakeNode>(-1, "vkd_output_download", "download");
        auto ffmpeg_output = std::make_shared<vkd::FakeNode>(-1, "vkd_output_ffmpeg", "ffmpeg_output");

        for (auto&& term : terms) {
            merge->add_input(term);
        }

        download->add_input(merge);
        ffmpeg_output->add_input(download);

        FrameRange range;
        range._frame_ranges.emplace(FrameInterval{Frame{_range.x}, Frame{_range.y}});
        merge->set_range(range);
        download->set_range(range);
        ffmpeg_output->set_range(range);

        ffmpeg_output->set_param("path", std::string(_path));

        graph_builder.add(merge);
        graph_builder.add(download);
        graph_builder.add(ffmpeg_output);
    }

    void RenderWindow::_execute(Graph& graph) {
        _timeline.increment();
        graph.set_frame(_timeline.current_frame());
        auto before = std::chrono::high_resolution_clock::now();
        graph.update(ExecutionType::Execution);
        graph.execute(ExecutionType::Execution);
        auto after = std::chrono::high_resolution_clock::now();

        auto diff = std::chrono::duration_cast<std::chrono::microseconds>(after - before).count();
        _render_time += std::chrono::duration_cast<std::chrono::microseconds>(after - before);

        std::string report = "Execution (" + std::to_string(graph.graph().size()) + ")";

        std::string extra = "Frame " + std::to_string(_timeline.current_frame().index);
        _performance.add(report, diff, extra);

        if (_running_render_target == _running_render_current) {
            auto before = std::chrono::high_resolution_clock::now();
            graph.finish();
            auto after = std::chrono::high_resolution_clock::now();


            auto diff = std::chrono::duration_cast<std::chrono::microseconds>(after - before).count();

            std::string report = "Finalise (" + std::to_string(graph.graph().size()) + ")";

            std::string extra = "Total: " + std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(_render_time).count()) + "ms";
            _render_time = std::chrono::microseconds(0);
            _performance.add(report, diff, extra);

            _running_render = false;
        }
        _running_render_current++;
    }

}