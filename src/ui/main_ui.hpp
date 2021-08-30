#pragma once
        
#include <memory>
#include <map>

#include "bin.hpp"
#include "node_window.hpp"
#include "timeline.hpp"
#include "performance.hpp"
#include "preferences.hpp"
#include "render_window.hpp"

namespace vkd {
    class Graph;
    class DrawFullscreen;
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

        void commands(VkCommandBuffer buf, uint32_t width, uint32_t height);
        std::vector<VkSemaphore> semaphores();
        void update();
        void execute();

        void load(std::string path);
        void save(std::string path);
        void clear();

        bool has_quit() const { return _has_quit; }

        void load_preferences();
        void save_preferences() const;

        bool& vulkan_window_open() { return _vulkan_window_open; }
        
        template<class Archive>
        void serialize(Archive& archive, const uint32_t version) {
            if (version == 0) {
                archive(_bin, _node_windows, _timeline, _execute_graph_flag, _performance, _vulkan_window_open, _node_window_id);
            }
        }
    private:
        void _rebuild_draws();
        void _execute_graph(ExecutionType type);

        std::shared_ptr<Device> _device = nullptr;

        std::unique_ptr<Bin> _bin = nullptr;
        std::vector<std::unique_ptr<NodeWindow>> _node_windows;
        std::shared_ptr<Timeline> _timeline = nullptr;
        
        std::unique_ptr<vkd::Graph> _graph = nullptr;
        std::vector<std::shared_ptr<vkd::EngineNode>> _draws;
        bool _execute_graph_flag = false;

        std::mutex _submitted_semaphore_mutex;
        std::vector<VkSemaphore> _submitted_semaphores;

        Performance _performance;

        ImGui::FileBrowser _load_dialog;
        ImGui::FileBrowser _save_dialog;

        Preferences _preferences;

        std::unique_ptr<RenderWindow> _render_window;

        bool _vulkan_window_open = false;
        bool _has_quit = false;

        int32_t _node_window_id = 1;
    };
}