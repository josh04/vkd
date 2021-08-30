#include <iostream>
#include <vector>

extern "C" {
	#include "GL/gl3w.h"
}

#include "SDL2/SDL.h"
#include "sdl2.h"
#undef main

#include "ui.hpp"

#include "glexception.hpp"
#include "vulkan.hpp"
#include "render/draw_ui.hpp"
#include "ui/node_window.hpp"

namespace vkd {
	class Device;
}

namespace {
	std::unique_ptr<imguiDrawer> ui = nullptr;
}

void render(bool& quit) {
	ui->preRender();
	vkd::ui(quit);
	ui->render();
	vkd::draw();
}

int main(int argc, char** argv) {
	int width_sm = 1280;
	int height_sm = 720;

    auto sys = sdl2::make_sdlsystem(SDL_INIT_VIDEO);
    if (!sys) {
        std::cerr << "Error creating SDL2 system: " << SDL_GetError() << std::endl;
        return 1;
    }

	SDL_DisplayMode dm;
	SDL_GetCurrentDisplayMode(0, &dm);
	width_sm = dm.w;
	height_sm = dm.h;

#if defined(unix) and !defined(__APPLE__)
	int winflag = SDL_WINDOW_OPENGL;
#else
	int winflag = SDL_WINDOW_VULKAN;
#endif

	int x = 0;
	int y = 0;
#ifdef __APPLE__
	//winflag |= SDL_WINDOW_FULLSCREEN_DESKTOP;
	height_sm -= 22;
	y += 22;
#endif

    auto win = sdl2::make_window("Demo", x, y, width_sm, height_sm, SDL_WINDOW_BORDERLESS | SDL_WINDOW_SHOWN | winflag);
    if (!win) {
        std::cerr << "Error creating window: " << SDL_GetError() << std::endl;
        return 1;
    }
	
	auto renderer = sdl2::make_renderer(win.get(), -1, SDL_RENDERER_PRESENTVSYNC);


#if defined(unix) and !defined(__APPLE__)
	constexpr int major_version = 3;
	constexpr int minor_version = 1;

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, major_version);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, minor_version);
#if defined(unix) and !defined(__APPLE__)
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
#else
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
#endif

    auto glContext = SDL_GL_CreateContext(win.get());
    if (!glContext) {
        std::cerr << "Error creating gl context: " << SDL_GetError() << std::endl;
        return 1;
    }

	SDL_GL_SetSwapInterval(0);


    if (gl3wInit()) {
        std::cerr << "gl3w failed to initialize OpenGL" << std::endl;
        return -1;
    }

    if (!gl3wIsSupported(major_version, minor_version)) {
        std::cerr << "gl3w context was not OpenGL " << major_version << "." << minor_version << std::endl;
        return -1;
    }
	
#endif

	ui = std::make_unique<imguiDrawer>(width_sm, height_sm, true);
	try {
		vkd::init(win.get(), renderer.get());
	} catch (std::runtime_error& e) {
		std::cout << e.what() << std::endl;
		return 0;
	}
	ui->resize(width_sm, height_sm, width_sm, height_sm);

	bool quit = false;
	
#if defined(unix) and !defined(__APPLE__)
	SDL_GL_SwapWindow(win.get());
#endif

	while (!quit) {
		SDL_Event e;

		while (SDL_PollEvent(&e) != 0) {
			if (e.type == SDL_MOUSEMOTION) {
				ui->mouseMove(e.motion.x, height_sm - e.motion.y);
			} else if (e.type == SDL_QUIT) {
				quit = true;
			}
			else if (e.type == SDL_TEXTINPUT) {
				ui->textEntry(&e.text.text[0]);
			}
			else if (e.type == SDL_MOUSEBUTTONDOWN) {
				ui->mouseDown(e.button.button);
			}
			else if (e.type == SDL_MOUSEBUTTONUP) {
				ui->mouseUp(e.button.button);
			}
			else if (e.type == SDL_MOUSEWHEEL) {
				ui->mouseScroll(e.wheel.y);
			} else if (e.type == SDL_KEYDOWN || e.type == SDL_KEYUP) {
				int key = e.key.keysym.scancode;
				ui->keyChange(key, (e.type == SDL_KEYDOWN));
				
				if (e.type == SDL_KEYDOWN) {
					if (e.key.keysym.sym == SDLK_ESCAPE) {
						quit = true;
					} else if (e.key.keysym.sym == SDLK_TAB) {
						vkd::get_ui().toggle_ui();
					}
				}
			}
		}

		render(quit);

		//Update screen
		//SDL_GL_SwapWindow(win.get());
	}

	vkd::shutdown();

#if defined(unix) and !defined(__APPLE__)
	SDL_GL_DeleteContext(glContext);
#endif

    return 0;
}

