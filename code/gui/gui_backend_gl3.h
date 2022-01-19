#pragma once

#include "foundation/core.h"

typedef struct ImDrawData GuiDrawData;

typedef struct GlVersion
{
    I32 major, minor;
    /// Shader version for Dear Imgui OpenGL backend
    Cstr glsl;
} GlVersion;

// Backend API
CF_API bool guiGl3Init(GlVersion version);
CF_API void guiGl3Shutdown();
CF_API void guiGl3NewFrame();
CF_API void guiGl3Render(GuiDrawData *draw_data);

// (Optional) Called by Init/NewFrame/Shutdown
CF_API void guiGl3CreateFontsTexture();
CF_API void guiGl3DeleteFontsTexture();
CF_API bool guiGl3CreateDeviceObjects();
CF_API void guiGl3DeleteDeviceObjects();
