#include <iostream>

extern "C" {
	#include "GL/gl3w.h"
}

#include "SDL2/SDL.h"


#include "sdl2.h"
#undef main

#include "ui.hpp"

#include "glexception.hpp"
#include "vulkan.hpp"

namespace {

	std::unique_ptr<imguiDrawer> ui = nullptr;

	// GL state
	bool gRenderQuad = true;
	GLuint gProgramID = 0;
	GLint gVertexPos2DLocation = -1;
	GLuint gVBO = 0;
	GLuint gIBO = 0;
	GLuint gVAO = 0;
	GLuint gTex = 0;

	GLuint imageVBO = 0;

	float* host_pinned = nullptr;

	constexpr int width_ = 3480;
	constexpr int height_ = 2160;
	constexpr int mem_dim_ = width_ * height_ * 4 * sizeof(float);
}

bool initGL()
{
	//Success flag
	bool success = true;

	//Generate program
	gProgramID = glCreateProgram();
	_glException();

	//Create vertex shader
	GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
	_glException();

	//Get vertex source
	const GLchar* vertexShaderSource[] = { R"src(#version 300 es

in vec2 LVertexPos2D;

out vec2 texCoord;

void main() { 
	texCoord = (LVertexPos2D + vec2(1.0)) / 2.0;
	gl_Position = vec4( LVertexPos2D.x, LVertexPos2D.y, 0, 1 ); 
}
)src" };

	//Set vertex source
	glShaderSource(vertexShader, 1, vertexShaderSource, NULL);
	_glException();

	//Compile vertex source
	glCompileShader(vertexShader);
	_glException();

	//Check vertex shader for errors
	GLint vShaderCompiled = GL_FALSE;
	glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &vShaderCompiled);
	_glException();
	if (vShaderCompiled != GL_TRUE)
	{
		printf("Unable to compile vertex shader %d!\n", vertexShader);
		//(vertexShader);
		success = false;
	}
	else
	{
		//Attach vertex shader to program
		glAttachShader(gProgramID, vertexShader);
		_glException();


		//Create fragment shader
		GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
		_glException();

		//Get fragment source
		const GLchar* fragmentShaderSource[] = { R"src(#version 300 es

precision highp float;

in vec2 texCoord;

out vec4 LFragment; 

uniform sampler2D tex; 

void main() { 
	LFragment = texture(tex, texCoord); //vec4( 1.0, 1.0, 1.0, 1.0 ); 
}

)src" };

		//Set fragment source
		glShaderSource(fragmentShader, 1, fragmentShaderSource, NULL);
		_glException();

		//Compile fragment source
		glCompileShader(fragmentShader);
		_glException();

		//Check fragment shader for errors
		GLint fShaderCompiled = GL_FALSE;
		glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &fShaderCompiled);
		_glException();
		if (fShaderCompiled != GL_TRUE)
		{
			printf("Unable to compile fragment shader %d!\n", fragmentShader);
			//printShaderLog(fragmentShader);
			success = false;
		}
		else
		{
			//Attach fragment shader to program
			glAttachShader(gProgramID, fragmentShader);
			_glException();


			//Link program
			glLinkProgram(gProgramID);
			_glException();

			//Check for errors
			GLint programSuccess = GL_TRUE;
			glGetProgramiv(gProgramID, GL_LINK_STATUS, &programSuccess);
			_glException();
			if (programSuccess != GL_TRUE)
			{
				printf("Error linking program %d!\n", gProgramID);
				//printProgramLog(gProgramID);
				success = false;
			}
			else
			{
				//Get vertex attribute location
				gVertexPos2DLocation = glGetAttribLocation(gProgramID, "LVertexPos2D");
				_glException();
				if (gVertexPos2DLocation == -1)
				{
					printf("LVertexPos2D is not a valid glsl program variable!\n");
					success = false;
				}
				else
				{
					//Initialize clear color
					glClearColor(0.f, 1.f, 0.f, 1.f);
					_glException();

					//VBO data
					//GLfloat* vertexData = (GLfloat*)malloc(256 * 256 * 4 * 4);
					GLfloat vertexData[] =
					{
						-1.0f, -1.0f,
						 1.0f, -1.0f,
						-1.0f,  1.0f,
						 1.0f,  1.0f
					};

					//IBO data
					GLushort indexData[] = { 0, 1, 2, 3 };

					glGenVertexArrays(1, &gVAO);
					_glException();
					glBindVertexArray(gVAO);
					_glException();

					//Create VBO
					glGenBuffers(1, &imageVBO);
					_glException();
					glBindBuffer(GL_ARRAY_BUFFER, imageVBO);
					_glException();
					glBufferData(GL_ARRAY_BUFFER, mem_dim_, nullptr, GL_STATIC_DRAW);
					_glException();

					//Create VBO
					glGenBuffers(1, &gVBO);
					_glException();
					glBindBuffer(GL_ARRAY_BUFFER, gVBO);
					_glException();
					glBufferData(GL_ARRAY_BUFFER, 2 * 4 * sizeof(GLfloat), vertexData, GL_STATIC_DRAW);
					_glException();

					//Create IBO
					glGenBuffers(1, &gIBO);
					_glException();
					glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gIBO);
					_glException();
					glBufferData(GL_ELEMENT_ARRAY_BUFFER, 4 * sizeof(GLushort), indexData, GL_STATIC_DRAW);
					_glException();

					//glBindVertexArray(0);

					glGenTextures(1, &gTex);
					_glException();
					glBindTexture(GL_TEXTURE_2D, gTex);
					_glException();
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
					_glException();
					float* mem = (float*)malloc(mem_dim_);
					glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width_, height_, 0, GL_RGBA, GL_FLOAT, 0);
					free(mem);
					_glException();
				}
			}
		}
	}

	return success;
}

#include <vector>

void render_host(float anim_time) {
	int end = width_ * height_ / 2;
	float tick = M_PI * 2.0f / 360.0;
	for (int i = end; i < end + end; ++i) {
		host_pinned[i * 4 + 0] = 1.0f; sin(tick * anim_time);
		host_pinned[i * 4 + 1] = 1.0f; cos(tick * anim_time);
		host_pinned[i * 4 + 2] = 1.0f; //tan(tick * anim_time);
		host_pinned[i * 4 + 3] = 1.0f;
	}
}

void render(float anim_time)
{
	//render_host(anim_time);

	//Clear color buffer
	glGetError();
	glClear(GL_COLOR_BUFFER_BIT);
	_glException();

	//Render quad
	if (gRenderQuad)
	{
		//Bind program
		glUseProgram(gProgramID);
		_glException();

		//Enable vertex position
		glEnableVertexAttribArray(gVertexPos2DLocation);
		_glException();

		//Set vertex data
		glBindBuffer(GL_ARRAY_BUFFER, gVBO);
		_glException();
		glVertexAttribPointer(gVertexPos2DLocation, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), NULL);
		_glException();

		//Set index data and render
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gIBO);
		_glException();
		glDrawElements(GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_SHORT, NULL);
		_glException();

		//Disable vertex position
		glDisableVertexAttribArray(gVertexPos2DLocation);
		_glException();

		//Unbind program
		glUseProgram(0);
		_glException();
	}

	ui->preRender();
	ImGui::ShowDemoWindow();
	vulkan::ui();
	ui->render();
}

void close()
{
	free(host_pinned);
	//Deallocate program
	glDeleteProgram(gProgramID);
}


int main(int argc, char** argv)
{
	constexpr int width_sm = 1280;
	constexpr int height_sm = 720;

    auto sys = sdl2::make_sdlsystem(SDL_INIT_VIDEO);
    if (!sys) {
        std::cerr << "Error creating SDL2 system: " << SDL_GetError() << std::endl;
        return 1;
    }

    auto win = sdl2::make_window("Demo", 100, 100, width_sm, height_sm, SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL);
    if (!win) {
        std::cerr << "Error creating window: " << SDL_GetError() << std::endl;
        return 1;
    }

	constexpr int major_version = 3;
	constexpr int minor_version = 1;

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, major_version);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, minor_version);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);

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

	if (!initGL()) {
		std::cerr << "initGL failed" << std::endl;
		return -1;
	}

	host_pinned = (float*)malloc(mem_dim_);
	
	ui = std::make_unique<imguiDrawer>(width_sm, height_sm);
	ui->resize(width_sm, height_sm, width_sm, height_sm);

	bool quit = false;
	float anim_time = 0.0f;
	while (!quit) {

		SDL_Event e;

		//Handle events on queue
		while (SDL_PollEvent(&e) != 0)
		{
			if (e.type == SDL_MOUSEMOTION) {
				ui->mouseMove(e.motion.x, height_sm - e.motion.y);
			}
			else if (e.type == SDL_QUIT)
			{
				quit = true;
			}
			else if (e.type == SDL_TEXTINPUT)
			{
				ui->textEntry(&e.text.text[0]);
			}
			else if (e.type == SDL_MOUSEBUTTONDOWN)
			{
				ui->mouseDown(e.button.button);
			}
			else if (e.type == SDL_MOUSEBUTTONUP)
			{
				ui->mouseUp(e.button.button);
			}
			else if (e.type == SDL_KEYDOWN || e.type == SDL_KEYUP)
			{
				int key = e.key.keysym.scancode;
				ui->keyChange(key, (e.type == SDL_KEYDOWN));
			}
		}

		//Render quad
		render(anim_time);
		anim_time += 1.0f;

		//Update screen
		SDL_GL_SwapWindow(win.get());
	}

	close();

	SDL_GL_DeleteContext(glContext);

    return 0;
}

