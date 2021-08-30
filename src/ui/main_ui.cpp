#include <cereal/types/polymorphic.hpp>
#include "cereal/types/string.hpp"
#include "cereal/archives/binary.hpp"
#include "cereal/archives/json.hpp"

#include "main_ui.hpp"
#include "graph/fake_node.hpp"
#include "graph/graph.hpp"

#include "ghc/filesystem.hpp"

CEREAL_CLASS_VERSION(vkd::MainUI, 0);

namespace vkd {
    MainUI::MainUI() : _save_dialog(ImGuiFileBrowserFlags_EnterNewFilename) {
    	_timeline = std::make_shared<Timeline>();
        _render_window = std::make_unique<RenderWindow>(*_timeline, _performance);
        _bin = std::make_unique<Bin>();

        _load_dialog.SetTitle("load graph");
        _load_dialog.SetTypeFilters({ ".bin" });
        _save_dialog.SetTitle("save graph");
        _save_dialog.SetTypeFilters({ ".bin" });

        load_preferences();
        if (!_preferences.last_opened_project().empty()) {
            load(_preferences.last_opened_project());
        }
    }

    MainUI::~MainUI() = default;

    void MainUI::draw() {
            
        if (ImGui::BeginMainMenuBar()) {
            if (ImGui::BeginMenu("File")) {
                if(ImGui::MenuItem("New")) {
                    clear();
                }
                ImGui::Separator();
                if(ImGui::MenuItem("Load")) {
                    _load_dialog.Open();
                }
                if(ImGui::MenuItem("Save")) {
                    _save_dialog.Open();
                }
                ImGui::Separator();
                if(ImGui::MenuItem("Quit")) {
                    _has_quit = true;
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Window")) {
                if(ImGui::MenuItem("Render")) {
                    _render_window->open(true);
                }
                if(ImGui::MenuItem("Performance")) {
                    _performance.open(true);
                }
                if(ImGui::MenuItem("Vulkan")) {
                    _vulkan_window_open = true;
                }
                ImGui::EndMenu();
            }

            ImGui::EndMainMenuBar();
        }

        bool graph_changed = false;
        _timeline->draw(*this, graph_changed);
        _bin->draw();

        {
            _performance.draw();
        }

        if (_graph != nullptr) {
            _graph->ui();
        }

        if ( !_render_window->rendering()) {
            for (auto&& node : _node_windows) {
                node->draw(graph_changed);
            }
            // should grey out the node graphs, really
            if (graph_changed) {
                _execute_graph(ExecutionType::UI);
            }
        }
	    //node_window->draw();

        _load_dialog.Display();
        _save_dialog.Display();
        
        if(_load_dialog.HasSelected()) {
            auto file = _load_dialog.GetSelected().string();
            load(file);
            _load_dialog.ClearSelected();
        }

        if(_save_dialog.HasSelected()) {
            auto file = _save_dialog.GetSelected().string();
            save(file);
            _save_dialog.ClearSelected();
        }

        bool render_execute = false;
        _render_window->draw(render_execute);
        if (render_execute) {
            _execute_graph(ExecutionType::Execution);
        }
    }

    void MainUI::clear() {
        _node_windows.clear();
        _render_window = nullptr;
        _timeline = nullptr;
        _bin = nullptr;
        _execute_graph_flag = false;
        _vulkan_window_open = false;

        _preferences.last_opened_project() = "";

    	_timeline = std::make_shared<Timeline>();
        _render_window = std::make_unique<RenderWindow>(*_timeline, _performance);
        _bin = std::make_unique<Bin>();
    }

    void MainUI::load(std::string path) {
        try {
            {
                std::ifstream os(path, std::ios::binary);
                cereal::BinaryInputArchive archive(os);

                archive(*this);
            }
            _preferences.last_opened_project() = path;
        } catch (...) {
            std::cerr << "failed to save project" << std::endl;
        }
        
        save_preferences();
    }
    
    void MainUI::save(std::string path) {
        try {
            ghc::filesystem::path p{path};
            if (p.extension() != ".bin") {
                p += ".bin";
            }
            {
                std::ofstream os(p.generic_string(), std::ios::binary);
                cereal::BinaryOutputArchive archive(os);

                archive(*this);
            }
            
            _preferences.last_opened_project() = path;
        } catch (...) {
            std::cerr << "failed to save project" << std::endl;
        }

        save_preferences();
    }

    void MainUI::load_preferences() {
        try {
            if (ghc::filesystem::exists(Preferences::Path())) {
                std::ifstream os(Preferences::Path(), std::ios::in);
                cereal::JSONInputArchive archive(os);

                archive(_preferences);
            }
        } catch (...) {
            std::cerr << "failed to load preferences" << std::endl;
        }
    }

    void MainUI::save_preferences() const {
        try {
            std::ofstream os(Preferences::Path(), std::ios::out);
            cereal::JSONOutputArchive archive(os);

            archive(_preferences);
        } catch (...) {
            std::cerr << "failed to save preferences" << std::endl;
        }
    }

    void MainUI::add_node_graph() {
        Bin::Entry entry;
        entry.path = "";
        add_node_graph(entry);
    }

    void MainUI::add_node_graph(const Bin::Entry& entry) {
        _node_windows.emplace_back(std::make_unique<NodeWindow>(_node_window_id));
        _node_window_id++;

        if (entry.path != "") {
            _node_windows.back()->initial_input(entry);
            
            auto graph_builder = std::make_unique<vkd::GraphBuilder>();
            
            for (auto&& node : _node_windows) {
                node->build_nodes(*graph_builder);
            }

            auto graph = graph_builder->bake(_device);
            if (graph) {
                _node_windows.back()->sequencer_size_to_loaded_content();
            }
        }

        _timeline->add_line(_node_windows.back()->sequencer_line());
    }

    void MainUI::commands(VkCommandBuffer buf, uint32_t width, uint32_t height) {
        if (_graph != nullptr) {
            _graph->commands(buf, width, height);
        }

        for (auto&& draw : _draws) {
            if (draw && draw->range_contains(_timeline->current_frame())) {
                draw->commands(buf, width, height);
            }
        }
    }

    std::vector<VkSemaphore> MainUI::semaphores() {
        std::lock_guard<std::mutex> lock(_submitted_semaphore_mutex);
        auto ret = std::move(_submitted_semaphores);
        _submitted_semaphores.clear();
        return ret;
    }

    void MainUI::update() {
        if (_graph != nullptr) {
            if (!_render_window->rendering()) {
                if (_timeline->play()) {
                    _timeline->increment();
                }

                _graph->set_frame(_timeline->current_frame());
                _execute_graph_flag = _graph->update(ExecutionType::UI) || _timeline->play();
            }
        }
    }

    void MainUI::execute() {
        if (_graph) {
            _render_window->execute(*_graph);
        }

        if (_graph != nullptr && _execute_graph_flag) {
            auto before = std::chrono::high_resolution_clock::now();
            _graph->execute(ExecutionType::UI);
            auto after = std::chrono::high_resolution_clock::now();

            auto diff = std::chrono::duration_cast<std::chrono::microseconds>(after - before).count();

            std::lock_guard<std::mutex> lock(_submitted_semaphore_mutex);

            auto sems =  _graph->semaphores();
            _submitted_semaphores.insert(_submitted_semaphores.end(), sems.begin(), sems.end());

            _execute_graph_flag = false;

            std::string report = "Graph (" + std::to_string(_graph->graph().size()) + ")";

            std::string extra = std::to_string(sems.size()) + " semaphores";
            _performance.add(report, diff, extra);
        }

        for (auto&& draw : _draws) {
            if (draw && draw->range_contains(_timeline->current_frame())) {
                draw->execute(ExecutionType::UI, VK_NULL_HANDLE, VK_NULL_HANDLE);
            }
        }
    }

    void MainUI::_rebuild_draws() {
        auto&& terms = _graph->terminals();
        _draws.resize(terms.size());
        
        int i = 0;
        for (auto&& term : terms) {
            auto cast = std::dynamic_pointer_cast<vkd::ImageNode>(term);
            if (cast != nullptr) {
                _draws[i] = std::make_shared<vkd::DrawFullscreen>();
                _draws[i]->graph_inputs({term});
                vkd::engine_node_init(_draws[i], "____draw");
                _draws[i]->init();
                _draws[i]->set_range(term->range());
                ++i;
            } else {
                _draws[i] = nullptr;
            }
        }
    }

    void MainUI::_execute_graph(ExecutionType type) {
        auto before = std::chrono::high_resolution_clock::now();

        std::deque<int32_t> node_queue;

        auto graph_builder = std::make_unique<vkd::GraphBuilder>();
        if (_graph) { _graph->flush(); }
        _graph = nullptr;
        
        for (auto&& node : _node_windows) {
            node->build_nodes(*graph_builder);
        }

        if (type == ExecutionType::Execution) {
            auto terms = graph_builder->unbaked_terminals();

            _render_window->attach_renderer(*graph_builder.get(), terms);
        }

        _graph = graph_builder->bake(_device);

        if (_graph && type == ExecutionType::UI) {
            _rebuild_draws();
            _execute_graph_flag = true;
        }

        auto after = std::chrono::high_resolution_clock::now();

        auto diff = std::chrono::duration_cast<std::chrono::microseconds>(after - before).count();

        std::string report = _graph ? "Build" : "Build (failed)";

        _performance.add(report, diff, "");
    }
}