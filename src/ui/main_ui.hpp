#pragma once
        
#include <memory>
#include <map>

#include "bin.hpp"
#include "nodes.hpp"
#include "timeline.hpp"
#include "performance.hpp"
#include "preferences.hpp"

namespace vkd {
    class Graph;
    class DrawFullscreen;
    class MainUI {
    public:
        MainUI();
        ~MainUI();
        MainUI(MainUI&&) = delete;
        MainUI(const MainUI&) = delete;

        void draw();

        void add_node_graph(const Bin::Entry& entry);

        void commands(VkCommandBuffer buf, uint32_t width, uint32_t height);
        std::vector<VkSemaphore> semaphores();
        void update();
        void execute();

        void load(std::string path);
        void save(std::string path);

        void load_preferences();
        void save_preferences() const;

    private:
        void _rebuild_draws();
        void _execute_graph();

        std::unique_ptr<Bin> _bin = nullptr;
        std::vector<std::unique_ptr<NodeWindow>> _node_windows;
        std::shared_ptr<Timeline> _timeline = nullptr;
        
        std::unique_ptr<vkd::Graph> _graph = nullptr;
        std::vector<std::shared_ptr<vkd::DrawFullscreen>> _draws;
        bool _execute_graph_flag = false;

        std::mutex _submitted_semaphore_mutex;
        std::vector<VkSemaphore> _submitted_semaphores;

        Performance _performance;

        ImGui::FileBrowser _load_dialog;
        ImGui::FileBrowser _save_dialog;

        Preferences _preferences;
    };
}