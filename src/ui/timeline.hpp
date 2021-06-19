#pragma once
        
#include <memory>
#include <vector>
#include <string>
#include "parameter.hpp"
#include "imgui/imgui.h"
#include "imgui/ImSequencer.h"

namespace vkd {
    class MainUI;
    struct SequencerLine {
        std::string name;
        struct Block {
            int32_t start = 0;
            int32_t end = 100;
            uint32_t colour = 0x999999FF;

            template <class Archive>
            void serialize(Archive & ar, const uint32_t version) {
                if (version == 0) {
                    ar(start, end, colour);
                }
            }
        };
        std::vector<Block> blocks{1};

        int32_t frame_count = 0;

        bool frame_jumped = false;

        template <class Archive>
        void serialize(Archive & ar, const uint32_t version) {
            if (version == 0) {
                ar(name, blocks, frame_count, frame_jumped);
            }
        }
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
                    *start = &lines[index]->blocks[0].start;
                }
                if (end) {
                    *end = &lines[index]->blocks[0].end;
                }
                if (colour) {
                    *colour = lines[index]->blocks[0].colour;
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

        template <class Archive>
        void serialize(Archive & ar, const uint32_t version) {
            if (version == 0) {
                ar(lines, p1, p2, selected_entry, first_frame, frame_max);
            }
        }
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

        void draw(MainUI& ui, bool& graph_changed);

        bool play() const { return _play; }
        auto current_frame() const { return _current_frame; }
        void increment() { _current_frame.index++; }

        template <class Archive>
        void serialize(Archive & ar, const uint32_t version) {
            if (version == 0) {
                ar(_sequencer, _play, _current_frame, _expanded);
            }
        }
    private:
        std::unique_ptr<SequencerImpl> _sequencer = nullptr;
        bool _play = false;
        Frame _current_frame = {0};

        bool _expanded = true;
    };
}