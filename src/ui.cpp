#define NOMINMAX
extern "C" {
	#include "GL/gl3w.h"
}

#include "SDL2/SDL.h"
#include "ui.hpp"
#include "imgui_draw_funcs_gl.hpp"
#include "imgui/imnodes.h"
#include <iostream>

imguiDrawer::imguiDrawer(unsigned int width, unsigned int height, bool vulkan) : _vulkan(vulkan), _width(width), _height(height) {
    mouseButtons = { false, false, false };
    _imgui_context = ImGui::CreateContext();
    imnodes::Initialize();
    ImGui::StyleColorsClassic();

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

