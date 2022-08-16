#pragma once

#include "gui.h"

typedef struct ImDrawData GuiDrawData;

typedef struct GuiOpenGLVersion
{
    U32 major, minor;
    /// Shader version for Dear Imgui OpenGL backend
    U32 glsl;
} GuiOpenGLVersion;

// Backend API
CF_API bool guiGl3Init(GuiOpenGLVersion version);
CF_API void guiGl3Shutdown(void);
CF_API void guiGl3NewFrame(void);
CF_API void guiGl3Render(GuiDrawData *draw_data);

// Utility
CF_API void guiGl3UpdateFontsTexture(void);
