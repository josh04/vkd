#pragma once
        
#include <memory>
#include <map>
#include <optional>

#include "bin.hpp"
#include "photo_browser.hpp"
#include "node_window.hpp"
#include "timeline.hpp"
#include "performance.hpp"
#include "preferences.hpp"
#include "render_window.hpp"
#include "memory_window.hpp"
#include "console_window.hpp"
#include "inspector.hpp"

#include "TaskScheduler.h"

namespace vkd {
    class Graph;
    class DrawFullscreen;
    class Stream;
    class MainUI {
    public:
        MainUI();
        ~MainUI();
        MainUI(MainUI&&) = delete;
        MainUI(const MainUI&) = delete;

        void set_device(const std::shared_ptr<Device>& device) { _device = device; }

        void draw();

        void add_node_graph();
        void add_node_graph(const Bin::Entry& entry);
        void add_node_graph_if_not_open(const Bin::Entry& entry);

        void commands(VkCommandBuffer buf, uint32_t width, uint32_t height);
        void update();
        void execute();

        void load(std::string path);
        void save(std::string path);
        void clear();

        bool has_quit() const { return _has_quit; }

        void load_preferences();
        void save_preferences() const;

        bool& vulkan_window_open() { return _vulkan_window_open; }
        
        auto& stream() { return *_stream; }
        
        template<class Archive>
        void serialize(Archive& archive, const uint32_t version) {
            if (version >= 0) {
                bool execute_graph_flag = false;
                archive(_bin, _node_windows, _timeline, execute_graph_flag, _performance, _vulkan_window_open, _node_window_id);
            }
            if (version >= 1) {
                bool render_window = _render_window->open();
                archive(render_window, _memory_window);
                _render_window->open(render_window);
            }
            if (version >= 2) {
                archive(_inspector);
            }
            if (version >= 3) {
                archive(_photo_browser);
            }
            if (version >= 4) {
                archive(_console_window);
            }
        }

    private:
        void _rebuild_draws();
        void _execute_graph(ExecutionType type);

        std::shared_ptr<Device> _device = nullptr;
        std::shared_ptr<Stream> _stream = nullptr;

        std::unique_ptr<Bin> _bin = nullptr;
        std::unique_ptr<PhotoBrowser> _photo_browser = nullptr;
        std::vector<std::unique_ptr<NodeWindow>> _node_windows;
        std::shared_ptr<Timeline> _timeline = nullptr;
        
        std::unique_ptr<vkd::Graph> _graph = nullptr;
        std::shared_ptr<vkd::DrawFullscreen> _viewer_draw = nullptr;
        std::shared_ptr<vkd::DrawFullscreen> _previous_viewer_draw = nullptr;
        int64_t _viewer_count = 0;
        int32_t _viewer_selection = 0;

        std::vector<std::shared_ptr<EngineNode>> _live_nodes;

        Performance _performance;

        ImGui::FileBrowser _load_dialog;
        ImGui::FileBrowser _save_dialog;
        ImGui::FileBrowser _export_dialog;

        Preferences _preferences;

        std::unique_ptr<RenderWindow> _render_window;
        std::unique_ptr<MemoryWindow> _memory_window;
        std::unique_ptr<ConsoleWindow> _console_window;
        std::unique_ptr<Inspector> _inspector;

        bool _vulkan_window_open = false;
        bool _has_quit = false;

        int32_t _node_window_id = 1;

        std::optional<ExecutionType> _execution_to_run;
        std::optional<std::string> _loaded_path;

        std::string _current_loaded = "untitled";
        enki::TaskSet * _export_task = nullptr;
        std::string _export_name = "";

        std::deque<std::pair<std::string, std::string>> _popups;

        bool _trigger_renderdoc = false;
        
        std::chrono::high_resolution_clock::time_point _last_saved_prefs;
    };
}