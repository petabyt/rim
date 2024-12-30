#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#undef IMGUI_HAS_DOCK
#include "cimgui/cimgui.h"
#include "./cimgui/generator/output/cimgui_impl.h"
#include <stdio.h>
#define SDL_MAIN_HANDLED
#include <SDL.h>
#ifdef _MSC_VER
#include <windows.h>
#endif
#include <GL/gl.h>
#include <GL/glu.h>

SDL_Window *window = NULL;

struct Context {
	SDL_GLContext gl_context;
	ImGuiIO *ioptr;
};

void *imgui_init(void) {
	struct Context *ctx = malloc(sizeof(struct Context));
	if (SDL_Init(SDL_INIT_VIDEO) < 0) {
	  SDL_Log("failed to init: %s", SDL_GetError());
	  return NULL;
	}
 
	// Decide GL+GLSL versions
#if __APPLE__
	  // GL 3.2 Core + GLSL 150
	  const char* glsl_version = "#version 150";
	  SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG); // Always required on Mac
	  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
#else
	  // GL 3.0 + GLSL 130
	  const char* glsl_version = "#version 130";
	  SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
	  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#endif

	// and prepare OpenGL stuff
	SDL_SetHint(SDL_HINT_RENDER_DRIVER, "opengl");
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_DisplayMode current;
	SDL_GetCurrentDisplayMode(0, &current);
	
	window = SDL_CreateWindow(
	    "Hello", 0, 0, 1024, 768,
	    SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE
	    );
	if (window == NULL) {
	  SDL_Log("Failed to create window: %s", SDL_GetError());
	  return NULL;
	}

	ctx->gl_context = SDL_GL_CreateContext(window);
	SDL_GL_SetSwapInterval(1);  // enable vsync


	// check opengl version sdl uses
	SDL_Log("opengl version: %s", (char*)glGetString(GL_VERSION));

	// setup imgui
	igCreateContext(NULL);
	
	//set docking
	ctx->ioptr = igGetIO();
	ctx->ioptr->ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;       // Enable Keyboard Controls
	//ioptr->ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
#ifdef IMGUI_HAS_DOCK
	ctx->ioptr->ConfigFlags |= ImGuiConfigFlags_DockingEnable;           // Enable Docking
	ctx->ioptr->ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;         // Enable Multi-Viewport / Platform Windows
#endif
	
	ImGui_ImplSDL2_InitForOpenGL(window, ctx->gl_context);
	ImGui_ImplOpenGL3_Init(glsl_version);
	return ctx;
}

int imgui_while_open(void *priv) {
	struct Context *ctx = ctx;
	SDL_Event e;

	// we need to call SDL_PollEvent to let window rendered, otherwise
	// no window will be shown
	while (SDL_PollEvent(&e) != 0)
	{
	  ImGui_ImplSDL2_ProcessEvent(&e);
	  if (e.type == SDL_QUIT)
	    return 0;
	  if (e.type == SDL_WINDOWEVENT && e.window.event == SDL_WINDOWEVENT_CLOSE && e.window.windowID == SDL_GetWindowID(window))
	    return 0;
	}
	
	// start imgui frame
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplSDL2_NewFrame();
	igNewFrame();
	return 1;
}

void imgui_end(void *priv) {
	struct Context *ctx = ctx;
	// render
	igRender();
	SDL_GL_MakeCurrent(window, ctx->gl_context);
	glViewport(0, 0, (int)ctx->ioptr->DisplaySize.x, (int)ctx->ioptr->DisplaySize.y);
//	glClearColor(clearColor.x, clearColor.y, clearColor.z, clearColor.w);
	glClear(GL_COLOR_BUFFER_BIT);
	ImGui_ImplOpenGL3_RenderDrawData(igGetDrawData());
#ifdef IMGUI_HAS_DOCK
	if (ctx->ioptr->ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
	    {
	        SDL_Window* backup_current_window = SDL_GL_GetCurrentWindow();
	        SDL_GLContext backup_current_context = SDL_GL_GetCurrentContext();
	        igUpdatePlatformWindows();
	        igRenderPlatformWindowsDefault(NULL,NULL);
	        SDL_GL_MakeCurrent(backup_current_window, backup_current_context);
	    }
#endif
	SDL_GL_SwapWindow(window);
}

void imgui_close(void *priv) {
	struct Context *ctx = ctx;
	// clean up
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplSDL2_Shutdown();
	igDestroyContext(NULL);

	SDL_GL_DeleteContext(ctx->gl_context);
	if (window != NULL)
	{
		SDL_DestroyWindow(window);
		window = NULL;
	}
	SDL_Quit();
}

//int main() {
//	void *ctx = imgui_init();
//
//	while (imgui_while_open(ctx)) {
//		imgui_end();
//	}
//
//	return 0;
//}
