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

#include "inspector.hpp"
#include "host_scheduler.hpp"

#include <set>
#include <vector>
#include <algorithm>
#include <deque>
#include <optional>

#include "cereal/cereal.hpp"

#include "outputs/immediate_exr.hpp"

#include "imgui/ImGuiColorTextEdit/TextEditor.h"

#include "services/graph_requests.hpp"
#include "outputs/async_image_output.hpp"

CEREAL_CLASS_VERSION(ImVec2, 0);
CEREAL_CLASS_VERSION(vkd::NodeWindow, 0);
CEREAL_CLASS_VERSION(vkd::NodeWindow::Node, 1);
CEREAL_CLASS_VERSION(vkd::NodeWindow::Link, 0);
CEREAL_CLASS_VERSION(vkd::SerialiseGraph, 4);

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

    NodeWindow::NodeWindow(int32_t id, const std::string& name_id) : NodeWindow() {
        _id = id;
        if (name_id.size() > 0) {
            _window_name += "(" + name_id + ")";
        } else {
            _window_name += std::to_string(id);
        }
        _name_id = name_id;
    }

    NodeWindow::NodeWindow() {
        imnodes::IO& io = imnodes::GetIO();
        io.link_detach_with_modifier_click.modifier = &_remove_link_mode;

        _imnodes_context = imnodes::EditorContextCreate();

        imnodes::EditorContextSet(_imnodes_context);
        _display_node = _add_node("display");

        _file_dialog.SetTitle("ffmpeg picker");
        _file_dialog.SetTypeFilters({ ".mp4", ".mkv", ".exr", ".RAF" });

        _sequencer_line = std::make_shared<SequencerLine>();

        _sequencer_line->name = "empty";
        
    }

    NodeWindow::~NodeWindow() {
        imnodes::EditorContextFree(_imnodes_context);
    }
    
    void NodeWindow::initial_input(const Bin::Entry& entry) {
        fs::path path{entry.path};
        path = fs::absolute(path);

        std::string exts = path.extension().string();        
        std::transform(exts.begin(), exts.end(), exts.begin(), [](unsigned char c){ return std::tolower(c); });

        int32_t node = -1;
        if (exts == ".mp4" || exts == ".mkv" || exts == ".mov") {
            node = _add_node("ffmpeg");
        } else if (exts == ".exr") {
            node = _add_node("exr");
        } else if (exts == ".raf") {
            node = _add_node("raw");
        } else {
            console << "Unknown file type picked for initial read: " << exts << std::endl;
            return;
        }
        
        auto search = _nodes.find(node);
        auto new_link_s = *search->second.outputs.begin();

        auto disp = _nodes.find(_display_node);

        if (disp != _nodes.end()) {
            auto new_link_e = *disp->second.inputs.rbegin();
            _add_link(new_link_s, new_link_e);
        }

        ShaderParamMap& map = search->second.node->params();
        map["_"]["path"] = make_param<ParameterType::p_string>(search->second.type, "path", 0);
        map["_"]["path"]->as<std::string>().set(path.string());
        
        _sequencer_line->name = path.string();
    }

    void NodeWindow::sequencer_size_to_loaded_content() {
        int64_t total_frame_count = 100;
        for (auto&& node : _nodes) {
            if (node.second.type == "ffmpeg") {
                total_frame_count = std::max(total_frame_count, (int64_t)node.second.node->params().at("_").at("_total_frame_count")->as<int>().get());
            }
        }

        _sequencer_line->frame_count = total_frame_count;
    }

    void NodeWindow::draw(bool& updated, Inspector& insp) {
        ImGuiIO& io = ImGui::GetIO();
        

        ImGui::SetNextWindowPos(ImVec2(600, 25), ImGuiCond_Once);
        ImGui::SetNextWindowSize(ImVec2(1000, 700), ImGuiCond_Once);

        ImGui::PushID(_id);

        ImGui::Begin(_window_name.c_str());

        if (_focus) {
            _focus = false;
            ImGui::SetWindowFocus();
        }

        auto&& nodemap = vkd::EngineNode::node_type_map();

        std::set<std::string> blacklist = {"draw"};
        // x pos of end of window, for wrapping
        const float window_far_x = ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMax().x;
        float last_button_far_x = 0;

        ImGuiStyle& style = ImGui::GetStyle();
        bool first = true;
        for (auto&& node : nodemap) {
            if (blacklist.find(node.second.type) != blacklist.end()) {
                continue;
            }
            
            float exp_button_size_x = ImGui::CalcTextSize(node.second.display_name.c_str()).x +  2* style.FramePadding.x;

            float next_button_far_x = last_button_far_x + style.ItemSpacing.x + exp_button_size_x; // Expected position if next button was on same line

            if (next_button_far_x < window_far_x && !first) {
                ImGui::SameLine();
            }
            first = false;

            if (ImGui::Button(node.second.display_name.c_str())) {
                _add_node(node.second.type.c_str());
            }
            last_button_far_x = ImGui::GetItemRectMax().x;
        }

        ImGui::BeginChild("nodes", {0.0f, -20.0f});

        imnodes::EditorContextSet(_imnodes_context);

        imnodes::BeginNodeEditor();


        imnodes::PushAttributeFlag(imnodes::AttributeFlags::AttributeFlags_EnableLinkDetachWithDragClick);
        for (auto&& node : _nodes) {

            if (node.second.node && node.second.node->get_state() == UINodeState::error) {
                imnodes::PushColorStyle(imnodes::ColorStyle_TitleBar, IM_COL32(166, 0, 22, 255));
                imnodes::PushColorStyle(imnodes::ColorStyle_TitleBarHovered, IM_COL32(250, 150, 66, 255));
                imnodes::PushColorStyle(imnodes::ColorStyle_TitleBarSelected, IM_COL32(250, 150, 66, 255));
            } else if (node.second.node && node.second.node->get_state() == UINodeState::unconfigured) {
                imnodes::PushColorStyle(imnodes::ColorStyle_TitleBar, IM_COL32(166, 0, 22, 255));
                imnodes::PushColorStyle(imnodes::ColorStyle_TitleBarHovered, IM_COL32(250, 150, 66, 255));
                imnodes::PushColorStyle(imnodes::ColorStyle_TitleBarSelected, IM_COL32(250, 150, 66, 255));
            }

            imnodes::BeginNode(node.first);

            if (node.second.node && (node.second.node->get_state() == UINodeState::error 
            || node.second.node->get_state() == UINodeState::unconfigured)) {
                imnodes::PopColorStyle(3);
            }

            imnodes::BeginNodeTitleBar();
            auto titlestr = node.second.display_name;

            if (node.second.node && node.second.node->real_node() && node.second.node->real_node()->working()) {
                titlestr += " ";
                static int tick = 0;
                for (int i = 0; i < tick / 30; ++i) {
                    titlestr += ".";
                }
                tick = (tick + 1) % 120;
            }
            
            ImGui::TextUnformatted(titlestr.c_str());

            imnodes::EndNodeTitleBar();

            int i = 0;
            bool lastEmpty = false;
            for (auto pin : node.second.inputs) {
                if (node.second.extendable_inputs) {
                    auto search = node.second.inPinToLink.find(pin);
                    if (search == node.second.inPinToLink.end()) {
                        // either there are no links or we're one past the last connected link, either way don't show
                        if ((node.second.inPinToLink.empty() || node.second.inPinToLink.rbegin()->first < pin) && lastEmpty) {
                            ++i;
                            continue;
                        }
                        lastEmpty = true;
                    } else {
                        lastEmpty = false;
                    }
                }

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
            
            if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
            {
                if (node.second.node && node.second.node->get_state() == UINodeState::error) {
                    ImGui::SetTooltip("%s is in an error state", node.second.display_name.c_str());
                } else if (node.second.node && node.second.node->get_state() == UINodeState::unconfigured) {
                    ImGui::SetTooltip("%s requires further configuration", node.second.display_name.c_str());
                } else if (node.second.node && node.second.node->get_state() == UINodeState::normal) {
                    ImGui::SetTooltip("%s is okay", node.second.display_name.c_str());
                }
            }
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
                auto copy = search->second.linkToPin;
                for (auto&& link : copy) {
                    _remove_link(link.first);
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

        decltype(_open_node_windows) nodes_to_focus;
        int32_t hovered_node = -1;
        if (ImGui::IsMouseDoubleClicked(0) && imnodes::IsNodeHovered(&hovered_node)) {
            _open_node_windows.insert(hovered_node);
            nodes_to_focus.insert(hovered_node);
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
                        tot = 30*tot;
                        
                        auto real_image_node = std::dynamic_pointer_cast<ImageNode>(fake_node->real_node());
                        if (real_image_node && real_image_node->get_output_image()) {
                            tot += (290 + 30);
                        }

                        if (nodes_to_focus.find(node_id) != nodes_to_focus.end()) {
                            ImGui::SetNextWindowFocus();
                        }

                        ImGui::SetNextWindowPos(ImVec2(400, 40), ImGuiCond_Once);
                        ImGui::SetNextWindowSize(ImVec2(520, 20 + tot), ImGuiCond_Once);
                        std::string win_name = node.display_name + "###" + std::to_string(node_id) + " " + std::to_string(_id);
                        ImGui::Begin(win_name.c_str(), &open);
                        if (!open) {
                            close_windows.insert(node_id);
                            //_open_node_windows.erase
                        }

                        if (fake_node->get_state() == UINodeState::normal)
                        {
                            auto block_ = fake_node->real_node()->block_edit_params();
                            if (block_.has_value()) {
                                auto&& block = *block_;
                                ImGui::Text("frame count: %d", block.total_frame_count->as<int32_t>().get());
                                if (ImGui::Button("resize block")) {
                                    _sequencer_line->frame_count = block.total_frame_count->as<int32_t>().get();
                                    _sequencer_line->blocks[0].end = _sequencer_line->blocks[0].start + _sequencer_line->frame_count;
                                }
                                if (ImGui::Button("set block name")) {
                                    _sequencer_line->name = fake_node->node_name();
                                }
                            }
                        }

//void ImGui::Image(ImTextureID user_texture_id, const ImVec2& size, const ImVec2& uv0, const ImVec2& uv1, const ImVec4& tint_col, const ImVec4& border_col)

                        if (real_image_node && real_image_node->get_output_image()) {
                            ImVec2 uv_min = ImVec2(0.0f, 0.0f);                 // Top-left
                            ImVec2 uv_max = ImVec2(1.0f, 1.0f);                 // Lower-right
                            ImVec4 tint_col = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);   // No tint
                            ImVec4 border_col = ImVec4(1.0f, 1.0f, 1.0f, 0.5f); // 50% opaque white
                            //ImGui::Image(real_image_node->get_output_image()->ui_desc_set(), {480, 270}, uv_min, uv_max, tint_col, border_col);

                            std::optional<ImmediateFormat> format = std::nullopt;
                            if (ImGui::Button("exr")) {
                                format = {ImmediateFormat::EXR};
                            }
                            ImGui::SameLine();
                            if (ImGui::Button("png")) {
                                format = {ImmediateFormat::PNG};
                            }
                            ImGui::SameLine();
                            if (ImGui::Button("jpg")) {
                                format = {ImmediateFormat::JPG};
                            }

                            if (format.has_value()) {
                                // create im/dl node
                                // do these two
                                //_viewer_draw->graph_inputs({term});
                                //node->set_range(term->range());
                                // push into main ui requests
                                auto dl_node = std::make_shared<AsyncImageOutput>(*format);
                                dl_node->graph_inputs({fake_node->real_node()});
                                dl_node->set_range(fake_node->real_node()->range());
                                fake_node->real_node()->output_count(fake_node->real_node()->output_count() + 1);

                                GraphRequests::Request request;
                                request.type = ExecutionType::UI;
                                request.extra_nodes = {dl_node};

                                //auto str = immediate_exr(fake_node->real_node()->device(), fake_node->node_name(), *format, real_image_node->get_output_image(), task);
                                //fake_node->set_saved_as(str);
                                _save_tasks.emplace_back(std::make_pair(node_id, dl_node->get_future()));
                                GraphRequests::Get().add(request);
                            }

                            bool unfin = false;
                            int i = 0;
                            int erase = -1;
                            for (auto&& st : _save_tasks) {
                                if (st.first == node_id) {
                                    if (st.second.wait_for(std::chrono::microseconds(0)) == std::future_status::timeout) {
                                        unfin = true;
                                        break;
                                    } else {
                                        erase = i;
                                    }
                                }
                                ++i;
                            }
                            if (erase >= 0) {
                                _save_tasks.erase(_save_tasks.begin() + erase);
                            }

                            if (unfin) {
                                ImGui::SameLine();
                                ImGui::Text("saving...");
                            } else if (!fake_node->saved_as().empty()) {
                                ImGui::SameLine();
                                ImGui::Text("saved as %s", fake_node->saved_as().c_str());
                            }
                        }

                        auto search_editor = _open_text_editors.find(node_id);

                        if (search_editor == _open_text_editors.end()) {
                            _open_text_editors[node_id] = nullptr;
                        }
                        
                        for (auto&& map : param_map) {
                            std::vector<std::pair<int, vkd::ParameterInterface*>> params_to_draw;
                            
                            for (auto&& param : map.second) {
                                if (param.first.substr(0, 1) != "_" && param.first.substr(0, 4) != "vkd_") {
                                    params_to_draw.emplace_back(std::make_pair(param.second->order(), param.second.get()));
                                }
                            }

                            std::sort(params_to_draw.begin(), params_to_draw.end(), [](const auto& lhs, const auto& rhs) { return lhs.first < rhs.first; });

                            for (auto&& pair : params_to_draw) {
                                bool changed = _ui_for_param(*pair.second, insp, _open_text_editors[node_id]);
                                if (changed && fake_node->get_state() == UINodeState::error) {
                                    updated = true;
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
                    start_node->second.linkToPin.erase(link);
                    start_node->second.outPinToLinks[linke->second.start].erase(link);
                }
            }
            auto search_end = _pin_to_node.find(linke->second.end);
            if (search_end != _pin_to_node.end()) {
                auto end_node = _nodes.find(search_end->second);
                if (end_node != _nodes.end()) {
                    end_node->second.linkToPin.erase(link);
                    end_node->second.inPinToLink.erase(linke->second.end);
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
                node->second.linkToPin[_next_link] = start;
                node->second.outPinToLinks[start].insert(_next_link);
            }
        }
        auto node_e = _pin_to_node.find(end);
        if (node_e != _pin_to_node.end()) {
            auto node = _nodes.find(node_e->second);
            if (node != _nodes.end()) {
                auto currentLink = node->second.inPinToLink.find(end);
                if (currentLink != node->second.inPinToLink.end()) {
                    _remove_link(currentLink->second);
                }
                node->second.linkToPin[_next_link] = end;
                node->second.inPinToLink[end] = _next_link;
                if (node->second.extendable_inputs) {
                    if (node->second.inPinToLink.size() == node->second.inputs.size()) {
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
        node.node = vkd::make(node.type);
    }
*/
    int32_t NodeWindow::_add_node(std::string node_type) {
        Node node{};
        node.type = node_type;


        auto&& nodemap = vkd::EngineNode::node_type_map();
        auto search = nodemap.find(node_type);
        if (search != nodemap.end()) {
            node.extendable_inputs = false;
            node.type = search->second.type;
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

        bool added_link = false;
        int selected_node_count = imnodes::NumSelectedNodes(); 
        if (selected_node_count) { // node is selected in window
            std::vector<int> selected_nodes(selected_node_count);
            imnodes::GetSelectedNodes(selected_nodes.data());

            auto search = _nodes.find(selected_nodes[0]);
            if (search != _nodes.end()) {
                for (auto&& o : search->second.outputs) {
                    auto search_l = search->second.outPinToLinks.find(o);
                    if (search_l == search->second.outPinToLinks.end()) {
                        _add_link(o, *node.inputs.begin());
                        added_link = true;
                        break;
                    }
                }
            }
        }
        
        if (!added_link && _last_added_node_output != -1 && node.inputs.size() > 0) {
            _add_link(_last_added_node_output, *node.inputs.begin());
            added_link = true;
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
                    //console << "inited " << ptr << std::endl;
                }

                for (auto&& link : node.linkToPin) {
                    auto searchl = _links.find(link.first);
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

    bool NodeWindow::_ui_for_param(vkd::ParameterInterface& param, Inspector& insp, std::unique_ptr<TextEditor>& editor) {
        ImGui::PushID(&param);
        auto&& name = param.name();
        auto type = param.type();
        bool changed = false;
        using vkd::ParameterType;
        switch (type) {
        case ParameterType::p_float:
        {
            if (param.ui_changed_last_tick()) {
                param.ui_changed_last_tick() = false;
                break;
            }

            float p = param.as<float>().get();
            float min = param.as<float>().min();
            float max = param.as<float>().max();

            bool reset_button = ImGui::Button("r");
            ImGui::SameLine();

            if (ImGui::SliderFloat(name.c_str(), &p, min, max)) {
                param.as<float>().set(p);
                changed = true;
            }
            
            if ((ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0)) || reset_button)
            {
                param.as<float>().set(param.as<float>().get_default());
                param.ui_changed_last_tick() = true;
                changed = true;
            }
            
            break;
        }
        case ParameterType::p_int:
        {
            bool reset_button = ImGui::Button("r");
            ImGui::SameLine();
            if (reset_button) {
                param.as<int>().set(param.as<int>().get_default());
                param.ui_changed_last_tick() = true;
                changed = true;
            }

            int p = param.as<int>().get();
            auto&& tags = param.tags();

            if (tags.find("enum") != tags.end()) {
                auto&& enums = param.enum_names();
                auto&& values = param.enum_values();
                if (enums.size()) {
                    if (values.size()) {
                        int q = 0;
                        for (auto&& i : values) {
                            if (p == i) {
                                p = q;
                                break;
                            }
                            q++;
                        }
                    }
                    p = std::clamp(p, 0, (int)enums.size() - 1);

                    if (ImGui::BeginCombo(name.c_str(), enums[p].c_str())) {
                        for (int i = 0; i < enums.size(); ++i) {
                            auto&& name = enums[i];
                            bool is_selected = (p == i);
                            if (ImGui::Selectable(name.c_str(), is_selected)) {
                                if (values.size() > i) {
                                    param.as<int>().set_force(values[i]);
                                } else {
                                    param.as<int>().set(i);
                                }
                                changed = true;
                            }
                            if (is_selected) {
                                ImGui::SetItemDefaultFocus(); 
                            }
                        }
                        ImGui::EndCombo();
                    }
                }
            } else if (tags.find("raw") != tags.end()) {
                int min = param.as<int>().min();
                int max = param.as<int>().max();
                if (ImGui::InputInt(name.c_str(), &p, min, max)) {
                    param.as<int>().set(p);
                    changed = true;
                }
            } else {
                int min = param.as<int>().min();
                int max = param.as<int>().max();
                if (ImGui::SliderInt(name.c_str(), &p, min, max)) {
                    param.as<int>().set(p);
                    changed = true;
                }
            }
            break;
        }
        case ParameterType::p_uint:
        {
            bool reset_button = ImGui::Button("r");
            ImGui::SameLine();
            if (reset_button) {
                param.as<unsigned int>().set(param.as<unsigned int>().get_default());
                param.ui_changed_last_tick() = true;
                changed = true;
            }

            auto&& tags = param.tags();
            if (tags.find("raw") != tags.end()) {
                unsigned int p = param.as<unsigned int>().get();
                unsigned int min = param.as<unsigned int>().min();
                unsigned int max = param.as<unsigned int>().max();
                if (ImGui::InputInt(name.c_str(), (int*)&p, min, max)) {
                    param.as<unsigned int>().set(p);
                    changed = true;
                }
            } else {
                unsigned int p = param.as<unsigned int>().get();
                unsigned int min = param.as<unsigned int>().min();
                unsigned int max = param.as<unsigned int>().max();
                if (ImGui::SliderInt(name.c_str(), (int*)&p, min, max)) {
                    param.as<unsigned int>().set(p);
                    changed = true;
                }
            }
            break;
        }
        case ParameterType::p_bool:
        {
            bool p = param.as<bool>().get();
            auto&& tags = param.tags();
            if (tags.find("button") != tags.end()) {
                if (ImGui::Button(name.c_str())) {
                    param.as<bool>().set(true);
                    changed = true;
                }
            } else {
                if (ImGui::Checkbox(name.c_str(), (bool*)&p)) {
                    param.as<bool>().set(p);
                    changed = true;
                }
            }
            break;
        }
        case ParameterType::p_vec2:
        {
            glm::vec2 p = param.as<glm::vec2>().get();
            glm::vec2 min = param.as<glm::vec2>().min();
            glm::vec2 max = param.as<glm::vec2>().max();
            glm::vec2 def = param.as<glm::vec2>().get_default();

            bool reset_button = ImGui::Button("r");
            ImGui::SameLine();
            if (ImGui::SliderFloat2(name.c_str(), (float*)&p, min.x, max.x)) {
                param.as<glm::vec2>().set(p);
                param.ui_changed_last_tick() = true;
                changed = true;
            }

            if (reset_button) {
                param.as<glm::vec2>().set(def);
                param.ui_changed_last_tick() = true;
                changed = true;
            }
            break;
        }
        case ParameterType::p_vec4:
        {
            auto&& tags = param.tags();
            glm::vec4 p = param.as<glm::vec4>().get();
            bool vec3 = tags.find("vec3") != tags.end();
            if (tags.find("sliders") != tags.end()) {
                
                if (param.ui_changed_last_tick()) {
                    param.ui_changed_last_tick() = false;
                    break;
                }

                glm::vec4 min = param.as<glm::vec4>().min();
                glm::vec4 max = param.as<glm::vec4>().max();
                glm::vec4 def = param.as<glm::vec4>().get_default();

                ImGui::PushID(0);
                bool reset_button1 = ImGui::Button("r");
                ImGui::SameLine();
                if (ImGui::SliderFloat((name + ".x").c_str(), &p.x, min.x, max.x)) {
                    param.as<glm::vec4>().set(p);
                    changed = true;
                }
                ImGui::PopID();
                
                if ((ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0)) || reset_button1)
                {
                    p.x = def.x;
                    param.as<glm::vec4>().set(p);
                    param.ui_changed_last_tick() = true;
                    changed = true;
                }
                ImGui::PopID();

                ImGui::PushID(1);
                bool reset_button2 = ImGui::Button("r");
                ImGui::SameLine();
                if (ImGui::SliderFloat((name + ".y").c_str(), &p.y, min.y, max.y)) {
                    param.as<glm::vec4>().set(p);
                    changed = true;
                }
                
                if ((ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0)) || reset_button2)
                {
                    p.y = def.y;
                    param.as<glm::vec4>().set(p);
                    param.ui_changed_last_tick() = true;
                    changed = true;
                }
                ImGui::PopID();

                ImGui::PushID(2);
                bool reset_button3 = ImGui::Button("r");
                ImGui::SameLine();
                if (ImGui::SliderFloat((name + ".z").c_str(), &p.z, min.z, max.z)) {
                    param.as<glm::vec4>().set(p);
                    changed = true;
                }
                
                if ((ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0)) || reset_button3)
                {
                    p.z = def.z;
                    param.as<glm::vec4>().set(p);
                    param.ui_changed_last_tick() = true;
                    changed = true;
                }
                ImGui::PopID();

                if (!vec3) {
                    ImGui::PushID(3);
                    bool reset_button4 = ImGui::Button("r");
                    ImGui::SameLine();
                    if (ImGui::SliderFloat((name + ".w").c_str(), &p.w, min.w, max.w)) {
                        param.as<glm::vec4>().set(p);
                        changed = true;
                    }
                    
                    if ((ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0)) || reset_button4)
                    {
                        p.w = def.w;
                        param.as<glm::vec4>().set(p);
                        param.ui_changed_last_tick() = true;
                        changed = true;
                    }
                ImGui::PopID();
                }
            } else if (tags.find("label") != tags.end()) {
                if (tags.find("rgba") != tags.end()) {
                    ImGui::Text("%s r: %f g: %f b: %f a: %f", name.c_str(), p.x, p.y, p.z, p.w);
                } else {
                    ImGui::Text("%s x: %f y: %f z: %f w: %f", name.c_str(), p.x, p.y, p.z, p.w);
                }
            } else {

                bool reset_button = ImGui::Button("r");
                ImGui::SameLine();
                if (reset_button) {
                    param.as<glm::vec4>().set(param.as<glm::vec4>().get_default());
                    param.ui_changed_last_tick() = true;
                    changed = true;
                }

                if (!vec3) {
                    if (ImGui::InputFloat4(name.c_str(), (float*)&p)) {
                        param.as<glm::vec4>().set(p);
                        changed = true;
                    }
                } else {
                    if (ImGui::InputFloat3(name.c_str(), (float*)&p)) {
                        param.as<glm::vec4>().set(p);
                        changed = true;
                    }
                }
            }
            break;
        }
        case ParameterType::p_ivec2:
        {
            auto&& tags = param.tags();
            if (tags.find("picker") != tags.end()) {
                if (ImGui::Button(param.name().c_str())) {
                    insp.set_current_parameter(param.as_ptr<glm::ivec2>());
                }
            } else {

                bool reset_button = ImGui::Button("r");
                ImGui::SameLine();
                if (reset_button) {
                    param.as<glm::ivec2>().set(param.as<glm::ivec2>().get_default());
                    param.ui_changed_last_tick() = true;
                    changed = true;
                }

                glm::ivec2 p = param.as<glm::ivec2>().get();
                glm::ivec2 min = param.as<glm::ivec2>().min();
                glm::ivec2 max = param.as<glm::ivec2>().max();
                if (ImGui::SliderInt2(name.c_str(), (int*)&p, min.x, max.x)) {
                    param.as<glm::ivec2>().set(p);
                    changed = true;
                }
            }
            break;
        }
        case ParameterType::p_ivec4:
        {
            bool reset_button = ImGui::Button("r");
            ImGui::SameLine();
            if (reset_button) {
                param.as<glm::ivec4>().set(param.as<glm::ivec4>().get_default());
                param.ui_changed_last_tick() = true;
                changed = true;
            }

            glm::ivec4 p = param.as<glm::ivec4>().get();
            if (ImGui::InputInt4(name.c_str(), (int*)&p)) {
                param.as<glm::ivec4>().set(p);
                changed = true;
            }
            break;
        }
        case ParameterType::p_uvec2:
        {
            bool reset_button = ImGui::Button("r");
            ImGui::SameLine();
            if (reset_button) {
                param.as<glm::uvec2>().set(param.as<glm::uvec2>().get_default());
                param.ui_changed_last_tick() = true;
                changed = true;
            }

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
            bool reset_button = ImGui::Button("r");
            ImGui::SameLine();
            if (reset_button) {
                param.as<glm::uvec4>().set(param.as<glm::uvec4>().get_default());
                param.ui_changed_last_tick() = true;
                changed = true;
            }

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
                _file_dialog_param = param.shared_from_this();
            } else if (tags.find("multiline") != tags.end()) {
                ImGui::TextWrapped("%s", p.c_str());
            } else if (tags.find("code") != tags.end()) {
                if (editor == nullptr) {
                    editor = std::make_unique<TextEditor>();
                    // set highlight or whatever here
	                auto lang = TextEditor::LanguageDefinition::GLSL();
                    editor->SetLanguageDefinition(lang);
                    editor->SetText(p);
                }
                editor->Render("Custom Kernel", {0, -20});
                param.as<std::string>().set(editor->GetText());

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
        _name_id = data._name_id;

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

        bool rebuildLinks = false;

        for (auto&& node : _nodes) {
            _next_node = std::max(_next_node, node.first + 1);
            if (node.second.type != "display") {
                node.second.node = std::make_shared<vkd::FakeNode>(node.first, node.second.display_name, node.second.type);

                auto search = data.save_map.find(node.first);
                if (search != data.save_map.end()) {
                    node.second.node->set_params(search->second);
                }
            }
            rebuildLinks = rebuildLinks || node.second.rebuildLinks;
            node.second.node->init();
        }

        if (rebuildLinks) {
            for (auto&& link : _links) {
                auto findStart = _pin_to_node.find(link.second.start);
                auto findEnd = _pin_to_node.find(link.second.end);
                if (findStart != _pin_to_node.end()) {
                    auto findSNode = _nodes.find(findStart->second);
                    if (findSNode != _nodes.end()) {
                        findSNode->second.linkToPin[link.first] = link.second.start;
                        findSNode->second.outPinToLinks[link.second.start].insert(link.first);
                    }
                }
                if (findEnd != _pin_to_node.end()) {
                    auto findENode = _nodes.find(findEnd->second);
                    if (findENode != _nodes.end()) {
                        findENode->second.linkToPin[link.first] = link.second.end;
                        findENode->second.inPinToLink[link.second.end] = link.first;
                    }
                }
            }
            
            for (auto&& node : _nodes) {
                node.second.extendable_inputs = false;
                auto&& nodemap = vkd::EngineNode::node_type_map();
                auto search = nodemap.find(node.second.type);
                if (search != nodemap.end()) {
                    if (search->second.inputs == -1) {
                        node.second.extendable_inputs = true;
                    }
                } else if (node.second.type == "display") {
                    node.second.extendable_inputs = true;
                }
            }
        }

        imnodes::EditorContextSet(_imnodes_context);
        for (auto&& pos : data.node_positions) {
            imnodes::SetNodeGridSpacePos(pos.first, pos.second);
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
        
        SerialiseGraph data;

        for (auto&& node : _nodes) {
            if (node.second.node) {
                data.save_map.emplace(node.first, node.second.node->params());
            }
            data.node_positions.emplace(node.first, imnodes::GetNodeGridSpacePos(node.first));
        }


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

        archive(data);
    }
    template void NodeWindow::load(cereal::BinaryInputArchive& archive, const uint32_t version);
    template void NodeWindow::save(cereal::BinaryOutputArchive& archive, const uint32_t version) const;
}