#include "photo_browser.hpp"
#include "application.hpp"
#include "main_ui.hpp"
#include "cereal/cereal.hpp"
#include "bin.hpp"
#include "ghc/filesystem.hpp"

CEREAL_CLASS_VERSION(vkd::PhotoBrowser, 0);

namespace vkd {
    
    bool PhotoBrowser::valid_extension(const fs::path& ext) {
        std::string exts = ext.string();
        
        std::transform(exts.begin(), exts.end(), exts.begin(), [](unsigned char c){ return std::tolower(c); });
        if (exts == ".mp4" ) {
            return true;
        } else if (exts == ".exr") {
            return true;
        } else if (exts == ".raf") {
            return true;
        }
        return false;
    }

    void PhotoBrowser::draw(MainUI& ui) {

        ImGui::SetNextWindowPos(ImVec2(30, 60), ImGuiCond_Once);
        ImGui::SetNextWindowSize(ImVec2(400, 400), ImGuiCond_Once);

        ImGui::Begin("file system");

        float window_width = ImGui::GetWindowWidth();

        fs::path dir{"../../../../data"};

        int width = std::max((int)std::floor((float)(window_width) / (thumbnail_width + 20)), 0);

        int i = 0;
        for (auto&& entry : fs::directory_iterator(dir)) {
            if (valid_extension(entry.path().extension())) {
                draw_entry(ui, entry.path(), i, width);
                i++;
            }
        }


        if (ImGui::Button("(new empty composition)")) {
            ui.add_node_graph();
        }

        ImGui::End();
    }
    
    void PhotoBrowser::draw_entry(MainUI& ui, const fs::path& entry, int i, int width) {
        

        const int per_column = width;

        int x = i % per_column;
        int y = std::floor(i / (float)per_column);

        //ImTextureID thumbnail_id = 0;
        // bool ImGui::ImageButton(ImTextureID user_texture_id, const ImVec2& size, const ImVec2& uv0, const ImVec2& uv1, int frame_padding, const ImVec4& bg_col, const ImVec4& tint_col)
        std::string chname = std::to_string(i);

        std::optional<ImTextureID> thumbnail_id = {};//ThumbnailManager::get(entry);

        ImGui::BeginChild(chname.c_str(), {thumbnail_width + 10, 140});

        if (thumbnail_id) {
            if (ImGui::ImageButton(*thumbnail_id, {thumbnail_width, thumbnail_height}, {0.0f, 0.0f}, {1.0f, 1.0f}, 3)) {
                ui.add_node_graph_if_not_open(Bin::Entry{entry.string()});
            }
            ImGui::TextWrapped("%s", entry.string().c_str());
        } else {
            if (ImGui::Button(entry.filename().string().c_str(), {thumbnail_width, thumbnail_height})) {
                ui.add_node_graph_if_not_open(Bin::Entry{entry.string()});
            }
            ImGui::TextWrapped("%s", entry.string().c_str());
        }
        ImGui::EndChild();
        if (x != per_column - 1) {
            ImGui::SameLine();
        }
    }

    void PhotoBrowser::add_entry(std::string path) {
        _entries.insert(path);
    }

    void PhotoBrowser::remove_entry(std::string path) {
        _entries.erase(path);
    }

}