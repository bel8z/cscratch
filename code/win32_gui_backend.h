#pragma once

#include "gui/gui.h"

typedef struct ImDrawData GuiDrawData;

typedef struct OpenGLVersion
{
    U32 major, minor;
    /// Shader version for Dear Imgui OpenGL backend
    U32 glsl;
} OpenGLVersion;

// Backend API
CF_API bool win32GuiInit(void *window, OpenGLVersion version);
CF_API void win32GuiShutdown(void);
CF_API void win32GuiNewFrame(void);
CF_API void win32GuiRender(GuiDrawData *draw_data);

// Utility
CF_API F32 win32GuiGetDpiScale(void *window);
CF_API void win32GuiUpdateFontsTexture(void);
