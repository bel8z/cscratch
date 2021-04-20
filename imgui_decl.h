#ifndef IMGUI_DECL_H

//------------------------------------------------------------------------------
// Safely include cimgui.h with C declarations
//------------------------------------------------------------------------------

#if defined(_MSC_VER)
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wlanguage-extension-token"
#else
#pragma warning(push)
#pragma warning(disable : 4201)
#pragma warning(disable : 4214)
#endif // defined(__clang__)
#endif // defined(_MSC_VER)

#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#include "ext/cimgui/cimgui.h"

#if defined(_MSC_VER)
#if defined(__clang__)
#pragma clang diagnostic pop
#else
#pragma warning(pop)
#endif // defined(__clang__)
#endif // defined(_MSC_VER)

//------------------------------------------------------------------------------
// Some common gui extensions
//------------------------------------------------------------------------------

static inline bool
guiButton(char const *label)
{
    return igButton(label, (ImVec2){0, 0});
}

static inline void
guiSameLine()
{
    igSameLine(0.0f, -1.0f);
}

//------------------------------------------------------------------------------

#define IMGUI_DECL_H
#endif