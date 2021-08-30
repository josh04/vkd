#include <cereal/types/polymorphic.hpp>
#include "cereal/types/string.hpp"
#include "cereal/archives/binary.hpp"

#include "node_window.hpp"

#include "imgui/imgui.h"
#include "imgui/imnodes.h"

#include "vulkan.hpp"
#include "engine_node.hpp"
#include "shader.hpp"
#include "image.hpp"

#include "render/draw_triangle.hpp"
#include "render/draw_particles.hpp"
#include "render/draw_fullscreen.hpp"
#include "compute/particles.hpp"
#include "compute/sand.hpp"

#include "graph/graph.hpp"
#include "graph/fake_node.hpp"

#include "timeline.hpp"

#include <set>
#include <vector>
#include <algorithm>
#include <deque>

#include "cereal/cereal.hpp"

CEREAL_CLASS_VERSION(ImVec2, 0);
CEREAL_CLASS_VERSION(vkd::NodeWindow, 1);
CEREAL_CLASS_VERSION(vkd::NodeWindow::Node, 0);
CEREAL_CLASS_VERSION(vkd::NodeWindow::Link, 0);
CEREAL_CLASS_VERSION(vkd::SerialiseGraph, 0);

template<class Archive>
void serialize(Archive & archive, ImVec2& vec, const uint32_t version) {
    if (version == 0) {
        archive( vec.x, vec.y );
    }
}

namespace vkd {
    int32_t NodeWindow::_next_node = 0;
    int32_t NodeWindow::_next_pin = 0;
    int32_t NodeWindow::_next_link = 0;

    NodeWindow::NodeWindow(int32_t id) : NodeWindow() {
        _id = id;
        _window_name += std::to_string(id);
    }

    NodeWindow::NodeWindow() {
        imnodes::IO& io = imnodes::GetIO();
        io.link_detach_with_modifier_click.modifier = &_remove_link_mode;

        _display_node = _add_node("display");

        _file_dialog.SetTitle("ffmpeg picker");
        _file_dialog.SetTypeFilters({ ".mp4", ".mkv" });

        _sequencer_line = std::make_shared<SequencerLine>();

        _sequencer_line->name = "empty";
        
        _imnodes_context = imnodes::EditorContextCreate();
    }

    NodeWindow::~NodeWindow() {
        imnodes::EditorContextFree(_imnodes_context);
    }
    
    void NodeWindow::initial_input(const Bin::Entry& entry) {
        auto node = _add_node("ffmpeg");
        auto search = _nodes.find(node);
        auto new_link_s = *search->second.outputs.begin();

        auto disp = _nodes.find(_display_node);

        if (disp != _nodes.end()) {
            auto new_link_e = *disp->second.inputs.rbegin();
            _add_link(new_link_s, new_link_e);
        }

        ShaderParamMap& map = search->second.node->params();
        map["_"]["path"] = make_param<ParameterType::p_string>(search->second.name, "path", 0);
        map["_"]["path"]->as<std::string>().set(entry.path);
        
        _sequencer_line->name = entry.path;
    }

    void NodeWindow::sequencer_size_to_loaded_content() {
        int64_t total_frame_count = 100;
        for (auto&& node : _nodes) {
            if (node.second.name == "ffmpeg") {
                total_frame_count = std::max(total_frame_count, (int64_t)node.second.node->params().at("_").at("_total_frame_count")->as<int>().get());
            }
        }

        _sequencer_line->frame_count = total_frame_count;
    }

    void NodeWindow::draw(bool& updated) {
        ImGuiIO& io = ImGui::GetIO();
        
        ImGui::SetNextWindowPos(ImVec2(600, 25), ImGuiCond_Once);
        ImGui::SetNextWindowSize(ImVec2(1000, 700), ImGuiCond_Once);

        ImGui::PushID(_id);

        ImGui::Begin(_window_name.c_str());

        auto&& nodemap = vkd::EngineNode::node_type_map();

        std::set<std::string> blacklist = {"draw"};

        for (auto&& node : nodemap) {
            if (blacklist.find(node.second.name) != blacklist.end()) {
                continue;
            }
            ImGui::SameLine();
            if (ImGui::Button(node.second.display_name.c_str())) {
                _add_node(node.second.name.c_str());
            }
        }

        ImGui::BeginChild("nodes", {0.0f, -20.0f});

        imnodes::EditorContextSet(_imnodes_context);

        imnodes::BeginNodeEditor();
        imnodes::PushAttributeFlag(imnodes::AttributeFlags::AttributeFlags_EnableLinkDetachWithDragClick);
        for (auto&& node : _nodes) {

            if (node.second.node && node.second.node->get_state() == UINodeState::error) {
                imnodes::PushColorStyle(imnodes::ColorStyle_TitleBar, IM_COL32(122, 74, 41, 255));
                imnodes::PushColorStyle(imnodes::ColorStyle_TitleBarHovered, IM_COL32(250, 150, 66, 255));
                imnodes::PushColorStyle(imnodes::ColorStyle_TitleBarSelected, IM_COL32(250, 150, 66, 255));
            }

            imnodes::BeginNode(node.first);

            if (node.second.node && node.second.node->get_state() == UINodeState::error) {
                imnodes::PopColorStyle(3);
            }

            imnodes::BeginNodeTitleBar();
            ImGui::TextUnformatted(node.second.display_name.c_str());
            imnodes::EndNodeTitleBar();

            int i = 0;
            for (auto pin : node.second.inputs) {
                imnodes::BeginInputAttribute(pin);
                // in between Begin|EndAttribute calls, you can call ImGui
                // UI functions
                ImGui::Text("input %d", i);
                imnodes::EndInputAttribute();
                ++i;
            }
            i = 0;
            for (auto pin : node.second.outputs) {
                imnodes::BeginOutputAttribute(pin);
                // in between Begin|EndAttribute calls, you can call ImGui
                // UI functions
                ImGui::Text("output %d", i);
                imnodes::EndOutputAttribute();
                ++i;
            }

            ImGui::Dummy(ImVec2(80.0f, 45.0f));
            imnodes::EndNode();
        }

        for (auto&& link : _links) {
            imnodes::Link(link.first, link.second.start, link.second.end);
        }

        imnodes::PopAttributeFlag();
        // ---------------
        imnodes::EndNodeEditor();
        
        int32_t link = -1;
        if (imnodes::IsLinkDestroyed(&link)) {
            _remove_link(link);
            updated = true;
        }

        int new_link_s, new_link_e;
        if (imnodes::IsLinkCreated(&new_link_s, &new_link_e)) {
            _add_link(new_link_s, new_link_e);
            updated = true;
        }

        int32_t node_id = -1;
        if (imnodes::IsNodeHovered(&node_id) && io.MouseClicked[2] && node_id != _display_node) {
            auto&& search = _nodes.find(node_id);
            if (search != _nodes.end()) {
                for (auto&& link : search->second.links) {
                    _links.erase(link);
                }
                for (auto&& pin : search->second.inputs) {
                    _pin_to_node.erase(pin);
                }
                for (auto&& pin : search->second.outputs) {
                    if (_last_added_node_output == pin) {
                        _last_added_node_output = -1;
                    }
                    _pin_to_node.erase(pin);
                }
                _nodes.erase(search);
                updated = true;
            }

        }

        ImGui::EndChild();
        
        ImGui::End();

        int32_t hovered_node = -1;
        if (ImGui::IsMouseDoubleClicked(0) && imnodes::IsNodeHovered(&hovered_node)) {
            _open_node_windows.insert(hovered_node);
        }
        
        ImGui::PushStyleColor(ImGuiCol_WindowBg, IM_COL32(50, 50, 50, 255)); // stole this col from nodes
        ImGui::PushStyleColor(ImGuiCol_TitleBg, IM_COL32(41, 74, 122, 255)); // stole this col from nodes
        ImGui::PushStyleColor(ImGuiCol_TitleBgActive, IM_COL32(66, 150, 250, 255)); // stole this col from nodes

        decltype(_open_node_windows) close_windows;
        for (auto node_id : _open_node_windows) {
            
            auto search = _nodes.find(node_id);
            if (search != _nodes.end()) {
                auto&& node = search->second;

                auto fake_node = node.node;
                if (fake_node) {
                    auto&& param_map = fake_node->params();
                    if (param_map.size() > 0) {
                        bool open = true;

                        int32_t tot = 0;
                        for (auto&& map : param_map) {
                            tot += map.second.size();
                        }
                        ImGui::SetNextWindowPos(ImVec2(400, 40), ImGuiCond_Once);
                        ImGui::SetNextWindowSize(ImVec2(400, 20 + 30*tot), ImGuiCond_Once);
                        std::string win_name = node.name + "###" + node.name + std::to_string(_id);
                        ImGui::Begin(win_name.c_str(), &open);
                        if (!open) {
                            close_windows.insert(node_id);
                            //_open_node_windows.erase
                        }

//void ImGui::Image(ImTextureID user_texture_id, const ImVec2& size, const ImVec2& uv0, const ImVec2& uv1, const ImVec4& tint_col, const ImVec4& border_col)

                        if (fake_node->real_node()) {
                            auto ptr = std::dynamic_pointer_cast<ImageNode>(fake_node->real_node());
                            if (ptr && ptr->get_output_image()) {
                                ImVec2 uv_min = ImVec2(0.0f, 0.0f);                 // Top-left
                                ImVec2 uv_max = ImVec2(1.0f, 1.0f);                 // Lower-right
                                ImVec4 tint_col = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);   // No tint
                                ImVec4 border_col = ImVec4(1.0f, 1.0f, 1.0f, 0.5f); // 50% opaque white
                                ImGui::Image(ptr->get_output_image()->ui_desc_set(), {480, 270}, uv_min, uv_max, tint_col, border_col);
                            }
                        }
                        
                        for (auto&& map : param_map) {
                            for (auto&& param : map.second) {
                                if (param.first.substr(0, 1) != "_" && param.first.substr(0, 4) != "vkd_") {
                                    bool changed = _ui_for_param(param.second);
                                    if (changed && fake_node->get_state() == UINodeState::error) {
                                        updated = true;
                                    }
                                }
                            }
                        }

                        ImGui::End();
                    }
                }

            }
        }
        ImGui::PopStyleColor(3);

        for (auto&& e : close_windows) {
            _open_node_windows.erase(e);
        }

        _file_dialog.Display();
        
        if(_file_dialog.HasSelected()) {
            auto lock = _file_dialog_param.lock();
            if (lock) {
                lock->as<std::string>().set(_file_dialog.GetSelected().string());
                _file_dialog.ClearSelected();
                updated = true;
            }
        }
        
        ImGui::PopID();
    }

    void NodeWindow::_remove_link(int32_t link) {
        auto linke = _links.find(link);
        if (linke != _links.end()) {
            auto search_start = _pin_to_node.find(linke->second.start);
            if (search_start != _pin_to_node.end()) {
                auto start_node = _nodes.find(search_start->second);
                if (start_node != _nodes.end()) {
                    start_node->second.links.erase(std::remove(start_node->second.links.begin(), start_node->second.links.end(), link), start_node->second.links.end());
                }
            }
            auto search_end = _pin_to_node.find(linke->second.end);
            if (search_end != _pin_to_node.end()) {
                auto end_node = _nodes.find(search_end->second);
                if (end_node != _nodes.end()) {
                    end_node->second.links.erase(std::remove(end_node->second.links.begin(), end_node->second.links.end(), link), end_node->second.links.end());
                }
            }
        }

        _links.erase(link);
    }

    int32_t NodeWindow::_add_link(int32_t start, int32_t end) {
        _links.emplace(_next_link, Link{start, end});
        auto node_s = _pin_to_node.find(start);
        if (node_s != _pin_to_node.end()) {
            auto node = _nodes.find(node_s->second);
            if (node != _nodes.end()) {
                node->second.links.push_back(_next_link);
            }
        }
        auto node_e = _pin_to_node.find(end);
        if (node_e != _pin_to_node.end()) {
            auto node = _nodes.find(node_e->second);
            if (node != _nodes.end()) {
                node->second.links.push_back(_next_link);
                if (node->second.extendable_inputs) {
                    if (node->second.links.size() == node->second.inputs.size()) {
                        int32_t np = next_pin_();
                        node->second.inputs.insert(np);
                        _pin_to_node[np] = node->first;
                    }
                }
            }
        }
        auto ret = _next_link;
        _next_link++;
        return ret;
    }
/*
    template <class N>
    void NodeWindow::add_node_(Node& node) {
        node.inputs = _pins(N::input_count());
        node.outputs = _pins(N::output_count());
        node.node = vkd::make(node.name);
    }
*/
    int32_t NodeWindow::_add_node(std::string node_type) {
        Node node{};
        node.name = node_type;


        auto&& nodemap = vkd::EngineNode::node_type_map();
        auto search = nodemap.find(node_type);
        if (search != nodemap.end()) {
            node.extendable_inputs = false;
            node.name = search->second.name;
            node.display_name = search->second.display_name;
            if (search->second.inputs == -1) {
                node.inputs = _pins(1);
                node.extendable_inputs = true;
            } else {
                node.inputs = _pins(search->second.inputs);
            }
            node.outputs = _pins(search->second.outputs);
            node.node = std::make_shared<vkd::FakeNode>(_next_node, node.display_name, node_type);
        } else if (node_type == "display") {
            // NOP
            node.inputs = _pins(1);
            node.extendable_inputs = true;
        }/* else if (name == "read") {
            
        } else if (name == "write") {
            
        } else if (name == "particles") {
            add_node_<vkd::Particles>(node);
        } else if (name == "draw_particles") {
            add_node_<vkd::DrawParticles>(node);
        } else if (name == "triangles") {
            add_node_<vkd::DrawTriangle>(node);
        } else if (name == "history") {

        } else if (name == "sand") {
            add_node_<vkd::Sand>(node);
        } else if (name == "draw") {
            add_node_<vkd::DrawFullscreen>(node);
        }*/

        _nodes.emplace(_next_node, node);
        if (_nodes.size() == 1) {
            imnodes::SetNodeEditorSpacePos(_next_node, {500, 250});
        } else {
            imnodes::SetNodeEditorSpacePos(_next_node, _next_node_loc);
            _next_node_loc.x = ((int)(_next_node_loc.x + 110) % 500);
            _next_node_loc.y = ((int)(_next_node_loc.y + 20) % 500);
        }

        if (_last_added_node_output != -1 && node.inputs.size() > 0) {
            _add_link(_last_added_node_output, *node.inputs.begin());
        }

        if (node.outputs.size() > 0) {
            _last_added_node_output = *node.outputs.begin();
        } else {
            _last_added_node_output = -1;
        }

        int32_t ret = _next_node;
        _next_node++;

        return ret;
    }

    std::set<int32_t> NodeWindow::_pins(int32_t count) {
        std::set<int32_t> puts;
        for (int i = 0; i < count; ++i) {
            int32_t np = next_pin_();
            puts.insert(np);
            _pin_to_node[np] = _next_node;
        }
        return puts;
    }

    int32_t NodeWindow::next_pin_() {
        int32_t pin = _next_pin;
        _next_pin++;
        return pin;
    }

    void NodeWindow::build_nodes(GraphBuilder& graph_builder) {
        Frame frame_start_block = {_sequencer_line->blocks[0].start};
        Frame frame_end_block = {_sequencer_line->blocks[0].end};

        FrameRange range;
        range._frame_ranges.emplace(FrameInterval{frame_start_block, frame_end_block});

        std::deque<int32_t> node_queue;
        node_queue.push_back(_display_node);

        std::map<int32_t, std::shared_ptr<vkd::FakeNode>> node_map;
        std::vector<std::shared_ptr<vkd::FakeNode>> rnodes;
        std::map<int32_t, std::vector<int32_t>> input_map;

        while (node_queue.size() > 0) {
            auto ini = node_queue.front();
            node_queue.pop_front();

            auto&& node = _nodes.at(ini);

            if (node_map.find(ini) == node_map.end()) {
                auto ptr = node.node;
                if (ptr) {
                    rnodes.push_back(ptr);
                    node_map.emplace(ini, ptr);
                    node.node = ptr;
                    //ptr->init();
                    //std::cout << "inited " << ptr << std::endl;
                }

                for (auto&& link : node.links) {
                    auto searchl = _links.find(link);
                    if (searchl != _links.end()) {
                        auto search = node.inputs.find(searchl->second.end);
                        if (search != node.inputs.end()) {
                            auto pin_search = _pin_to_node.find(searchl->second.start);
                            if (pin_search != _pin_to_node.end()) {
                                node_queue.push_back(pin_search->second);
                                input_map[ini].push_back(pin_search->second);
                            }
                        }
                    }
                }
               
            }
        }

        auto fsb = make_param<ParameterType::p_frame>(_window_name + std::to_string(_id), "frame_start_block", 0);
        fsb->as<Frame>().set_force(frame_start_block);
        auto feb = make_param<ParameterType::p_frame>(_window_name + std::to_string(_id), "frame_end_block", 0);
        feb->as<Frame>().set_force(frame_end_block);

        for (auto&& node : node_map) {
            node.second->flush();
        }

        for (auto&& node : node_map) {
            auto&& inps = input_map[node.first];
            std::vector<std::shared_ptr<vkd::FakeNode>> inputs;

            for (auto&& inp : inps) {
                auto ptr = node_map.at(inp);
                inputs.push_back(ptr);
                node.second->add_input(ptr);
            }

            node.second->params()["_"]["frame_start_block"] = fsb;
            node.second->params()["_"]["frame_end_block"] = feb;

            node.second->set_range(range);

            graph_builder.add(node.second);
        }

        //_execute_graph = true;

    }

    bool NodeWindow::_ui_for_param(const std::shared_ptr<vkd::ParameterInterface>& paramPtr) {
        ImGui::PushID(paramPtr.get());
        auto&& param = *paramPtr;
        auto&& name = param.name();
        auto type = param.type();
        bool changed = false;
        using vkd::ParameterType;
        switch (type) {
        case ParameterType::p_float:
        {
            float p = param.as<float>().get();
            float min = param.as<float>().min();
            float max = param.as<float>().max();

            if (ImGui::SliderFloat(name.c_str(), &p, min, max)) {
                param.as<float>().set(p);
                changed = true;
            }
            break;
        }
        case ParameterType::p_int:
        {
            int p = param.as<int>().get();
            int min = param.as<int>().min();
            int max = param.as<int>().max();
            if (ImGui::SliderInt(name.c_str(), &p, min, max)) {
                param.as<int>().set(p);
                changed = true;
            }
            break;
        }
        case ParameterType::p_uint:
        {
            unsigned int p = param.as<unsigned int>().get();
            unsigned int min = param.as<unsigned int>().min();
            unsigned int max = param.as<unsigned int>().max();
            if (ImGui::SliderInt(name.c_str(), (int*)&p, min, max)) {
                param.as<unsigned int>().set(p);
                changed = true;
            }
            break;
        }
        case ParameterType::p_vec2:
        {
            glm::vec2 p = param.as<glm::vec2>().get();
            glm::vec2 min = param.as<glm::vec2>().min();
            glm::vec2 max = param.as<glm::vec2>().max();
            if (ImGui::SliderFloat2(name.c_str(), (float*)&p, min.x, max.x)) {
                param.as<glm::vec2>().set(p);
                changed = true;
            }
            break;
        }
        case ParameterType::p_vec4:
        {
            glm::vec4 p = param.as<glm::vec4>().get();
            if (ImGui::InputFloat4(name.c_str(), (float*)&p)) {
                param.as<glm::vec4>().set(p);
                changed = true;
            }
            break;
        }
        case ParameterType::p_ivec2:
        {
            glm::ivec2 p = param.as<glm::ivec2>().get();
            glm::ivec2 min = param.as<glm::ivec2>().min();
            glm::ivec2 max = param.as<glm::ivec2>().max();
            if (ImGui::SliderInt2(name.c_str(), (int*)&p, min.x, max.x)) {
                param.as<glm::ivec2>().set(p);
                changed = true;
            }
            break;
        }
        case ParameterType::p_ivec4:
        {
            glm::ivec4 p = param.as<glm::ivec4>().get();
            if (ImGui::InputInt4(name.c_str(), (int*)&p)) {
                param.as<glm::ivec4>().set(p);
                changed = true;
            }
            break;
        }
        case ParameterType::p_uvec2:
        {
            glm::uvec2 p = param.as<glm::uvec2>().get();
            glm::uvec2 min = param.as<glm::uvec2>().min();
            glm::uvec2 max = param.as<glm::uvec2>().max();
            if (ImGui::SliderInt2(name.c_str(), (int*)&p, min.x, max.x)) {
                param.as<glm::uvec2>().set(p);
                changed = true;
            }
            break;
        }
        case ParameterType::p_uvec4:
        {
            glm::uvec4 p = param.as<glm::uvec4>().get();
            if (ImGui::InputInt4(name.c_str(), (int*)&p)) {
                param.as<glm::uvec4>().set(p);
                changed = true;
            }
            break;
        }
        case ParameterType::p_string:
        {
            auto&& tags = param.tags();
            std::string p = param.as<std::string>().get();
            if (tags.find("filepath") != tags.end()) {
                ImGui::Text("path: %s", p.c_str());
                ImGui::SameLine();
                if (ImGui::Button("open")) {
                    _file_dialog.Open();
                }
                _file_dialog_param = paramPtr;
            } else {
                char pathc[1024];
                strncpy(pathc, p.c_str(), std::min((size_t)1024, p.size()+1));
                // InputText(const char* label, char* buf, size_t buf_size, ImGuiInputTextFlags flags = 0, ImGuiInputTextCallback callback = NULL, void* user_data = NULL);
                if (ImGui::InputText(param.name().c_str(), pathc, 1024)) {
                    p = pathc;
                    param.as<std::string>().set(p);
                    changed = true;
                }
            }
            break;
        }
        case ParameterType::p_frame:
        {
            vkd::Frame p = param.as<vkd::Frame>().get();
            ImGui::Text("%s: %llu", name.c_str(), p.index);
            break;
        }
        }
        ImGui::PopID();

        return changed;
    }

    template <class Archive>
    void NodeWindow::load(Archive& archive, const uint32_t version) {

        if (version != 0) {
            throw std::runtime_error("NodeWindow bad version");
        }

        std::map<std::string, vkd::ShaderParamMap> save_map;

        SerialiseGraph data;
        archive(data);

        _id = data._id;
        _window_name = data._window_name;

        _next_node = data._next_node;
        _next_pin = data._next_pin;
        _next_link = data._next_link;
        _nodes = data._nodes;

        _pin_to_node = data._pin_to_node;
        _links = data._links;
        _display_node = data._display_node;
        _remove_link_mode = data._remove_link_mode;

        _next_node_loc = data._next_node_loc;
        _last_added_node_output = data._last_added_node_output;
        _open_node_windows = data._open_node_windows;
        //save_map = save_map;

        for (auto&& node : _nodes) {
            _next_node = std::max(_next_node, node.first + 1);
            node.second.node = std::make_shared<vkd::FakeNode>(node.first, node.second.display_name, node.second.name);
            node.second.node->init();
        }
        for (auto&& link : _links) {
            _next_link = std::max(_next_link, link.first + 1);
        }
        for (auto&& pin : _pin_to_node) {
            _next_pin = std::max(_next_pin, pin.first + 1);
        }
        //_execute();
    }

    template <class Archive>
    void NodeWindow::save(Archive & archive, const uint32_t version) const {

        if (version != 0) {
            throw std::runtime_error("NodeWindow bad version");
        }
        
        std::map<std::string, vkd::ShaderParamMap> save_map;
        for (auto&& node : _nodes) {
            if (node.second.node) {
                save_map.emplace(node.second.name, node.second.node->params());
            }
        }

        SerialiseGraph data;

        data._id = _id;
        data._window_name = _window_name;

        data._next_node = _next_node;
        data._next_pin = _next_pin;
        data._next_link = _next_link;
        data._nodes = _nodes;

        data._pin_to_node = _pin_to_node;
        data._links = _links;
        data._display_node = _display_node;
        data._remove_link_mode = _remove_link_mode;

        data._next_node_loc = _next_node_loc;
        data._last_added_node_output = _last_added_node_output;
        data._open_node_windows = _open_node_windows;
        data.save_map = save_map;

        archive(data);
    }
    template void NodeWindow::load(cereal::BinaryInputArchive& archive, const uint32_t version);
    template void NodeWindow::save(cereal::BinaryOutputArchive& archive, const uint32_t version) const;
}