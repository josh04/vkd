#include "timeline.hpp"
#include "bin.hpp"
#include "main_ui.hpp"

#include "cereal/cereal.hpp"

CEREAL_CLASS_VERSION(vkd::SequencerLine::Block, 0);
CEREAL_CLASS_VERSION(vkd::SequencerLine, 0);
CEREAL_CLASS_VERSION(vkd::SequencerImpl, 0);
CEREAL_CLASS_VERSION(vkd::Timeline, 0);

namespace vkd {
    void Timeline::draw(MainUI& ui, bool& graph_changed) {
        auto&& io = ImGui::GetIO();
        ImGui::SetNextWindowCollapsed(false, ImGuiCond_Once);
        ImGui::SetNextWindowPos(ImVec2(10, io.DisplaySize.y - 210), ImGuiCond_Once );
        ImGui::SetNextWindowSize(ImVec2(io.DisplaySize.x - 10*2, 200), ImGuiCond_Once );
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
        }
        _sequencer->frame_max = count;

        ImGui::End();

    }   
}