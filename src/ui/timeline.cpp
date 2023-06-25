#include "timeline.hpp"
#include "bin.hpp"
#include "main_ui.hpp"

#include "cereal/cereal.hpp"

CEREAL_CLASS_VERSION(vkd::SequencerLine::Block, 0);
CEREAL_CLASS_VERSION(vkd::SequencerLine, 0);
CEREAL_CLASS_VERSION(vkd::SequencerImpl, 0);
CEREAL_CLASS_VERSION(vkd::Timeline, 0);

namespace vkd {

    void SequencerImpl::DoubleClick(int index) {
        open_editor_callback(index);
    }

    void Timeline::draw(MainUI& ui, bool& graph_changed) {
        auto&& io = ImGui::GetIO();
        ImGui::SetNextWindowCollapsed(false, ImGuiCond_Once);
        ImGui::SetNextWindowPos(ImVec2(10, io.DisplaySize.y - 210), ImGuiCond_Once);
        ImGui::SetNextWindowSize(ImVec2(io.DisplaySize.x - 10*2, 200), ImGuiCond_Once);
        ImGui::Begin("scrub");

        if (_play) {
            if (ImGui::Button("pause")) {
                _play = false;
            }
        } else {
            if (ImGui::Button("play")) {
                _play = true;
            }
        }
        int32_t current_frame = _current_frame.index;
        ImSequencer::Sequencer(_sequencer.get(), &current_frame, &_expanded, &_sequencer->selected_entry, &_sequencer->first_frame, ImSequencer::SEQUENCER_CHANGE_FRAME | ImSequencer::SEQUENCER_EDIT_STARTEND);

        if (ImGui::BeginDragDropTarget()) {
            Bin::Entry to_add;
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("BIN_ENTRY")) {
                to_add = **(Bin::Entry**)payload->Data;
                ui.add_node_graph(to_add);
                graph_changed = true;
            }
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("BIN_EMPTY")) {
                ui.add_node_graph(to_add);
                graph_changed = true;
            }
            ImGui::EndDragDropTarget();
        }

        if (current_frame != _current_frame.index) {
            _current_frame.index = current_frame;
            for (auto&& line : _sequencer->lines) {
                line->frame_jumped = true;
            }
        }

        int32_t count = 0;
        for (auto&& line : _sequencer->lines) {
            count = std::max(line->frame_count, count);
            count = std::max(line->blocks[0].end, count);
        }
        _sequencer->frame_max = count;

        for (auto it = _open_bar_editors.begin(); it != _open_bar_editors.end();) {
            auto&& bar = *it;
            auto&& io = ImGui::GetIO();
            ImGui::SetNextWindowCollapsed(false, ImGuiCond_Once);
            ImGui::SetNextWindowPos(ImVec2(200, 200), ImGuiCond_Once);
            ImGui::SetNextWindowSize(ImVec2(200, 200), ImGuiCond_Once);

            std::string lineEdit = "Bar editor##" + std::to_string(bar);
            bool isOpen = true;
            ImGui::Begin(lineEdit.c_str(), &isOpen);

            char text[1024] = "";
            if (ImGui::InputText("name", text, 1024)) {
                _sequencer->lines[bar]->name = text;
            }

            std::array<int32_t, 2> vals;
            vals[0] = _sequencer->lines[bar]->blocks[0].start;
            vals[1] = _sequencer->lines[bar]->blocks[0].end;

            if (ImGui::InputInt2("start/end", vals.data())) {
                _sequencer->lines[bar]->blocks[0].start = vals[0];
                vals[1] = _sequencer->lines[bar]->blocks[0].end = vals[1];
                _sequencer->edited = true;
            }

            ImGui::End();
            if (!isOpen) {
                it = _open_bar_editors.erase(it);
            } else {
                it++;
            }
        }

        graph_changed = graph_changed || _sequencer->edited;
        _sequencer->edited = false;

        ImGui::End();

    }
    void Timeline::increment() { 
        _current_frame.index = std::clamp(_current_frame.index + 1, (int64_t)0, _sequencer->frame_max);
        //std::cout << _sequencer->frame_max << std::endl;
    }
}