#include <cereal/types/polymorphic.hpp>
#include "cereal/types/string.hpp"
#include "cereal/archives/binary.hpp"
#include "cereal/archives/json.hpp"

#include "main_ui.hpp"
#include "graph/fake_node.hpp"
#include "graph/graph.hpp"

#include "ghc/filesystem.hpp"

namespace vkd {
    MainUI::MainUI() : _save_dialog(ImGuiFileBrowserFlags_EnterNewFilename) {
    	_timeline = std::make_shared<Timeline>();
        _bin = std::make_unique<Bin>();
    	//node_window = std::make_unique<vkd::NodeWindow>(timeline);

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
                if(ImGui::MenuItem("Load")) {
                    _load_dialog.Open();
                }
                if(ImGui::MenuItem("Save")) {
                    _save_dialog.Open();
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

        for (auto&& node : _node_windows) {
            node->draw(graph_changed);
        }
        if (graph_changed) {
            _execute_graph();
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
    }

    void MainUI::load(std::string path) {
        try {
            {
                std::ifstream os(path, std::ios::binary);
                cereal::BinaryInputArchive archive(os);

                archive(_bin, _node_windows, _timeline, _execute_graph_flag, _performance);
            }
            _preferences.last_opened_project() = path;
        } catch (...) {
            std::cerr << "failed to save preferences" << std::endl;
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

                archive(_bin, _node_windows, _timeline, _execute_graph_flag, _performance);
            }
            
            _preferences.last_opened_project() = path;
        } catch (...) {
            std::cerr << "failed to save preferences" << std::endl;
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

    void MainUI::add_node_graph(const Bin::Entry& entry) {
        _node_windows.emplace_back(std::make_unique<NodeWindow>());
        if (entry.path != "") {
            _node_windows.back()->add_input(entry);
        }

        _timeline->add_line(_node_windows.back()->sequencer_line());
    }

    void MainUI::commands(VkCommandBuffer buf, uint32_t width, uint32_t height) {
        if (_graph != nullptr) {
            _graph->commands(buf, width, height);
        }

        for (auto&& draw : _draws) {
            draw->commands(buf, width, height);
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
            if (_timeline->play()) {
                _timeline->increment();
            }

            _graph->set_param("frame", _timeline->current_frame());
            _execute_graph_flag = _graph->update() || _timeline->play();
        }
    }

    void MainUI::execute() {
        if (_graph != nullptr && _execute_graph_flag) {
            auto before = std::chrono::high_resolution_clock::now();
            _graph->execute();
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
            draw->execute(VK_NULL_HANDLE, VK_NULL_HANDLE);
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
                _draws[i]->inputs({term});
                vkd::engine_node_init(_draws[i], "____draw");
                _draws[i]->init();
                ++i;
            } else {
                _draws[i] = nullptr;
            }
        }
    }

    void MainUI::_execute_graph() {
        auto before = std::chrono::high_resolution_clock::now();

        std::deque<int32_t> node_queue;

        auto graph_builder = std::make_unique<vkd::GraphBuilder>();
        if (_graph) { _graph->flush(); }
        _graph = nullptr;
        
        for (auto&& node : _node_windows) {
            node->build_nodes(*graph_builder);
        }

        _graph = graph_builder->bake(_timeline->current_frame());
        _rebuild_draws();

        _execute_graph_flag = true;

        auto after = std::chrono::high_resolution_clock::now();

        auto diff = std::chrono::duration_cast<std::chrono::microseconds>(after - before).count();

        std::string report = "Build";

        _performance.add(report, diff, "");
    }
}