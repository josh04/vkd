#pragma once

#include <map>
#include <set>
#include <string>
#include <vector>
#include <memory>
#include <mutex>

#include "TaskScheduler.h"

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

namespace imnodes {
    struct EditorContext;
}

class TextEditor;

namespace vkd {
    class EngineNode;
    class ParameterInterface;
    class GraphBuilder;
    class FakeNode;
    class Graph;
    class DrawFullscreen;
    class Timeline;
    struct SequencerLine;
    class Inspector;
}

namespace vkd {
    class NodeWindow {
    public:
        NodeWindow();
        NodeWindow(int32_t id);
        ~NodeWindow();
        NodeWindow(NodeWindow&&) = delete;
        NodeWindow(const NodeWindow&) = delete;

        void initial_input(const Bin::Entry& entry);
        void sequencer_size_to_loaded_content();
        void draw(bool& updated, Inspector& insp);

        struct Node {
            std::string type;
            std::string display_name;
            using LinkID = int32_t;
            using PinID = int32_t;
            std::set<PinID> inputs;
            std::set<PinID> outputs;
            std::map<LinkID, PinID> linkToPin;
            std::map<PinID, LinkID> inPinToLink;
            std::map<PinID, std::set<LinkID>> outPinToLinks;

            bool extendable_inputs = false;

            std::shared_ptr<vkd::FakeNode> node;

            bool close = false;

            bool rebuildLinks = false;
            
            template <class Archive>
            void serialize(Archive& ar, const uint32_t version) {
                if (version >= 0) {
                    std::vector<LinkID> links_;
                    ar(type, display_name, inputs, outputs, links_, close);
                    if (links_.size()) {
                        rebuildLinks = true;
                    }
                }
                if (version >= 1) {
                    ar(linkToPin, inPinToLink, outPinToLinks, extendable_inputs);
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
        imnodes::EditorContext * _imnodes_context = nullptr;
        std::string _window_name = "node window ";

        int32_t _id = -1;

        //template <class N> void add_node_(Node& node);
        int32_t _add_node(std::string name);
        int32_t _add_link(int32_t start, int32_t end);
        void _remove_link(int32_t link);

        bool _ui_for_param(vkd::ParameterInterface& param, Inspector& insp, std::unique_ptr<TextEditor>& editor);

        std::set<int32_t> _pins(int32_t count);
        int32_t next_pin_();

        // begin saved elements
        static int32_t _next_node;
        static int32_t _next_pin;
        static int32_t _next_link;
        std::map<int32_t, Node> _nodes;
        std::map<int32_t, int32_t> _pin_to_node;
        std::map<int32_t, Link> _links;

        int32_t _display_node = -1;

        bool _remove_link_mode = true;

        ImVec2 _next_node_loc = {20, 20};

        int32_t _last_added_node_output = -1;

        std::set<int32_t> _open_node_windows;
        std::map<int32_t, std::unique_ptr<TextEditor>> _open_text_editors;
        // end saved elements

        ImGui::FileBrowser _file_dialog;
        std::weak_ptr<vkd::ParameterInterface> _file_dialog_param;

        std::shared_ptr<SequencerLine> _sequencer_line = nullptr;

        std::vector<std::pair<int32_t, std::unique_ptr<enki::TaskSet>>> _save_tasks;

    };

    struct SerialiseGraph {
        int _id;
        std::string _window_name;
        
        int32_t _next_node = 0;
        int32_t _next_pin = 0;
        int32_t _next_link = 0;
        std::map<int32_t, NodeWindow::Node> _nodes;
        std::map<int32_t, ImVec2> node_positions;
        std::map<int32_t, int32_t> _pin_to_node;
        std::map<int32_t, NodeWindow::Link> _links;

        int32_t _display_node = -1;

        bool _remove_link_mode = true;

        ImVec2 _next_node_loc = {20, 20};

        int32_t _last_added_node_output = -1;

        std::set<int32_t> _open_node_windows;

        std::map<int32_t, vkd::ShaderParamMap> save_map;

        template <class Archive>
        void serialize(Archive & ar, const uint32_t version)
        {
            std::map<std::string, vkd::ShaderParamMap> fake_save_map;
            if (version >= 0) {
                ar(_next_node, _next_pin, _next_link, _nodes, 
                    _pin_to_node, _links, _display_node, _remove_link_mode, 
                    _next_node_loc, _last_added_node_output, _open_node_windows,
                    fake_save_map);
            }
            if (version >= 1) {
                ar(_id, _window_name);
            }
            if (version >= 2) {
                ar(node_positions);
            }
            if (version >= 3) {
                ar(save_map);
            }
        }
    };
}