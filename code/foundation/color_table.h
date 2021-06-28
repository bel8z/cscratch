#pragma once

#include "color.h"

#define COLOR_LIST(X)                        \
    X(ALICE_BLUE, 0xF0, 0xF8, 0xFF)          \
    X(ANTIQUE_WHITE, 0xFA, 0xEB, 0xD7)       \
    X(AQUA, 0x00, 0xFF, 0xFF)                \
    X(AQUAMARINE, 0x7F, 0xFF, 0xD4)          \
    X(AZURE, 0xF0, 0xFF, 0xFF)               \
    X(BEIGE, 0xF5, 0xF5, 0xDC)               \
    X(BISQUE, 0xFF, 0xE4, 0xC4)              \
    X(BLACK, 0x00, 0x00, 0x00)               \
    X(BLANCHED_ALMOND, 0xFF, 0xEB, 0xCD)     \
    X(BLUE, 0x00, 0x00, 0xFF)                \
    X(BLUE_VIOLET, 0x8A, 0x2B, 0xE2)         \
    X(BROWN, 0xA5, 0x2A, 0x2A)               \
    X(BURLYWOOD, 0xDE, 0xB8, 0x87)           \
    X(CADET_BLUE, 0x5F, 0x9E, 0xA0)          \
    X(CHARTREUSE, 0x7F, 0xFF, 0x00)          \
    X(CHOCOLATE, 0xD2, 0x69, 0x1E)           \
    X(CORAL, 0xFF, 0x7F, 0x50)               \
    X(CORNFLOWER_BLUE, 0x64, 0x95, 0xED)     \
    X(CORNSILK, 0xFF, 0xF8, 0xDC)            \
    X(CRIMSON, 0xDC, 0x14, 0x3C)             \
    X(CYAN, 0x00, 0xFF, 0xFF)                \
    X(DARK_BLUE, 0x00, 0x00, 0x8B)           \
    X(DARK_CYAN, 0x00, 0x8B, 0x8B)           \
    X(DARK_GOLDENROD, 0xB8, 0x86, 0x0B)      \
    X(DARK_GRAY, 0xA9, 0xA9, 0xA9)           \
    X(DARK_GREEN, 0x00, 0x64, 0x00)          \
    X(DARK_KHAKI, 0xBD, 0xB7, 0x6B)          \
    X(DARK_MAGENTA, 0x8B, 0x00, 0x8B)        \
    X(DARK_OLIVE_GREEN, 0x55, 0x6B, 0x2F)    \
    X(DARK_ORANGE, 0xFF, 0x8C, 0x00)         \
    X(DARK_ORCHID, 0x99, 0x32, 0xCC)         \
    X(DARK_RED, 0x8B, 0x00, 0x00)            \
    X(DARK_SALMON, 0xE9, 0x96, 0x7A)         \
    X(DARK_SEA_GREEN, 0x8F, 0xBC, 0x8F)      \
    X(DARK_SILVER, 0xAF, 0xAF, 0xAF)         \
    X(DARK_SLATE_BLUE, 0x48, 0x3D, 0x8B)     \
    X(DARK_SLATE_GRAY, 0x2F, 0x4F, 0x4F)     \
    X(DARK_TURQUOISE, 0x00, 0xCE, 0xD1)      \
    X(DARK_VIOLET, 0x94, 0x00, 0xD3)         \
    X(DEEP_PINK, 0xFF, 0x14, 0x93)           \
    X(DEEP_SKY_BLUE, 0x00, 0xBF, 0xFF)       \
    X(DIM_GRAY, 0x69, 0x69, 0x69)            \
    X(DODGER_BLUE, 0x1E, 0x90, 0xFF)         \
    X(FIREBRICK, 0xB2, 0x22, 0x22)           \
    X(FLORAL_WHITE, 0xFF, 0xFA, 0xF0)        \
    X(FOREST_GREEN, 0x22, 0x8B, 0x22)        \
    X(FUCHSIA, 0xFF, 0x00, 0xFF)             \
    X(GAINSBORO, 0xDC, 0xDC, 0xDC)           \
    X(GHOST_WHITE, 0xF8, 0xF8, 0xFF)         \
    X(GOLD, 0xFF, 0xD7, 0x00)                \
    X(GOLDENROD, 0xDA, 0xA5, 0x20)           \
    X(GRAY, 0xBE, 0xBE, 0xBE)                \
    X(WEB_GRAY, 0x80, 0x80, 0x80)            \
    X(GREEN, 0x00, 0xFF, 0x00)               \
    X(WEB_GREEN, 0x00, 0x80, 0x00)           \
    X(GREEN_YELLOW, 0xAD, 0xFF, 0x2F)        \
    X(HONEYDEW, 0xF0, 0xFF, 0xF0)            \
    X(HOT_PINK, 0xFF, 0x69, 0xB4)            \
    X(INDIAN_RED, 0xCD, 0x5C, 0x5C)          \
    X(INDIGO, 0x4B, 0x00, 0x82)              \
    X(IVORY, 0xFF, 0xFF, 0xF0)               \
    X(KHAKI, 0xF0, 0xE6, 0x8C)               \
    X(LAVENDER, 0xE6, 0xE6, 0xFA)            \
    X(LAVENDER_BLUSH, 0xFF, 0xF0, 0xF5)      \
    X(LAWN_GREEN, 0x7C, 0xFC, 0x00)          \
    X(LEMON_CHIFFON, 0xFF, 0xFA, 0xCD)       \
    X(LIGHT_BLUE, 0xAD, 0xD8, 0xE6)          \
    X(LIGHT_CORAL, 0xF0, 0x80, 0x80)         \
    X(LIGHT_CYAN, 0xE0, 0xFF, 0xFF)          \
    X(LIGHT_GOLDENROD, 0xFA, 0xFA, 0xD2)     \
    X(LIGHT_GRAY, 0xD3, 0xD3, 0xD3)          \
    X(LIGHT_GREEN, 0x90, 0xEE, 0x90)         \
    X(LIGHT_PINK, 0xFF, 0xB6, 0xC1)          \
    X(LIGHT_SALMON, 0xFF, 0xA0, 0x7A)        \
    X(LIGHT_SEA_GREEN, 0x20, 0xB2, 0xAA)     \
    X(LIGHT_SKY_BLUE, 0x87, 0xCE, 0xFA)      \
    X(LIGHT_SLATE_GRAY, 0x77, 0x88, 0x99)    \
    X(LIGHT_STEEL_BLUE, 0xB0, 0xC4, 0xDE)    \
    X(LIGHT_YELLOW, 0xFF, 0xFF, 0xE0)        \
    X(LIME, 0x00, 0xFF, 0x00)                \
    X(LIME_GREEN, 0x32, 0xCD, 0x32)          \
    X(LINEN, 0xFA, 0xF0, 0xE6)               \
    X(MAGENTA, 0xFF, 0x00, 0xFF)             \
    X(MAROON, 0xB0, 0x30, 0x60)              \
    X(WEB_MAROON, 0x80, 0x00, 0x00)          \
    X(MEDIUM_AQUAMARINE, 0x66, 0xCD, 0xAA)   \
    X(MEDIUM_BLUE, 0x00, 0x00, 0xCD)         \
    X(MEDIUM_ORCHID, 0xBA, 0x55, 0xD3)       \
    X(MEDIUM_PURPLE, 0x93, 0x70, 0xDB)       \
    X(MEDIUM_SEA_GREEN, 0x3C, 0xB3, 0x71)    \
    X(MEDIUM_SLATE_BLUE, 0x7B, 0x68, 0xEE)   \
    X(MEDIUM_SPRING_GREEN, 0x00, 0xFA, 0x9A) \
    X(MEDIUM_TURQUOISE, 0x48, 0xD1, 0xCC)    \
    X(MEDIUM_VIOLET_RED, 0xC7, 0x15, 0x85)   \
    X(MIDNIGHT_BLUE, 0x19, 0x19, 0x70)       \
    X(MINT_CREAM, 0xF5, 0xFF, 0xFA)          \
    X(MISTY_ROSE, 0xFF, 0xE4, 0xE1)          \
    X(MOCCASIN, 0xFF, 0xE4, 0xB5)            \
    X(NAVAJO_WHITE, 0xFF, 0xDE, 0xAD)        \
    X(NAVY_BLUE, 0x00, 0x00, 0x80)           \
    X(OLD_LACE, 0xFD, 0xF5, 0xE6)            \
    X(OLIVE, 0x80, 0x80, 0x00)               \
    X(OLIVE_DRAB, 0x6B, 0x8E, 0x23)          \
    X(ORANGE, 0xFF, 0xA5, 0x00)              \
    X(ORANGE_RED, 0xFF, 0x45, 0x00)          \
    X(ORCHID, 0xDA, 0x70, 0xD6)              \
    X(PALE_GOLDENROD, 0xEE, 0xE8, 0xAA)      \
    X(PALE_GREEN, 0x98, 0xFB, 0x98)          \
    X(PALE_TURQUOISE, 0xAF, 0xEE, 0xEE)      \
    X(PALE_VIOLET_RED, 0xDB, 0x70, 0x93)     \
    X(PAPAYA_WHIP, 0xFF, 0xEF, 0xD5)         \
    X(PEACH_PUFF, 0xFF, 0xDA, 0xB9)          \
    X(PERU, 0xCD, 0x85, 0x3F)                \
    X(PINK, 0xFF, 0xC0, 0xCB)                \
    X(PLUM, 0xDD, 0xA0, 0xDD)                \
    X(POWDER_BLUE, 0xB0, 0xE0, 0xE6)         \
    X(PURPLE, 0xA0, 0x20, 0xF0)              \
    X(WEB_PURPLE, 0x80, 0x00, 0x80)          \
    X(REBECCA_PURPLE, 0x66, 0x33, 0x99)      \
    X(RED, 0xFF, 0x00, 0x00)                 \
    X(ROSY_BROWN, 0xBC, 0x8F, 0x8F)          \
    X(ROYAL_BLUE, 0x41, 0x69, 0xE1)          \
    X(SADDLE_BROWN, 0x8B, 0x45, 0x13)        \
    X(SALMON, 0xFA, 0x80, 0x72)              \
    X(SANDY_BROWN, 0xF4, 0xA4, 0x60)         \
    X(SEA_GREEN, 0x2E, 0x8B, 0x57)           \
    X(SEASHELL, 0xFF, 0xF5, 0xEE)            \
    X(SIENNA, 0xA0, 0x52, 0x2D)              \
    X(SILVER, 0xC0, 0xC0, 0xC0)              \
    X(SKY_BLUE, 0x87, 0xCE, 0xEB)            \
    X(SLATE_BLUE, 0x6A, 0x5A, 0xCD)          \
    X(SLATE_GRAY, 0x70, 0x80, 0x90)          \
    X(SNOW, 0xFF, 0xFA, 0xFA)                \
    X(SPRING_GREEN, 0x00, 0xFF, 0x7F)        \
    X(STEEL_BLUE, 0x46, 0x82, 0xB4)          \
    X(TAN, 0xD2, 0xB4, 0x8C)                 \
    X(TEAL, 0x00, 0x80, 0x80)                \
    X(THISTLE, 0xD8, 0xBF, 0xD8)             \
    X(TOMATO, 0xFF, 0x63, 0x47)              \
    X(TURQUOISE, 0x40, 0xE0, 0xD0)           \
    X(VIOLET, 0xEE, 0x82, 0xEE)              \
    X(WHEAT, 0xF5, 0xDE, 0xB3)               \
    X(WHITE, 0xFF, 0xFF, 0xFF)               \
    X(WHITE_SMOKE, 0xF5, 0xF5, 0xF5)         \
    X(YELLOW, 0xFF, 0xFF, 0x00)              \
    X(YELLOW_GREEN, 0x9A, 0xCD, 0x32)

enum
{
#define COLOR_ENUM_ENTRY(name, ...) RGBA32__##name,
    COLOR_LIST(COLOR_ENUM_ENTRY)
#undef COLOR_ENUM_ENTRY
};

#define COLOR_STRING(name, ...) CF_STRINGIFY(name),
static const char *g_color_names[] = {COLOR_LIST(COLOR_STRING)};
#undef COLOR_STRING

#define COLOR_VALUE(name, r, g, b) [RGBA32__##name] = RGBA32_SOLID(r, g, b),
static const Rgba32 g_color_values[] = {COLOR_LIST(COLOR_VALUE)};
#undef COLOR_VALUE

#define RGBA32_ALICE_BLUE g_color_values[RGBA32__ALICE_BLUE]
#define RGBA32_ANTIQUE_WHITE g_color_values[RGBA32__ANTIQUE_WHITE]
#define RGBA32_AQUA g_color_values[RGBA32__AQUA]
#define RGBA32_AQUAMARINE g_color_values[RGBA32__AQUAMARINE]
#define RGBA32_AZURE g_color_values[RGBA32__AZURE]
#define RGBA32_BEIGE g_color_values[RGBA32__BEIGE]
#define RGBA32_BISQUE g_color_values[RGBA32__BISQUE]
#define RGBA32_BLACK g_color_values[RGBA32__BLACK]
#define RGBA32_BLANCHED_ALMOND g_color_values[RGBA32__BLANCHED_ALMOND]
#define RGBA32_BLUE g_color_values[RGBA32__BLUE]
#define RGBA32_BLUE_VIOLET g_color_values[RGBA32__BLUE_VIOLET]
#define RGBA32_BROWN g_color_values[RGBA32__BROWN]
#define RGBA32_BURLYWOOD g_color_values[RGBA32__BURLYWOOD]
#define RGBA32_CADET_BLUE g_color_values[RGBA32__CADET_BLUE]
#define RGBA32_CHARTREUSE g_color_values[RGBA32__CHARTREUSE]
#define RGBA32_CHOCOLATE g_color_values[RGBA32__CHOCOLATE]
#define RGBA32_CORAL g_color_values[RGBA32__CORAL]
#define RGBA32_CORNFLOWER_BLUE g_color_values[RGBA32__CORNFLOWER_BLUE]
#define RGBA32_CORNSILK g_color_values[RGBA32__CORNSILK]
#define RGBA32_CRIMSON g_color_values[RGBA32__CRIMSON]
#define RGBA32_CYAN g_color_values[RGBA32__CYAN]
#define RGBA32_DARK_BLUE g_color_values[RGBA32__DARK_BLUE]
#define RGBA32_DARK_CYAN g_color_values[RGBA32__DARK_CYAN]
#define RGBA32_DARK_GOLDENROD g_color_values[RGBA32__DARK_GOLDENROD]
#define RGBA32_DARK_GRAY g_color_values[RGBA32__DARK_GRAY]
#define RGBA32_DARK_GREEN g_color_values[RGBA32__DARK_GREEN]
#define RGBA32_DARK_KHAKI g_color_values[RGBA32__DARK_KHAKI]
#define RGBA32_DARK_MAGENTA g_color_values[RGBA32__DARK_MAGENTA]
#define RGBA32_DARK_OLIVE_GREEN g_color_values[RGBA32__DARK_OLIVE_GREEN]
#define RGBA32_DARK_ORANGE g_color_values[RGBA32__DARK_ORANGE]
#define RGBA32_DARK_ORCHID g_color_values[RGBA32__DARK_ORCHID]
#define RGBA32_DARK_RED g_color_values[RGBA32__DARK_RED]
#define RGBA32_DARK_SALMON g_color_values[RGBA32__DARK_SALMON]
#define RGBA32_DARK_SEA_GREEN g_color_values[RGBA32__DARK_SEA_GREEN]
#define RGBA32_DARK_SILVER g_color_values[RGBA32__DARK_SILVER]
#define RGBA32_DARK_SLATE_BLUE g_color_values[RGBA32__DARK_SLATE_BLUE]
#define RGBA32_DARK_SLATE_GRAY g_color_values[RGBA32__DARK_SLATE_GRAY]
#define RGBA32_DARK_TURQUOISE g_color_values[RGBA32__DARK_TURQUOISE]
#define RGBA32_DARK_VIOLET g_color_values[RGBA32__DARK_VIOLET]
#define RGBA32_DEEP_PINK g_color_values[RGBA32__DEEP_PINK]
#define RGBA32_DEEP_SKY_BLUE g_color_values[RGBA32__DEEP_SKY_BLUE]
#define RGBA32_DIM_GRAY g_color_values[RGBA32__DIM_GRAY]
#define RGBA32_DODGER_BLUE g_color_values[RGBA32__DODGER_BLUE]
#define RGBA32_FIREBRICK g_color_values[RGBA32__FIREBRICK]
#define RGBA32_FLORAL_WHITE g_color_values[RGBA32__FLORAL_WHITE]
#define RGBA32_FOREST_GREEN g_color_values[RGBA32__FOREST_GREEN]
#define RGBA32_FUCHSIA g_color_values[RGBA32__FUCHSIA]
#define RGBA32_GAINSBORO g_color_values[RGBA32__GAINSBORO]
#define RGBA32_GHOST_WHITE g_color_values[RGBA32__GHOST_WHITE]
#define RGBA32_GOLD g_color_values[RGBA32__GOLD]
#define RGBA32_GOLDENROD g_color_values[RGBA32__GOLDENROD]
#define RGBA32_GRAY g_color_values[RGBA32__GRAY]
#define RGBA32_WEB_GRAY g_color_values[RGBA32__WEB_GRAY]
#define RGBA32_GREEN g_color_values[RGBA32__GREEN]
#define RGBA32_WEB_GREEN g_color_values[RGBA32__WEB_GREEN]
#define RGBA32_GREEN_YELLOW g_color_values[RGBA32__GREEN_YELLOW]
#define RGBA32_HONEYDEW g_color_values[RGBA32__HONEYDEW]
#define RGBA32_HOT_PINK g_color_values[RGBA32__HOT_PINK]
#define RGBA32_INDIAN_RED g_color_values[RGBA32__INDIAN_RED]
#define RGBA32_INDIGO g_color_values[RGBA32__INDIGO]
#define RGBA32_IVORY g_color_values[RGBA32__IVORY]
#define RGBA32_KHAKI g_color_values[RGBA32__KHAKI]
#define RGBA32_LAVENDER g_color_values[RGBA32__LAVENDER]
#define RGBA32_LAVENDER_BLUSH g_color_values[RGBA32__LAVENDER_BLUSH]
#define RGBA32_LAWN_GREEN g_color_values[RGBA32__LAWN_GREEN]
#define RGBA32_LEMON_CHIFFON g_color_values[RGBA32__LEMON_CHIFFON]
#define RGBA32_LIGHT_BLUE g_color_values[RGBA32__LIGHT_BLUE]
#define RGBA32_LIGHT_CORAL g_color_values[RGBA32__LIGHT_CORAL]
#define RGBA32_LIGHT_CYAN g_color_values[RGBA32__LIGHT_CYAN]
#define RGBA32_LIGHT_GOLDENROD g_color_values[RGBA32__LIGHT_GOLDENROD]
#define RGBA32_LIGHT_GRAY g_color_values[RGBA32__LIGHT_GRAY]
#define RGBA32_LIGHT_GREEN g_color_values[RGBA32__LIGHT_GREEN]
#define RGBA32_LIGHT_PINK g_color_values[RGBA32__LIGHT_PINK]
#define RGBA32_LIGHT_SALMON g_color_values[RGBA32__LIGHT_SALMON]
#define RGBA32_LIGHT_SEA_GREEN g_color_values[RGBA32__LIGHT_SEA_GREEN]
#define RGBA32_LIGHT_SKY_BLUE g_color_values[RGBA32__LIGHT_SKY_BLUE]
#define RGBA32_LIGHT_SLATE_GRAY g_color_values[RGBA32__LIGHT_SLATE_GRAY]
#define RGBA32_LIGHT_STEEL_BLUE g_color_values[RGBA32__LIGHT_STEEL_BLUE]
#define RGBA32_LIGHT_YELLOW g_color_values[RGBA32__LIGHT_YELLOW]
#define RGBA32_LIME g_color_values[RGBA32__LIME]
#define RGBA32_LIME_GREEN g_color_values[RGBA32__LIME_GREEN]
#define RGBA32_LINEN g_color_values[RGBA32__LINEN]
#define RGBA32_MAGENTA g_color_values[RGBA32__MAGENTA]
#define RGBA32_MAROON g_color_values[RGBA32__MAROON]
#define RGBA32_WEB_MAROON g_color_values[RGBA32__WEB_MAROON]
#define RGBA32_MEDIUM_AQUAMARINE g_color_values[RGBA32__MEDIUM_AQUAMARINE]
#define RGBA32_MEDIUM_BLUE g_color_values[RGBA32__MEDIUM_BLUE]
#define RGBA32_MEDIUM_ORCHID g_color_values[RGBA32__MEDIUM_ORCHID]
#define RGBA32_MEDIUM_PURPLE g_color_values[RGBA32__MEDIUM_PURPLE]
#define RGBA32_MEDIUM_SEA_GREEN g_color_values[RGBA32__MEDIUM_SEA_GREEN]
#define RGBA32_MEDIUM_SLATE_BLUE g_color_values[RGBA32__MEDIUM_SLATE_BLUE]
#define RGBA32_MEDIUM_SPRING_GREEN g_color_values[RGBA32__MEDIUM_SPRING_GREEN]
#define RGBA32_MEDIUM_TURQUOISE g_color_values[RGBA32__MEDIUM_TURQUOISE]
#define RGBA32_MEDIUM_VIOLET_RED g_color_values[RGBA32__MEDIUM_VIOLET_RED]
#define RGBA32_MIDNIGHT_BLUE g_color_values[RGBA32__MIDNIGHT_BLUE]
#define RGBA32_MINT_CREAM g_color_values[RGBA32__MINT_CREAM]
#define RGBA32_MISTY_ROSE g_color_values[RGBA32__MISTY_ROSE]
#define RGBA32_MOCCASIN g_color_values[RGBA32__MOCCASIN]
#define RGBA32_NAVAJO_WHITE g_color_values[RGBA32__NAVAJO_WHITE]
#define RGBA32_NAVY_BLUE g_color_values[RGBA32__NAVY_BLUE]
#define RGBA32_OLD_LACE g_color_values[RGBA32__OLD_LACE]
#define RGBA32_OLIVE g_color_values[RGBA32__OLIVE]
#define RGBA32_OLIVE_DRAB g_color_values[RGBA32__OLIVE_DRAB]
#define RGBA32_ORANGE g_color_values[RGBA32__ORANGE]
#define RGBA32_ORANGE_RED g_color_values[RGBA32__ORANGE_RED]
#define RGBA32_ORCHID g_color_values[RGBA32__ORCHID]
#define RGBA32_PALE_GOLDENROD g_color_values[RGBA32__PALE_GOLDENROD]
#define RGBA32_PALE_GREEN g_color_values[RGBA32__PALE_GREEN]
#define RGBA32_PALE_TURQUOISE g_color_values[RGBA32__PALE_TURQUOISE]
#define RGBA32_PALE_VIOLET_RED g_color_values[RGBA32__PALE_VIOLET_RED]
#define RGBA32_PAPAYA_WHIP g_color_values[RGBA32__PAPAYA_WHIP]
#define RGBA32_PEACH_PUFF g_color_values[RGBA32__PEACH_PUFF]
#define RGBA32_PERU g_color_values[RGBA32__PERU]
#define RGBA32_PINK g_color_values[RGBA32__PINK]
#define RGBA32_PLUM g_color_values[RGBA32__PLUM]
#define RGBA32_POWDER_BLUE g_color_values[RGBA32__POWDER_BLUE]
#define RGBA32_PURPLE g_color_values[RGBA32__PURPLE]
#define RGBA32_WEB_PURPLE g_color_values[RGBA32__WEB_PURPLE]
#define RGBA32_REBECCA_PURPLE g_color_values[RGBA32__REBECCA_PURPLE]
#define RGBA32_RED g_color_values[RGBA32__RED]
#define RGBA32_ROSY_BROWN g_color_values[RGBA32__ROSY_BROWN]
#define RGBA32_ROYAL_BLUE g_color_values[RGBA32__ROYAL_BLUE]
#define RGBA32_SADDLE_BROWN g_color_values[RGBA32__SADDLE_BROWN]
#define RGBA32_SALMON g_color_values[RGBA32__SALMON]
#define RGBA32_SANDY_BROWN g_color_values[RGBA32__SANDY_BROWN]
#define RGBA32_SEA_GREEN g_color_values[RGBA32__SEA_GREEN]
#define RGBA32_SEASHELL g_color_values[RGBA32__SEASHELL]
#define RGBA32_SIENNA g_color_values[RGBA32__SIENNA]
#define RGBA32_SILVER g_color_values[RGBA32__SILVER]
#define RGBA32_SKY_BLUE g_color_values[RGBA32__SKY_BLUE]
#define RGBA32_SLATE_BLUE g_color_values[RGBA32__SLATE_BLUE]
#define RGBA32_SLATE_GRAY g_color_values[RGBA32__SLATE_GRAY]
#define RGBA32_SNOW g_color_values[RGBA32__SNOW]
#define RGBA32_SPRING_GREEN g_color_values[RGBA32__SPRING_GREEN]
#define RGBA32_STEEL_BLUE g_color_values[RGBA32__STEEL_BLUE]
#define RGBA32_TAN g_color_values[RGBA32__TAN]
#define RGBA32_TEAL g_color_values[RGBA32__TEAL]
#define RGBA32_THISTLE g_color_values[RGBA32__THISTLE]
#define RGBA32_TOMATO g_color_values[RGBA32__TOMATO]
#define RGBA32_TURQUOISE g_color_values[RGBA32__TURQUOISE]
#define RGBA32_VIOLET g_color_values[RGBA32__VIOLET]
#define RGBA32_WHEAT g_color_values[RGBA32__WHEAT]
#define RGBA32_WHITE g_color_values[RGBA32__WHITE]
#define RGBA32_WHITE_SMOKE g_color_values[RGBA32__WHITE_SMOKE]
#define RGBA32_YELLOW g_color_values[RGBA32__YELLOW]
#define RGBA32_YELLOW_GREEN g_color_values[RGBA32__YELLOW_GREEN]
