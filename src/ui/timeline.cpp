#include "timeline.hpp"

namespace vulkan {
    void Timeline::draw() {
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

        int32_t current_frame = _current_frame;
        ImSequencer::Sequencer(_sequencer.get(), &current_frame, &_expanded, &_sequencer->selected_entry, &_sequencer->first_frame, ImSequencer::SEQUENCER_CHANGE_FRAME);

        if (current_frame != _current_frame) {
            _current_frame = current_frame;
            for (auto&& line : _sequencer->lines) {
                line->frame_jumped = true;
            }
        }
        // don't do the above in this case because uhhhh
        if (_play) {
            _current_frame++;
        }

        int32_t count = 0;
        for (auto&& line : _sequencer->lines) {
            count = std::max(line->frame_count, count);
        }
        _sequencer->frame_max = count;

        ImGui::End();

    }   
}