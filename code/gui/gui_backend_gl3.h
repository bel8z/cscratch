#pragma once

#include "gui.h"

typedef struct ImDrawData GuiDrawData;

// Backend API
CF_API bool guiGl3Init(GuiOpenGLVersion version);
CF_API void guiGl3Shutdown();
CF_API void guiGl3NewFrame();
CF_API void guiGl3Render(GuiDrawData *draw_data);

// (Optional) Called by Init/NewFrame/Shutdown
CF_API void guiGl3UpdateFontsTexture();
CF_API void guiGl3CreateFontsTexture();
CF_API void guiGl3DeleteFontsTexture();
CF_API bool guiGl3CreateDeviceObjects();
CF_API void guiGl3DeleteDeviceObjects();
