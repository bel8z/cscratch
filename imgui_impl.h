#ifndef IMGUI_IMPL_H

#include "foundation/common.h"

//------------------------------------------------------------------------------
// SDL2 backend declarations
//------------------------------------------------------------------------------

typedef struct SDL_Window SDL_Window;
typedef union SDL_Event SDL_Event;

extern bool ImGui_ImplSDL2_InitForOpenGL(SDL_Window *window, void *sdl_gl_context);
extern bool ImGui_ImplSDL2_InitForVulkan(SDL_Window *window);
extern bool ImGui_ImplSDL2_InitForD3D(SDL_Window *window);
extern bool ImGui_ImplSDL2_InitForMetal(SDL_Window *window);
extern void ImGui_ImplSDL2_Shutdown();
extern void ImGui_ImplSDL2_NewFrame(SDL_Window *window);
extern bool ImGui_ImplSDL2_ProcessEvent(const SDL_Event *event);

//------------------------------------------------------------------------------
// OpenGL3 backend declarations
//------------------------------------------------------------------------------

typedef struct ImDrawData ImDrawData;

extern bool ImGui_ImplOpenGL3_Init(const char *glsl_version);
extern void ImGui_ImplOpenGL3_Shutdown();
extern void ImGui_ImplOpenGL3_NewFrame();
extern void ImGui_ImplOpenGL3_RenderDrawData(ImDrawData *draw_data);
extern bool ImGui_ImplOpenGL3_CreateFontsTexture();
extern void ImGui_ImplOpenGL3_DestroyFontsTexture();
extern bool ImGui_ImplOpenGL3_CreateDeviceObjects();
extern void ImGui_ImplOpenGL3_DestroyDeviceObjects();

#define IMGUI_IMPL_H
#endif
