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
#include "cimgui.h"

#if defined(_MSC_VER)
#if defined(__clang__)
#pragma clang diagnostic pop
#else
#pragma warning(pop)
#endif // defined(__clang__)
#endif // defined(_MSC_VER)

#include "foundation/vec.h"

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

static inline ImVec2
guiFromV2(Vec2 v)
{
    return (ImVec2){.x = v.x, .y = v.y};
}

static inline Vec2
guiToV2(ImVec2 v)
{
    return (Vec2){.x = v.x, .y = v.y};
}

static inline ImVec4
guiFromV4(Vec4 v)
{
    return (ImVec4){.x = v.x, .y = v.y, .z = v.z, .w = v.w};
}

static inline Vec4
guiToV4(ImVec4 v)
{
    return (Vec4){.x = v.x, .y = v.y, .z = v.z, .w = v.w};
}

//------------------------------------------------------------------------------

#define IMGUI_DECL_H
#endif
