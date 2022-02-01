#pragma once
        
#include <memory>
#include <string>
#include <vector>
#include "imgui/imgui.h"
#include "imgui/imfilebrowser.h"

namespace vkd {
    class MainUI;
    class Bin {
    public:
        Bin() = default;
        ~Bin() = default;
        Bin(Bin&&) = delete;
        Bin(const Bin&) = delete;

        void draw(MainUI& ui);

        void add_entry(std::string path);
        void remove_entry(int i);

        std::string get_entry(int i);

        struct Entry {
            std::string path;
            
            template <class Archive>
            void serialize(Archive & ar, const uint32_t version) {
                if (version == 0) {
                    ar(path);
                }
            }

        };

        template <class Archive>
        void serialize(Archive & ar, const uint32_t version) {
            if (version == 0) {
                ar(_entries, _selected);
            }
        }

    private:
        std::vector<Entry> _entries;
        ImGui::FileBrowser _file_dialog;
        int _selected = -1;
    };
}