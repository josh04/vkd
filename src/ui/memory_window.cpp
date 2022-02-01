#include "main_ui.hpp"
#include "memory_window.hpp"
#include "device.hpp"
#include "memory/memory_pool.hpp"

namespace vkd {
    namespace {
        std::string mem_flag_string(VkMemoryPropertyFlags flags) {
            std::string outp = "";
            if (flags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) {
                outp += "(device local) ";
            }
            if (flags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) {
                outp += "(host visible) ";
            }
            if (flags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) {
                outp += "(host coherent) ";
            }
            if (flags & VK_MEMORY_PROPERTY_HOST_CACHED_BIT) {
                outp += "(host cached) ";
            }
            if (flags & VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT) {
                outp += "(lazy allocated) ";
            }
            if (flags & VK_MEMORY_PROPERTY_PROTECTED_BIT) {
                outp += "(protected) ";
            }
            if (flags & VK_MEMORY_PROPERTY_DEVICE_COHERENT_BIT_AMD) {
                outp += "(coherent amd) ";
            }
            if (flags & VK_MEMORY_PROPERTY_DEVICE_UNCACHED_BIT_AMD) {
                outp += "(uncached amd) ";
            }
            return outp;
        }
    }
    void MemoryWindow::draw(Device& d)
    {
        if (!_open) {
            return;
        }

        ImGui::SetNextWindowPos(ImVec2(40, 100), ImGuiCond_Once);
        ImGui::SetNextWindowSize(ImVec2(450, 300), ImGuiCond_Once);
        ImGui::SetNextWindowCollapsed(false, ImGuiCond_FirstUseEver);

        ImGui::Begin("memory", &_open);

        auto c = d.memory_manager().get_report();

        const char * format_string = R"src(device buffer memory: %lld mb
device buffer count: %lld 
device image memory: %lld mb
device image count: %lld 
host buffer memory: %lld mb
host buffer count: %lld 
)src";

        ImGui::Text(format_string, c.device_buffer_memory / (1024 * 1024),
                                   c.device_buffer_count,
                                   c.device_image_memory / (1024 * 1024),
                                   c.device_image_count,
                                   c.host_buffer_memory / (1024 * 1024),
                                   c.host_buffer_count);

        auto pool = d.pool().pool();

        std::string poolt;
        double count = 0.0;
        for (auto&& p : pool) {
            count += p.size / (1024.0 * 1024.0);
            poolt += std::to_string(p.size / (1024.0 * 1024.0)) + "mb " + mem_flag_string(p.memory_property_flags) + " type index: " + std::to_string(p.memory_type_index) + "\n";
        }

        ImGui::Text("%s \n", poolt.c_str());
        ImGui::Text("%f mb in pool", count);

        ImGui::End();

    }

}