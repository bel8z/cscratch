#pragma once

#include "gui.h"

#if !defined(__cplusplus)
#    error "This is a C++ header"
#endif

CF_DIAGNOSTIC_PUSH()
CF_DIAGNOSTIC_IGNORE_CLANG("-Wlanguage-extension-token")
CF_DIAGNOSTIC_IGNORE_CLANG("-Wdouble-promotion")
CF_DIAGNOSTIC_IGNORE_MSVC(4201)
CF_DIAGNOSTIC_IGNORE_MSVC(4214)

#include "imgui.h"
#include "imgui_internal.h"
#if defined(IMGUI_ENABLE_FREETYPE)
#    include "imgui_freetype.h"
#endif
#include "implot.h"

CF_DIAGNOSTIC_POP()

struct GuiData
{
    MemAllocator allocator;
    GuiMemory memory;

    void *main_window;
    void *user_data;

    ImPlotContext *implot;

    GuiTheme theme;
};

void *guiAlloc(Usize size, void *state);
void guiFree(void *mem, void *state);
GuiData &guiData();
