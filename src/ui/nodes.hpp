#pragma once

#include <map>
#include <set>
#include <string>
#include <vector>
#include <memory>
#include <mutex>

#include "imgui/imgui.h"
#include "imgui/imfilebrowser.h"

#include <cereal/types/map.hpp>
#include <cereal/types/set.hpp>
#include <cereal/types/memory.hpp>
#include <cereal/types/vector.hpp>
#include <cereal/archives/binary.hpp>

#include "engine_node.hpp"
#include "graph/graph.hpp"

#include "performance.hpp"

#include "render/draw_fullscreen.hpp"
#include "bin.hpp"

namespace vkd {
    class EngineNode;
    class ParameterInterface;
    class GraphBuilder;
    class FakeNode;
    class Graph;
    class DrawFullscreen;
    class Timeline;
    class SequencerLine;
}

namespace vkd {
    class NodeWindow {
    public:
        NodeWindow();
        ~NodeWindow();
        NodeWindow(NodeWindow&&) = delete;
        NodeWindow(const NodeWindow&) = delete;

        void add_input(const Bin::Entry& entry);
        void draw(bool& updated);

        struct Node {
            std::string name;
            std::string display_name;
            std::set<int32_t> inputs;
            std::set<int32_t> outputs;
            std::vector<int32_t> links;

            std::shared_ptr<vkd::FakeNode> node;

            bool close = false;
            
            template <class Archive>
            void serialize(Archive& ar, const uint32_t version) {
                if (version == 0) {
                    ar(name, display_name, inputs, outputs, 
                        links, close);
                }
            }
        };

        struct Link {
            int32_t start;
            int32_t end;

            template <class Archive>
            void serialize(Archive& ar, const uint32_t version) {
                if (version == 0) {
                    ar(start, end);
                }
            }
        };

        template <class Archive>
        void load(Archive& ar, const uint32_t version);
        template <class Archive>
        void save(Archive& ar, const uint32_t version) const;

        void build_nodes(GraphBuilder& graph_builder);

        const auto& sequencer_line() const { return _sequencer_line; }
    private:

        //template <class N> void add_node_(Node& node);
        int32_t _add_node(std::string name);
        int32_t _add_link(int32_t start, int32_t end);

        void _ui_for_param(const std::shared_ptr<vkd::ParameterInterface>& param);

        std::set<int32_t> _pins(int32_t count);
        int32_t next_pin_();

        // begin saved elements
        int32_t _next_node = 0;
        int32_t _next_pin = 0;
        int32_t _next_link = 0;
        std::map<int32_t, Node> _nodes;
        std::map<int32_t, int32_t> _pin_to_node;
        std::map<int32_t, Link> _links;

        int32_t _display_node = -1;

        bool _remove_link_mode = true;

        ImVec2 _next_node_loc = {20, 20};

        int32_t _last_added_node_output = -1;

        std::set<int32_t> _open_node_windows;
        // end saved elements

        ImGui::FileBrowser _file_dialog;
        std::weak_ptr<vkd::ParameterInterface> _file_dialog_param;

        std::shared_ptr<SequencerLine> _sequencer_line = nullptr;
    };

    struct SerialiseGraph {
        
        int32_t _next_node = 0;
        int32_t _next_pin = 0;
        int32_t _next_link = 0;
        std::map<int32_t, NodeWindow::Node> _nodes;
        std::map<int32_t, int32_t> _pin_to_node;
        std::map<int32_t, NodeWindow::Link> _links;

        int32_t _display_node = -1;

        bool _remove_link_mode = true;

        ImVec2 _next_node_loc = {20, 20};

        int32_t _last_added_node_output = -1;

        std::set<int32_t> _open_node_windows;

        std::map<std::string, vkd::ShaderParamMap> save_map;

        template <class Archive>
        void serialize(Archive & ar, const uint32_t version)
        {
            if (version == 0) {
                ar(_next_node, _next_pin, _next_link, _nodes, 
                    _pin_to_node, _links, _display_node, _remove_link_mode, 
                    _next_node_loc, _last_added_node_output, _open_node_windows,
                    save_map);
            }
        }
    };
}