#pragma once
        
#include <memory>
#include <string>
#include <vector>
#include "imgui/imgui.h"
#include "imgui/imfilebrowser.h"

namespace vkd {
    class MainUI;
    class PhotoBrowser {
    public:
        PhotoBrowser() = default;
        ~PhotoBrowser() = default;
        PhotoBrowser(PhotoBrowser&&) = delete;
        PhotoBrowser(const PhotoBrowser&) = delete;

        void draw(MainUI& ui);

        void add_entry(std::string path);
        void remove_entry(std::string path);

        template <class Archive>
        void serialize(Archive & ar, const uint32_t version) {
            if (version == 0) {
                ar(_entries, _selected);
            }
        }

        bool valid_extension(const fs::path& ext);

    private:
        void draw_entry(MainUI& ui, const fs::path& entry, int i, int width);
        std::set<std::string> _entries;
        ImGui::FileBrowser _file_dialog;
        int _selected = -1;

        static constexpr int thumbnail_width = 140;
        static constexpr int thumbnail_height = (thumbnail_width * 4) / 5;
    };
}