#pragma once

#include <array>


#include "imgui/imgui.h"


// This is the main rendering function that you have to implement and provide to ImGui (via setting up 'RenderDrawListsFn' in the ImGuiIO structure)
// If text or lines are blurry when integrating ImGui in your engine:
// - in your Render function, try translating your projection matrix by (0.5f,0.5f) or (0.375f,0.375f)
void ImGui_ImplGlfwGL3_RenderDrawLists(ImDrawData* draw_data);
void ImGui_ImplGlfwGL3_CreateFontsTexture();
bool ImGui_ImplGlfwGL3_CreateDeviceObjects();

class imguiDrawer {
public:
    imguiDrawer(unsigned int width, unsigned int height);

    ~imguiDrawer();

    void preRender();
    void render();

    void keyChange(int k, bool down);

    void mouseDown(int k) {
        mouseButtons[(int)k - 1] = true;
    }

    void mouseUp(int k) {
        mouseButtons[(int)k - 1] = false;
    }
    
    void mouseScroll(int s) {
#ifdef __APPLE__
        scroll += s / 5.0f; // magic number
#endif
#ifdef _WIN32
        scroll += s / 5.0f; // magic number
#endif
    }

    void mouseMove(int x, int y) {

        m_x = x * (_width / (float)_s_width);
        m_y = y * (_height / (float)_s_height);

    }

    void textEntry(std::string text) {
        ImGuiIO& io = ImGui::GetIO();
        io.AddInputCharactersUTF8(text.c_str());
    }

    void resize(int width, int height, int s_width, int s_height);

    ImFont** getFont() { return &_font; }

private:
    unsigned int _width = 0, _height = 0;
    unsigned int _s_width = 0, _s_height = 0;

    int m_x = -1, m_y = -1;
    float scroll = 0;
    std::array<bool, 3> mouseButtons;

    ImFont* _font = nullptr;

    ImGuiContext* _imgui_context = nullptr;
};
