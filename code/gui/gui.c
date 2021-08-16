#include "gui.h"

#include "foundation/array.h"
#include "foundation/colors.h"
#include "foundation/log.h"
#include "foundation/memory.h"
#include "foundation/strings.h"

// === Memory management ===

static void *
guiAlloc(Usize size, void *state)
{
    MemAllocator alloc = *(MemAllocator *)state;
    Usize *buf = memAlloc(alloc, size + sizeof(*buf));

    if (buf) *(buf++) = size;

    return buf;
}

static void
guiFree(void *mem, void *state)
{
    if (mem)
    {
        MemAllocator alloc = *(MemAllocator *)state;
        Usize *buf = mem;
        buf--;
        cfMemFree(alloc, buf, *buf + sizeof(*buf));
    }
}

// === Initialization ===

void
guiInit(Gui *gui)
{
    CF_ASSERT_NOT_NULL(gui);
    CF_ASSERT_NOT_NULL(gui->alloc);

    igDebugCheckVersionAndDataLayout(igGetVersion(), sizeof(ImGuiIO), sizeof(ImGuiStyle),
                                     sizeof(ImVec2), sizeof(ImVec4), sizeof(ImDrawVert),
                                     sizeof(ImDrawIdx));

    igSetAllocatorFunctions(guiAlloc, guiFree, gui->alloc);

    if (gui->ctx)
    {
        igSetCurrentContext(gui->ctx);
    }
    else
    {
        gui->ctx = igCreateContext(gui->shared_atlas);
    }
}

// === Themes & styling ===

void
guiSetTheme(GuiTheme theme)
{
    igStyleColorsClassic(NULL);

    ImVec4 *colors = igGetStyle()->Colors;

    if (theme == GuiTheme_Dark)
    {
        // clang-format off
        colors[ImGuiCol_Text]                   = (ImVec4){1.00f, 1.00f, 1.00f, 1.00f};
        colors[ImGuiCol_TextDisabled]           = (ImVec4){0.50f, 0.50f, 0.50f, 1.00f};
        colors[ImGuiCol_WindowBg]               = (ImVec4){0.06f, 0.06f, 0.06f, 0.94f};
        colors[ImGuiCol_ChildBg]                = (ImVec4){0.00f, 0.00f, 0.00f, 0.00f};
        colors[ImGuiCol_PopupBg]                = (ImVec4){0.08f, 0.08f, 0.08f, 0.94f};
        colors[ImGuiCol_Border]                 = (ImVec4){0.43f, 0.43f, 0.50f, 0.50f};
        colors[ImGuiCol_BorderShadow]           = (ImVec4){0.00f, 0.00f, 0.00f, 0.00f};
        colors[ImGuiCol_FrameBg]                = (ImVec4){0.44f, 0.44f, 0.44f, 0.60f};
        colors[ImGuiCol_FrameBgHovered]         = (ImVec4){0.57f, 0.57f, 0.57f, 0.70f};
        colors[ImGuiCol_FrameBgActive]          = (ImVec4){0.76f, 0.76f, 0.76f, 0.80f};
        colors[ImGuiCol_TitleBg]                = (ImVec4){0.04f, 0.04f, 0.04f, 1.00f};
        colors[ImGuiCol_TitleBgActive]          = (ImVec4){0.16f, 0.16f, 0.16f, 1.00f};
        colors[ImGuiCol_TitleBgCollapsed]       = (ImVec4){0.00f, 0.00f, 0.00f, 0.60f};
        colors[ImGuiCol_MenuBarBg]              = (ImVec4){0.14f, 0.14f, 0.14f, 1.00f};
        colors[ImGuiCol_ScrollbarBg]            = (ImVec4){0.02f, 0.02f, 0.02f, 0.53f};
        colors[ImGuiCol_ScrollbarGrab]          = (ImVec4){0.31f, 0.31f, 0.31f, 1.00f};
        colors[ImGuiCol_ScrollbarGrabHovered]   = (ImVec4){0.41f, 0.41f, 0.41f, 1.00f};
        colors[ImGuiCol_ScrollbarGrabActive]    = (ImVec4){0.51f, 0.51f, 0.51f, 1.00f};
        colors[ImGuiCol_CheckMark]              = (ImVec4){0.13f, 0.75f, 0.55f, 0.80f};
        colors[ImGuiCol_SliderGrab]             = (ImVec4){0.13f, 0.75f, 0.75f, 0.80f};
        colors[ImGuiCol_SliderGrabActive]       = (ImVec4){0.13f, 0.75f, 1.00f, 0.80f};
        colors[ImGuiCol_Button]                 = (ImVec4){0.13f, 0.75f, 0.55f, 0.40f};
        colors[ImGuiCol_ButtonHovered]          = (ImVec4){0.13f, 0.75f, 0.75f, 0.60f};
        colors[ImGuiCol_ButtonActive]           = (ImVec4){0.13f, 0.75f, 1.00f, 0.80f};
        colors[ImGuiCol_Header]                 = (ImVec4){0.13f, 0.75f, 0.55f, 0.40f};
        colors[ImGuiCol_HeaderHovered]          = (ImVec4){0.13f, 0.75f, 0.75f, 0.60f};
        colors[ImGuiCol_HeaderActive]           = (ImVec4){0.13f, 0.75f, 1.00f, 0.80f};
        colors[ImGuiCol_Separator]              = (ImVec4){0.13f, 0.75f, 0.55f, 0.40f};
        colors[ImGuiCol_SeparatorHovered]       = (ImVec4){0.13f, 0.75f, 0.75f, 0.60f};
        colors[ImGuiCol_SeparatorActive]        = (ImVec4){0.13f, 0.75f, 1.00f, 0.80f};
        colors[ImGuiCol_ResizeGrip]             = (ImVec4){0.13f, 0.75f, 0.55f, 0.40f};
        colors[ImGuiCol_ResizeGripHovered]      = (ImVec4){0.13f, 0.75f, 0.75f, 0.60f};
        colors[ImGuiCol_ResizeGripActive]       = (ImVec4){0.13f, 0.75f, 1.00f, 0.80f};
        colors[ImGuiCol_Tab]                    = (ImVec4){0.13f, 0.75f, 0.55f, 0.80f};
        colors[ImGuiCol_TabHovered]             = (ImVec4){0.13f, 0.75f, 0.75f, 0.80f};
        colors[ImGuiCol_TabActive]              = (ImVec4){0.13f, 0.75f, 1.00f, 0.80f};
        colors[ImGuiCol_TabUnfocused]           = (ImVec4){0.18f, 0.18f, 0.18f, 1.00f};
        colors[ImGuiCol_TabUnfocusedActive]     = (ImVec4){0.36f, 0.36f, 0.36f, 0.54f};
        colors[ImGuiCol_DockingPreview]         = (ImVec4){0.13f, 0.75f, 0.55f, 0.80f};
        colors[ImGuiCol_DockingEmptyBg]         = (ImVec4){0.13f, 0.13f, 0.13f, 0.80f};
        colors[ImGuiCol_PlotLines]              = (ImVec4){0.61f, 0.61f, 0.61f, 1.00f};
        colors[ImGuiCol_PlotLinesHovered]       = (ImVec4){1.00f, 0.43f, 0.35f, 1.00f};
        colors[ImGuiCol_PlotHistogram]          = (ImVec4){0.90f, 0.70f, 0.00f, 1.00f};
        colors[ImGuiCol_PlotHistogramHovered]   = (ImVec4){1.00f, 0.60f, 0.00f, 1.00f};
        colors[ImGuiCol_TableHeaderBg]          = (ImVec4){0.19f, 0.19f, 0.20f, 1.00f};
        colors[ImGuiCol_TableBorderStrong]      = (ImVec4){0.31f, 0.31f, 0.35f, 1.00f};
        colors[ImGuiCol_TableBorderLight]       = (ImVec4){0.23f, 0.23f, 0.25f, 1.00f};
        colors[ImGuiCol_TableRowBg]             = (ImVec4){0.00f, 0.00f, 0.00f, 0.00f};
        colors[ImGuiCol_TableRowBgAlt]          = (ImVec4){1.00f, 1.00f, 1.00f, 0.07f};
        colors[ImGuiCol_TextSelectedBg]         = (ImVec4){0.26f, 0.59f, 0.98f, 0.35f};
        colors[ImGuiCol_DragDropTarget]         = (ImVec4){1.00f, 1.00f, 0.00f, 0.90f};
        colors[ImGuiCol_NavHighlight]           = (ImVec4){0.26f, 0.59f, 0.98f, 1.00f};
        colors[ImGuiCol_NavWindowingHighlight]  = (ImVec4){1.00f, 1.00f, 1.00f, 0.70f};
        colors[ImGuiCol_NavWindowingDimBg]      = (ImVec4){0.80f, 0.80f, 0.80f, 0.20f};
        colors[ImGuiCol_ModalWindowDimBg]       = (ImVec4){0.80f, 0.80f, 0.80f, 0.35f};
        // clang-format on
    }
    else if (theme == GuiTheme_Light)
    {
        // clang-format off
        colors[ImGuiCol_Text]                   = (ImVec4){0.00f, 0.00f, 0.00f, 1.00f};
        colors[ImGuiCol_TextDisabled]           = (ImVec4){0.60f, 0.60f, 0.60f, 1.00f};
        colors[ImGuiCol_WindowBg]               = (ImVec4){0.89f, 0.89f, 0.89f, 1.00f};
        colors[ImGuiCol_ChildBg]                = (ImVec4){0.00f, 0.00f, 0.00f, 0.00f};
        colors[ImGuiCol_PopupBg]                = (ImVec4){1.00f, 1.00f, 1.00f, 0.98f};
        colors[ImGuiCol_Border]                 = (ImVec4){0.00f, 0.00f, 0.00f, 0.30f};
        colors[ImGuiCol_BorderShadow]           = (ImVec4){0.00f, 0.00f, 0.00f, 0.00f};
        colors[ImGuiCol_FrameBg]                = (ImVec4){1.00f, 1.00f, 1.00f, 1.00f};
        colors[ImGuiCol_FrameBgHovered]         = (ImVec4){0.26f, 0.59f, 0.98f, 0.40f};
        colors[ImGuiCol_FrameBgActive]          = (ImVec4){0.26f, 0.59f, 0.98f, 0.67f};
        colors[ImGuiCol_TitleBg]                = (ImVec4){0.96f, 0.96f, 0.96f, 1.00f};
        colors[ImGuiCol_TitleBgActive]          = (ImVec4){0.82f, 0.82f, 0.82f, 1.00f};
        colors[ImGuiCol_TitleBgCollapsed]       = (ImVec4){1.00f, 1.00f, 1.00f, 0.51f};
        colors[ImGuiCol_MenuBarBg]              = (ImVec4){0.86f, 0.86f, 0.86f, 1.00f};
        colors[ImGuiCol_ScrollbarBg]            = (ImVec4){0.98f, 0.98f, 0.98f, 0.53f};
        colors[ImGuiCol_ScrollbarGrab]          = (ImVec4){0.69f, 0.69f, 0.69f, 0.80f};
        colors[ImGuiCol_ScrollbarGrabHovered]   = (ImVec4){0.49f, 0.49f, 0.49f, 0.80f};
        colors[ImGuiCol_ScrollbarGrabActive]    = (ImVec4){0.49f, 0.49f, 0.49f, 1.00f};
        colors[ImGuiCol_CheckMark]              = (ImVec4){0.26f, 0.59f, 0.98f, 1.00f};
        colors[ImGuiCol_SliderGrab]             = (ImVec4){0.26f, 0.59f, 0.98f, 0.78f};
        colors[ImGuiCol_SliderGrabActive]       = (ImVec4){0.46f, 0.54f, 0.80f, 0.60f};
        colors[ImGuiCol_Button]                 = (ImVec4){0.26f, 0.59f, 0.98f, 0.40f};
        colors[ImGuiCol_ButtonHovered]          = (ImVec4){0.26f, 0.59f, 0.98f, 1.00f};
        colors[ImGuiCol_ButtonActive]           = (ImVec4){0.06f, 0.53f, 0.98f, 1.00f};
        colors[ImGuiCol_Header]                 = (ImVec4){0.26f, 0.59f, 0.98f, 0.31f};
        colors[ImGuiCol_HeaderHovered]          = (ImVec4){0.26f, 0.59f, 0.98f, 0.80f};
        colors[ImGuiCol_HeaderActive]           = (ImVec4){0.26f, 0.59f, 0.98f, 1.00f};
        colors[ImGuiCol_Separator]              = (ImVec4){0.39f, 0.39f, 0.39f, 0.62f};
        colors[ImGuiCol_SeparatorHovered]       = (ImVec4){0.14f, 0.44f, 0.80f, 0.78f};
        colors[ImGuiCol_SeparatorActive]        = (ImVec4){0.14f, 0.44f, 0.80f, 1.00f};
        colors[ImGuiCol_ResizeGrip]             = (ImVec4){0.35f, 0.35f, 0.35f, 0.17f};
        colors[ImGuiCol_ResizeGripHovered]      = (ImVec4){0.26f, 0.59f, 0.98f, 0.67f};
        colors[ImGuiCol_ResizeGripActive]       = (ImVec4){0.26f, 0.59f, 0.98f, 0.95f};
        colors[ImGuiCol_Tab]                    = (ImVec4){0.76f, 0.80f, 0.84f, 0.93f};
        colors[ImGuiCol_TabHovered]             = (ImVec4){0.26f, 0.59f, 0.98f, 0.80f};
        colors[ImGuiCol_TabActive]              = (ImVec4){0.60f, 0.73f, 0.88f, 1.00f};
        colors[ImGuiCol_TabUnfocused]           = (ImVec4){0.92f, 0.93f, 0.94f, 0.99f};
        colors[ImGuiCol_TabUnfocusedActive]     = (ImVec4){0.74f, 0.82f, 0.91f, 1.00f};
        colors[ImGuiCol_DockingPreview]         = (ImVec4){0.26f, 0.59f, 0.98f, 0.22f};
        colors[ImGuiCol_DockingEmptyBg]         = (ImVec4){0.20f, 0.20f, 0.20f, 1.00f};
        colors[ImGuiCol_PlotLines]              = (ImVec4){0.39f, 0.39f, 0.39f, 1.00f};
        colors[ImGuiCol_PlotLinesHovered]       = (ImVec4){1.00f, 0.43f, 0.35f, 1.00f};
        colors[ImGuiCol_PlotHistogram]          = (ImVec4){0.90f, 0.70f, 0.00f, 1.00f};
        colors[ImGuiCol_PlotHistogramHovered]   = (ImVec4){1.00f, 0.45f, 0.00f, 1.00f};
        colors[ImGuiCol_TableHeaderBg]          = (ImVec4){0.78f, 0.87f, 0.98f, 1.00f};
        colors[ImGuiCol_TableBorderStrong]      = (ImVec4){0.57f, 0.57f, 0.64f, 1.00f};
        colors[ImGuiCol_TableBorderLight]       = (ImVec4){0.68f, 0.68f, 0.74f, 1.00f};
        colors[ImGuiCol_TableRowBg]             = (ImVec4){0.00f, 0.00f, 0.00f, 0.00f};
        colors[ImGuiCol_TableRowBgAlt]          = (ImVec4){0.30f, 0.30f, 0.30f, 0.09f};
        colors[ImGuiCol_TextSelectedBg]         = (ImVec4){0.26f, 0.59f, 0.98f, 0.35f};
        colors[ImGuiCol_DragDropTarget]         = (ImVec4){0.26f, 0.59f, 0.98f, 0.95f};
        colors[ImGuiCol_NavHighlight]           = (ImVec4){0.26f, 0.59f, 0.98f, 0.80f};
        colors[ImGuiCol_NavWindowingHighlight]  = (ImVec4){0.70f, 0.70f, 0.70f, 0.70f};
        colors[ImGuiCol_NavWindowingDimBg]      = (ImVec4){0.20f, 0.20f, 0.20f, 0.20f};
        colors[ImGuiCol_ModalWindowDimBg]       = (ImVec4){0.20f, 0.20f, 0.20f, 0.35f};
        // clang-format on
    }
}

// === Fonts handling ===

bool
guiFontOptionsEdit(GuiFontOptions *state)
{
    bool rebuild_fonts = false;

    igShowFontSelector("Fonts");

    if (igRadioButton_Bool("FreeType", state->freetype_enabled))
    {
        state->freetype_enabled = true;
        rebuild_fonts = true;
    }
    guiSameLine();
    if (igRadioButton_Bool("Stb (Default)", !state->freetype_enabled))
    {
        state->freetype_enabled = false;
        rebuild_fonts = true;
    }

    rebuild_fonts |= igDragInt("TexGlyphPadding", &state->tex_glyph_padding, 0.1f, 1, 16, NULL,
                               ImGuiSliderFlags_None);

    rebuild_fonts |= igDragFloat("RasterizerMultiply", &state->rasterizer_multiply, 0.001f, 0.0f,
                                 2.0f, NULL, ImGuiSliderFlags_None);

    igSeparator();

    if (state->freetype_enabled)
    {

        rebuild_fonts |= igCheckboxFlags_UintPtr("NoHinting", &state->freetype_flags,
                                                 ImGuiFreeTypeBuilderFlags_NoHinting);
        rebuild_fonts |= igCheckboxFlags_UintPtr("NoAutoHint", &state->freetype_flags,
                                                 ImGuiFreeTypeBuilderFlags_NoAutoHint);
        rebuild_fonts |= igCheckboxFlags_UintPtr("ForceAutoHint", &state->freetype_flags,
                                                 ImGuiFreeTypeBuilderFlags_ForceAutoHint);
        rebuild_fonts |= igCheckboxFlags_UintPtr("LightHinting", &state->freetype_flags,
                                                 ImGuiFreeTypeBuilderFlags_LightHinting);
        rebuild_fonts |= igCheckboxFlags_UintPtr("MonoHinting", &state->freetype_flags,
                                                 ImGuiFreeTypeBuilderFlags_MonoHinting);
        rebuild_fonts |=
            igCheckboxFlags_UintPtr("Bold", &state->freetype_flags, ImGuiFreeTypeBuilderFlags_Bold);
        rebuild_fonts |= igCheckboxFlags_UintPtr("Oblique", &state->freetype_flags,
                                                 ImGuiFreeTypeBuilderFlags_Oblique);
        rebuild_fonts |= igCheckboxFlags_UintPtr("Monochrome", &state->freetype_flags,
                                                 ImGuiFreeTypeBuilderFlags_Monochrome);
    }
    else
    {
        rebuild_fonts |= igDragInt("Oversample H", &state->oversample_h, 0.1f, 1, 5, NULL,
                                   ImGuiSliderFlags_None);
        rebuild_fonts |= igDragInt("Oversample V", &state->oversample_v, 0.1f, 1, 5, NULL,
                                   ImGuiSliderFlags_None);
    }

    return rebuild_fonts;
}

void
guiUpdateAtlas(ImFontAtlas *fonts, GuiFontOptions *font_opts)
{
    if (font_opts->tex_glyph_padding != 0)
    {
        fonts->TexGlyphPadding = font_opts->tex_glyph_padding;
    }

    for (I32 i = 0; i < fonts->ConfigData.Size; ++i)
    {
        fonts->ConfigData.Data[i].RasterizerMultiply = font_opts->rasterizer_multiply;
        fonts->ConfigData.Data[i].OversampleH = font_opts->oversample_h;
        fonts->ConfigData.Data[i].OversampleV = font_opts->oversample_v;
    }

    if (font_opts->freetype_enabled)
    {
        fonts->FontBuilderIO = ImGuiFreeType_GetBuilderForFreeType();
        fonts->FontBuilderFlags = (U32)font_opts->freetype_flags;
    }
    else
    {
        fonts->FontBuilderIO = igImFontAtlasGetBuilderForStbTruetype();
    }

    ImFontAtlas_Build(fonts);

    font_opts->tex_glyph_padding = fonts->TexGlyphPadding;
}

// === Log ===

void
guiLogBox(CfLog *log, bool readonly)
{
    bool copy = false;

    if (!readonly)
    {
        if (guiButton("Clear")) cfLogClear(log);
        guiSameLine();
    }

    copy = guiButton("Copy");

    igSeparator();
    igBeginChild_Str("scrolling", (ImVec2){0}, false, ImGuiWindowFlags_HorizontalScrollbar);
    {
        if (copy) igLogToClipboard(-1);

        igPushStyleVar_Vec2(ImGuiStyleVar_ItemSpacing, (ImVec2){0});
        Str log_str = cfLogString(log);
        igTextUnformatted(log_str.buf, strEnd(log_str));
        igPopStyleVar(1);
        if (igGetScrollY() >= igGetScrollMaxY())
        {
            igSetScrollHereY(1.0f);
        }
    }
    igEndChild();
}

// === Miscellanea ===

void
guiBeginFullScreen(Cstr label, bool docking, bool menu_bar)
{
    ImGuiViewport const *viewport = igGetMainViewport();
    igSetNextWindowPos(viewport->WorkPos, 0, (ImVec2){0, 0});
    igSetNextWindowSize(viewport->WorkSize, 0);
    igPushStyleVar_Float(ImGuiStyleVar_WindowRounding, 0.0f);
    igPushStyleVar_Float(ImGuiStyleVar_WindowBorderSize, 0.0f);
    igPushStyleVar_Vec2(ImGuiStyleVar_WindowPadding, (ImVec2){0.0f, 0.0f});

    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
                                    ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                                    ImGuiWindowFlags_NoBringToFrontOnFocus |
                                    ImGuiWindowFlags_NoNavFocus;

    if (!docking) window_flags |= ImGuiWindowFlags_NoDocking;
    if (menu_bar) window_flags |= ImGuiWindowFlags_MenuBar;

    igBegin(label, NULL, window_flags);
    igPopStyleVar(3);
}

void
guiEndFullScreen(void)
{
    igEnd();
}

bool
guiCenteredButton(Cstr label)
{
    ImGuiStyle *style = igGetStyle();

    // NOTE (Matteo): Button size calculation copied from ImGui::ButtonEx
    ImVec2 label_size, button_size;
    igCalcTextSize(&label_size, label, NULL, false, -1.0f);
    igCalcItemSize(&button_size, (ImVec2){0, 0}, //
                   label_size.x + style->FramePadding.x * 2.0f,
                   label_size.y + style->FramePadding.y * 2.0f);

    ImVec2 available_size;
    igGetContentRegionAvail(&available_size);

    if (available_size.x > button_size.x)
    {
        igSetCursorPosX((available_size.x - button_size.x) / 2);
    }

    return guiButton(label);
}

bool
guiColorEdit(Cstr label, Rgba32 *color)
{
    // TODO (Matteo): Fix redundant label

    static Rgba32 const colors[] = CF_COLOR_VALUES;
    static Cstr const names[] = CF_COLOR_NAMES;

    bool color_changed = false;
    Usize color_index = USIZE_MAX;
    Cstr color_name = NULL;
    Char8 label_buffer[1024];

    // Test if the color is a known named one

    for (Usize i = 0; i < CF_ARRAY_SIZE(colors) && color_index == USIZE_MAX; ++i)
    {
        if (colors[i] == *color)
        {
            color_index = i;
            color_name = names[color_index];
        }
    }

    // Combo box with named colors

    strPrintf(label_buffer, CF_ARRAY_SIZE(label_buffer), "%s##Combo", label);

    if (igBeginCombo(label_buffer, color_name, 0))
    {
        for (Usize i = 0; i < CF_ARRAY_SIZE(colors); ++i)
        {
            bool const selected = i == color_index;
            if (igSelectable_Bool(names[i], selected, 0, (ImVec2){0}))
            {
                color_changed = color_index != i;
                *color = colors[i];
            }
            if (selected) igSetItemDefaultFocus();
        }

        igEndCombo();
    }

    // Free color editing

    strPrintf(label_buffer, CF_ARRAY_SIZE(label_buffer), "%s##Picker", label);

    Rgba color4 = rgbaUnpack32(*color);
    U32 edit_flags = ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_AlphaPreviewHalf |
                     ImGuiColorEditFlags_PickerHueWheel;

    if (igColorEdit4(label_buffer, color4.channel, edit_flags))
    {
        *color = rgbaPack32(color4);
        color_changed = true;
    }

    return color_changed;
}

// === File dialogs ===

// NOTE (Matteo): On windows I use the system dialogs for lazyness (and better experience actually)

#if CF_OS_WIN32
#    include "foundation/win32.h"

typedef CfArray(Char16) StrBuf16;
typedef BOOL(APIENTRY *Win32FileDialog)(LPOPENFILENAMEW);

static const Win32FileDialog win32FileDialog[2] = {
    [GuiFileDialog_Open] = GetOpenFileNameW,
    [GuiFileDialog_Save] = GetSaveFileNameW,
};

static const DWORD win32FileDialogFlags[2] = {
    [GuiFileDialog_Open] = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST,
    [GuiFileDialog_Save] = OFN_OVERWRITEPROMPT,
};

static StrBuf16
win32BuildFilterString(GuiFileDialogFilter *filters, Usize num_filters, MemAllocator alloc)
{
    StrBuf16 out_filter = {0};
    cfArrayInitCap(&out_filter, alloc, 1024);

    if (num_filters == 0) return out_filter;

    for (GuiFileDialogFilter *filter = filters, *end = filter + num_filters; //
         filter < end; ++filter)
    {
        Usize name_size = win32Utf8To16C(filter->name, NULL, 0);

        cfArrayReserve(&out_filter, name_size);
        win32Utf8To16C(filter->name, cfArrayEnd(&out_filter), name_size);
        cfArrayExtend(&out_filter, name_size);

        for (Usize ext_no = 0; ext_no < filter->num_extensions; ++ext_no)
        {
            Cstr ext = filter->extensions[ext_no];
            Usize ext_size = win32Utf8To16C(ext, NULL, 0);

            // Prepend '*' to the extension - not documented but actually required
            cfArrayPush(&out_filter, L'*');
            cfArrayReserve(&out_filter, ext_size);
            win32Utf8To16C(ext, cfArrayEnd(&out_filter), ext_size);
            cfArrayExtend(&out_filter, ext_size);

            // Replace null terminator with ';' to separate extensions
            *cfArrayLast(&out_filter) = L';';
        }

        // Append 2 null terminators (required since null terminators are used
        // internally to separate filters)
        cfArrayPush(&out_filter, 0);
        cfArrayPush(&out_filter, 0);
    }

    return out_filter;
}

GuiFileDialogResult
guiFileDialog(GuiFileDialogParms *parms, MemAllocator alloc)
{
    GuiFileDialogResult result = {.code = GuiFileDialogResult_Error};

    Char16 name[MAX_PATH] = {0};

    if (strValid(parms->filename_hint))
    {
        Usize name_size = win32Utf8To16(parms->filename_hint, NULL, 0);
        if (name_size >= MAX_PATH) return result;
        win32Utf8To16(parms->filename_hint, name, name_size);
    }

    StrBuf16 filt = win32BuildFilterString(parms->filters, parms->num_filters, alloc);

    OPENFILENAMEW ofn = {0};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = NULL;
    ofn.lpstrFile = name;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrFilter = filt.buf; // L"Image files\0*.jpg;*.jpeg;*.bmp;*.png\0";
    ofn.nFilterIndex = 1;
    ofn.lpstrFileTitle = NULL;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = NULL;
    ofn.Flags = win32FileDialogFlags[parms->type];

    if (win32FileDialog[parms->type](&ofn))
    {
        result.filename.len = win32Utf16To8C(ofn.lpstrFile, NULL, 0) - 1;
        result.filename.buf = memAlloc(alloc, result.filename.len);

        if (result.filename.buf)
        {
            result.code = GuiFileDialogResult_Ok;
            win32Utf16To8C(ofn.lpstrFile, (Char8 *)result.filename.buf, result.filename.len);
        }
        else
        {
            result.filename.len = 0;
        }
    }
    else
    {
        result.code = GuiFileDialogResult_Cancel;
    }

    cfArrayFree(&filt);

    return result;
}

#else

GuiFileDialogResult
guiOpenFileDialog(Str filename_hint, GuiFileDialogFilter *filters, Usize num_filters,
                  MemAllocator alloc)
{
    // TODO (Matteo): Implement purely using IMGUI
    return (GuiFileDialogResult){.code = FileDialogResult_Error};
}

#endif
