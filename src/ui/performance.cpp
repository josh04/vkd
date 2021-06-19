#include "performance.hpp"
#include "imgui/imgui.h"

#include "cereal/cereal.hpp"

CEREAL_CLASS_VERSION(vkd::Performance, 0);
CEREAL_CLASS_VERSION(vkd::Performance::Report, 0);

namespace vkd {
    void Performance::draw() {
        ImGui::SetNextWindowPos(ImVec2(50, 50), ImGuiCond_Once);
        ImGui::SetNextWindowSize(ImVec2(300, 300), ImGuiCond_Once);
        ImGui::SetNextWindowCollapsed(true, ImGuiCond_Once);

        ImGui::Begin("performance");

        ImGui::InputInt("reports to keep", &_reports_to_keep);

        while (_reports.size() > _reports_to_keep) {
            _reports.pop_front();
        }

        if (_reports.size() > 0 && ImGui::BeginTable("perf", 4, ImGuiTableFlags_Resizable | ImGuiTableFlags_NoSavedSettings)) {


            ImGui::TableSetupColumn("no.", ImGuiTableColumnFlags_WidthFixed);
            ImGui::TableSetupColumn("type", ImGuiTableColumnFlags_WidthFixed);
            ImGui::TableSetupColumn("time (ms)", ImGuiTableColumnFlags_WidthFixed);
            ImGui::TableSetupColumn("details", ImGuiTableColumnFlags_WidthStretch);

            ImGui::TableHeadersRow();

            for (auto&& report : _reports) {
                ImGui::TableNextRow();
                
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("%lld", report.index);
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("%s", report.type.c_str());
                ImGui::TableSetColumnIndex(2);
                ImGui::Text("%lld", report.duration_ms);
                ImGui::TableSetColumnIndex(3);
                ImGui::Text("%s", report.extra.c_str());
            }

            ImGui::EndTable();
        }
        
        ImGui::End();
    }
}