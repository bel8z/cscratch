#include "gui.h"
#include "foundation/core.h"
#include "foundation/error.h"
#include "foundation/strings.h"
#include "gui_internal.h"

#include "foundation/colors.h"
#include "foundation/log.h"
#include "foundation/memory.h"
#include "imgui.h"
#include "implot.h"

#if !defined GUI_VIEWPORTS
#    define GUI_VIEWPORTS 0
#endif // GUI_VIEWPORTS

//=== Type conversions ===//

CF_STATIC_ASSERT(sizeof(Vec2) == sizeof(ImVec2), "Vec2 not compatible with ImVec2");
CF_STATIC_ASSERT(sizeof(LinearColor) == sizeof(ImVec4), "LinearColor not compatible with ImVec4");

//=== Memory management ===//

void *
guiAlloc(Usize size, void *state)
{
    // TODO (Matteo): Check for misaligned access

    CF_ASSERT_NOT_NULL(state);

    GuiData *data = (GuiData *)state;
    Usize total_size = size + sizeof(Usize);
    Usize *buf = (Usize *)memAlloc(data->allocator, total_size);

    if (buf)
    {
        data->memory.size += total_size;
        data->memory.blocks++;
        *(buf++) = total_size;
    }

    return buf;
}

void
guiFree(void *mem, void *state)
{
    CF_ASSERT_NOT_NULL(state);

    if (mem)
    {
        GuiData *data = (GuiData *)state;
        Usize *buf = (Usize *)mem;
        Usize total_size = *(--buf);

        CF_ASSERT_NOT_NULL(total_size);

        data->memory.size -= total_size;
        data->memory.blocks--;
        memFree(data->allocator, buf, total_size);
    }
}
//=== Initialization ===//

GuiData &
guiData()
{
    return *(GuiData *)ImGui::GetIO().UserData;
}

void
guiSetContext(GuiContext *ctx)
{
    IMGUI_CHECKVERSION();

    CF_ASSERT_NOT_NULL(ctx);

    ImGui::SetCurrentContext(ctx);
    ImGui::SetAllocatorFunctions(guiAlloc, guiFree, &guiData());
    ImPlot::SetCurrentContext(guiData().implot);
}

GuiMemory
guiMemoryInfo(void)
{
    return guiData().memory;
}

void *
guiUserData(void)
{
    return guiData().user_data;
}

void
guiNewFrame(void)
{
    ImGui::NewFrame();
}

GuiDrawData *
guiRender()
{
    ImGui::Render();
    return ImGui::GetDrawData();
}

//=== Themes & styling ===//

// TODO (Matteo): Handle SRGB colorspace correctly

CF_INTERNAL ImVec4
gui_Color(F32 r, F32 g, F32 b, F32 a)
{
    return {r, g, b, a};
}

CF_INTERNAL ImVec4
gui_InvertColor(ImVec4 in)
{
    // NOTE (Matteo): Here I should apply a linear->HSV->linear transformation
    // but currently IMGUI colors are handled as in a SRGB colorspace, so I
    // treat them as such without gamma correction.

    Srgb32 const srgb_in = SRGB32(ImClamp(in.x * 255.0f, 0.0f, 255.0f), //
                                  ImClamp(in.y * 255.0f, 0.0f, 255.0f), //
                                  ImClamp(in.z * 255.0f, 0.0f, 255.0f), //
                                  ImClamp(in.w * 255.0f, 0.0f, 255.0f));

    HsvColor hsv = colorSrgbToHsv(srgb_in);

    if (hsv.s == 0)
    {
        hsv.v = 1.0 - hsv.v;
    }
    else
    {
        hsv.h = ImFmod(hsv.h + 180, 360);
    }

    Srgb32 const srgb_out = colorHsvToSrgb(hsv);
    F32 const f = 1.0f / 255.0f;

    return {
        ImClamp(SRGB32_R(srgb_out) * f, 0.0f, 1.0f),
        ImClamp(SRGB32_G(srgb_out) * f, 0.0f, 1.0f),
        ImClamp(SRGB32_B(srgb_out) * f, 0.0f, 1.0f),
        ImClamp(SRGB32_A(srgb_out) * f, 0.0f, 1.0f),
    };
}

void
guiThemeSelector(Cstr label)
{
#define GUI_THEME_NAME(Name) #Name,

    static Cstr name[GuiTheme_Count] = {GUI_THEMES(GUI_THEME_NAME)};

    GuiData &gui_data = guiData();
    GuiTheme curr = gui_data.theme;
    GuiTheme next = curr;

    if (ImGui::BeginCombo(label, name[curr], 0))
    {
        for (GuiTheme theme = 0; theme < GuiTheme_Count; ++theme)
        {
            bool selected = theme == curr;
            if (ImGui::Selectable(name[theme], selected))
            {
                next = theme;
            }
            if (selected) ImGui::SetItemDefaultFocus();
        }

        ImGui::EndCombo();
    }

    if (next != curr) guiSetTheme(next);
}

Srgb32
guiGetStyledColor(Srgb32 in)
{
    return ImGui::GetColorU32(in);
}

Srgb32
guiGetBackColor(void)
{
    return ImGui::GetColorU32(ImGuiCol_WindowBg, 1.0f);
}

void
guiGammaCorrection(bool enabled)
{
    typedef F32 (*ConvertFn)(F32);
    ConvertFn convert = (enabled) ? colorSrgbDecode : colorSrgbEncode;

    ImVec4 *colors;

    colors = ImGui::GetStyle().Colors;
    for (U32 i = 0; i < ImGuiCol_COUNT; ++i)
    {

        colors[i].x = convert(colors[i].x);
        colors[i].y = convert(colors[i].y);
        colors[i].z = convert(colors[i].z);
    }

    colors = ImPlot::GetStyle().Colors;
    for (U32 i = 0; i < ImPlotCol_COUNT; ++i)
    {
        colors[i].x = convert(colors[i].x);
        colors[i].y = convert(colors[i].y);
        colors[i].z = convert(colors[i].z);
    }
}

void
guiSetTheme(GuiTheme theme)
{
    guiData().theme = theme;

    switch (theme)
    {
        case GuiTheme_ImguiClassic: ImGui::StyleColorsClassic(NULL); break;
        case GuiTheme_ImguiDark: ImGui::StyleColorsDark(NULL); break;
        case GuiTheme_ImguiLight: ImGui::StyleColorsLight(NULL); break;

        case GuiTheme_EmeraldDark:
        case GuiTheme_EmeraldLight:
        {
            ImGui::StyleColorsClassic(NULL);
            ImGuiStyle &style = ImGui::GetStyle();
            // clang-format off
            style.Colors[ImGuiCol_Text]                   = gui_Color(1.00f, 1.00f, 1.00f, 1.00f);
            style.Colors[ImGuiCol_TextDisabled]           = gui_Color(0.50f, 0.50f, 0.50f, 1.00f);
            style.Colors[ImGuiCol_WindowBg]               = gui_Color(0.06f, 0.06f, 0.06f, 0.94f);
            style.Colors[ImGuiCol_ChildBg]                = gui_Color(0.00f, 0.00f, 0.00f, 0.00f);
            style.Colors[ImGuiCol_PopupBg]                = gui_Color(0.08f, 0.08f, 0.08f, 0.94f);
            style.Colors[ImGuiCol_Border]                 = gui_Color(0.43f, 0.43f, 0.50f, 0.50f);
            style.Colors[ImGuiCol_BorderShadow]           = gui_Color(0.00f, 0.00f, 0.00f, 0.00f);
            style.Colors[ImGuiCol_FrameBg]                = gui_Color(0.44f, 0.44f, 0.44f, 0.60f);
            style.Colors[ImGuiCol_FrameBgHovered]         = gui_Color(0.57f, 0.57f, 0.57f, 0.70f);
            style.Colors[ImGuiCol_FrameBgActive]          = gui_Color(0.76f, 0.76f, 0.76f, 0.80f);
            style.Colors[ImGuiCol_TitleBg]                = gui_Color(0.04f, 0.04f, 0.04f, 1.00f);
            style.Colors[ImGuiCol_TitleBgActive]          = gui_Color(0.16f, 0.16f, 0.16f, 1.00f);
            style.Colors[ImGuiCol_TitleBgCollapsed]       = gui_Color(0.00f, 0.00f, 0.00f, 0.60f);
            style.Colors[ImGuiCol_MenuBarBg]              = gui_Color(0.14f, 0.14f, 0.14f, 1.00f);
            style.Colors[ImGuiCol_ScrollbarBg]            = gui_Color(0.02f, 0.02f, 0.02f, 0.53f);
            style.Colors[ImGuiCol_ScrollbarGrab]          = gui_Color(0.31f, 0.31f, 0.31f, 1.00f);
            style.Colors[ImGuiCol_ScrollbarGrabHovered]   = gui_Color(0.41f, 0.41f, 0.41f, 1.00f);
            style.Colors[ImGuiCol_ScrollbarGrabActive]    = gui_Color(0.51f, 0.51f, 0.51f, 1.00f);
            style.Colors[ImGuiCol_CheckMark]              = gui_Color(0.13f, 0.75f, 0.55f, 0.80f);
            style.Colors[ImGuiCol_SliderGrab]             = gui_Color(0.13f, 0.75f, 0.75f, 0.80f);
            style.Colors[ImGuiCol_SliderGrabActive]       = gui_Color(0.13f, 0.75f, 1.00f, 0.80f);
            style.Colors[ImGuiCol_Button]                 = gui_Color(0.13f, 0.75f, 0.55f, 0.40f);
            style.Colors[ImGuiCol_ButtonHovered]          = gui_Color(0.13f, 0.75f, 0.75f, 0.60f);
            style.Colors[ImGuiCol_ButtonActive]           = gui_Color(0.13f, 0.75f, 1.00f, 0.80f);
            style.Colors[ImGuiCol_Header]                 = gui_Color(0.13f, 0.75f, 0.55f, 0.40f);
            style.Colors[ImGuiCol_HeaderHovered]          = gui_Color(0.13f, 0.75f, 0.75f, 0.60f);
            style.Colors[ImGuiCol_HeaderActive]           = gui_Color(0.13f, 0.75f, 1.00f, 0.80f);
            style.Colors[ImGuiCol_Separator]              = gui_Color(0.13f, 0.75f, 0.55f, 0.40f);
            style.Colors[ImGuiCol_SeparatorHovered]       = gui_Color(0.13f, 0.75f, 0.75f, 0.60f);
            style.Colors[ImGuiCol_SeparatorActive]        = gui_Color(0.13f, 0.75f, 1.00f, 0.80f);
            style.Colors[ImGuiCol_ResizeGrip]             = gui_Color(0.13f, 0.75f, 0.55f, 0.40f);
            style.Colors[ImGuiCol_ResizeGripHovered]      = gui_Color(0.13f, 0.75f, 0.75f, 0.60f);
            style.Colors[ImGuiCol_ResizeGripActive]       = gui_Color(0.13f, 0.75f, 1.00f, 0.80f);
            style.Colors[ImGuiCol_Tab]                    = gui_Color(0.13f, 0.75f, 0.55f, 0.80f);
            style.Colors[ImGuiCol_TabHovered]             = gui_Color(0.13f, 0.75f, 0.75f, 0.80f);
            style.Colors[ImGuiCol_TabActive]              = gui_Color(0.13f, 0.75f, 1.00f, 0.80f);
            style.Colors[ImGuiCol_TabUnfocused]           = gui_Color(0.18f, 0.18f, 0.18f, 1.00f);
            style.Colors[ImGuiCol_TabUnfocusedActive]     = gui_Color(0.36f, 0.36f, 0.36f, 0.54f);
            style.Colors[ImGuiCol_DockingPreview]         = gui_Color(0.13f, 0.75f, 0.55f, 0.80f);
            style.Colors[ImGuiCol_DockingEmptyBg]         = gui_Color(0.13f, 0.13f, 0.13f, 0.80f);
            style.Colors[ImGuiCol_PlotLines]              = gui_Color(0.61f, 0.61f, 0.61f, 1.00f);
            style.Colors[ImGuiCol_PlotLinesHovered]       = gui_Color(1.00f, 0.43f, 0.35f, 1.00f);
            style.Colors[ImGuiCol_PlotHistogram]          = gui_Color(0.90f, 0.70f, 0.00f, 1.00f);
            style.Colors[ImGuiCol_PlotHistogramHovered]   = gui_Color(1.00f, 0.60f, 0.00f, 1.00f);
            style.Colors[ImGuiCol_TableHeaderBg]          = gui_Color(0.19f, 0.19f, 0.20f, 1.00f);
            style.Colors[ImGuiCol_TableBorderStrong]      = gui_Color(0.31f, 0.31f, 0.35f, 1.00f);
            style.Colors[ImGuiCol_TableBorderLight]       = gui_Color(0.23f, 0.23f, 0.25f, 1.00f);
            style.Colors[ImGuiCol_TableRowBg]             = gui_Color(0.00f, 0.00f, 0.00f, 0.00f);
            style.Colors[ImGuiCol_TableRowBgAlt]          = gui_Color(1.00f, 1.00f, 1.00f, 0.07f);
            style.Colors[ImGuiCol_TextSelectedBg]         = gui_Color(0.26f, 0.59f, 0.98f, 0.35f);
            style.Colors[ImGuiCol_DragDropTarget]         = gui_Color(1.00f, 1.00f, 0.00f, 0.90f);
            style.Colors[ImGuiCol_NavHighlight]           = gui_Color(0.26f, 0.59f, 0.98f, 1.00f);
            style.Colors[ImGuiCol_NavWindowingHighlight]  = gui_Color(1.00f, 1.00f, 1.00f, 0.70f);
            style.Colors[ImGuiCol_NavWindowingDimBg]      = gui_Color(0.80f, 0.80f, 0.80f, 0.20f);
            style.Colors[ImGuiCol_ModalWindowDimBg]       = gui_Color(0.80f, 0.80f, 0.80f, 0.35f);
            // clang-format on
        }
        break;

        case GuiTheme_LightBlue:
        {
            ImGui::StyleColorsClassic(NULL);
            ImGuiStyle &style = ImGui::GetStyle();
            // clang-format off
            style.Colors[ImGuiCol_Text]                   = gui_Color(0.00f, 0.00f, 0.00f, 1.00f);
            style.Colors[ImGuiCol_TextDisabled]           = gui_Color(0.60f, 0.60f, 0.60f, 1.00f);
            style.Colors[ImGuiCol_WindowBg]               = gui_Color(0.89f, 0.89f, 0.89f, 1.00f);
            style.Colors[ImGuiCol_ChildBg]                = gui_Color(0.00f, 0.00f, 0.00f, 0.00f);
            style.Colors[ImGuiCol_PopupBg]                = gui_Color(1.00f, 1.00f, 1.00f, 0.98f);
            style.Colors[ImGuiCol_Border]                 = gui_Color(0.00f, 0.00f, 0.00f, 0.30f);
            style.Colors[ImGuiCol_BorderShadow]           = gui_Color(0.00f, 0.00f, 0.00f, 0.00f);
            style.Colors[ImGuiCol_FrameBg]                = gui_Color(1.00f, 1.00f, 1.00f, 1.00f);
            style.Colors[ImGuiCol_FrameBgHovered]         = gui_Color(0.26f, 0.59f, 0.98f, 0.40f);
            style.Colors[ImGuiCol_FrameBgActive]          = gui_Color(0.26f, 0.59f, 0.98f, 0.67f);
            style.Colors[ImGuiCol_TitleBg]                = gui_Color(0.96f, 0.96f, 0.96f, 1.00f);
            style.Colors[ImGuiCol_TitleBgActive]          = gui_Color(0.82f, 0.82f, 0.82f, 1.00f);
            style.Colors[ImGuiCol_TitleBgCollapsed]       = gui_Color(1.00f, 1.00f, 1.00f, 0.51f);
            style.Colors[ImGuiCol_MenuBarBg]              = gui_Color(0.86f, 0.86f, 0.86f, 1.00f);
            style.Colors[ImGuiCol_ScrollbarBg]            = gui_Color(0.98f, 0.98f, 0.98f, 0.53f);
            style.Colors[ImGuiCol_ScrollbarGrab]          = gui_Color(0.69f, 0.69f, 0.69f, 0.80f);
            style.Colors[ImGuiCol_ScrollbarGrabHovered]   = gui_Color(0.49f, 0.49f, 0.49f, 0.80f);
            style.Colors[ImGuiCol_ScrollbarGrabActive]    = gui_Color(0.49f, 0.49f, 0.49f, 1.00f);
            style.Colors[ImGuiCol_CheckMark]              = gui_Color(0.26f, 0.59f, 0.98f, 1.00f);
            style.Colors[ImGuiCol_SliderGrab]             = gui_Color(0.26f, 0.59f, 0.98f, 0.78f);
            style.Colors[ImGuiCol_SliderGrabActive]       = gui_Color(0.46f, 0.54f, 0.80f, 0.60f);
            style.Colors[ImGuiCol_Button]                 = gui_Color(0.26f, 0.59f, 0.98f, 0.40f);
            style.Colors[ImGuiCol_ButtonHovered]          = gui_Color(0.26f, 0.59f, 0.98f, 1.00f);
            style.Colors[ImGuiCol_ButtonActive]           = gui_Color(0.06f, 0.53f, 0.98f, 1.00f);
            style.Colors[ImGuiCol_Header]                 = gui_Color(0.26f, 0.59f, 0.98f, 0.31f);
            style.Colors[ImGuiCol_HeaderHovered]          = gui_Color(0.26f, 0.59f, 0.98f, 0.80f);
            style.Colors[ImGuiCol_HeaderActive]           = gui_Color(0.26f, 0.59f, 0.98f, 1.00f);
            style.Colors[ImGuiCol_Separator]              = gui_Color(0.39f, 0.39f, 0.39f, 0.62f);
            style.Colors[ImGuiCol_SeparatorHovered]       = gui_Color(0.14f, 0.44f, 0.80f, 0.78f);
            style.Colors[ImGuiCol_SeparatorActive]        = gui_Color(0.14f, 0.44f, 0.80f, 1.00f);
            style.Colors[ImGuiCol_ResizeGrip]             = gui_Color(0.35f, 0.35f, 0.35f, 0.17f);
            style.Colors[ImGuiCol_ResizeGripHovered]      = gui_Color(0.26f, 0.59f, 0.98f, 0.67f);
            style.Colors[ImGuiCol_ResizeGripActive]       = gui_Color(0.26f, 0.59f, 0.98f, 0.95f);
            style.Colors[ImGuiCol_Tab]                    = gui_Color(0.76f, 0.80f, 0.84f, 0.93f);
            style.Colors[ImGuiCol_TabHovered]             = gui_Color(0.26f, 0.59f, 0.98f, 0.80f);
            style.Colors[ImGuiCol_TabActive]              = gui_Color(0.60f, 0.73f, 0.88f, 1.00f);
            style.Colors[ImGuiCol_TabUnfocused]           = gui_Color(0.92f, 0.93f, 0.94f, 0.99f);
            style.Colors[ImGuiCol_TabUnfocusedActive]     = gui_Color(0.74f, 0.82f, 0.91f, 1.00f);
            style.Colors[ImGuiCol_DockingPreview]         = gui_Color(0.26f, 0.59f, 0.98f, 0.22f);
            style.Colors[ImGuiCol_DockingEmptyBg]         = gui_Color(0.20f, 0.20f, 0.20f, 1.00f);
            style.Colors[ImGuiCol_PlotLines]              = gui_Color(0.39f, 0.39f, 0.39f, 1.00f);
            style.Colors[ImGuiCol_PlotLinesHovered]       = gui_Color(1.00f, 0.43f, 0.35f, 1.00f);
            style.Colors[ImGuiCol_PlotHistogram]          = gui_Color(0.90f, 0.70f, 0.00f, 1.00f);
            style.Colors[ImGuiCol_PlotHistogramHovered]   = gui_Color(1.00f, 0.45f, 0.00f, 1.00f);
            style.Colors[ImGuiCol_TableHeaderBg]          = gui_Color(0.78f, 0.87f, 0.98f, 1.00f);
            style.Colors[ImGuiCol_TableBorderStrong]      = gui_Color(0.57f, 0.57f, 0.64f, 1.00f);
            style.Colors[ImGuiCol_TableBorderLight]       = gui_Color(0.68f, 0.68f, 0.74f, 1.00f);
            style.Colors[ImGuiCol_TableRowBg]             = gui_Color(0.00f, 0.00f, 0.00f, 0.00f);
            style.Colors[ImGuiCol_TableRowBgAlt]          = gui_Color(0.30f, 0.30f, 0.30f, 0.09f);
            style.Colors[ImGuiCol_TextSelectedBg]         = gui_Color(0.26f, 0.59f, 0.98f, 0.35f);
            style.Colors[ImGuiCol_DragDropTarget]         = gui_Color(0.26f, 0.59f, 0.98f, 0.95f);
            style.Colors[ImGuiCol_NavHighlight]           = gui_Color(0.26f, 0.59f, 0.98f, 0.80f);
            style.Colors[ImGuiCol_NavWindowingHighlight]  = gui_Color(0.70f, 0.70f, 0.70f, 0.70f);
            style.Colors[ImGuiCol_NavWindowingDimBg]      = gui_Color(0.20f, 0.20f, 0.20f, 0.20f);
            style.Colors[ImGuiCol_ModalWindowDimBg]       = gui_Color(0.20f, 0.20f, 0.20f, 0.35f);
            // clang-format on
        }
        break;

        case GuiTheme_LightGreen:
        {
            ImGuiStyle &style = ImGui::GetStyle();

            // clang-format off
            style.Colors[ImGuiCol_Text]                  = gui_Color(0.00f, 0.00f, 0.00f, 1.00f);
            style.Colors[ImGuiCol_TextDisabled]          = gui_Color(0.60f, 0.60f, 0.60f, 1.00f);
            style.Colors[ImGuiCol_WindowBg]              = gui_Color(0.86f, 0.86f, 0.86f, 1.00f);
            style.Colors[ImGuiCol_ChildBg]               = gui_Color(0.00f, 0.00f, 0.00f, 0.00f);
            style.Colors[ImGuiCol_PopupBg]               = gui_Color(0.93f, 0.93f, 0.93f, 0.98f);
            style.Colors[ImGuiCol_Border]                = gui_Color(0.71f, 0.71f, 0.71f, 0.08f);
            style.Colors[ImGuiCol_BorderShadow]          = gui_Color(0.00f, 0.00f, 0.00f, 0.04f);
            style.Colors[ImGuiCol_FrameBg]               = gui_Color(0.71f, 0.71f, 0.71f, 0.55f);
            style.Colors[ImGuiCol_FrameBgHovered]        = gui_Color(0.94f, 0.94f, 0.94f, 0.55f);
            style.Colors[ImGuiCol_FrameBgActive]         = gui_Color(0.71f, 0.78f, 0.69f, 0.98f);
            style.Colors[ImGuiCol_TitleBg]               = gui_Color(0.85f, 0.85f, 0.85f, 1.00f);
            style.Colors[ImGuiCol_TitleBgCollapsed]      = gui_Color(0.82f, 0.78f, 0.78f, 0.51f);
            style.Colors[ImGuiCol_TitleBgActive]         = gui_Color(0.78f, 0.78f, 0.78f, 1.00f);
            style.Colors[ImGuiCol_MenuBarBg]             = gui_Color(0.86f, 0.86f, 0.86f, 1.00f);
            style.Colors[ImGuiCol_ScrollbarBg]           = gui_Color(0.20f, 0.25f, 0.30f, 0.61f);
            style.Colors[ImGuiCol_ScrollbarGrab]         = gui_Color(0.90f, 0.90f, 0.90f, 0.30f);
            style.Colors[ImGuiCol_ScrollbarGrabHovered]  = gui_Color(0.92f, 0.92f, 0.92f, 0.78f);
            style.Colors[ImGuiCol_ScrollbarGrabActive]   = gui_Color(1.00f, 1.00f, 1.00f, 1.00f);
            style.Colors[ImGuiCol_CheckMark]             = gui_Color(0.184f, 0.407f, 0.193f, 1.00f);
            style.Colors[ImGuiCol_SliderGrab]            = gui_Color(0.26f, 0.59f, 0.98f, 0.78f);
            style.Colors[ImGuiCol_SliderGrabActive]      = gui_Color(0.26f, 0.59f, 0.98f, 1.00f);
            style.Colors[ImGuiCol_Button]                = gui_Color(0.71f, 0.78f, 0.69f, 0.40f);
            style.Colors[ImGuiCol_ButtonHovered]         = gui_Color(0.725f, 0.805f, 0.702f, 1.00f);
            style.Colors[ImGuiCol_ButtonActive]          = gui_Color(0.793f, 0.900f, 0.836f, 1.00f);
            style.Colors[ImGuiCol_Header]                = gui_Color(0.71f, 0.78f, 0.69f, 0.31f);
            style.Colors[ImGuiCol_HeaderHovered]         = gui_Color(0.71f, 0.78f, 0.69f, 0.80f);
            style.Colors[ImGuiCol_HeaderActive]          = gui_Color(0.71f, 0.78f, 0.69f, 1.00f);
            style.Colors[ImGuiCol_Separator]             = gui_Color(0.39f, 0.39f, 0.39f, 1.00f);
            style.Colors[ImGuiCol_SeparatorHovered]      = gui_Color(0.14f, 0.44f, 0.80f, 0.78f);
            style.Colors[ImGuiCol_SeparatorActive]       = gui_Color(0.14f, 0.44f, 0.80f, 1.00f);
            style.Colors[ImGuiCol_ResizeGrip]            = gui_Color(1.00f, 1.00f, 1.00f, 0.00f);
            style.Colors[ImGuiCol_ResizeGripHovered]     = gui_Color(0.26f, 0.59f, 0.98f, 0.45f);
            style.Colors[ImGuiCol_ResizeGripActive]      = gui_Color(0.26f, 0.59f, 0.98f, 0.78f);
            style.Colors[ImGuiCol_PlotLines]             = gui_Color(0.39f, 0.39f, 0.39f, 1.00f);
            style.Colors[ImGuiCol_PlotLinesHovered]      = gui_Color(1.00f, 0.43f, 0.35f, 1.00f);
            style.Colors[ImGuiCol_PlotHistogram]         = gui_Color(0.90f, 0.70f, 0.00f, 1.00f);
            style.Colors[ImGuiCol_PlotHistogramHovered]  = gui_Color(1.00f, 0.60f, 0.00f, 1.00f);
            style.Colors[ImGuiCol_TextSelectedBg]        = gui_Color(0.26f, 0.59f, 0.98f, 0.35f);
            style.Colors[ImGuiCol_DragDropTarget]        = gui_Color(0.26f, 0.59f, 0.98f, 0.95f);
            style.Colors[ImGuiCol_NavHighlight]          = style.Colors[ImGuiCol_HeaderHovered];
            style.Colors[ImGuiCol_NavWindowingHighlight] = gui_Color(0.70f, 0.70f, 0.70f, 0.70f);
            // clang-format on
        }
        break;

        case GuiTheme_Dummy:
        {
            ImGui::StyleColorsLight(NULL);
            ImGuiStyle &style = ImGui::GetStyle();
            // clang-format off
            style.Colors[ImGuiCol_Text]                 = gui_Color(0.31f, 0.25f, 0.24f, 1.00f);
            style.Colors[ImGuiCol_WindowBg]             = gui_Color(0.94f, 0.94f, 0.94f, 1.00f);
            style.Colors[ImGuiCol_MenuBarBg]            = gui_Color(0.74f, 0.74f, 0.94f, 1.00f);
            style.Colors[ImGuiCol_ChildBg]              = gui_Color(0.68f, 0.68f, 0.68f, 0.00f);
            style.Colors[ImGuiCol_Border]               = gui_Color(0.50f, 0.50f, 0.50f, 0.60f);
            style.Colors[ImGuiCol_BorderShadow]         = gui_Color(0.00f, 0.00f, 0.00f, 0.00f);
            style.Colors[ImGuiCol_FrameBg]              = gui_Color(0.62f, 0.70f, 0.72f, 0.56f);
            style.Colors[ImGuiCol_FrameBgHovered]       = gui_Color(0.95f, 0.33f, 0.14f, 0.47f);
            style.Colors[ImGuiCol_FrameBgActive]        = gui_Color(0.97f, 0.31f, 0.13f, 0.81f);
            style.Colors[ImGuiCol_TitleBg]              = gui_Color(0.42f, 0.75f, 1.00f, 0.53f);
            style.Colors[ImGuiCol_TitleBgCollapsed]     = gui_Color(0.40f, 0.65f, 0.80f, 0.20f);
            style.Colors[ImGuiCol_ScrollbarBg]          = gui_Color(0.40f, 0.62f, 0.80f, 0.15f);
            style.Colors[ImGuiCol_ScrollbarGrab]        = gui_Color(0.39f, 0.64f, 0.80f, 0.30f);
            style.Colors[ImGuiCol_ScrollbarGrabHovered] = gui_Color(0.28f, 0.67f, 0.80f, 0.59f);
            style.Colors[ImGuiCol_ScrollbarGrabActive]  = gui_Color(0.25f, 0.48f, 0.53f, 0.67f);
            style.Colors[ImGuiCol_CheckMark]            = gui_Color(0.48f, 0.47f, 0.47f, 0.71f);
            style.Colors[ImGuiCol_SliderGrabActive]     = gui_Color(0.31f, 0.47f, 0.99f, 1.00f);
            style.Colors[ImGuiCol_Button]               = gui_Color(1.00f, 0.79f, 0.18f, 0.78f);
            style.Colors[ImGuiCol_ButtonHovered]        = gui_Color(0.42f, 0.82f, 1.00f, 0.81f);
            style.Colors[ImGuiCol_ButtonActive]         = gui_Color(0.72f, 1.00f, 1.00f, 0.86f);
            style.Colors[ImGuiCol_Header]               = gui_Color(0.65f, 0.78f, 0.84f, 0.80f);
            style.Colors[ImGuiCol_HeaderHovered]        = gui_Color(0.75f, 0.88f, 0.94f, 0.80f);
            style.Colors[ImGuiCol_HeaderActive]         = gui_Color(0.55f, 0.68f, 0.74f, 0.80f);//Rgba(0.46f, 0.84f, 0.90f, 1.00f);
            style.Colors[ImGuiCol_ResizeGrip]           = gui_Color(0.60f, 0.60f, 0.80f, 0.30f);
            style.Colors[ImGuiCol_ResizeGripHovered]    = gui_Color(1.00f, 1.00f, 1.00f, 0.60f);
            style.Colors[ImGuiCol_ResizeGripActive]     = gui_Color(1.00f, 1.00f, 1.00f, 0.90f);
            style.Colors[ImGuiCol_TextSelectedBg]       = gui_Color(1.00f, 0.99f, 0.54f, 0.43f);
            style.Colors[ImGuiCol_PopupBg]              = gui_Color(0.82f, 0.92f, 1.00f, 0.90f); // LinearColor(0.89f, 0.98f, 1.00f, 0.99f)
            // colors[ImGuiCol_ComboBg] = LinearColor(0.89f, 0.98f, 1.00f, 0.99f);
            // colors[ImGuiCol_CloseButton] = LinearColor(0.41f, 0.75f, 0.98f, 0.50f);
            // colors[ImGuiCol_CloseButtonHovered] = LinearColor(1.00f, 0.47f, 0.41f, 0.60f);
            // colors[ImGuiCol_CloseButtonActive] = LinearColor(1.00f, 0.16f, 0.00f, 1.00f);
            // clang-format on
        }
        break;

        case GuiTheme_Rugged:
        {
            ImGuiStyle &style = ImGui::GetStyle();
            // clang-format off
            style.Colors[ImGuiCol_Text]                  = gui_Color(1.00f, 1.00f, 1.00f, 1.00f);
            style.Colors[ImGuiCol_TextDisabled]          = gui_Color(0.50f, 0.50f, 0.50f, 1.00f);
            style.Colors[ImGuiCol_WindowBg]              = gui_Color(0.29f, 0.34f, 0.26f, 1.00f);
            style.Colors[ImGuiCol_ChildBg]               = gui_Color(0.29f, 0.34f, 0.26f, 1.00f);
            style.Colors[ImGuiCol_PopupBg]               = gui_Color(0.24f, 0.27f, 0.20f, 1.00f);
            style.Colors[ImGuiCol_Border]                = gui_Color(0.54f, 0.57f, 0.51f, 0.50f);
            style.Colors[ImGuiCol_BorderShadow]          = gui_Color(0.14f, 0.16f, 0.11f, 0.52f);
            style.Colors[ImGuiCol_FrameBg]               = gui_Color(0.24f, 0.27f, 0.20f, 1.00f);
            style.Colors[ImGuiCol_FrameBgHovered]        = gui_Color(0.27f, 0.30f, 0.23f, 1.00f);
            style.Colors[ImGuiCol_FrameBgActive]         = gui_Color(0.30f, 0.34f, 0.26f, 1.00f);
            style.Colors[ImGuiCol_TitleBg]               = gui_Color(0.24f, 0.27f, 0.20f, 1.00f);
            style.Colors[ImGuiCol_TitleBgActive]         = gui_Color(0.29f, 0.34f, 0.26f, 1.00f);
            style.Colors[ImGuiCol_TitleBgCollapsed]      = gui_Color(0.00f, 0.00f, 0.00f, 0.51f);
            style.Colors[ImGuiCol_MenuBarBg]             = gui_Color(0.24f, 0.27f, 0.20f, 1.00f);
            style.Colors[ImGuiCol_ScrollbarBg]           = gui_Color(0.35f, 0.42f, 0.31f, 1.00f);
            style.Colors[ImGuiCol_ScrollbarGrab]         = gui_Color(0.28f, 0.32f, 0.24f, 1.00f);
            style.Colors[ImGuiCol_ScrollbarGrabHovered]  = gui_Color(0.25f, 0.30f, 0.22f, 1.00f);
            style.Colors[ImGuiCol_ScrollbarGrabActive]   = gui_Color(0.23f, 0.27f, 0.21f, 1.00f);
            style.Colors[ImGuiCol_CheckMark]             = gui_Color(0.59f, 0.54f, 0.18f, 1.00f);
            style.Colors[ImGuiCol_SliderGrab]            = gui_Color(0.35f, 0.42f, 0.31f, 1.00f);
            style.Colors[ImGuiCol_SliderGrabActive]      = gui_Color(0.54f, 0.57f, 0.51f, 0.50f);
            style.Colors[ImGuiCol_Button]                = gui_Color(0.29f, 0.34f, 0.26f, 0.40f);
            style.Colors[ImGuiCol_ButtonHovered]         = gui_Color(0.35f, 0.42f, 0.31f, 1.00f);
            style.Colors[ImGuiCol_ButtonActive]          = gui_Color(0.54f, 0.57f, 0.51f, 0.50f);
            style.Colors[ImGuiCol_Header]                = gui_Color(0.35f, 0.42f, 0.31f, 1.00f);
            style.Colors[ImGuiCol_HeaderHovered]         = gui_Color(0.35f, 0.42f, 0.31f, 0.6f);
            style.Colors[ImGuiCol_HeaderActive]          = gui_Color(0.54f, 0.57f, 0.51f, 0.50f);
            style.Colors[ImGuiCol_Separator]             = gui_Color(0.14f, 0.16f, 0.11f, 1.00f);
            style.Colors[ImGuiCol_SeparatorHovered]      = gui_Color(0.54f, 0.57f, 0.51f, 1.00f);
            style.Colors[ImGuiCol_SeparatorActive]       = gui_Color(0.59f, 0.54f, 0.18f, 1.00f);
            style.Colors[ImGuiCol_ResizeGrip]            = gui_Color(0.19f, 0.23f, 0.18f, 0.00f); // grip invis
            style.Colors[ImGuiCol_ResizeGripHovered]     = gui_Color(0.54f, 0.57f, 0.51f, 1.00f);
            style.Colors[ImGuiCol_ResizeGripActive]      = gui_Color(0.59f, 0.54f, 0.18f, 1.00f);
            style.Colors[ImGuiCol_Tab]                   = gui_Color(0.35f, 0.42f, 0.31f, 1.00f);
            style.Colors[ImGuiCol_TabHovered]            = gui_Color(0.54f, 0.57f, 0.51f, 0.78f);
            style.Colors[ImGuiCol_TabActive]             = gui_Color(0.59f, 0.54f, 0.18f, 1.00f);
            style.Colors[ImGuiCol_TabUnfocused]          = gui_Color(0.24f, 0.27f, 0.20f, 1.00f);
            style.Colors[ImGuiCol_TabUnfocusedActive]    = gui_Color(0.35f, 0.42f, 0.31f, 1.00f);
            style.Colors[ImGuiCol_DockingPreview]        = gui_Color(0.59f, 0.54f, 0.18f, 1.00f);
            style.Colors[ImGuiCol_DockingEmptyBg]        = gui_Color(0.20f, 0.20f, 0.20f, 1.00f);
            style.Colors[ImGuiCol_PlotLines]             = gui_Color(0.61f, 0.61f, 0.61f, 1.00f);
            style.Colors[ImGuiCol_PlotLinesHovered]      = gui_Color(0.59f, 0.54f, 0.18f, 1.00f);
            style.Colors[ImGuiCol_PlotHistogram]         = gui_Color(1.00f, 0.78f, 0.28f, 1.00f);
            style.Colors[ImGuiCol_PlotHistogramHovered]  = gui_Color(1.00f, 0.60f, 0.00f, 1.00f);
            style.Colors[ImGuiCol_TextSelectedBg]        = gui_Color(0.59f, 0.54f, 0.18f, 1.00f);
            style.Colors[ImGuiCol_DragDropTarget]        = gui_Color(0.73f, 0.67f, 0.24f, 1.00f);
            style.Colors[ImGuiCol_NavHighlight]          = gui_Color(0.59f, 0.54f, 0.18f, 1.00f);
            style.Colors[ImGuiCol_NavWindowingHighlight] = gui_Color(1.00f, 1.00f, 1.00f, 0.70f);
            style.Colors[ImGuiCol_NavWindowingDimBg]     = gui_Color(0.80f, 0.80f, 0.80f, 0.20f);
            style.Colors[ImGuiCol_ModalWindowDimBg]      = gui_Color(0.80f, 0.80f, 0.80f, 0.35f);
            // clang-format on
        }
        break;

        default: CF_INVALID_CODE_PATH(); break;
    }

    if (theme == GuiTheme_EmeraldLight)
    {
        ImGuiStyle &style = ImGui::GetStyle();
        for (U32 i = 0; i < ImGuiCol_COUNT; ++i)
        {
            style.Colors[i] = gui_InvertColor(style.Colors[i]);
        }
    }
}

CF_INTERNAL void
gui_SetSizes(ImGuiStyle *style)
{
    // Main
    // **DEFAULT**

    // Borders
    style->WindowBorderSize = 1.0f;
    style->ChildBorderSize = 1.0f;
    style->PopupBorderSize = 1.0f;
    style->FrameBorderSize = 1.0f;
    style->TabBorderSize = 1.0f;

    // Rounding
    style->WindowRounding = 0.0f;
    style->ChildRounding = 0.0f;
    style->FrameRounding = 0.0f;
    style->PopupRounding = 2.0f;
    style->ScrollbarRounding = style->GrabRounding = 1.0f;
    style->LogSliderDeadzone = 3.0f;
    style->TabRounding = 2.0f;

    // Alignment
    // **DEFAULT**
}

void
guiSetupStyle(GuiTheme theme, F32 dpi_scale)
{
    ImGuiStyle &style = ImGui::GetStyle();

    guiSetTheme(theme);
    gui_SetSizes(&style);

    // When viewports are enabled we tweak WindowRounding/WindowBg so platform
    // windows can look identical to regular ones.
    if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        style.WindowRounding = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }

    style.ScaleAllSizes(dpi_scale);
}

//=== Fonts handling ===//

bool
guiFontOptionsEdit(GuiFontOptions *state)
{
    bool rebuild_fonts = false;

    ImGui::ShowFontSelector("Fonts");

#if GUI_FREETYPE
    if (ImGui::RadioButton("FreeType", !state->freetype_disabled))
    {
        state->freetype_disabled = false;
        rebuild_fonts = true;
    }
    guiSameLine();
    if (ImGui::RadioButton("Stb (Default)", state->freetype_disabled))
    {
        state->freetype_disabled = true;
        rebuild_fonts = true;
    }
#endif

    rebuild_fonts |= ImGui::DragInt("TexGlyphPadding", &state->tex_glyph_padding, 0.1f, 1, 16, NULL,
                                    ImGuiSliderFlags_None);

    rebuild_fonts |= ImGui::DragFloat("RasterizerMultiply", &state->rasterizer_multiply, 0.001f,
                                      0.0f, 2.0f, NULL, ImGuiSliderFlags_None);

    ImGui::Separator();

#if GUI_FREETYPE
    if (!state->freetype_disabled)
    {

        rebuild_fonts |= ImGui::CheckboxFlags("NoHinting", &state->freetype_flags,
                                              ImGuiFreeTypeBuilderFlags_NoHinting);
        rebuild_fonts |= ImGui::CheckboxFlags("NoAutoHint", &state->freetype_flags,
                                              ImGuiFreeTypeBuilderFlags_NoAutoHint);
        rebuild_fonts |= ImGui::CheckboxFlags("ForceAutoHint", &state->freetype_flags,
                                              ImGuiFreeTypeBuilderFlags_ForceAutoHint);
        rebuild_fonts |= ImGui::CheckboxFlags("LightHinting", &state->freetype_flags,
                                              ImGuiFreeTypeBuilderFlags_LightHinting);
        rebuild_fonts |= ImGui::CheckboxFlags("MonoHinting", &state->freetype_flags,
                                              ImGuiFreeTypeBuilderFlags_MonoHinting);
        rebuild_fonts |=
            ImGui::CheckboxFlags("Bold", &state->freetype_flags, ImGuiFreeTypeBuilderFlags_Bold);
        rebuild_fonts |= ImGui::CheckboxFlags("Oblique", &state->freetype_flags,
                                              ImGuiFreeTypeBuilderFlags_Oblique);
        rebuild_fonts |= ImGui::CheckboxFlags("Monochrome", &state->freetype_flags,
                                              ImGuiFreeTypeBuilderFlags_Monochrome);
    }
    else
#endif
    {
        rebuild_fonts |= ImGui::DragInt("Oversample H", &state->oversample_h, 0.1f, 1, 5, NULL,
                                        ImGuiSliderFlags_None);
        rebuild_fonts |= ImGui::DragInt("Oversample V", &state->oversample_v, 0.1f, 1, 5, NULL,
                                        ImGuiSliderFlags_None);
    }

    return rebuild_fonts;
}

void
guiUpdateAtlas(ImFontAtlas *fonts, GuiFontOptions *font_opts)
{
    if (font_opts->oversample_h < 1) font_opts->oversample_h = 2;
    if (font_opts->oversample_v < 1) font_opts->oversample_v = 2;

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

#if GUI_FREETYPE
    if (!font_opts->freetype_disabled)
    {
        fonts->FontBuilderIO = ImGuiFreeType::GetBuilderForFreeType();
        fonts->FontBuilderFlags = (U32)font_opts->freetype_flags;
    }
    else
#endif
    {
        fonts->FontBuilderIO = ImFontAtlasGetBuilderForStbTruetype();
    }

    fonts->Build();

    font_opts->tex_glyph_padding = fonts->TexGlyphPadding;
}

ImFontAtlas *
guiFonts(void)
{
    return ImGui::GetIO().Fonts;
}

ImFont *
guiLoadFont(ImFontAtlas *fonts, Cstr file_name, F32 font_size)
{
    return fonts->AddFontFromFileTTF(file_name, font_size, NULL, fonts->GetGlyphRangesDefault());
}

ImFont *
guiLoadDefaultFont(ImFontAtlas *fonts)
{
    return fonts->AddFontDefault(NULL);
}

//=== Windows ===//

CF_INTERNAL U32
gui_DockSplit(GuiDockLayout *layout, ImGuiDir dir, F32 size_ratio)
{
    U32 id = U32_MAX;
    if (layout->open)
    {
        id = ImGui::DockBuilderSplitNode(layout->id, dir, size_ratio, NULL, &layout->id);
    }
    return id;
}

CF_INTERNAL U32
gui_DockSpaceOnMainViewport(ImGuiDockNodeFlags dock_flags)
{
    ImGuiViewport const *viewport = ImGui::GetMainViewport();
    return ImGui::DockSpaceOverViewport(viewport, dock_flags, NULL);
}

void
guiDockSpace(GuiDockStyle style)
{
    CF_STATIC_ASSERT(ImGuiDockNodeFlags_PassthruCentralNode ==
                         (GuiDockStyle)GuiDockStyle_Transparent,
                     "ImGuiDockNodeFlags changed");

    CF_STATIC_ASSERT(ImGuiDockNodeFlags_NoDockingInCentralNode ==
                         (GuiDockStyle)GuiDockStyle_CentralViewport,
                     "ImGuiDockNodeFlags changed");

    gui_DockSpaceOnMainViewport(style);
}

void
guiTransparentDockSpace(bool can_fill)
{
    ImGuiDockNodeFlags dock_flags = ImGuiDockNodeFlags_PassthruCentralNode;
    if (!can_fill) dock_flags |= ImGuiDockNodeFlags_NoDockingInCentralNode;
    gui_DockSpaceOnMainViewport(dock_flags);
}

GuiDockLayout
guiDockLayout(void)
{
    // NOTE (Matteo): Setup docking layout on first run (if the dockspace node is already split
    // the layout has been setup and maybe modified by the user). This code is partially copied
    // from github since the DockBuilder API is not documented - understand it better!

    ImGuiDockNodeFlags const dock_flags = ImGuiDockNodeFlags_NoDockingInCentralNode;

    GuiDockLayout layout{};

    layout.id = gui_DockSpaceOnMainViewport(dock_flags);
    layout.node = ImGui::DockBuilderGetNode(layout.id);
    layout.open = (!layout.node || !layout.node->IsSplitNode());

    return layout;
}

U32
guiDockSplitUp(GuiDockLayout *layout, F32 size_ratio)
{
    return gui_DockSplit(layout, ImGuiDir_Up, size_ratio);
}

U32
guiDockSplitDown(GuiDockLayout *layout, F32 size_ratio)
{
    return gui_DockSplit(layout, ImGuiDir_Down, size_ratio);
}

U32
guiDockSplitLeft(GuiDockLayout *layout, F32 size_ratio)
{
    return gui_DockSplit(layout, ImGuiDir_Left, size_ratio);
}

U32
guiDockSplitRight(GuiDockLayout *layout, F32 size_ratio)
{
    return gui_DockSplit(layout, ImGuiDir_Right, size_ratio);
}

bool
guiDockWindow(GuiDockLayout *layout, Cstr name, U32 dock_id)
{
    if (layout->open && dock_id != U32_MAX)
    {
        ImGui::DockBuilderDockWindow(name, dock_id);
        return true;
    }

    return false;
}

void
guiSetNextWindowSize(Vec2 size, GuiCond cond)
{
    ImGui::SetNextWindowSize(size, cond);
}

bool
guiBegin(Cstr name, bool *p_open)
{
    return ImGui::Begin(name, p_open, ImGuiWindowFlags_HorizontalScrollbar);
}

bool
guiBeginAutoResize(Cstr name, bool *p_open)
{
    return ImGui::Begin(name, p_open, ImGuiWindowFlags_AlwaysAutoResize);
}

bool
guiBeginLayout(Cstr name, GuiDockLayout *layout)
{
    if (layout->open)
    {
        ImGui::DockBuilderDockWindow(name, layout->id);
        ImGui::DockBuilderFinish(layout->id);
    }

    ImGuiWindowFlags const window_flags =
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus |
        ImGuiWindowFlags_NoNavFocus;

    bool result = ImGui::Begin(name, NULL, window_flags);

    // NOTE (Matteo): Instruct the docking system to consider the window's node always as the
    // central one, thus not using it as a docking target (there's the backing dockspace
    // already)
    ImGuiDockNode *main_node = ImGui::GetWindowDockNode();
    main_node->LocalFlags |= ImGuiDockNodeFlags_NoTabBar | ImGuiDockNodeFlags_CentralNode;

    return result;
}

void
guiEnd(void)
{
    ImGui::End();
}

void
guiMetricsWindow(bool *p_open)
{
    ImGui::ShowMetricsWindow(p_open);
}

void
guiDemoWindow(bool *p_open)
{
    ImGui::ShowDemoWindow(p_open);
}

void
guiPlotDemoWindow(bool *p_open)
{
    ImPlot::ShowDemoWindow(p_open);
}

//=== Log ===//

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

    ImGui::Separator();
    ImGui::BeginChild("scrolling", ImVec2{0, 0}, false, ImGuiWindowFlags_HorizontalScrollbar);
    {
        if (copy) ImGui::LogToClipboard(-1);

        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{0, 0});
        Str log_str = cfLogString(log);
        ImGui::TextUnformatted(log_str.buf, log_str.buf + log_str.len);
        ImGui::PopStyleVar(1);
        if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
        {
            ImGui::SetScrollHereY(1.0f);
        }
    }
    ImGui::EndChild();
}

//=== Canvas ===//

void
guiCanvasBegin(GuiCanvas *canvas)
{
    canvas->size = ImGui::GetContentRegionAvail();
    ImGui::InvisibleButton("Canvas", canvas->size, ImGuiButtonFlags_None);
    canvas->p0 = ImGui::GetItemRectMin();
    canvas->p1 = ImGui::GetItemRectMax();

    canvas->draw_list = ImGui::GetWindowDrawList();
    canvas->draw_list->PushClipRect(canvas->p0, canvas->p1, true);

    canvas->stroke_thick = 1.0f;
}

void
guiCanvasEnd(GuiCanvas *canvas)
{
    canvas->draw_list->PopClipRect();
}

void
guiCanvasDrawLine(GuiCanvas *canvas, Vec2 p0, Vec2 p1)
{
    canvas->draw_list->AddLine(p0, p1, canvas->stroke_color, canvas->stroke_thick);
}

void
guiCanvasDrawPolyline(GuiCanvas *canvas, Vec2 points[], Usize count)
{
    canvas->draw_list->AddPolyline((ImVec2 *)points, (I32)count, canvas->stroke_color, 0,
                                   canvas->stroke_thick);
}

void
guiCanvasDrawRect(GuiCanvas *canvas, Vec2 p0, Vec2 p1)
{
    canvas->draw_list->AddRect(p0, p1, canvas->stroke_color, 0.0f, 0, canvas->stroke_thick);
}

void
guiCanvasFillRect(GuiCanvas *canvas, Vec2 p0, Vec2 p1)
{
    canvas->draw_list->AddRectFilled(p0, p1, canvas->fill_color, 0.0f, 0);
}

void
guiCanvasDrawCircle(GuiCanvas *canvas, Vec2 center, F32 radius)
{
    canvas->draw_list->AddCircle(center, radius, canvas->stroke_color, 0, canvas->stroke_thick);
}

void
guiCanvasFillCircle(GuiCanvas *canvas, Vec2 center, F32 radius)
{
    canvas->draw_list->AddCircleFilled(center, radius, canvas->fill_color, 0);
}

void
guiCanvasDrawText(GuiCanvas *canvas, Str text, Vec2 pos, Srgb32 color)
{
    canvas->draw_list->AddText(pos, color, text.buf, text.buf + text.len);
}

void
guiCanvasDrawImage(GuiCanvas *canvas, U32 texture, //
                   Vec2 image_min, Vec2 image_max, //
                   Vec2 uv_min, Vec2 uv_max)
{
    canvas->draw_list->AddImage((ImTextureID)(Iptr)texture, image_min, image_max, uv_min, uv_max,
                                ImGui::GetColorU32(SRGB32_WHITE));
}

//=== IO ===//

F32
guiGetFramerate(void)
{
    return ImGui::GetIO().Framerate;
}

bool
guiKeyPressed(GuiKey key)
{
    return ImGui::IsKeyPressed(key, true);
}

I32
guiKeyPressedCount(GuiKey key)
{
    auto &io = ImGui::GetIO();
    return ImGui::GetKeyPressedAmount(key, io.KeyRepeatDelay, io.KeyRepeatRate);
}

bool
guiKeyCtrl(void)
{
    return ImGui::GetIO().KeyCtrl;
}

bool
guiKeyAlt(void)
{
    return ImGui::GetIO().KeyAlt;
}

bool
guiKeyShift(void)
{
    return ImGui::GetIO().KeyShift;
}

Vec2
guiGetMousePos(void)
{
    return ImGui::GetMousePos();
}

Vec2
guiGetMouseDelta(void)
{
    return ImGui::GetIO().MouseDelta;
}

F32
guiGetMouseWheel(void)
{
    return ImGui::GetIO().MouseWheel;
}

F32
guiGetMouseDownDuration(GuiMouseButton button)
{
    return ImGui::GetIO().MouseDownDuration[button];
}

bool
guiGetMouseDragging(GuiMouseButton button, Vec2 *out_delta)
{
    if (ImGui::IsMouseDragging(button, -1.0f))
    {
        if (out_delta)
        {
            *out_delta = ImGui::GetMouseDragDelta(button, -1.0f);
        }
        return true;
    }

    return false;
}

//=== Modals ===//

bool
guiBeginPopupModal(Cstr name, bool *p_open)
{
    return ImGui::BeginPopupModal(name, p_open, ImGuiWindowFlags_AlwaysAutoResize);
}

void
guiEndPopup(void)
{
    ImGui::EndPopup();
}

void
guiOpenPopup(Cstr name)
{
    ImGui::OpenPopup(name, 0);
}

void
guiClosePopup(void)
{
    ImGui::CloseCurrentPopup();
}

//=== Widgets ===//

bool
guiIsItemHovered(void)
{
    return ImGui::IsItemHovered(0);
}

//=== Miscellanea ===//

void
guiBeginFullScreen(Cstr label, bool docking, bool menu_bar)
{
    ImGuiViewport const *viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos, 0, {0, 0});
    ImGui::SetNextWindowSize(viewport->WorkSize, 0);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {0, 0});

    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
                                    ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                                    ImGuiWindowFlags_NoBringToFrontOnFocus |
                                    ImGuiWindowFlags_NoNavFocus;

    if (!docking) window_flags |= ImGuiWindowFlags_NoDocking;
    if (menu_bar) window_flags |= ImGuiWindowFlags_MenuBar;

    ImGui::Begin(label, NULL, window_flags);
    ImGui::PopStyleVar(3);
}

void
guiEndFullScreen(void)
{
    ImGui::End();
}

void
guiSameLine(void)
{
    ImGui::SameLine(0.0f, -1.0f);
}

void
guiSeparator(void)
{
    ImGui::Separator();
}

bool
guiButton(Cstr label)
{
    return ImGui::Button(label, {0, 0});
}

bool
guiCenteredButton(Cstr label)
{
    ImGuiStyle &style = ImGui::GetStyle();

    // NOTE (Matteo): Button size calculation copied from ImGui::ButtonEx
    ImVec2 label_size = ImGui::CalcTextSize(label, NULL, false, -1.0f);
    ImVec2 button_size = ImGui::CalcItemSize({0, 0}, //
                                             label_size.x + style.FramePadding.x * 2.0f,
                                             label_size.y + style.FramePadding.y * 2.0f);

    ImVec2 available_size = ImGui::GetContentRegionAvail();

    if (available_size.x > button_size.x)
    {
        ImGui::SetCursorPosX((available_size.x - button_size.x) / 2);
    }

    return guiButton(label);
}

bool
guiCheckbox(Cstr label, bool *checked)
{
    return ImGui::Checkbox(label, checked);
}

bool
guiSlider(Cstr label, F32 *value, F32 min_value, F32 max_value)
{
    return ImGui::SliderFloat(label, value, min_value, max_value, "%.3f",
                              ImGuiSliderFlags_AlwaysClamp);
}

CF_API bool
guiInput(Cstr label, F32 *value, GuiInputInfo *info)
{
    return ImGui::InputFloat(label, value, info->step, info->step_fast,
                             info->format ? info->format : "%.3f");
}

void
guiText(Cstr fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    guiTextV(fmt, args);
    va_end(args);
}

void
guiTextV(Cstr fmt, va_list args)
{
    ImGui::TextV(fmt, args);
}

bool
guiBeginMainMenuBar(void)
{
    return ImGui::BeginMainMenuBar();
}

void
guiEndMainMenuBar(void)
{
    ImGui::EndMainMenuBar();
}

bool
guiBeginMenu(Cstr label, bool enabled)
{
    return ImGui::BeginMenu(label, enabled);
}

void
guiEndMenu(void)
{
    return ImGui::EndMenu();
}

bool
guiMenuItem(Cstr label, bool *p_selected)
{
    return ImGui::MenuItem(label, NULL, p_selected, true);
}

bool
guiCombo(Cstr label, Cstr preview, Cstr const *values, Usize count, Usize *selected_index)
{
    bool changed = false;

    if (ImGui::BeginCombo(label, preview, 0))
    {
        for (Usize i = 0; i < count; ++i)
        {
            bool const selected = (i == *selected_index);
            if (ImGui::Selectable(values[i], selected) && !selected)
            {
                changed = true;
                *selected_index = i;
            }
            if (selected) ImGui::SetItemDefaultFocus();
        }

        ImGui::EndCombo();
    }

    return changed;
}

bool
guiColorEdit(Cstr label, Srgb32 *color)
{
    // TODO (Matteo): Fix redundant label

    CF_DIAGNOSTIC_PUSH()
    CF_DIAGNOSTIC_IGNORE_CLANG("-Wc99-designator")
    // TODO (Matteo): MSVC Diagnostics

    static Srgb32 const colors[] = CF_COLOR_VALUES;
    static Cstr const names[] = CF_COLOR_NAMES;

    CF_DIAGNOSTIC_POP()

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
    ImFormatString(label_buffer, CF_ARRAY_SIZE(label_buffer), "%s##Combo", label);
    if (guiCombo(label_buffer, color_name, names, CF_ARRAY_SIZE(colors), &color_index))
    {
        color_changed = true;
        *color = colors[color_index];
    }

    // Free color editing
    ImFormatString(label_buffer, CF_ARRAY_SIZE(label_buffer), "%s##Picker", label);

    // TODO (Matteo): Color picker works best in sRGB space?
    LinearColor color4 = colorToLinear(*color);
    I32 edit_flags = ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_AlphaPreviewHalf |
                     ImGuiColorEditFlags_PickerHueWheel;

    if (ImGui::ColorEdit4(label_buffer, color4.channel, edit_flags))
    {
        *color = colorToSrgb(color4);
        color_changed = true;
    }

    return color_changed;
}

void
guiStyleEditor(void)
{
    ImGui::ShowStyleEditor(NULL);
}

//=== Plots ===//

bool
guiPlotBegin(Cstr label, GuiPlotSetup *setup)
{
    ImPlotFlags plot_flags = ImPlotFlags_None;
    ImPlotLegendFlags leg_flags = ImPlotLegendFlags_None;
    ImPlotLocation leg_location = 0;

    if (setup->legend)
    {
        leg_location = setup->legend->location;
        if (setup->legend->outside) leg_flags |= ImPlotLegendFlags_Outside;
    }
    else
    {
        plot_flags |= ImPlotFlags_NoLegend;
    }

    if (!ImPlot::BeginPlot(label)) return false;

    ImPlot::SetupLegend(leg_location, leg_flags);

    CF_ASSERT_NOT_NULL(setup);

    for (GuiAxis ax = 0; ax < GuiAxis_COUNT; ++ax)
    {
        GuiAxisInfo *info = setup->info[ax];
        if (!info) continue;

        ImPlotAxisFlags ax_flags = ImPlotAxisFlags_None;
        if (info->autofit) ax_flags |= ImPlotAxisFlags_AutoFit;

        ImPlot::SetupAxis(ax, info->label, ax_flags);

        if (info->range)
        {
            ImPlot::SetupAxisLimits(ax,               //
                                    info->range->min, //
                                    info->range->max,
                                    (info->range->locked) ? ImPlotCond_Always : ImPlotCond_Once);
        }
    }

    return true;
}

void
guiPlotEnd()
{
    ImPlot::EndPlot();
}

void
guiPlotLineF32(Cstr id, F32 *xy, Usize count, Usize offset, Usize stride)
{
    ImPlot::PlotLine(id, &xy[0], &xy[1], count, offset, stride);
}

void
guiPlotLineF64(Cstr id, F64 *xy, Usize count, Usize offset, Usize stride)
{
    ImPlot::PlotLine(id, &xy[0], &xy[1], count, offset, stride);
}

void
guiPlotScatterF32(Cstr id, F32 *xy, Usize count, Usize offset, Usize stride)
{
    ImPlot::PlotScatter(id, &xy[0], &xy[1], count, offset, stride);
}

void
guiPlotScatterF64(Cstr id, F64 *xy, Usize count, Usize offset, Usize stride)
{
    ImPlot::PlotScatter(id, &xy[0], &xy[1], count, offset, stride);
}

//=== File dialogs ===//

// NOTE (Matteo): On windows I use the system dialogs for lazyness (and better experience
// actually)

#if CF_OS_WIN32

extern GuiFileDialogResult guiFileDialog(GuiFileDialogParms *parms, MemAllocator alloc);

#else

GuiFileDialogResult
guiOpenFileDialog(Str filename_hint, GuiFileDialogFilter *filters, Usize num_filters,
                  MemAllocator alloc)
{
    // TODO (Matteo): Implement purely using IMGUI
    return (GuiFileDialogResult){.code = FileDialogResult_Error};
}

#endif
