#include "bin.hpp"
#include "application.hpp"
#include "main_ui.hpp"
#include "cereal/cereal.hpp"

CEREAL_CLASS_VERSION(vkd::Bin::Entry, 0);
CEREAL_CLASS_VERSION(vkd::Bin, 0);

namespace vkd {
    void Bin::draw(MainUI& ui) {

        ImGui::SetNextWindowPos(ImVec2(30, 60), ImGuiCond_Once);
        ImGui::SetNextWindowSize(ImVec2(400, 400), ImGuiCond_Once);

        ImGui::Begin("project");

        if (ImGui::BeginListBox("##items", {-FLT_MIN, -30})) {
            int i = 0;
            if (ImGui::Selectable("(new empty sequence)", _selected == i)) {
                if (_selected == i) {
                    _selected = -1;
                } else {
                    _selected = i;
                }
            }
            if (ImGui::BeginDragDropSource())
            {
                ImGui::SetDragDropPayload("BIN_EMPTY", nullptr, 0);
                ImGui::EndDragDropSource();
            }
            i++;
            for (auto&& item : _entries) {
                if (ImGui::Selectable(item.path.c_str(), _selected == i)) {
                    if (_selected == i) {
                        _selected = -1;
                    } else {
                        _selected = i;
                    }
                }

                //if (Mode() == ApplicationMode::Photo) {
                    if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0)) {
                        ui.add_node_graph(item);
                    }
                //}

                if (ImGui::BeginDragDropSource())
                {
                    auto iptr = &item;
                    ImGui::SetDragDropPayload("BIN_ENTRY", &iptr, sizeof(Entry*));
                    ImGui::EndDragDropSource();
                }
                ++i;
            }

            ImGui::EndListBox();
        }

        if (ImGui::Button("add")) {
            _file_dialog.Open();
        }

        _file_dialog.Display();
        
        if(_file_dialog.HasSelected()) {
            auto file = _file_dialog.GetSelected().string();
            add_entry(file);
            _file_dialog.ClearSelected();
        }

        ImGui::End();
    }

    void Bin::add_entry(std::string path) {
        _entries.push_back({path});
    }

    void Bin::remove_entry(int i) {
        _entries.erase(_entries.begin() + i);
    }

    std::string Bin::get_entry(int i) {
        if (i >= 0 && i < _entries.size()) {
            return _entries[i].path;
        }
        return "";
    }
}