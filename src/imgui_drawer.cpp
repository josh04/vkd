#define NOMINMAX
extern "C" {
	#include "GL/gl3w.h"
}

#include "SDL2/SDL.h"
#include "imgui_drawer.hpp"
#include "imgui_draw_funcs_gl.hpp"
#include "imgui/imnodes.h"
#include <iostream>

namespace {
    // from CookiePLMonster on the imgui github issue #707 (https://github.com/ocornut/imgui/issues/707)
    void StyleColorsDeusEx() {
        ImGuiStyle* style = &ImGui::GetStyle();
        ImVec4* colors = style->Colors;

        colors[ImGuiCol_Text]                   = ImVec4(0.92f, 0.92f, 0.92f, 1.00f);
        colors[ImGuiCol_TextDisabled]           = ImVec4(0.44f, 0.44f, 0.44f, 1.00f);
        colors[ImGuiCol_WindowBg]               = ImVec4(0.06f, 0.06f, 0.06f, 1.00f);
        colors[ImGuiCol_ChildBg]                = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
        colors[ImGuiCol_PopupBg]                = ImVec4(0.08f, 0.08f, 0.08f, 0.94f);
        colors[ImGuiCol_Border]                 = ImVec4(0.51f, 0.36f, 0.15f, 1.00f);
        colors[ImGuiCol_BorderShadow]           = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
        colors[ImGuiCol_FrameBg]                = ImVec4(0.11f, 0.11f, 0.11f, 1.00f);
        colors[ImGuiCol_FrameBgHovered]         = ImVec4(0.51f, 0.36f, 0.15f, 1.00f);
        colors[ImGuiCol_FrameBgActive]          = ImVec4(0.78f, 0.55f, 0.21f, 1.00f);
        colors[ImGuiCol_TitleBg]                = ImVec4(0.51f, 0.36f, 0.15f, 1.00f);
        colors[ImGuiCol_TitleBgActive]          = ImVec4(0.91f, 0.64f, 0.13f, 1.00f);
        colors[ImGuiCol_TitleBgCollapsed]       = ImVec4(0.00f, 0.00f, 0.00f, 0.51f);
        colors[ImGuiCol_MenuBarBg]              = ImVec4(0.11f, 0.11f, 0.11f, 1.00f);
        colors[ImGuiCol_ScrollbarBg]            = ImVec4(0.06f, 0.06f, 0.06f, 0.53f);
        colors[ImGuiCol_ScrollbarGrab]          = ImVec4(0.21f, 0.21f, 0.21f, 1.00f);
        colors[ImGuiCol_ScrollbarGrabHovered]   = ImVec4(0.47f, 0.47f, 0.47f, 1.00f);
        colors[ImGuiCol_ScrollbarGrabActive]    = ImVec4(0.81f, 0.83f, 0.81f, 1.00f);
        colors[ImGuiCol_CheckMark]              = ImVec4(0.78f, 0.55f, 0.21f, 1.00f);
        colors[ImGuiCol_SliderGrab]             = ImVec4(0.91f, 0.64f, 0.13f, 1.00f);
        colors[ImGuiCol_SliderGrabActive]       = ImVec4(0.91f, 0.64f, 0.13f, 1.00f);
        colors[ImGuiCol_Button]                 = ImVec4(0.51f, 0.36f, 0.15f, 1.00f);
        colors[ImGuiCol_ButtonHovered]          = ImVec4(0.91f, 0.64f, 0.13f, 1.00f);
        colors[ImGuiCol_ButtonActive]           = ImVec4(0.78f, 0.55f, 0.21f, 1.00f);
        colors[ImGuiCol_Header]                 = ImVec4(0.51f, 0.36f, 0.15f, 1.00f);
        colors[ImGuiCol_HeaderHovered]          = ImVec4(0.91f, 0.64f, 0.13f, 1.00f);
        colors[ImGuiCol_HeaderActive]           = ImVec4(0.93f, 0.65f, 0.14f, 1.00f);
        colors[ImGuiCol_Separator]              = ImVec4(0.21f, 0.21f, 0.21f, 1.00f);
        colors[ImGuiCol_SeparatorHovered]       = ImVec4(0.91f, 0.64f, 0.13f, 1.00f);
        colors[ImGuiCol_SeparatorActive]        = ImVec4(0.78f, 0.55f, 0.21f, 1.00f);
        colors[ImGuiCol_ResizeGrip]             = ImVec4(0.21f, 0.21f, 0.21f, 1.00f);
        colors[ImGuiCol_ResizeGripHovered]      = ImVec4(0.91f, 0.64f, 0.13f, 1.00f);
        colors[ImGuiCol_ResizeGripActive]       = ImVec4(0.78f, 0.55f, 0.21f, 1.00f);
        colors[ImGuiCol_Tab]                    = ImVec4(0.51f, 0.36f, 0.15f, 1.00f);
        colors[ImGuiCol_TabHovered]             = ImVec4(0.91f, 0.64f, 0.13f, 1.00f);
        colors[ImGuiCol_TabActive]              = ImVec4(0.78f, 0.55f, 0.21f, 1.00f);
        colors[ImGuiCol_TabUnfocused]           = ImVec4(0.07f, 0.10f, 0.15f, 0.97f);
        colors[ImGuiCol_TabUnfocusedActive]     = ImVec4(0.14f, 0.26f, 0.42f, 1.00f);
        colors[ImGuiCol_PlotLines]              = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
        colors[ImGuiCol_PlotLinesHovered]       = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
        colors[ImGuiCol_PlotHistogram]          = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
        colors[ImGuiCol_PlotHistogramHovered]   = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
        colors[ImGuiCol_TextSelectedBg]         = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
        colors[ImGuiCol_DragDropTarget]         = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
        colors[ImGuiCol_NavHighlight]           = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
        colors[ImGuiCol_NavWindowingHighlight]  = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
        colors[ImGuiCol_NavWindowingDimBg]      = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
        colors[ImGuiCol_ModalWindowDimBg]       = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);

        auto&& nodeStyle = imnodes::GetStyle();
        nodeStyle.colors[imnodes::ColorStyle_NodeBackground] = (ImU32)ImColor(0.06f, 0.06f, 0.06f, 1.00f);
        nodeStyle.colors[imnodes::ColorStyle_NodeBackgroundHovered] = (ImU32)ImColor(0.51f, 0.36f, 0.15f, 1.00f);
        nodeStyle.colors[imnodes::ColorStyle_NodeBackgroundSelected] = (ImU32)ImColor(0.78f, 0.55f, 0.21f, 1.00f);
        nodeStyle.colors[imnodes::ColorStyle_NodeOutline] = (ImU32)ImColor(0.51f, 0.36f, 0.15f, 1.00f);

        // title bar colors match ImGui's titlebg colors
        nodeStyle.colors[imnodes::ColorStyle_TitleBar] = (ImU32)ImColor(0.51f, 0.36f, 0.15f, 1.00f);
        nodeStyle.colors[imnodes::ColorStyle_TitleBarHovered] = (ImU32)ImColor(0.91f, 0.64f, 0.13f, 1.00f);
        nodeStyle.colors[imnodes::ColorStyle_TitleBarSelected] = (ImU32)ImColor(0.91f, 0.64f, 0.13f, 1.00f);

        // link colors match ImGui's slider grab colors
        nodeStyle.colors[imnodes::ColorStyle_Link] = (ImU32)ImColor(0.91f, 0.64f, 0.13f, 1.00f);
        nodeStyle.colors[imnodes::ColorStyle_LinkHovered] = (ImU32)ImColor(0.91f, 0.64f, 0.13f, 1.00f);
        nodeStyle.colors[imnodes::ColorStyle_LinkSelected] = (ImU32)ImColor(0.91f, 0.64f, 0.13f, 1.00f);

        // pin colors match ImGui's button colors
        nodeStyle.colors[imnodes::ColorStyle_Pin] = (ImU32)ImColor(0.51f, 0.36f, 0.15f, 1.00f);
        nodeStyle.colors[imnodes::ColorStyle_PinHovered] = (ImU32)ImColor(0.91f, 0.64f, 0.13f, 1.00f);

        nodeStyle.colors[imnodes::ColorStyle_BoxSelector] = IM_COL32(61, 133, 224, 30);
        nodeStyle.colors[imnodes::ColorStyle_BoxSelectorOutline] = IM_COL32(61, 133, 224, 150);

        nodeStyle.colors[imnodes::ColorStyle_GridBackground] = IM_COL32(40, 40, 50, 200);
        nodeStyle.colors[imnodes::ColorStyle_GridLine] = IM_COL32(200, 200, 200, 40);
/*
        style->FramePadding = ImVec2(4, 2);
        style->ItemSpacing = ImVec2(10, 2);
        style->IndentSpacing = 12;
        style->ScrollbarSize = 10;

        style->WindowRounding = 4;
        style->FrameRounding = 4;
        style->PopupRounding = 4;
        style->ScrollbarRounding = 6;
        style->GrabRounding = 4;
        style->TabRounding = 4;

        style->WindowTitleAlign = ImVec2(1.0f, 0.5f);
        style->WindowMenuButtonPosition = ImGuiDir_Right;

        style->DisplaySafeAreaPadding = ImVec2(4, 4);
        */
    }
}

imguiDrawer::imguiDrawer(unsigned int width, unsigned int height, bool vulkan) : _vulkan(vulkan), _width(width), _height(height) {
    mouseButtons = { false, false, false };
    _imgui_context = ImGui::CreateContext();
    imnodes::Initialize();
    ImGui::StyleColorsClassic();

    StyleColorsDeusEx();

    //draw::ui::imguiTheme();


    auto&& style = ImGui::GetStyle();
    style.FrameBorderSize = 0.0f;
    style.WindowBorderSize = 0.0f;
    style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.20f, 0.25f, 0.30f, 0.00f);

    ImGuiIO& io = ImGui::GetIO();
    // Keyboard mapping. ImGui will use those indices to peek into the io.KeyDown[] array.

    io.KeyMap[ImGuiKey_Tab] = SDL_SCANCODE_TAB;
    io.KeyMap[ImGuiKey_LeftArrow] = SDL_SCANCODE_LEFT;
    io.KeyMap[ImGuiKey_RightArrow] = SDL_SCANCODE_RIGHT;
    io.KeyMap[ImGuiKey_UpArrow] = SDL_SCANCODE_UP;
    io.KeyMap[ImGuiKey_DownArrow] = SDL_SCANCODE_DOWN;
    io.KeyMap[ImGuiKey_PageUp] = SDL_SCANCODE_PAGEUP;
    io.KeyMap[ImGuiKey_PageDown] = SDL_SCANCODE_PAGEDOWN;
    io.KeyMap[ImGuiKey_Home] = SDL_SCANCODE_HOME;
    io.KeyMap[ImGuiKey_End] = SDL_SCANCODE_END;
    io.KeyMap[ImGuiKey_Insert] = SDL_SCANCODE_INSERT;
    io.KeyMap[ImGuiKey_Delete] = SDL_SCANCODE_DELETE;
    io.KeyMap[ImGuiKey_Backspace] = SDL_SCANCODE_BACKSPACE;
    io.KeyMap[ImGuiKey_Space] = SDL_SCANCODE_SPACE;
    io.KeyMap[ImGuiKey_Enter] = SDL_SCANCODE_RETURN;
    io.KeyMap[ImGuiKey_Escape] = SDL_SCANCODE_ESCAPE;
    io.KeyMap[ImGuiKey_KeyPadEnter] = SDL_SCANCODE_RETURN2;

    io.KeyMap[ImGuiKey_A] = SDL_SCANCODE_A;
    io.KeyMap[ImGuiKey_C] = SDL_SCANCODE_C;
    io.KeyMap[ImGuiKey_V] = SDL_SCANCODE_V;
    io.KeyMap[ImGuiKey_X] = SDL_SCANCODE_X;
    io.KeyMap[ImGuiKey_Y] = SDL_SCANCODE_Y;
    io.KeyMap[ImGuiKey_Z] = SDL_SCANCODE_Z;
/*
    if (_vulkan) {
        io.RenderDrawListsFn = NULL;
    } else {
        io.RenderDrawListsFn = ImGui_ImplGlfwGL3_RenderDrawLists;
    }
*/ // removed imgui 1.80
    // Alternatively you can set this to NULL and call ImGui::GetDrawData() after ImGui::Render() to get the same ImDrawData pointer.

    /*io.SetClipboardTextFn = ImGui_ImplGlfwGL3_SetClipboardText;
     io.GetClipboardTextFn = ImGui_ImplGlfwGL3_GetClipboardText;*/

     //auto path = load_image::get_path("cabin-bold.ttf");
     //ImFont * font = io.Fonts->AddFontFromFileTTF(path.c_str(), 24.0f);
}

imguiDrawer::~imguiDrawer() {
    
    if (!_vulkan) {
        ImGui_ImplGlfwGL3_Shutdown();
    }

    imnodes::Shutdown();
    ImGui::DestroyContext(_imgui_context);
    _imgui_context = nullptr;
    //ImGui::Shutdown();
}

void imguiDrawer::keyChange(int k, bool down) {
    ImGuiIO& io = ImGui::GetIO();
    io.KeysDown[(int)k] = down;

    io.KeyShift = ((SDL_GetModState() & KMOD_SHIFT) != 0);
    io.KeyCtrl = ((SDL_GetModState() & KMOD_CTRL) != 0);
    io.KeyAlt = ((SDL_GetModState() & KMOD_ALT) != 0);
#ifdef _WIN32
    io.KeySuper = false;
#else
    io.KeySuper = ((SDL_GetModState() & KMOD_GUI) != 0);
#endif
}

void imguiDrawer::preRender() {
    if (_first) {
        if (!_vulkan) {
            ImGui_ImplGlfwGL3_CreateDeviceObjects(&_font);
        }
        _first = false;
    }

    ImGuiIO& io = ImGui::GetIO();

    static uint64_t g_Time = 0; 
    static uint64_t frequency = SDL_GetPerformanceFrequency();
    uint64_t current_time = SDL_GetPerformanceCounter();
    io.DeltaTime = g_Time > 0 ? (float)((double)(current_time - g_Time) / frequency) : (float)(1.0f / 60.0f);
    g_Time = current_time;

    // Setup display size (every frame to accommodate for window resizing)
    int w = _width, h = _height;
    int display_w = _width, display_h = _height;
    //glfwGetWindowSize(g_Window, &w, &h);
    //glfwGetFramebufferSize(g_Window, &display_w, &display_h);
    io.DisplaySize = ImVec2((float)w, (float)h);
    io.DisplayFramebufferScale = ImVec2(_width / (float)_width, _height / (float)_height);


    // Setup time step
    //azure::TimePoint current_time = azure::Clock::now();
    //io.DeltaTime = g_Time > azure::TimePoint::min() ? (float)(current_time - g_Time).count() * 1e-9 : (float)(1.0f / 60.0f);
    //std::cout << io.DeltaTime << std::endl;
    //g_Time = current_time;

    // Setup inputs
    // (we already got mouse wheel, keyboard keys & characters from glfw callbacks polled in glfwPollEvents())

    int mx, my;
    Uint32 mouse_buttons = SDL_GetMouseState(&mx, &my);

    io.MouseDown[0] = mouseButtons[0] || (mouse_buttons & SDL_BUTTON(SDL_BUTTON_LEFT)) != 0;  // If a mouse press event came, always pass it as "mouse held this frame", so we don't miss click-release events that are shorter than 1 frame.
    io.MouseDown[1] = mouseButtons[1] || (mouse_buttons & SDL_BUTTON(SDL_BUTTON_RIGHT)) != 0;
    io.MouseDown[2] = mouseButtons[2] || (mouse_buttons & SDL_BUTTON(SDL_BUTTON_MIDDLE)) != 0;

    io.MouseWheel = scroll;
    scroll = 0;

    io.MousePos = ImVec2((float)m_x, (float)(_height - m_y));
    io.IniFilename = nullptr;
    // Hide OS mouse cursor if ImGui is drawing it
    //glfwSetInputMode(g_Window, GLFW_CURSOR, io.MouseDrawCursor ? GLFW_CURSOR_HIDDEN : GLFW_CURSOR_NORMAL);

    for (auto&& mb : mouseButtons) {
        mb = false;
    }

    // Start the frame
    ImGui::NewFrame();
}

void imguiDrawer::render() {
    /*
    ImGui::SetNextWindowCollapsed(true, ImGuiSetCond_Once);
    ImGui::SetNextWindowPos(ImVec2(20, 0), ImGuiSetCond_FirstUseEver);
    ImGui::ShowTestWindow();
    */
    ImGui::Render();
}

void imguiDrawer::resize(int width, int height, int s_width, int s_height) {
    auto& io = ImGui::GetIO();
    _width = width;
    _height = height;

    _s_width = s_width;
    _s_height = s_height;

    io.DisplaySize = ImVec2((float)_width, (float)_height);
    io.DisplayFramebufferScale = ImVec2(_width / (float)_width, _height / (float)_height);
}

