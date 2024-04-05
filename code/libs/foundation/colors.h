#pragma once

#include "core.h"

//--------------------------------------------------------------------------------------------------
// Packed RGBA 32 bit format implementation

#if CF_LITTLE_ENDIAN
// NOTE (Matteo): Technically, the integer content is ABGR, but the byte order on little endian
// systems is RGBA
#    define SRGB32_R_SHIFT 0
#    define SRGB32_G_SHIFT 8
#    define SRGB32_B_SHIFT 16
#    define SRGB32_A_SHIFT 24
#    define SRGB32_A_MASK 0xFF000000
#else
// TODO (Matteo): How does this interact with OpenGL/DearImgui?
#    define SRGB32_R_SHIFT 24
#    define SRGB32_G_SHIFT 16
#    define SRGB32_B_SHIFT 8
#    define SRGB32_A_SHIFT 0
#    define SRGB32_A_MASK 0x000000FF
#endif

#define SRGB32(R, G, B, A)                                               \
    (((Srgb32)(A) << SRGB32_A_SHIFT) | ((Srgb32)(B) << SRGB32_B_SHIFT) | \
     ((Srgb32)(G) << SRGB32_G_SHIFT) | ((Srgb32)(R) << SRGB32_R_SHIFT))

#define SRGB32_R(col) ((col >> SRGB32_R_SHIFT) & 0xFF)
#define SRGB32_G(col) ((col >> SRGB32_G_SHIFT) & 0xFF)
#define SRGB32_B(col) ((col >> SRGB32_B_SHIFT) & 0xFF)
#define SRGB32_A(col) ((col >> SRGB32_A_SHIFT) & 0xFF)

#define SRGB32_SOLID(R, G, B) SRGB32(R, G, B, 0xFF)

//--------------------------------------------------------------------------------------------------
// Color space manipulation utilities

CF_API float colorSrgbEncode(float channel);
CF_API float colorSrgbDecode(float channel);

/// Convert linear space RBGA color to sRGB space, packed
CF_API Srgb32 colorToSrgb(LinearColor in);

/// Convert sRGB packed color to linear space
CF_API LinearColor colorToLinear(Srgb32 in);

/// Convert sRGB packed color to linear space, multiplying the alpha component
CF_API LinearColor colorToLinearMultiplied(Srgb32 col);

/// Multiply the alpha channel to the other channels of the color
CF_API LinearColor colorMultiplyAlpha(LinearColor col);

CF_API HsvColor colorSrgbToHsv(Srgb32 in);
CF_API Srgb32 colorHsvToSrgb(HsvColor in);

CF_API HsvColor colorLinearToHsv(LinearColor in);
CF_API LinearColor colorHsvToLinear(HsvColor in);

//--------------------------------------------------------------------------------------------------
// Common colors definition based on X11 names (see https://en.wikipedia.org/wiki/X11_color_names)

#define SRGB32_TRANSPARENT SRGB32(0x00, 0x00, 0x00, 0x00)

// clang-format off
#define SRGB32_ALICE_BLUE          SRGB32_SOLID(0xF0, 0xF8, 0xFF)
#define SRGB32_ANTIQUE_WHITE       SRGB32_SOLID(0xFA, 0xEB, 0xD7)
#define SRGB32_AQUA                SRGB32_SOLID(0x00, 0xFF, 0xFF)
#define SRGB32_AQUAMARINE          SRGB32_SOLID(0x7F, 0xFF, 0xD4)
#define SRGB32_AZURE               SRGB32_SOLID(0xF0, 0xFF, 0xFF)
#define SRGB32_BEIGE               SRGB32_SOLID(0xF5, 0xF5, 0xDC)
#define SRGB32_BISQUE              SRGB32_SOLID(0xFF, 0xE4, 0xC4)
#define SRGB32_BLACK               SRGB32_SOLID(0x00, 0x00, 0x00)
#define SRGB32_BLANCHED_ALMOND     SRGB32_SOLID(0xFF, 0xEB, 0xCD)
#define SRGB32_BLUE                SRGB32_SOLID(0x00, 0x00, 0xFF)
#define SRGB32_BLUE_VIOLET         SRGB32_SOLID(0x8A, 0x2B, 0xE2)
#define SRGB32_BROWN               SRGB32_SOLID(0xA5, 0x2A, 0x2A)
#define SRGB32_BURLYWOOD           SRGB32_SOLID(0xDE, 0xB8, 0x87)
#define SRGB32_CADET_BLUE          SRGB32_SOLID(0x5F, 0x9E, 0xA0)
#define SRGB32_CHARTREUSE          SRGB32_SOLID(0x7F, 0xFF, 0x00)
#define SRGB32_CHOCOLATE           SRGB32_SOLID(0xD2, 0x69, 0x1E)
#define SRGB32_CORAL               SRGB32_SOLID(0xFF, 0x7F, 0x50)
#define SRGB32_CORNFLOWER_BLUE     SRGB32_SOLID(0x64, 0x95, 0xED)
#define SRGB32_CORNSILK            SRGB32_SOLID(0xFF, 0xF8, 0xDC)
#define SRGB32_CRIMSON             SRGB32_SOLID(0xDC, 0x14, 0x3C)
#define SRGB32_CYAN                SRGB32_SOLID(0x00, 0xFF, 0xFF)
#define SRGB32_DARK_BLUE           SRGB32_SOLID(0x00, 0x00, 0x8B)
#define SRGB32_DARK_CYAN           SRGB32_SOLID(0x00, 0x8B, 0x8B)
#define SRGB32_DARK_GOLDENROD      SRGB32_SOLID(0xB8, 0x86, 0x0B)
#define SRGB32_DARK_GRAY           SRGB32_SOLID(0xA9, 0xA9, 0xA9)
#define SRGB32_DARK_GREEN          SRGB32_SOLID(0x00, 0x64, 0x00)
#define SRGB32_DARK_KHAKI          SRGB32_SOLID(0xBD, 0xB7, 0x6B)
#define SRGB32_DARK_MAGENTA        SRGB32_SOLID(0x8B, 0x00, 0x8B)
#define SRGB32_DARK_OLIVE_GREEN    SRGB32_SOLID(0x55, 0x6B, 0x2F)
#define SRGB32_DARK_ORANGE         SRGB32_SOLID(0xFF, 0x8C, 0x00)
#define SRGB32_DARK_ORCHID         SRGB32_SOLID(0x99, 0x32, 0xCC)
#define SRGB32_DARK_RED            SRGB32_SOLID(0x8B, 0x00, 0x00)
#define SRGB32_DARK_SALMON         SRGB32_SOLID(0xE9, 0x96, 0x7A)
#define SRGB32_DARK_SEA_GREEN      SRGB32_SOLID(0x8F, 0xBC, 0x8F)
#define SRGB32_DARK_SILVER         SRGB32_SOLID(0xAF, 0xAF, 0xAF)
#define SRGB32_DARK_SLATE_BLUE     SRGB32_SOLID(0x48, 0x3D, 0x8B)
#define SRGB32_DARK_SLATE_GRAY     SRGB32_SOLID(0x2F, 0x4F, 0x4F)
#define SRGB32_DARK_TURQUOISE      SRGB32_SOLID(0x00, 0xCE, 0xD1)
#define SRGB32_DARK_VIOLET         SRGB32_SOLID(0x94, 0x00, 0xD3)
#define SRGB32_DEEP_PINK           SRGB32_SOLID(0xFF, 0x14, 0x93)
#define SRGB32_DEEP_SKY_BLUE       SRGB32_SOLID(0x00, 0xBF, 0xFF)
#define SRGB32_DIM_GRAY            SRGB32_SOLID(0x69, 0x69, 0x69)
#define SRGB32_DODGER_BLUE         SRGB32_SOLID(0x1E, 0x90, 0xFF)
#define SRGB32_FIREBRICK           SRGB32_SOLID(0xB2, 0x22, 0x22)
#define SRGB32_FLORAL_WHITE        SRGB32_SOLID(0xFF, 0xFA, 0xF0)
#define SRGB32_FOREST_GREEN        SRGB32_SOLID(0x22, 0x8B, 0x22)
#define SRGB32_FUCHSIA             SRGB32_SOLID(0xFF, 0x00, 0xFF)
#define SRGB32_GAINSBORO           SRGB32_SOLID(0xDC, 0xDC, 0xDC)
#define SRGB32_GHOST_WHITE         SRGB32_SOLID(0xF8, 0xF8, 0xFF)
#define SRGB32_GOLD                SRGB32_SOLID(0xFF, 0xD7, 0x00)
#define SRGB32_GOLDENROD           SRGB32_SOLID(0xDA, 0xA5, 0x20)
#define SRGB32_GRAY                SRGB32_SOLID(0xBE, 0xBE, 0xBE)
#define SRGB32_WEB_GRAY            SRGB32_SOLID(0x80, 0x80, 0x80)
#define SRGB32_GREEN               SRGB32_SOLID(0x00, 0xFF, 0x00)
#define SRGB32_WEB_GREEN           SRGB32_SOLID(0x00, 0x80, 0x00)
#define SRGB32_GREEN_YELLOW        SRGB32_SOLID(0xAD, 0xFF, 0x2F)
#define SRGB32_HONEYDEW            SRGB32_SOLID(0xF0, 0xFF, 0xF0)
#define SRGB32_HOT_PINK            SRGB32_SOLID(0xFF, 0x69, 0xB4)
#define SRGB32_INDIAN_RED          SRGB32_SOLID(0xCD, 0x5C, 0x5C)
#define SRGB32_INDIGO              SRGB32_SOLID(0x4B, 0x00, 0x82)
#define SRGB32_IVORY               SRGB32_SOLID(0xFF, 0xFF, 0xF0)
#define SRGB32_KHAKI               SRGB32_SOLID(0xF0, 0xE6, 0x8C)
#define SRGB32_LAVENDER            SRGB32_SOLID(0xE6, 0xE6, 0xFA)
#define SRGB32_LAVENDER_BLUSH      SRGB32_SOLID(0xFF, 0xF0, 0xF5)
#define SRGB32_LAWN_GREEN          SRGB32_SOLID(0x7C, 0xFC, 0x00)
#define SRGB32_LEMON_CHIFFON       SRGB32_SOLID(0xFF, 0xFA, 0xCD)
#define SRGB32_LIGHT_BLUE          SRGB32_SOLID(0xAD, 0xD8, 0xE6)
#define SRGB32_LIGHT_CORAL         SRGB32_SOLID(0xF0, 0x80, 0x80)
#define SRGB32_LIGHT_CYAN          SRGB32_SOLID(0xE0, 0xFF, 0xFF)
#define SRGB32_LIGHT_GOLDENROD     SRGB32_SOLID(0xFA, 0xFA, 0xD2)
#define SRGB32_LIGHT_GRAY          SRGB32_SOLID(0xD3, 0xD3, 0xD3)
#define SRGB32_LIGHT_GREEN         SRGB32_SOLID(0x90, 0xEE, 0x90)
#define SRGB32_LIGHT_PINK          SRGB32_SOLID(0xFF, 0xB6, 0xC1)
#define SRGB32_LIGHT_SALMON        SRGB32_SOLID(0xFF, 0xA0, 0x7A)
#define SRGB32_LIGHT_SEA_GREEN     SRGB32_SOLID(0x20, 0xB2, 0xAA)
#define SRGB32_LIGHT_SKY_BLUE      SRGB32_SOLID(0x87, 0xCE, 0xFA)
#define SRGB32_LIGHT_SLATE_GRAY    SRGB32_SOLID(0x77, 0x88, 0x99)
#define SRGB32_LIGHT_STEEL_BLUE    SRGB32_SOLID(0xB0, 0xC4, 0xDE)
#define SRGB32_LIGHT_YELLOW        SRGB32_SOLID(0xFF, 0xFF, 0xE0)
#define SRGB32_LIME                SRGB32_SOLID(0x00, 0xFF, 0x00)
#define SRGB32_LIME_GREEN          SRGB32_SOLID(0x32, 0xCD, 0x32)
#define SRGB32_LINEN               SRGB32_SOLID(0xFA, 0xF0, 0xE6)
#define SRGB32_MAGENTA             SRGB32_SOLID(0xFF, 0x00, 0xFF)
#define SRGB32_MAROON              SRGB32_SOLID(0xB0, 0x30, 0x60)
#define SRGB32_WEB_MAROON          SRGB32_SOLID(0x80, 0x00, 0x00)
#define SRGB32_MEDIUM_AQUAMARINE   SRGB32_SOLID(0x66, 0xCD, 0xAA)
#define SRGB32_MEDIUM_BLUE         SRGB32_SOLID(0x00, 0x00, 0xCD)
#define SRGB32_MEDIUM_ORCHID       SRGB32_SOLID(0xBA, 0x55, 0xD3)
#define SRGB32_MEDIUM_PURPLE       SRGB32_SOLID(0x93, 0x70, 0xDB)
#define SRGB32_MEDIUM_SEA_GREEN    SRGB32_SOLID(0x3C, 0xB3, 0x71)
#define SRGB32_MEDIUM_SLATE_BLUE   SRGB32_SOLID(0x7B, 0x68, 0xEE)
#define SRGB32_MEDIUM_SPRING_GREEN SRGB32_SOLID(0x00, 0xFA, 0x9A)
#define SRGB32_MEDIUM_TURQUOISE    SRGB32_SOLID(0x48, 0xD1, 0xCC)
#define SRGB32_MEDIUM_VIOLET_RED   SRGB32_SOLID(0xC7, 0x15, 0x85)
#define SRGB32_MIDNIGHT_BLUE       SRGB32_SOLID(0x19, 0x19, 0x70)
#define SRGB32_MINT_CREAM          SRGB32_SOLID(0xF5, 0xFF, 0xFA)
#define SRGB32_MISTY_ROSE          SRGB32_SOLID(0xFF, 0xE4, 0xE1)
#define SRGB32_MOCCASIN            SRGB32_SOLID(0xFF, 0xE4, 0xB5)
#define SRGB32_NAVAJO_WHITE        SRGB32_SOLID(0xFF, 0xDE, 0xAD)
#define SRGB32_NAVY_BLUE           SRGB32_SOLID(0x00, 0x00, 0x80)
#define SRGB32_OLD_LACE            SRGB32_SOLID(0xFD, 0xF5, 0xE6)
#define SRGB32_OLIVE               SRGB32_SOLID(0x80, 0x80, 0x00)
#define SRGB32_OLIVE_DRAB          SRGB32_SOLID(0x6B, 0x8E, 0x23)
#define SRGB32_ORANGE              SRGB32_SOLID(0xFF, 0xA5, 0x00)
#define SRGB32_ORANGE_RED          SRGB32_SOLID(0xFF, 0x45, 0x00)
#define SRGB32_ORCHID              SRGB32_SOLID(0xDA, 0x70, 0xD6)
#define SRGB32_PALE_GOLDENROD      SRGB32_SOLID(0xEE, 0xE8, 0xAA)
#define SRGB32_PALE_GREEN          SRGB32_SOLID(0x98, 0xFB, 0x98)
#define SRGB32_PALE_TURQUOISE      SRGB32_SOLID(0xAF, 0xEE, 0xEE)
#define SRGB32_PALE_VIOLET_RED     SRGB32_SOLID(0xDB, 0x70, 0x93)
#define SRGB32_PAPAYA_WHIP         SRGB32_SOLID(0xFF, 0xEF, 0xD5)
#define SRGB32_PEACH_PUFF          SRGB32_SOLID(0xFF, 0xDA, 0xB9)
#define SRGB32_PERU                SRGB32_SOLID(0xCD, 0x85, 0x3F)
#define SRGB32_PINK                SRGB32_SOLID(0xFF, 0xC0, 0xCB)
#define SRGB32_PLUM                SRGB32_SOLID(0xDD, 0xA0, 0xDD)
#define SRGB32_POWDER_BLUE         SRGB32_SOLID(0xB0, 0xE0, 0xE6)
#define SRGB32_PURPLE              SRGB32_SOLID(0xA0, 0x20, 0xF0)
#define SRGB32_WEB_PURPLE          SRGB32_SOLID(0x80, 0x00, 0x80)
#define SRGB32_REBECCA_PURPLE      SRGB32_SOLID(0x66, 0x33, 0x99)
#define SRGB32_RED                 SRGB32_SOLID(0xFF, 0x00, 0x00)
#define SRGB32_ROSY_BROWN          SRGB32_SOLID(0xBC, 0x8F, 0x8F)
#define SRGB32_ROYAL_BLUE          SRGB32_SOLID(0x41, 0x69, 0xE1)
#define SRGB32_SADDLE_BROWN        SRGB32_SOLID(0x8B, 0x45, 0x13)
#define SRGB32_SALMON              SRGB32_SOLID(0xFA, 0x80, 0x72)
#define SRGB32_SANDY_BROWN         SRGB32_SOLID(0xF4, 0xA4, 0x60)
#define SRGB32_SEA_GREEN           SRGB32_SOLID(0x2E, 0x8B, 0x57)
#define SRGB32_SEASHELL            SRGB32_SOLID(0xFF, 0xF5, 0xEE)
#define SRGB32_SIENNA              SRGB32_SOLID(0xA0, 0x52, 0x2D)
#define SRGB32_SILVER              SRGB32_SOLID(0xC0, 0xC0, 0xC0)
#define SRGB32_SKY_BLUE            SRGB32_SOLID(0x87, 0xCE, 0xEB)
#define SRGB32_SLATE_BLUE          SRGB32_SOLID(0x6A, 0x5A, 0xCD)
#define SRGB32_SLATE_GRAY          SRGB32_SOLID(0x70, 0x80, 0x90)
#define SRGB32_SNOW                SRGB32_SOLID(0xFF, 0xFA, 0xFA)
#define SRGB32_SPRING_GREEN        SRGB32_SOLID(0x00, 0xFF, 0x7F)
#define SRGB32_STEEL_BLUE          SRGB32_SOLID(0x46, 0x82, 0xB4)
#define SRGB32_TAN                 SRGB32_SOLID(0xD2, 0xB4, 0x8C)
#define SRGB32_TEAL                SRGB32_SOLID(0x00, 0x80, 0x80)
#define SRGB32_THISTLE             SRGB32_SOLID(0xD8, 0xBF, 0xD8)
#define SRGB32_TOMATO              SRGB32_SOLID(0xFF, 0x63, 0x47)
#define SRGB32_TURQUOISE           SRGB32_SOLID(0x40, 0xE0, 0xD0)
#define SRGB32_VIOLET              SRGB32_SOLID(0xEE, 0x82, 0xEE)
#define SRGB32_WHEAT               SRGB32_SOLID(0xF5, 0xDE, 0xB3)
#define SRGB32_WHITE               SRGB32_SOLID(0xFF, 0xFF, 0xFF)
#define SRGB32_WHITE_SMOKE         SRGB32_SOLID(0xF5, 0xF5, 0xF5)
#define SRGB32_YELLOW              SRGB32_SOLID(0xFF, 0xFF, 0x00)
#define SRGB32_YELLOW_GREEN        SRGB32_SOLID(0x9A, 0xCD, 0x32)
// clang-format on

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

#define CF__COLOR_ENUM_ENTRY(name, ...) SRGB32__##name,
#define CF__COLOR_VALUE(name, ...) /* [SRGB32__##name] = */ SRGB32_##name,
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
