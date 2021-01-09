#include <cereal/types/polymorphic.hpp>
#include "cereal/types/string.hpp"
#include "cereal/archives/binary.hpp"

#include "nodes.hpp"

#include "imgui/imgui.h"
#include "imgui/imnodes.h"

#include "vulkan.hpp"
#include "engine_node.hpp"
#include "shader.hpp"

#include "render/draw_triangle.hpp"
#include "render/draw_particles.hpp"
#include "render/draw_fullscreen.hpp"
#include "compute/particles.hpp"
#include "compute/sand.hpp"

#include <set>
#include <vector>
#include <algorithm>
#include <deque>

template<class Archive>
void serialize(Archive & archive, ImVec2& vec) {
    archive( vec.x, vec.y );
}

namespace app_ui {

    NodeWindow::NodeWindow() : _save_dialog(ImGuiFileBrowserFlags_EnterNewFilename) {
        imnodes::IO& io = imnodes::GetIO();
        io.link_detach_with_modifier_click.modifier = &_remove_link_mode;

        _display_node = _add_node("display");

        _file_dialog.SetTitle("ffmpeg picker");
        _file_dialog.SetTypeFilters({ ".mp4" });
        _save_dialog.SetTitle("save graph");
        _save_dialog.SetTypeFilters({ ".bin" });
        _load_dialog.SetTitle("load graph");
        _load_dialog.SetTypeFilters({ ".bin" });
    }
    
    void NodeWindow::draw() {
        ImGuiIO& io = ImGui::GetIO();
        
        ImGui::SetNextWindowPos(ImVec2(600, 25), ImGuiCond_Once);
        ImGui::SetNextWindowSize(ImVec2(1000, 700), ImGuiCond_Once);

        ImGui::Begin("node editor");

        auto&& nodemap = vulkan::EngineNode::node_type_map();

        for (auto&& node : nodemap) {

            ImGui::SameLine();
            if (ImGui::Button(node.second.display_name.c_str())) {
                _add_node(node.second.name.c_str());
            }
        }

        ImGui::BeginChild("nodes", {0.0f, -20.0f});

        imnodes::BeginNodeEditor();
        imnodes::PushAttributeFlag(imnodes::AttributeFlags::AttributeFlags_EnableLinkDetachWithDragClick);
        for (auto&& node : _nodes) {
            imnodes::BeginNode(node.first);

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
            _links.erase(link);
        }

        int new_link_s, new_link_e;
        if (imnodes::IsLinkCreated(&new_link_s, &new_link_e)) {
            _add_link(new_link_s, new_link_e);
        }

        int32_t node_id = -1;
        if (imnodes::IsNodeHovered(&node_id) && io.MouseClicked[2]) {
            if (_last_added_node_output == node_id) {
                _last_added_node_output = -1;
            }
            auto&& search = _nodes.find(node_id);
            if (search != _nodes.end()) {
                for (auto&& link : search->second.links) {
                    _links.erase(link);
                }
                for (auto&& pin : search->second.inputs) {
                    _pin_to_node.erase(pin);
                }
                for (auto&& pin : search->second.outputs) {
                    _pin_to_node.erase(pin);
                }
                _nodes.erase(search);
            }

        }

        ImGui::EndChild();

        if (ImGui::Button("execute")) {
            _execute();
        }
        ImGui::SameLine();
        if (ImGui::Button("load")) {
            _load_dialog.Open();
        }
        ImGui::SameLine();
        if (ImGui::Button("save")) {
            _save_dialog.Open();
        }

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

                auto lock = node.node;
                if (lock) {
                    auto&& param_map = lock->params();
                    if (param_map.size() > 0) {
                        bool open = true;

                        int32_t tot = 0;
                        for (auto&& map : param_map) {
                            tot += map.second.size();
                        }
                        ImGui::SetNextWindowPos(ImVec2(400, 40), ImGuiCond_Once);
                        ImGui::SetNextWindowSize(ImVec2(400, 20 + 30*tot), ImGuiCond_Once);
                        ImGui::Begin(node.name.c_str(), &open);
                        if (!open) {
                            close_windows.insert(node_id);
                            //_open_node_windows.erase
                        }
                        for (auto&& map : param_map) {
                            for (auto&& param : map.second) {
                                _ui_for_param(param.second);
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
            }
        }

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
                if (node->first == _display_node) {
                    int32_t np = next_pin_();
                    node->second.inputs.insert(np);
                    _pin_to_node[np] = _display_node;
                }
                node->second.links.push_back(_next_link);
            }
        }
        auto ret = _next_link;
        _next_link++;
        return ret;
    }

    template <class N>
    void NodeWindow::add_node_(Node& node) {
        node.inputs = _pins(N::input_count());
        node.outputs = _pins(N::output_count());
        node.node = vulkan::make(node.name);
    }

    int32_t NodeWindow::_add_node(std::string name) {
        Node node{};
        node.name = name;


        auto&& nodemap = vulkan::EngineNode::node_type_map();
        auto search = nodemap.find(name);
        if (search != nodemap.end()) {
            node.name = search->second.name;
            node.display_name = search->second.display_name;
            node.inputs = _pins(search->second.inputs);
            node.outputs = _pins(search->second.outputs);
            node.node = vulkan::make(node.name);
        } else if (name == "display") {
            // NOP
            node.inputs = _pins(1);
        } else if (name == "read") {
            
        } else if (name == "write") {
            
        } else if (name == "particles") {
            add_node_<vulkan::Particles>(node);
        } else if (name == "draw_particles") {
            add_node_<vulkan::DrawParticles>(node);
        } else if (name == "triangles") {
            add_node_<vulkan::DrawTriangle>(node);
        } else if (name == "history") {

        } else if (name == "sand") {
            add_node_<vulkan::Sand>(node);
        } else if (name == "draw") {
            add_node_<vulkan::DrawFullscreen>(node);
        }

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

    void NodeWindow::_execute() {
        _built_nodes.clear();
        std::deque<int32_t> node_queue;

        /*for (auto&& node : _nodes) {
            if (node.second.outputs.size() == 0) {
                node_queue.push_back(node.first);
            }
        }*/


        node_queue.push_back(_display_node);

        std::map<int32_t, std::shared_ptr<vulkan::EngineNode>> node_map;
        std::vector<std::shared_ptr<vulkan::EngineNode>> rnodes;
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

        for (auto&& node : node_map) {
            auto&& inps = input_map[node.first];
            std::vector<std::shared_ptr<vulkan::EngineNode>> inputs;
            for (auto&& inp : inps) {
                auto ptr = node_map.at(inp);
                inputs.push_back(ptr);
            }
            node.second->inputs(inputs);
           // node.second->init();
        }

        _built_nodes = {rnodes.rbegin(), rnodes.rend()};

        std::cout << ">>>>>>>>>>>>>>>>>> " << std::endl;
        for (auto&& node : rnodes) {
            node->init();
            std::cout << "inited " << node << std::endl;
        }
    }

    void NodeWindow::_ui_for_param(const std::shared_ptr<vulkan::ParameterInterface>& paramPtr) {
        ImGui::PushID(paramPtr.get());
        auto&& param = *paramPtr;
        auto&& name = param.name();
        auto type = param.type();
        using vulkan::ParameterType;
        switch (type) {
        case ParameterType::p_float:
        {
            float p = param.as<float>().get();
            float min = param.as<float>().min();
            float max = param.as<float>().max();
// SliderFloat(const char* label, float* v, float v_min, float v_max, const char* format = "%.3f", float power = 1.0f);     // adjust format to decorate the value with a prefix or a suffix for in-slider labels or unit display. Use power!=1.0 for power curve sliders
            if (ImGui::SliderFloat(name.c_str(), &p, min, max)) {
                param.as<float>().set(p);
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
            }
            break;
        }
        case ParameterType::p_vec4:
        {
            glm::vec4 p = param.as<glm::vec4>().get();
            if (ImGui::InputFloat4(name.c_str(), (float*)&p)) {
                param.as<glm::vec4>().set(p);
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
            }
            break;
        }
        case ParameterType::p_ivec4:
        {
            glm::ivec4 p = param.as<glm::ivec4>().get();
            if (ImGui::InputInt4(name.c_str(), (int*)&p)) {
                param.as<glm::ivec4>().set(p);
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
            }
            break;
        }
        case ParameterType::p_uvec4:
        {
            glm::uvec4 p = param.as<glm::uvec4>().get();
            if (ImGui::InputInt4(name.c_str(), (int*)&p)) {
                param.as<glm::uvec4>().set(p);
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
                }
            }
            break;
        }
        }
        ImGui::PopID();
    }

    void NodeWindow::load(std::string path) {

        std::map<std::string, vulkan::EngineNode::ShaderParamMap> save_map;
        for (auto&& node : _nodes) {
            if (node.second.node) {
                save_map.emplace(node.second.name, node.second.node->params());
            }
        }

        std::ifstream os(path, std::ios::binary);
        cereal::BinaryInputArchive archive(os);

        SerialiseGraph data;
        archive(data);

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


    }

    void NodeWindow::save(std::string path) {
        
        std::map<std::string, vulkan::EngineNode::ShaderParamMap> save_map;
        for (auto&& node : _nodes) {
            if (node.second.node) {
                save_map.emplace(node.second.name, node.second.node->params());
            }
        }

        std::ofstream os(path, std::ios::binary);
        cereal::BinaryOutputArchive archive( os );

        SerialiseGraph data;

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
}