#include <cereal/types/polymorphic.hpp>
#include "cereal/types/string.hpp"
#include "cereal/archives/binary.hpp"
#include "cereal/archives/json.hpp"

#include "main_ui.hpp"
#include "application.hpp"
#include "graph/fake_node.hpp"
#include "graph/graph.hpp"
#include "outputs/immediate_exr.hpp"
#include "renderdoc_integration.hpp"

#include "ghc/filesystem.hpp"

#include "imgui/imgui_internal.h"

#include "ocio/ocio_static.hpp"
#include "ocio/ocio_functional.hpp"

#include "host_scheduler.hpp"
#include "image.hpp"
#include "services/graph_requests.hpp"


CEREAL_CLASS_VERSION(vkd::MainUI, 4);

namespace vkd {
    MainUI::MainUI() : _save_dialog(ImGuiFileBrowserFlags_EnterNewFilename), _export_dialog(ImGuiFileBrowserFlags_EnterNewFilename) {
    	_timeline = std::make_shared<Timeline>();
        _render_window = std::make_unique<RenderWindow>(*_timeline, _performance);
        _console_window = std::make_unique<ConsoleWindow>();
        _memory_window = std::make_unique<MemoryWindow>();
        _inspector = std::make_unique<Inspector>();
        _bin = std::make_unique<Bin>();
        _photo_browser = std::make_unique<PhotoBrowser>();

        _load_dialog.SetTitle("load graph");
        _load_dialog.SetTypeFilters({ ".bin" });
        _save_dialog.SetTitle("save graph");
        _save_dialog.SetTypeFilters({ ".bin" });
        _export_dialog.SetTitle("export image");
        _export_dialog.SetTypeFilters({ ".exr", ".png", ".jpg" });

        load_preferences();

        if (Mode() == ApplicationMode::Standard) {
            if (!_preferences.last_opened_project().empty()) {
                load(_preferences.last_opened_project());
            }
        }

        if (Mode() == ApplicationMode::Photo) {
            if (fs::exists(_preferences.photo_project())) {
                load(_preferences.photo_project());
            }
        }

        _last_saved_prefs = std::chrono::high_resolution_clock::now();
    }

    MainUI::~MainUI() = default;

    void MainUI::draw() {

        auto start = std::chrono::high_resolution_clock::now();

        if (start - _last_saved_prefs > std::chrono::seconds(5)) {
            save_preferences();
            _last_saved_prefs = start;
        }


        // must run first so no naughty node windows can enqueue things including the existing graph
        if (_execution_to_run) {
            _execute_graph(*_execution_to_run);
            _execution_to_run = {};
        }

        if (ImGui::BeginMainMenuBar()) {
            if (ImGui::BeginMenu("File")) {
                if(ImGui::MenuItem("New")) {
                    clear();
                }
                ImGui::Separator();
                if(ImGui::MenuItem("Load")) {
                    _load_dialog.Open();
                }
                if (ImGui::BeginMenu("Recent")) {
                    auto&& recent = _preferences.recently_opened();
                    for (auto&& r : recent) {
                        if(ImGui::MenuItem(r.c_str())) {
                            load(r);
                        }
                    }
                    ImGui::EndMenu();
                }
                ImGui::Separator();
                if (Mode() == ApplicationMode::Standard) {
                    if (_loaded_path && ImGui::MenuItem("Save")) {
                        save(*_loaded_path);
                    }

                    if (ImGui::MenuItem("Save As")) {
                        _save_dialog.Open();
                    }
                }

                if (Mode() == ApplicationMode::Photo) {  
                    if (ImGui::MenuItem("Save")) {                      
                        save(_preferences.photo_project());
                    }
                }
                ImGui::Separator();
                if(ImGui::MenuItem("Preferences")) {
                    _preferences.open(true);
                }
                ImGui::Separator();
                if(ImGui::MenuItem("Quit")) {
                    _has_quit = true;
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Window")) {
                if(ImGui::MenuItem("Console")) {
                    _console_window->open(true);
                }
                if(ImGui::MenuItem("Render")) {
                    _render_window->open(true);
                }
                if(ImGui::MenuItem("Memory")) {
                    _memory_window->open(true);
                }
                if(ImGui::MenuItem("Performance")) {
                    _performance.open(true);
                }
                if(ImGui::MenuItem("Inspector")) {
                    _inspector->open(true);
                }
                if(ImGui::MenuItem("Vulkan")) {
                    _vulkan_window_open = true;
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Image")) {
                if(ImGui::MenuItem("Render")) {
                    _export_dialog.Open();
                }

                if (renderdoc_enabled()) {
                    ImGui::Separator();
                    ImGui::PushItemFlag(ImGuiItemFlags_SelectableDontClosePopup, true);
                    ImGui::MenuItem("Trigger RenderDoc", NULL, &_trigger_renderdoc);
                    ImGui::PopItemFlag();
                }

                ImGui::EndMenu();
            }

            ImGui::EndMainMenuBar();
        }

        bool graph_changed = false;

        if (Mode() == ApplicationMode::Standard) {
            _timeline->draw(*this, graph_changed);
            _bin->draw(*this);
        }

        
        if (Mode() == ApplicationMode::Photo) {
            _photo_browser->draw(*this);
        }

        {
            _performance.draw();
        }

        if (_graph != nullptr) {
            _graph->ui();
        }

        if ( !_render_window->rendering()) {
            for (auto&& node : _node_windows) {
                node->draw(graph_changed, *_inspector);
            }
            // should grey out the node graphs, really
            if (graph_changed) {
                _execution_to_run = std::optional<ExecutionType>{ExecutionType::UI};
                if (_trigger_renderdoc) {
                    renderdoc_capture();
                }
            }
        }
	    //node_window->draw();

        _load_dialog.Display();
        _save_dialog.Display();
        _export_dialog.Display();
        
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

        if (_export_dialog.HasSelected()) {
            auto file = ghc::filesystem::path(_export_dialog.GetSelected().string());
            ImmediateFormat format = ImmediateFormat::EXR;
            if (file.extension() == ".exr") {
                format = ImmediateFormat::EXR;
            } else if (file.extension() == ".jpg") {
                format = ImmediateFormat::JPG;
            } else if (file.extension() == ".png") {
                format = ImmediateFormat::PNG;
            }
            if (_viewer_draw && _viewer_draw->get_image() && _viewer_draw->get_image()->allocated()) {
                _export_name = immediate_exr(_device, file.string(), format, _viewer_draw->get_image(), _export_task);
            }
            _export_dialog.ClearSelected();
        }

        if (_export_task && ts().is_complete(_export_task)) {
            _export_task = nullptr;
            ImGui::OpenPopup("export");
        }

        if (ImGui::BeginPopup("export"))
        {
            auto str = std::string("image exported as ") + _export_name;
            ImGui::Text(str.c_str());
            ImGui::EndPopup();
        }

        bool render_execute = false;
        _render_window->draw(render_execute);
        if (render_execute) {
            _execution_to_run = std::optional<ExecutionType>{ExecutionType::Execution};
        }

        _console_window->draw();
        _preferences.draw();
        _memory_window->draw(*_device);
        if (_viewer_draw && _viewer_draw->input_node() && _viewer_draw->input_node()->get_output_image()) {
            _inspector->draw(*_device, *_viewer_draw->get_image(), _viewer_draw->offset_w_h());
        }
    }

    void MainUI::clear() {
        _loaded_path = std::nullopt;
        _node_windows.clear();
        _render_window = nullptr;
        _timeline = nullptr;
        _bin = nullptr;
        _photo_browser = nullptr;
        _vulkan_window_open = false;

        _preferences.last_opened_project() = "";

    	_timeline = std::make_shared<Timeline>();
        _render_window = std::make_unique<RenderWindow>(*_timeline, _performance);
        _bin = std::make_unique<Bin>();
        _photo_browser = std::make_unique<PhotoBrowser>();
        
        _current_loaded = "untitled";

        _viewer_draw = nullptr;
    }

    void MainUI::load(std::string path) {
        _graph = nullptr;
        try {
            {
                std::ifstream os(path, std::ios::binary);
                cereal::BinaryInputArchive archive(os);

                archive(*this);
                _execution_to_run = std::optional<ExecutionType>{ExecutionType::UI};
            }
            _preferences.last_opened_project() = path;
            _preferences.add_recently_opened(path);
            _loaded_path = path;
            ghc::filesystem::path pr(path);
            _current_loaded = pr.filename().string();
        } catch (...) {
            console << "failed to load project" << std::endl;
        }
        console << "loaded project at " << path << std::endl;
        
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
            
            _preferences.last_opened_project() = p.string();
            _preferences.add_recently_opened(path);
            _loaded_path = p.string();
            ghc::filesystem::path pr(path);
            _current_loaded = pr.filename().string();
        } catch (...) {
            console << "failed to save project" << std::endl;
        }
        console << "saved project as " << path << std::endl;

        save_preferences();
    }

    void MainUI::load_preferences() {
        try {
            if (ghc::filesystem::exists(Preferences::Path())) {
                std::ifstream os(Preferences::Path(), std::ios::in);
                cereal::JSONInputArchive archive(os);

                archive(_preferences);
                _preferences.loaded();
            }
        } catch (...) {
            console << "failed to load preferences" << std::endl;
        }
    }

    void MainUI::save_preferences() const {
        try {
            std::ofstream os(Preferences::Path(), std::ios::out);
            cereal::JSONOutputArchive archive(os);

            archive(_preferences);
            _preferences.saved();
        } catch (...) {
            console << "failed to save preferences" << std::endl;
        }
    }

    void MainUI::add_node_graph() {
        Bin::Entry entry;
        entry.path = "";
        add_node_graph(entry);
    }

    void MainUI::add_node_graph_if_not_open(const Bin::Entry& entry) {
        for (auto&& window : _node_windows) {
            if (window->name_id() == entry.path) {
                window->focus();
                return;
            }
        }

        add_node_graph(entry);
    }

    void MainUI::add_node_graph(const Bin::Entry& entry) {
        _node_windows.emplace_back(std::make_unique<NodeWindow>(_node_window_id, entry.path));
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
            
            _execution_to_run = std::optional<ExecutionType>{ExecutionType::UI};
        }

        if (Mode() == ApplicationMode::Standard) {
            _timeline->add_line(_node_windows.back()->sequencer_line());
        }
    }

    void MainUI::commands(VkCommandBuffer buf, uint32_t width, uint32_t height) {
        if (_graph != nullptr) {
            _graph->commands(buf, width, height);
        }

        if (_viewer_draw && _viewer_draw->range_contains(_timeline->current_frame())) {
            _viewer_draw->commands(buf, width, height);
        }

    }

    void MainUI::update() {
        
        if (_stream == nullptr) {
            _stream = std::make_shared<Stream>(_device);
            _stream->init();
        }
        if (_graph != nullptr) {
            if (!_render_window->rendering()) {
                if (_timeline->play()) {
                    _timeline->increment();
                }

                _graph->set_frame(_timeline->current_frame());

                auto update = _graph->update(ExecutionType::UI, _stream);
                if (update == Graph::GraphUpdate::Rebake) {
                    _execution_to_run = std::optional<ExecutionType>{ExecutionType::UI};
                } else if (update == Graph::GraphUpdate::Updated || _timeline->play()) {
                    GraphRequests::Get().add_ui_run_with(_viewer_draw);
                }
            }
        }
        if (_viewer_draw) {
            _viewer_draw->update(ExecutionType::UI);
        }
    }

    void MainUI::execute() {
        if (_graph) { 
            _render_window->execute(*_graph, _stream);
        }

/*         if (_graph != nullptr && _execute_graph_flag) {
            auto before = std::chrono::high_resolution_clock::now();
            _graph->execute(ExecutionType::UI, _stream, {_viewer_draw});

            auto after = std::chrono::high_resolution_clock::now();
            auto diff = std::chrono::duration_cast<std::chrono::microseconds>(after - before).count();
            
            _execute_graph_flag = false;

            std::string report = "Graph (" + std::to_string(_graph->graph().size()) + ")";

            _performance.add(report, diff, "stream");
        } */

        if (_graph != nullptr) {
            std::optional<GraphRequests::Request> orequest = GraphRequests::Get().take();
            while (orequest.has_value()) {
                auto&& request = *orequest;
                int i = 0;
                for (auto&& node : request.extra_nodes) {
                    if (node) {
                        std::string name = std::string("____extra_") + std::to_string(i);
                        vkd::engine_node_init(node, name);
                        node->init();
                    }
                    ++i;
                }

                auto before = std::chrono::high_resolution_clock::now();
                _graph->execute(request.type, _stream, request.extra_nodes);

                auto after = std::chrono::high_resolution_clock::now();
                auto diff = std::chrono::duration_cast<std::chrono::microseconds>(after - before).count();
                
                std::string report = "R Graph (" + std::to_string(_graph->graph().size()) + ")";

                _performance.add(report, diff, "stream");

                _live_nodes.insert(_live_nodes.end(), request.extra_nodes.begin(), request.extra_nodes.end());

                orequest = GraphRequests::Get().take();
            }
        }

    }

    void MainUI::_rebuild_draws() {
        _previous_viewer_draw = _viewer_draw; // can't destroy this until the frame draw has happened which could be whenever
        if (!_graph || _graph->terminals().empty()) {
            _viewer_draw = nullptr;
            return;
        }

        auto&& terms = _graph->terminals();
        
        int sel = 0;
        if (_viewer_selection >= 0 && _viewer_selection < terms.size()) {
            sel = _viewer_selection;
        }

        auto&& term = terms[sel];

        auto cast = std::dynamic_pointer_cast<vkd::ImageNode>(term);
        if (cast != nullptr) {
            _viewer_draw = std::make_shared<vkd::DrawFullscreen>();
            _viewer_count++; // uh get this into the debug somehow
            _viewer_draw->graph_inputs({term});
            vkd::engine_node_init(_viewer_draw, "____draw");
            _viewer_draw->pre_init();
            _viewer_draw->set_range(term->range());

            std::static_pointer_cast<EngineNode>(_viewer_draw)->output_count(1);
        } else {
            _viewer_draw = nullptr;
        }
    }

    void MainUI::_execute_graph(ExecutionType type) {
        auto before = std::chrono::high_resolution_clock::now();

        std::deque<int32_t> node_queue;

        auto graph_builder = std::make_unique<vkd::GraphBuilder>();

        _stream->flush();
        _graph = nullptr;
        
        for (auto&& node : _node_windows) {
            node->build_nodes(*graph_builder);
        }

        if (type == ExecutionType::Execution) {
            auto terms = graph_builder->unbaked_terminals();

            _render_window->attach_renderer(*graph_builder.get(), terms);
        }

        _graph = graph_builder->bake(_device);
        _rebuild_draws();

        if (_graph && type == ExecutionType::UI && _viewer_draw) {
            GraphRequests::Get().add_ui_run_with(_viewer_draw);
        }

        auto after = std::chrono::high_resolution_clock::now();

        auto diff = std::chrono::duration_cast<std::chrono::microseconds>(after - before).count();

        std::string report = _graph ? "Build" : "Build (failed)";

        _performance.add(report, diff, "");
    }
}