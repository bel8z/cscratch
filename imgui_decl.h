#ifndef GUI_H

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wlanguage-extension-token"
#elif defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4201)
#pragma warning(disable : 4214)
#endif

#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#include "ext/cimgui/cimgui.h"

#ifdef __clang__
#pragma clang diagnostic pop
#elif defined(_MSC_VER)
#pragma warning(pop)
#endif

#define GUI_H
#endif
