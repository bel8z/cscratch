#pragma once

#include "gui.h"

typedef struct ImDrawData GuiDrawData;

// Backend API
CF_API bool guiGl3Init(GuiOpenGLVersion version);
CF_API void guiGl3Shutdown();
CF_API void guiGl3NewFrame();
CF_API void guiGl3Render(GuiDrawData *draw_data);

// Utility
CF_API void guiGl3UpdateFontsTexture();
