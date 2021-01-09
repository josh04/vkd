#pragma once
        
#include <memory>
#include <vector>
#include <string>
#include "imgui/imgui.h"
#include "imgui/ImSequencer.h"

namespace vulkan {
    struct SequencerLine {
        std::string name;
        int32_t start = 0;
        int32_t end = 100;
        uint32_t colour = 0x999999FF;

        int32_t frame_count = 0;

        bool frame_jumped = false;
    };

    struct SequencerImpl : public ImSequencer::SequenceInterface {
        int GetFrameMin() const override { return 0; }

        int GetFrameMax() const override { 
            return frame_max;
        }

        int GetItemCount() const override { return lines.size(); }

        void Get(int index, int** start, int** end, int* type, unsigned int* colour) override {
            if (index < lines.size()) {
                if (start) {
                    *start = &lines[index]->start;
                }
                if (end) {
                    *end = &lines[index]->end;
                }
                if (colour) {
                    *colour = lines[index]->colour;
                }
            }
        }

        const char* GetItemLabel(int index) const override { 
            if (index < lines.size()) {
                return lines[index]->name.c_str();
            }

            return "";
        }

        std::vector<std::shared_ptr<SequencerLine>> lines;

        int p1 = 0;
        int p2 = 100;
        
        int selected_entry = 0;
        int first_frame = 0;

        int frame_max = 100;
    };

    class Timeline {
    public:
        Timeline() : _sequencer(std::make_unique<SequencerImpl>()) {
        }
        ~Timeline() = default;
        Timeline(Timeline&&) = delete;
        Timeline(const Timeline&) = delete;

        void add_line(std::shared_ptr<SequencerLine> line) { _sequencer->lines.push_back(line); }

        void remove_line(std::shared_ptr<SequencerLine> line) { 
            for (auto begin = _sequencer->lines.begin(); begin != _sequencer->lines.end(); begin++) {
                if (begin->get() == line.get()) {
                    _sequencer->lines.erase(begin);
                    break;
                }
            }
        }

        void draw();

        bool play() const { return _play; }
        auto current_frame() const { return _current_frame; }

    private:
        std::unique_ptr<SequencerImpl> _sequencer = nullptr;
        bool _play = false;
        int32_t _current_frame = 0;

        bool _expanded = true;
    };
}