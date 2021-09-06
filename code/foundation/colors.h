#pragma once

#include "core.h"

//--------------------------------------------------------------------------------------------------
// Packed RGBA 32 bit format implementation

#if CF_LITTLE_ENDIAN
// NOTE (Matteo): Technically, the integer content is ABGR, but the byte order on little endian
// systems is RGBA
#    define RGBA32_R_SHIFT 0
#    define RGBA32_G_SHIFT 8
#    define RGBA32_B_SHIFT 16
#    define RGBA32_A_SHIFT 24
#    define RGBA32_A_MASK 0xFF000000
#else
// TODO (Matteo): How does this interact with OpenGL/DearImgui?
#    define RGBA32_R_SHIFT 24
#    define RGBA32_G_SHIFT 16
#    define RGBA32_B_SHIFT 8
#    define RGBA32_A_SHIFT 0
#    define RGBA32_A_MASK 0x000000FF
#endif

#define RGBA32(R, G, B, A)                                               \
    (((Rgba32)(A) << RGBA32_A_SHIFT) | ((Rgba32)(B) << RGBA32_B_SHIFT) | \
     ((Rgba32)(G) << RGBA32_G_SHIFT) | ((Rgba32)(R) << RGBA32_R_SHIFT))

#define RGBA32_R(col) ((col >> RGBA32_R_SHIFT) & 0xFF)
#define RGBA32_G(col) ((col >> RGBA32_G_SHIFT) & 0xFF)
#define RGBA32_B(col) ((col >> RGBA32_B_SHIFT) & 0xFF)
#define RGBA32_A(col) ((col >> RGBA32_A_SHIFT) & 0xFF)

#define RGBA32_SOLID(R, G, B) RGBA32(R, G, B, 0xFF)

//--------------------------------------------------------------------------------------------------
// Common colors definition based on X11 names (see https://en.wikipedia.org/wiki/X11_color_names)

#define RGBA32_TRANSPARENT RGBA32(0x00, 0x00, 0x00, 0x00)

// clang-format off
#define RGBA32_ALICE_BLUE          RGBA32_SOLID(0xF0, 0xF8, 0xFF)
#define RGBA32_ANTIQUE_WHITE       RGBA32_SOLID(0xFA, 0xEB, 0xD7)
#define RGBA32_AQUA                RGBA32_SOLID(0x00, 0xFF, 0xFF)
#define RGBA32_AQUAMARINE          RGBA32_SOLID(0x7F, 0xFF, 0xD4)
#define RGBA32_AZURE               RGBA32_SOLID(0xF0, 0xFF, 0xFF)
#define RGBA32_BEIGE               RGBA32_SOLID(0xF5, 0xF5, 0xDC)
#define RGBA32_BISQUE              RGBA32_SOLID(0xFF, 0xE4, 0xC4)
#define RGBA32_BLACK               RGBA32_SOLID(0x00, 0x00, 0x00)
#define RGBA32_BLANCHED_ALMOND     RGBA32_SOLID(0xFF, 0xEB, 0xCD)
#define RGBA32_BLUE                RGBA32_SOLID(0x00, 0x00, 0xFF)
#define RGBA32_BLUE_VIOLET         RGBA32_SOLID(0x8A, 0x2B, 0xE2)
#define RGBA32_BROWN               RGBA32_SOLID(0xA5, 0x2A, 0x2A)
#define RGBA32_BURLYWOOD           RGBA32_SOLID(0xDE, 0xB8, 0x87)
#define RGBA32_CADET_BLUE          RGBA32_SOLID(0x5F, 0x9E, 0xA0)
#define RGBA32_CHARTREUSE          RGBA32_SOLID(0x7F, 0xFF, 0x00)
#define RGBA32_CHOCOLATE           RGBA32_SOLID(0xD2, 0x69, 0x1E)
#define RGBA32_CORAL               RGBA32_SOLID(0xFF, 0x7F, 0x50)
#define RGBA32_CORNFLOWER_BLUE     RGBA32_SOLID(0x64, 0x95, 0xED)
#define RGBA32_CORNSILK            RGBA32_SOLID(0xFF, 0xF8, 0xDC)
#define RGBA32_CRIMSON             RGBA32_SOLID(0xDC, 0x14, 0x3C)
#define RGBA32_CYAN                RGBA32_SOLID(0x00, 0xFF, 0xFF)
#define RGBA32_DARK_BLUE           RGBA32_SOLID(0x00, 0x00, 0x8B)
#define RGBA32_DARK_CYAN           RGBA32_SOLID(0x00, 0x8B, 0x8B)
#define RGBA32_DARK_GOLDENROD      RGBA32_SOLID(0xB8, 0x86, 0x0B)
#define RGBA32_DARK_GRAY           RGBA32_SOLID(0xA9, 0xA9, 0xA9)
#define RGBA32_DARK_GREEN          RGBA32_SOLID(0x00, 0x64, 0x00)
#define RGBA32_DARK_KHAKI          RGBA32_SOLID(0xBD, 0xB7, 0x6B)
#define RGBA32_DARK_MAGENTA        RGBA32_SOLID(0x8B, 0x00, 0x8B)
#define RGBA32_DARK_OLIVE_GREEN    RGBA32_SOLID(0x55, 0x6B, 0x2F)
#define RGBA32_DARK_ORANGE         RGBA32_SOLID(0xFF, 0x8C, 0x00)
#define RGBA32_DARK_ORCHID         RGBA32_SOLID(0x99, 0x32, 0xCC)
#define RGBA32_DARK_RED            RGBA32_SOLID(0x8B, 0x00, 0x00)
#define RGBA32_DARK_SALMON         RGBA32_SOLID(0xE9, 0x96, 0x7A)
#define RGBA32_DARK_SEA_GREEN      RGBA32_SOLID(0x8F, 0xBC, 0x8F)
#define RGBA32_DARK_SILVER         RGBA32_SOLID(0xAF, 0xAF, 0xAF)
#define RGBA32_DARK_SLATE_BLUE     RGBA32_SOLID(0x48, 0x3D, 0x8B)
#define RGBA32_DARK_SLATE_GRAY     RGBA32_SOLID(0x2F, 0x4F, 0x4F)
#define RGBA32_DARK_TURQUOISE      RGBA32_SOLID(0x00, 0xCE, 0xD1)
#define RGBA32_DARK_VIOLET         RGBA32_SOLID(0x94, 0x00, 0xD3)
#define RGBA32_DEEP_PINK           RGBA32_SOLID(0xFF, 0x14, 0x93)
#define RGBA32_DEEP_SKY_BLUE       RGBA32_SOLID(0x00, 0xBF, 0xFF)
#define RGBA32_DIM_GRAY            RGBA32_SOLID(0x69, 0x69, 0x69)
#define RGBA32_DODGER_BLUE         RGBA32_SOLID(0x1E, 0x90, 0xFF)
#define RGBA32_FIREBRICK           RGBA32_SOLID(0xB2, 0x22, 0x22)
#define RGBA32_FLORAL_WHITE        RGBA32_SOLID(0xFF, 0xFA, 0xF0)
#define RGBA32_FOREST_GREEN        RGBA32_SOLID(0x22, 0x8B, 0x22)
#define RGBA32_FUCHSIA             RGBA32_SOLID(0xFF, 0x00, 0xFF)
#define RGBA32_GAINSBORO           RGBA32_SOLID(0xDC, 0xDC, 0xDC)
#define RGBA32_GHOST_WHITE         RGBA32_SOLID(0xF8, 0xF8, 0xFF)
#define RGBA32_GOLD                RGBA32_SOLID(0xFF, 0xD7, 0x00)
#define RGBA32_GOLDENROD           RGBA32_SOLID(0xDA, 0xA5, 0x20)
#define RGBA32_GRAY                RGBA32_SOLID(0xBE, 0xBE, 0xBE)
#define RGBA32_WEB_GRAY            RGBA32_SOLID(0x80, 0x80, 0x80)
#define RGBA32_GREEN               RGBA32_SOLID(0x00, 0xFF, 0x00)
#define RGBA32_WEB_GREEN           RGBA32_SOLID(0x00, 0x80, 0x00)
#define RGBA32_GREEN_YELLOW        RGBA32_SOLID(0xAD, 0xFF, 0x2F)
#define RGBA32_HONEYDEW            RGBA32_SOLID(0xF0, 0xFF, 0xF0)
#define RGBA32_HOT_PINK            RGBA32_SOLID(0xFF, 0x69, 0xB4)
#define RGBA32_INDIAN_RED          RGBA32_SOLID(0xCD, 0x5C, 0x5C)
#define RGBA32_INDIGO              RGBA32_SOLID(0x4B, 0x00, 0x82)
#define RGBA32_IVORY               RGBA32_SOLID(0xFF, 0xFF, 0xF0)
#define RGBA32_KHAKI               RGBA32_SOLID(0xF0, 0xE6, 0x8C)
#define RGBA32_LAVENDER            RGBA32_SOLID(0xE6, 0xE6, 0xFA)
#define RGBA32_LAVENDER_BLUSH      RGBA32_SOLID(0xFF, 0xF0, 0xF5)
#define RGBA32_LAWN_GREEN          RGBA32_SOLID(0x7C, 0xFC, 0x00)
#define RGBA32_LEMON_CHIFFON       RGBA32_SOLID(0xFF, 0xFA, 0xCD)
#define RGBA32_LIGHT_BLUE          RGBA32_SOLID(0xAD, 0xD8, 0xE6)
#define RGBA32_LIGHT_CORAL         RGBA32_SOLID(0xF0, 0x80, 0x80)
#define RGBA32_LIGHT_CYAN          RGBA32_SOLID(0xE0, 0xFF, 0xFF)
#define RGBA32_LIGHT_GOLDENROD     RGBA32_SOLID(0xFA, 0xFA, 0xD2)
#define RGBA32_LIGHT_GRAY          RGBA32_SOLID(0xD3, 0xD3, 0xD3)
#define RGBA32_LIGHT_GREEN         RGBA32_SOLID(0x90, 0xEE, 0x90)
#define RGBA32_LIGHT_PINK          RGBA32_SOLID(0xFF, 0xB6, 0xC1)
#define RGBA32_LIGHT_SALMON        RGBA32_SOLID(0xFF, 0xA0, 0x7A)
#define RGBA32_LIGHT_SEA_GREEN     RGBA32_SOLID(0x20, 0xB2, 0xAA)
#define RGBA32_LIGHT_SKY_BLUE      RGBA32_SOLID(0x87, 0xCE, 0xFA)
#define RGBA32_LIGHT_SLATE_GRAY    RGBA32_SOLID(0x77, 0x88, 0x99)
#define RGBA32_LIGHT_STEEL_BLUE    RGBA32_SOLID(0xB0, 0xC4, 0xDE)
#define RGBA32_LIGHT_YELLOW        RGBA32_SOLID(0xFF, 0xFF, 0xE0)
#define RGBA32_LIME                RGBA32_SOLID(0x00, 0xFF, 0x00)
#define RGBA32_LIME_GREEN          RGBA32_SOLID(0x32, 0xCD, 0x32)
#define RGBA32_LINEN               RGBA32_SOLID(0xFA, 0xF0, 0xE6)
#define RGBA32_MAGENTA             RGBA32_SOLID(0xFF, 0x00, 0xFF)
#define RGBA32_MAROON              RGBA32_SOLID(0xB0, 0x30, 0x60)
#define RGBA32_WEB_MAROON          RGBA32_SOLID(0x80, 0x00, 0x00)
#define RGBA32_MEDIUM_AQUAMARINE   RGBA32_SOLID(0x66, 0xCD, 0xAA)
#define RGBA32_MEDIUM_BLUE         RGBA32_SOLID(0x00, 0x00, 0xCD)
#define RGBA32_MEDIUM_ORCHID       RGBA32_SOLID(0xBA, 0x55, 0xD3)
#define RGBA32_MEDIUM_PURPLE       RGBA32_SOLID(0x93, 0x70, 0xDB)
#define RGBA32_MEDIUM_SEA_GREEN    RGBA32_SOLID(0x3C, 0xB3, 0x71)
#define RGBA32_MEDIUM_SLATE_BLUE   RGBA32_SOLID(0x7B, 0x68, 0xEE)
#define RGBA32_MEDIUM_SPRING_GREEN RGBA32_SOLID(0x00, 0xFA, 0x9A)
#define RGBA32_MEDIUM_TURQUOISE    RGBA32_SOLID(0x48, 0xD1, 0xCC)
#define RGBA32_MEDIUM_VIOLET_RED   RGBA32_SOLID(0xC7, 0x15, 0x85)
#define RGBA32_MIDNIGHT_BLUE       RGBA32_SOLID(0x19, 0x19, 0x70)
#define RGBA32_MINT_CREAM          RGBA32_SOLID(0xF5, 0xFF, 0xFA)
#define RGBA32_MISTY_ROSE          RGBA32_SOLID(0xFF, 0xE4, 0xE1)
#define RGBA32_MOCCASIN            RGBA32_SOLID(0xFF, 0xE4, 0xB5)
#define RGBA32_NAVAJO_WHITE        RGBA32_SOLID(0xFF, 0xDE, 0xAD)
#define RGBA32_NAVY_BLUE           RGBA32_SOLID(0x00, 0x00, 0x80)
#define RGBA32_OLD_LACE            RGBA32_SOLID(0xFD, 0xF5, 0xE6)
#define RGBA32_OLIVE               RGBA32_SOLID(0x80, 0x80, 0x00)
#define RGBA32_OLIVE_DRAB          RGBA32_SOLID(0x6B, 0x8E, 0x23)
#define RGBA32_ORANGE              RGBA32_SOLID(0xFF, 0xA5, 0x00)
#define RGBA32_ORANGE_RED          RGBA32_SOLID(0xFF, 0x45, 0x00)
#define RGBA32_ORCHID              RGBA32_SOLID(0xDA, 0x70, 0xD6)
#define RGBA32_PALE_GOLDENROD      RGBA32_SOLID(0xEE, 0xE8, 0xAA)
#define RGBA32_PALE_GREEN          RGBA32_SOLID(0x98, 0xFB, 0x98)
#define RGBA32_PALE_TURQUOISE      RGBA32_SOLID(0xAF, 0xEE, 0xEE)
#define RGBA32_PALE_VIOLET_RED     RGBA32_SOLID(0xDB, 0x70, 0x93)
#define RGBA32_PAPAYA_WHIP         RGBA32_SOLID(0xFF, 0xEF, 0xD5)
#define RGBA32_PEACH_PUFF          RGBA32_SOLID(0xFF, 0xDA, 0xB9)
#define RGBA32_PERU                RGBA32_SOLID(0xCD, 0x85, 0x3F)
#define RGBA32_PINK                RGBA32_SOLID(0xFF, 0xC0, 0xCB)
#define RGBA32_PLUM                RGBA32_SOLID(0xDD, 0xA0, 0xDD)
#define RGBA32_POWDER_BLUE         RGBA32_SOLID(0xB0, 0xE0, 0xE6)
#define RGBA32_PURPLE              RGBA32_SOLID(0xA0, 0x20, 0xF0)
#define RGBA32_WEB_PURPLE          RGBA32_SOLID(0x80, 0x00, 0x80)
#define RGBA32_REBECCA_PURPLE      RGBA32_SOLID(0x66, 0x33, 0x99)
#define RGBA32_RED                 RGBA32_SOLID(0xFF, 0x00, 0x00)
#define RGBA32_ROSY_BROWN          RGBA32_SOLID(0xBC, 0x8F, 0x8F)
#define RGBA32_ROYAL_BLUE          RGBA32_SOLID(0x41, 0x69, 0xE1)
#define RGBA32_SADDLE_BROWN        RGBA32_SOLID(0x8B, 0x45, 0x13)
#define RGBA32_SALMON              RGBA32_SOLID(0xFA, 0x80, 0x72)
#define RGBA32_SANDY_BROWN         RGBA32_SOLID(0xF4, 0xA4, 0x60)
#define RGBA32_SEA_GREEN           RGBA32_SOLID(0x2E, 0x8B, 0x57)
#define RGBA32_SEASHELL            RGBA32_SOLID(0xFF, 0xF5, 0xEE)
#define RGBA32_SIENNA              RGBA32_SOLID(0xA0, 0x52, 0x2D)
#define RGBA32_SILVER              RGBA32_SOLID(0xC0, 0xC0, 0xC0)
#define RGBA32_SKY_BLUE            RGBA32_SOLID(0x87, 0xCE, 0xEB)
#define RGBA32_SLATE_BLUE          RGBA32_SOLID(0x6A, 0x5A, 0xCD)
#define RGBA32_SLATE_GRAY          RGBA32_SOLID(0x70, 0x80, 0x90)
#define RGBA32_SNOW                RGBA32_SOLID(0xFF, 0xFA, 0xFA)
#define RGBA32_SPRING_GREEN        RGBA32_SOLID(0x00, 0xFF, 0x7F)
#define RGBA32_STEEL_BLUE          RGBA32_SOLID(0x46, 0x82, 0xB4)
#define RGBA32_TAN                 RGBA32_SOLID(0xD2, 0xB4, 0x8C)
#define RGBA32_TEAL                RGBA32_SOLID(0x00, 0x80, 0x80)
#define RGBA32_THISTLE             RGBA32_SOLID(0xD8, 0xBF, 0xD8)
#define RGBA32_TOMATO              RGBA32_SOLID(0xFF, 0x63, 0x47)
#define RGBA32_TURQUOISE           RGBA32_SOLID(0x40, 0xE0, 0xD0)
#define RGBA32_VIOLET              RGBA32_SOLID(0xEE, 0x82, 0xEE)
#define RGBA32_WHEAT               RGBA32_SOLID(0xF5, 0xDE, 0xB3)
#define RGBA32_WHITE               RGBA32_SOLID(0xFF, 0xFF, 0xFF)
#define RGBA32_WHITE_SMOKE         RGBA32_SOLID(0xF5, 0xF5, 0xF5)
#define RGBA32_YELLOW              RGBA32_SOLID(0xFF, 0xFF, 0x00)
#define RGBA32_YELLOW_GREEN        RGBA32_SOLID(0x9A, 0xCD, 0x32)
// clang-format on

//--------------------------------------------------------------------------------------------------
// Color space manipulation utilities

Rgba rgbaUnpack32(Rgba32 in);
Rgba32 rgbaPack32(Rgba in);

Rgba rgbaMultiplyAlpha(Rgba col);
Rgba rgbaMultiplyAlpha32(Rgba32 col);

Hsva rgbaToHsva(Rgba in);
Rgba hsvaToRgba(Hsva in);

//--------------------------------------------------------------------------------------------------
// Macros for automatic generation of arrays storing the common color names and values

#define CF__COLOR_LIST(X)  \
    X(ALICE_BLUE)          \
    X(ANTIQUE_WHITE)       \
    X(AQUA)                \
    X(AQUAMARINE)          \
    X(AZURE)               \
    X(BEIGE)               \
    X(BISQUE)              \
    X(BLACK)               \
    X(BLANCHED_ALMOND)     \
    X(BLUE)                \
    X(BLUE_VIOLET)         \
    X(BROWN)               \
    X(BURLYWOOD)           \
    X(CADET_BLUE)          \
    X(CHARTREUSE)          \
    X(CHOCOLATE)           \
    X(CORAL)               \
    X(CORNFLOWER_BLUE)     \
    X(CORNSILK)            \
    X(CRIMSON)             \
    X(CYAN)                \
    X(DARK_BLUE)           \
    X(DARK_CYAN)           \
    X(DARK_GOLDENROD)      \
    X(DARK_GRAY)           \
    X(DARK_GREEN)          \
    X(DARK_KHAKI)          \
    X(DARK_MAGENTA)        \
    X(DARK_OLIVE_GREEN)    \
    X(DARK_ORANGE)         \
    X(DARK_ORCHID)         \
    X(DARK_RED)            \
    X(DARK_SALMON)         \
    X(DARK_SEA_GREEN)      \
    X(DARK_SILVER)         \
    X(DARK_SLATE_BLUE)     \
    X(DARK_SLATE_GRAY)     \
    X(DARK_TURQUOISE)      \
    X(DARK_VIOLET)         \
    X(DEEP_PINK)           \
    X(DEEP_SKY_BLUE)       \
    X(DIM_GRAY)            \
    X(DODGER_BLUE)         \
    X(FIREBRICK)           \
    X(FLORAL_WHITE)        \
    X(FOREST_GREEN)        \
    X(FUCHSIA)             \
    X(GAINSBORO)           \
    X(GHOST_WHITE)         \
    X(GOLD)                \
    X(GOLDENROD)           \
    X(GRAY)                \
    X(WEB_GRAY)            \
    X(GREEN)               \
    X(WEB_GREEN)           \
    X(GREEN_YELLOW)        \
    X(HONEYDEW)            \
    X(HOT_PINK)            \
    X(INDIAN_RED)          \
    X(INDIGO)              \
    X(IVORY)               \
    X(KHAKI)               \
    X(LAVENDER)            \
    X(LAVENDER_BLUSH)      \
    X(LAWN_GREEN)          \
    X(LEMON_CHIFFON)       \
    X(LIGHT_BLUE)          \
    X(LIGHT_CORAL)         \
    X(LIGHT_CYAN)          \
    X(LIGHT_GOLDENROD)     \
    X(LIGHT_GRAY)          \
    X(LIGHT_GREEN)         \
    X(LIGHT_PINK)          \
    X(LIGHT_SALMON)        \
    X(LIGHT_SEA_GREEN)     \
    X(LIGHT_SKY_BLUE)      \
    X(LIGHT_SLATE_GRAY)    \
    X(LIGHT_STEEL_BLUE)    \
    X(LIGHT_YELLOW)        \
    X(LIME)                \
    X(LIME_GREEN)          \
    X(LINEN)               \
    X(MAGENTA)             \
    X(MAROON)              \
    X(WEB_MAROON)          \
    X(MEDIUM_AQUAMARINE)   \
    X(MEDIUM_BLUE)         \
    X(MEDIUM_ORCHID)       \
    X(MEDIUM_PURPLE)       \
    X(MEDIUM_SEA_GREEN)    \
    X(MEDIUM_SLATE_BLUE)   \
    X(MEDIUM_SPRING_GREEN) \
    X(MEDIUM_TURQUOISE)    \
    X(MEDIUM_VIOLET_RED)   \
    X(MIDNIGHT_BLUE)       \
    X(MINT_CREAM)          \
    X(MISTY_ROSE)          \
    X(MOCCASIN)            \
    X(NAVAJO_WHITE)        \
    X(NAVY_BLUE)           \
    X(OLD_LACE)            \
    X(OLIVE)               \
    X(OLIVE_DRAB)          \
    X(ORANGE)              \
    X(ORANGE_RED)          \
    X(ORCHID)              \
    X(PALE_GOLDENROD)      \
    X(PALE_GREEN)          \
    X(PALE_TURQUOISE)      \
    X(PALE_VIOLET_RED)     \
    X(PAPAYA_WHIP)         \
    X(PEACH_PUFF)          \
    X(PERU)                \
    X(PINK)                \
    X(PLUM)                \
    X(POWDER_BLUE)         \
    X(PURPLE)              \
    X(WEB_PURPLE)          \
    X(REBECCA_PURPLE)      \
    X(RED)                 \
    X(ROSY_BROWN)          \
    X(ROYAL_BLUE)          \
    X(SADDLE_BROWN)        \
    X(SALMON)              \
    X(SANDY_BROWN)         \
    X(SEA_GREEN)           \
    X(SEASHELL)            \
    X(SIENNA)              \
    X(SILVER)              \
    X(SKY_BLUE)            \
    X(SLATE_BLUE)          \
    X(SLATE_GRAY)          \
    X(SNOW)                \
    X(SPRING_GREEN)        \
    X(STEEL_BLUE)          \
    X(TAN)                 \
    X(TEAL)                \
    X(THISTLE)             \
    X(TOMATO)              \
    X(TRANSPARENT)         \
    X(TURQUOISE)           \
    X(VIOLET)              \
    X(WHEAT)               \
    X(WHITE)               \
    X(WHITE_SMOKE)         \
    X(YELLOW)              \
    X(YELLOW_GREEN)

#define CF__COLOR_ENUM_ENTRY(name, ...) RGBA32__##name,
#define CF__COLOR_VALUE(name, ...) [RGBA32__##name] = RGBA32_##name,
#define CF__COLOR_STRING(name, ...) CF_STRINGIFY(name),

enum
{
    CF__COLOR_LIST(CF__COLOR_ENUM_ENTRY)
};

#define CF_COLOR_NAMES                   \
    {                                    \
        CF__COLOR_LIST(CF__COLOR_STRING) \
    }
#define CF_COLOR_VALUES                 \
    {                                   \
        CF__COLOR_LIST(CF__COLOR_VALUE) \
    }
