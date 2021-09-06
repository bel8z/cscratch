#include "gui.h"

// Restore warnings disabled for DearImgui compilation
#if CF_COMPILER_CLANG
#    pragma clang diagnostic warning "-Wsign-conversion"
#    pragma clang diagnostic warning "-Wimplicit-int-float-conversion"
#    pragma clang diagnostic warning "-Wunused-function"
#    pragma clang diagnostic warning "-Wfloat-conversion"
#elif CF_COMPILER_MSVC
#endif

#include "gui_config.h"

#if CF_COMPILER_MSVC
#    pragma warning(push)
#    pragma warning(disable : 4201)
#    pragma warning(disable : 4214)
#elif CF_COMPILER_CLANG
#    pragma clang diagnostic push
#    pragma clang diagnostic ignored "-Wlanguage-extension-token"
#endif

#include "imgui.h"
#include "imgui_freetype.h"
#include "imgui_internal.h"

#if CF_COMPILER_MSVC
#    pragma warning(pop)
#elif CF_COMPILER_CLANG
#    pragma clang diagnostic pop
#endif

#include "foundation/colors.h"
#include "foundation/log.h"
#include "foundation/memory.h"

#include <stdarg.h>

//=== Type conversions ===//

CF_STATIC_ASSERT(sizeof(Vec2) == sizeof(ImVec2), "Vec2 not compatible with ImVec2");
CF_STATIC_ASSERT(sizeof(Rgba) == sizeof(ImVec4), "Rgba not compatible with ImVec4");

//=== Memory management ===//

static void *
guiAlloc(Usize size, void *state)
{
    MemAllocator alloc = *(MemAllocator *)state;
    Usize *buf = (Usize *)memAlloc(alloc, size + sizeof(*buf));

    if (buf) *(buf++) = size;

    return buf;
}

static void
guiFree(void *mem, void *state)
{
    if (mem)
    {
        MemAllocator alloc = *(MemAllocator *)state;
        Usize *buf = (Usize *)mem;
        buf--;
        memFree(alloc, buf, *buf + sizeof(*buf));
    }
}

//=== Initialization ===//

void
guiInit(Gui *gui)
{
    CF_ASSERT_NOT_NULL(gui);
    CF_ASSERT_NOT_NULL(gui->alloc);

    IMGUI_CHECKVERSION();

    ImGui::SetAllocatorFunctions(guiAlloc, guiFree, gui->alloc);

    if (gui->ctx)
    {
        ImGui::SetCurrentContext(gui->ctx);
    }
    else
    {
        gui->ctx = ImGui::CreateContext(gui->shared_atlas);

        ImGuiIO &io = ImGui::GetIO();

        io.IniFilename = gui->ini_filename;

        // Enable Keyboard Controls
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        // Enable Docking
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
        // Enable Multi-Viewport / Platform Windows
        io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
        // Reduce visual noise while docking, also has a benefit for out-of-sync viewport rendering
        io.ConfigDockingTransparentPayload = true;

        guiSetTheme(GuiTheme_Dark);
    }
}

void
guiShutdown(Gui *gui)
{
    ImGui::DestroyContext(gui->ctx);
}

bool
guiViewportsEnabled(void)
{
    return ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable;
}

void
guiNewFrame(void)
{
    ImGui::NewFrame();
}

ImDrawData *
guiRender(void)
{
    ImGui::Render();
    return ImGui::GetDrawData();
}

void
guiUpdateAndRenderViewports(void)
{
    ImGui::UpdatePlatformWindows();
    ImGui::RenderPlatformWindowsDefault(NULL, NULL);
}

//=== Themes & styling ===//

void
guiThemeSelector(Cstr label)
{
#define GUI_THEME_NAME(Name) #Name,

    static Cstr name[GuiTheme_Count] = {GUI_THEMES(GUI_THEME_NAME)};

    GuiTheme curr = (GuiTheme)ImGui::GetIO().UserData;
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

Rgba32
guiGetStyledColor(Rgba32 in)
{
    return ImGui::GetColorU32(in);
}

Rgba32
guiGetBackColor(void)
{
    return ImGui::GetColorU32(ImGuiCol_WindowBg, 1.0f);
}

void
guiSetTheme(GuiTheme theme)
{
    ImGui::GetIO().UserData = (void *)theme;

    switch (theme)
    {
        case GuiTheme_Dark:
        {
            ImGui::StyleColorsClassic(NULL);
            ImGuiStyle &style = ImGui::GetStyle();
            // clang-format off
            style.Colors[ImGuiCol_Text]                   = Rgba{1.00f, 1.00f, 1.00f, 1.00f};
            style.Colors[ImGuiCol_TextDisabled]           = Rgba{0.50f, 0.50f, 0.50f, 1.00f};
            style.Colors[ImGuiCol_WindowBg]               = Rgba{0.06f, 0.06f, 0.06f, 0.94f};
            style.Colors[ImGuiCol_ChildBg]                = Rgba{0.00f, 0.00f, 0.00f, 0.00f};
            style.Colors[ImGuiCol_PopupBg]                = Rgba{0.08f, 0.08f, 0.08f, 0.94f};
            style.Colors[ImGuiCol_Border]                 = Rgba{0.43f, 0.43f, 0.50f, 0.50f};
            style.Colors[ImGuiCol_BorderShadow]           = Rgba{0.00f, 0.00f, 0.00f, 0.00f};
            style.Colors[ImGuiCol_FrameBg]                = Rgba{0.44f, 0.44f, 0.44f, 0.60f};
            style.Colors[ImGuiCol_FrameBgHovered]         = Rgba{0.57f, 0.57f, 0.57f, 0.70f};
            style.Colors[ImGuiCol_FrameBgActive]          = Rgba{0.76f, 0.76f, 0.76f, 0.80f};
            style.Colors[ImGuiCol_TitleBg]                = Rgba{0.04f, 0.04f, 0.04f, 1.00f};
            style.Colors[ImGuiCol_TitleBgActive]          = Rgba{0.16f, 0.16f, 0.16f, 1.00f};
            style.Colors[ImGuiCol_TitleBgCollapsed]       = Rgba{0.00f, 0.00f, 0.00f, 0.60f};
            style.Colors[ImGuiCol_MenuBarBg]              = Rgba{0.14f, 0.14f, 0.14f, 1.00f};
            style.Colors[ImGuiCol_ScrollbarBg]            = Rgba{0.02f, 0.02f, 0.02f, 0.53f};
            style.Colors[ImGuiCol_ScrollbarGrab]          = Rgba{0.31f, 0.31f, 0.31f, 1.00f};
            style.Colors[ImGuiCol_ScrollbarGrabHovered]   = Rgba{0.41f, 0.41f, 0.41f, 1.00f};
            style.Colors[ImGuiCol_ScrollbarGrabActive]    = Rgba{0.51f, 0.51f, 0.51f, 1.00f};
            style.Colors[ImGuiCol_CheckMark]              = Rgba{0.13f, 0.75f, 0.55f, 0.80f};
            style.Colors[ImGuiCol_SliderGrab]             = Rgba{0.13f, 0.75f, 0.75f, 0.80f};
            style.Colors[ImGuiCol_SliderGrabActive]       = Rgba{0.13f, 0.75f, 1.00f, 0.80f};
            style.Colors[ImGuiCol_Button]                 = Rgba{0.13f, 0.75f, 0.55f, 0.40f};
            style.Colors[ImGuiCol_ButtonHovered]          = Rgba{0.13f, 0.75f, 0.75f, 0.60f};
            style.Colors[ImGuiCol_ButtonActive]           = Rgba{0.13f, 0.75f, 1.00f, 0.80f};
            style.Colors[ImGuiCol_Header]                 = Rgba{0.13f, 0.75f, 0.55f, 0.40f};
            style.Colors[ImGuiCol_HeaderHovered]          = Rgba{0.13f, 0.75f, 0.75f, 0.60f};
            style.Colors[ImGuiCol_HeaderActive]           = Rgba{0.13f, 0.75f, 1.00f, 0.80f};
            style.Colors[ImGuiCol_Separator]              = Rgba{0.13f, 0.75f, 0.55f, 0.40f};
            style.Colors[ImGuiCol_SeparatorHovered]       = Rgba{0.13f, 0.75f, 0.75f, 0.60f};
            style.Colors[ImGuiCol_SeparatorActive]        = Rgba{0.13f, 0.75f, 1.00f, 0.80f};
            style.Colors[ImGuiCol_ResizeGrip]             = Rgba{0.13f, 0.75f, 0.55f, 0.40f};
            style.Colors[ImGuiCol_ResizeGripHovered]      = Rgba{0.13f, 0.75f, 0.75f, 0.60f};
            style.Colors[ImGuiCol_ResizeGripActive]       = Rgba{0.13f, 0.75f, 1.00f, 0.80f};
            style.Colors[ImGuiCol_Tab]                    = Rgba{0.13f, 0.75f, 0.55f, 0.80f};
            style.Colors[ImGuiCol_TabHovered]             = Rgba{0.13f, 0.75f, 0.75f, 0.80f};
            style.Colors[ImGuiCol_TabActive]              = Rgba{0.13f, 0.75f, 1.00f, 0.80f};
            style.Colors[ImGuiCol_TabUnfocused]           = Rgba{0.18f, 0.18f, 0.18f, 1.00f};
            style.Colors[ImGuiCol_TabUnfocusedActive]     = Rgba{0.36f, 0.36f, 0.36f, 0.54f};
            style.Colors[ImGuiCol_DockingPreview]         = Rgba{0.13f, 0.75f, 0.55f, 0.80f};
            style.Colors[ImGuiCol_DockingEmptyBg]         = Rgba{0.13f, 0.13f, 0.13f, 0.80f};
            style.Colors[ImGuiCol_PlotLines]              = Rgba{0.61f, 0.61f, 0.61f, 1.00f};
            style.Colors[ImGuiCol_PlotLinesHovered]       = Rgba{1.00f, 0.43f, 0.35f, 1.00f};
            style.Colors[ImGuiCol_PlotHistogram]          = Rgba{0.90f, 0.70f, 0.00f, 1.00f};
            style.Colors[ImGuiCol_PlotHistogramHovered]   = Rgba{1.00f, 0.60f, 0.00f, 1.00f};
            style.Colors[ImGuiCol_TableHeaderBg]          = Rgba{0.19f, 0.19f, 0.20f, 1.00f};
            style.Colors[ImGuiCol_TableBorderStrong]      = Rgba{0.31f, 0.31f, 0.35f, 1.00f};
            style.Colors[ImGuiCol_TableBorderLight]       = Rgba{0.23f, 0.23f, 0.25f, 1.00f};
            style.Colors[ImGuiCol_TableRowBg]             = Rgba{0.00f, 0.00f, 0.00f, 0.00f};
            style.Colors[ImGuiCol_TableRowBgAlt]          = Rgba{1.00f, 1.00f, 1.00f, 0.07f};
            style.Colors[ImGuiCol_TextSelectedBg]         = Rgba{0.26f, 0.59f, 0.98f, 0.35f};
            style.Colors[ImGuiCol_DragDropTarget]         = Rgba{1.00f, 1.00f, 0.00f, 0.90f};
            style.Colors[ImGuiCol_NavHighlight]           = Rgba{0.26f, 0.59f, 0.98f, 1.00f};
            style.Colors[ImGuiCol_NavWindowingHighlight]  = Rgba{1.00f, 1.00f, 1.00f, 0.70f};
            style.Colors[ImGuiCol_NavWindowingDimBg]      = Rgba{0.80f, 0.80f, 0.80f, 0.20f};
            style.Colors[ImGuiCol_ModalWindowDimBg]       = Rgba{0.80f, 0.80f, 0.80f, 0.35f};
            // clang-format on
        }
        break;

        case GuiTheme_Light:
        {
            ImGui::StyleColorsClassic(NULL);
            ImGuiStyle &style = ImGui::GetStyle();
            // clang-format off
            style.Colors[ImGuiCol_Text]                   = Rgba{0.00f, 0.00f, 0.00f, 1.00f};
            style.Colors[ImGuiCol_TextDisabled]           = Rgba{0.60f, 0.60f, 0.60f, 1.00f};
            style.Colors[ImGuiCol_WindowBg]               = Rgba{0.89f, 0.89f, 0.89f, 1.00f};
            style.Colors[ImGuiCol_ChildBg]                = Rgba{0.00f, 0.00f, 0.00f, 0.00f};
            style.Colors[ImGuiCol_PopupBg]                = Rgba{1.00f, 1.00f, 1.00f, 0.98f};
            style.Colors[ImGuiCol_Border]                 = Rgba{0.00f, 0.00f, 0.00f, 0.30f};
            style.Colors[ImGuiCol_BorderShadow]           = Rgba{0.00f, 0.00f, 0.00f, 0.00f};
            style.Colors[ImGuiCol_FrameBg]                = Rgba{1.00f, 1.00f, 1.00f, 1.00f};
            style.Colors[ImGuiCol_FrameBgHovered]         = Rgba{0.26f, 0.59f, 0.98f, 0.40f};
            style.Colors[ImGuiCol_FrameBgActive]          = Rgba{0.26f, 0.59f, 0.98f, 0.67f};
            style.Colors[ImGuiCol_TitleBg]                = Rgba{0.96f, 0.96f, 0.96f, 1.00f};
            style.Colors[ImGuiCol_TitleBgActive]          = Rgba{0.82f, 0.82f, 0.82f, 1.00f};
            style.Colors[ImGuiCol_TitleBgCollapsed]       = Rgba{1.00f, 1.00f, 1.00f, 0.51f};
            style.Colors[ImGuiCol_MenuBarBg]              = Rgba{0.86f, 0.86f, 0.86f, 1.00f};
            style.Colors[ImGuiCol_ScrollbarBg]            = Rgba{0.98f, 0.98f, 0.98f, 0.53f};
            style.Colors[ImGuiCol_ScrollbarGrab]          = Rgba{0.69f, 0.69f, 0.69f, 0.80f};
            style.Colors[ImGuiCol_ScrollbarGrabHovered]   = Rgba{0.49f, 0.49f, 0.49f, 0.80f};
            style.Colors[ImGuiCol_ScrollbarGrabActive]    = Rgba{0.49f, 0.49f, 0.49f, 1.00f};
            style.Colors[ImGuiCol_CheckMark]              = Rgba{0.26f, 0.59f, 0.98f, 1.00f};
            style.Colors[ImGuiCol_SliderGrab]             = Rgba{0.26f, 0.59f, 0.98f, 0.78f};
            style.Colors[ImGuiCol_SliderGrabActive]       = Rgba{0.46f, 0.54f, 0.80f, 0.60f};
            style.Colors[ImGuiCol_Button]                 = Rgba{0.26f, 0.59f, 0.98f, 0.40f};
            style.Colors[ImGuiCol_ButtonHovered]          = Rgba{0.26f, 0.59f, 0.98f, 1.00f};
            style.Colors[ImGuiCol_ButtonActive]           = Rgba{0.06f, 0.53f, 0.98f, 1.00f};
            style.Colors[ImGuiCol_Header]                 = Rgba{0.26f, 0.59f, 0.98f, 0.31f};
            style.Colors[ImGuiCol_HeaderHovered]          = Rgba{0.26f, 0.59f, 0.98f, 0.80f};
            style.Colors[ImGuiCol_HeaderActive]           = Rgba{0.26f, 0.59f, 0.98f, 1.00f};
            style.Colors[ImGuiCol_Separator]              = Rgba{0.39f, 0.39f, 0.39f, 0.62f};
            style.Colors[ImGuiCol_SeparatorHovered]       = Rgba{0.14f, 0.44f, 0.80f, 0.78f};
            style.Colors[ImGuiCol_SeparatorActive]        = Rgba{0.14f, 0.44f, 0.80f, 1.00f};
            style.Colors[ImGuiCol_ResizeGrip]             = Rgba{0.35f, 0.35f, 0.35f, 0.17f};
            style.Colors[ImGuiCol_ResizeGripHovered]      = Rgba{0.26f, 0.59f, 0.98f, 0.67f};
            style.Colors[ImGuiCol_ResizeGripActive]       = Rgba{0.26f, 0.59f, 0.98f, 0.95f};
            style.Colors[ImGuiCol_Tab]                    = Rgba{0.76f, 0.80f, 0.84f, 0.93f};
            style.Colors[ImGuiCol_TabHovered]             = Rgba{0.26f, 0.59f, 0.98f, 0.80f};
            style.Colors[ImGuiCol_TabActive]              = Rgba{0.60f, 0.73f, 0.88f, 1.00f};
            style.Colors[ImGuiCol_TabUnfocused]           = Rgba{0.92f, 0.93f, 0.94f, 0.99f};
            style.Colors[ImGuiCol_TabUnfocusedActive]     = Rgba{0.74f, 0.82f, 0.91f, 1.00f};
            style.Colors[ImGuiCol_DockingPreview]         = Rgba{0.26f, 0.59f, 0.98f, 0.22f};
            style.Colors[ImGuiCol_DockingEmptyBg]         = Rgba{0.20f, 0.20f, 0.20f, 1.00f};
            style.Colors[ImGuiCol_PlotLines]              = Rgba{0.39f, 0.39f, 0.39f, 1.00f};
            style.Colors[ImGuiCol_PlotLinesHovered]       = Rgba{1.00f, 0.43f, 0.35f, 1.00f};
            style.Colors[ImGuiCol_PlotHistogram]          = Rgba{0.90f, 0.70f, 0.00f, 1.00f};
            style.Colors[ImGuiCol_PlotHistogramHovered]   = Rgba{1.00f, 0.45f, 0.00f, 1.00f};
            style.Colors[ImGuiCol_TableHeaderBg]          = Rgba{0.78f, 0.87f, 0.98f, 1.00f};
            style.Colors[ImGuiCol_TableBorderStrong]      = Rgba{0.57f, 0.57f, 0.64f, 1.00f};
            style.Colors[ImGuiCol_TableBorderLight]       = Rgba{0.68f, 0.68f, 0.74f, 1.00f};
            style.Colors[ImGuiCol_TableRowBg]             = Rgba{0.00f, 0.00f, 0.00f, 0.00f};
            style.Colors[ImGuiCol_TableRowBgAlt]          = Rgba{0.30f, 0.30f, 0.30f, 0.09f};
            style.Colors[ImGuiCol_TextSelectedBg]         = Rgba{0.26f, 0.59f, 0.98f, 0.35f};
            style.Colors[ImGuiCol_DragDropTarget]         = Rgba{0.26f, 0.59f, 0.98f, 0.95f};
            style.Colors[ImGuiCol_NavHighlight]           = Rgba{0.26f, 0.59f, 0.98f, 0.80f};
            style.Colors[ImGuiCol_NavWindowingHighlight]  = Rgba{0.70f, 0.70f, 0.70f, 0.70f};
            style.Colors[ImGuiCol_NavWindowingDimBg]      = Rgba{0.20f, 0.20f, 0.20f, 0.20f};
            style.Colors[ImGuiCol_ModalWindowDimBg]       = Rgba{0.20f, 0.20f, 0.20f, 0.35f};
            // clang-format on
        }
        break;

        case GuiTheme_Dummy:
        {
            ImGui::StyleColorsLight(NULL);
            ImGuiStyle &style = ImGui::GetStyle();
            // clang-format off
            style.Colors[ImGuiCol_Text]                 = Rgba{0.31f, 0.25f, 0.24f, 1.00f};
            style.Colors[ImGuiCol_WindowBg]             = Rgba{0.94f, 0.94f, 0.94f, 1.00f};
            style.Colors[ImGuiCol_MenuBarBg]            = Rgba{0.74f, 0.74f, 0.94f, 1.00f};
            style.Colors[ImGuiCol_ChildBg]              = Rgba{0.68f, 0.68f, 0.68f, 0.00f};
            style.Colors[ImGuiCol_Border]               = Rgba{0.50f, 0.50f, 0.50f, 0.60f};
            style.Colors[ImGuiCol_BorderShadow]         = Rgba{0.00f, 0.00f, 0.00f, 0.00f};
            style.Colors[ImGuiCol_FrameBg]              = Rgba{0.62f, 0.70f, 0.72f, 0.56f};
            style.Colors[ImGuiCol_FrameBgHovered]       = Rgba{0.95f, 0.33f, 0.14f, 0.47f};
            style.Colors[ImGuiCol_FrameBgActive]        = Rgba{0.97f, 0.31f, 0.13f, 0.81f};
            style.Colors[ImGuiCol_TitleBg]              = Rgba{0.42f, 0.75f, 1.00f, 0.53f};
            style.Colors[ImGuiCol_TitleBgCollapsed]     = Rgba{0.40f, 0.65f, 0.80f, 0.20f};
            style.Colors[ImGuiCol_ScrollbarBg]          = Rgba{0.40f, 0.62f, 0.80f, 0.15f};
            style.Colors[ImGuiCol_ScrollbarGrab]        = Rgba{0.39f, 0.64f, 0.80f, 0.30f};
            style.Colors[ImGuiCol_ScrollbarGrabHovered] = Rgba{0.28f, 0.67f, 0.80f, 0.59f};
            style.Colors[ImGuiCol_ScrollbarGrabActive]  = Rgba{0.25f, 0.48f, 0.53f, 0.67f};
            style.Colors[ImGuiCol_CheckMark]            = Rgba{0.48f, 0.47f, 0.47f, 0.71f};
            style.Colors[ImGuiCol_SliderGrabActive]     = Rgba{0.31f, 0.47f, 0.99f, 1.00f};
            style.Colors[ImGuiCol_Button]               = Rgba{1.00f, 0.79f, 0.18f, 0.78f};
            style.Colors[ImGuiCol_ButtonHovered]        = Rgba{0.42f, 0.82f, 1.00f, 0.81f};
            style.Colors[ImGuiCol_ButtonActive]         = Rgba{0.72f, 1.00f, 1.00f, 0.86f};
            style.Colors[ImGuiCol_Header]               = Rgba{0.65f, 0.78f, 0.84f, 0.80f};
            style.Colors[ImGuiCol_HeaderHovered]        = Rgba{0.75f, 0.88f, 0.94f, 0.80f};
            style.Colors[ImGuiCol_HeaderActive]         = Rgba{0.55f, 0.68f, 0.74f, 0.80f};//Rgba(0.46f, 0.84f, 0.90f, 1.00f);
            style.Colors[ImGuiCol_ResizeGrip]           = Rgba{0.60f, 0.60f, 0.80f, 0.30f};
            style.Colors[ImGuiCol_ResizeGripHovered]    = Rgba{1.00f, 1.00f, 1.00f, 0.60f};
            style.Colors[ImGuiCol_ResizeGripActive]     = Rgba{1.00f, 1.00f, 1.00f, 0.90f};
            style.Colors[ImGuiCol_TextSelectedBg]       = Rgba{1.00f, 0.99f, 0.54f, 0.43f};
            style.Colors[ImGuiCol_PopupBg]              = Rgba{0.82f, 0.92f, 1.00f, 0.90f}; // Rgba(0.89f, 0.98f, 1.00f, 0.99f)
            // colors[ImGuiCol_ComboBg] = Rgba(0.89f, 0.98f, 1.00f, 0.99f);
            // colors[ImGuiCol_CloseButton] = Rgba(0.41f, 0.75f, 0.98f, 0.50f);
            // colors[ImGuiCol_CloseButtonHovered] = Rgba(1.00f, 0.47f, 0.41f, 0.60f);
            // colors[ImGuiCol_CloseButtonActive] = Rgba(1.00f, 0.16f, 0.00f, 1.00f);
            // clang-format on
        }
        break;

        default: CF_INVALID_CODE_PATH(); break;
    }
}

static void
guiSetSizes(ImGuiStyle *style)
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
    style->FrameRounding = 2.0f;
    style->PopupRounding = 2.0f;
    style->ScrollbarRounding = 4.0f;
    style->GrabRounding = style->FrameRounding;
    style->LogSliderDeadzone = 3.0f;
    style->TabRounding = 4.0f;

    // Alignment
    // **DEFAULT**
}

void
guiSetupStyle(GuiTheme theme, F32 dpi_scale)
{
    ImGuiStyle &style = ImGui::GetStyle();

    guiSetTheme(theme);
    guiSetSizes(&style);

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

    if (ImGui::RadioButton("FreeType", state->freetype_enabled))
    {
        state->freetype_enabled = true;
        rebuild_fonts = true;
    }
    guiSameLine();
    if (ImGui::RadioButton("Stb (Default)", !state->freetype_enabled))
    {
        state->freetype_enabled = false;
        rebuild_fonts = true;
    }

    rebuild_fonts |= ImGui::DragInt("TexGlyphPadding", &state->tex_glyph_padding, 0.1f, 1, 16, NULL,
                                    ImGuiSliderFlags_None);

    rebuild_fonts |= ImGui::DragFloat("RasterizerMultiply", &state->rasterizer_multiply, 0.001f,
                                      0.0f, 2.0f, NULL, ImGuiSliderFlags_None);

    ImGui::Separator();

    if (state->freetype_enabled)
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
        fonts->FontBuilderIO = ImGuiFreeType::GetBuilderForFreeType();
        fonts->FontBuilderFlags = (U32)font_opts->freetype_flags;
    }
    else
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

GuiDockLayout
guiDockLayout(void)
{
    // NOTE (Matteo): Setup docking layout on first run (if the dockspace node is already split the
    // layout has been setup and maybe modified by the user).
    // This code is partially copied from github since the DockBuilder API is not documented -
    // understand it better!

    ImGuiDockNodeFlags const dock_flags = ImGuiDockNodeFlags_NoDockingInCentralNode;
    ImGuiViewport const *viewport = ImGui::GetMainViewport();

    GuiDockLayout layout{};

    layout.id = ImGui::DockSpaceOverViewport(viewport, dock_flags, NULL);
    layout.node = ImGui::DockBuilderGetNode(layout.id);
    layout.open = (!layout.node || !layout.node->IsSplitNode());

    return layout;
}

static U32
gui_DockSplit(GuiDockLayout *layout, ImGuiDir dir, F32 size_ratio)
{
    U32 id = U32_MAX;
    if (layout->open)
    {
        id = ImGui::DockBuilderSplitNode(layout->id, dir, size_ratio, NULL, &layout->id);
    }
    return id;
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
    // central one, thus not using it as a docking target (there's the backing dockspace already)
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
    canvas->draw_list->AddRectFilled(p0, p1, canvas->stroke_color, 0.0f, 0);
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
guiCanvasDrawText(GuiCanvas *canvas, Str text, Vec2 pos, Rgba32 color)
{
    canvas->draw_list->AddText(pos, color, text.buf, text.buf + text.len);
}

void
guiCanvasDrawImage(GuiCanvas *canvas, U32 texture, //
                   Vec2 image_min, Vec2 image_max, //
                   Vec2 uv_min, Vec2 uv_max)
{
    canvas->draw_list->AddImage((ImTextureID)(Iptr)texture, image_min, image_max, uv_min, uv_max,
                                ImGui::GetColorU32(RGBA32_WHITE));
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
    return ImGui::IsKeyPressed(ImGui::GetIO().KeyMap[key], true);
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

void
guiText(Cstr fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    ImGui::TextV(fmt, args);
    va_end(args);
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
guiColorEdit(Cstr label, Rgba32 *color)
{
    // TODO (Matteo): Fix redundant label

#if CF_COMPILER_CLANG
#    pragma clang diagnostic push
#    pragma clang diagnostic ignored "-Wc99-designator"
#else
// TODO (Matteo): MSVC Diagnostics
#endif

    static Rgba32 const colors[] = CF_COLOR_VALUES;
    static Cstr const names[] = CF_COLOR_NAMES;

#if CF_COMPILER_CLANG
#    pragma clang diagnostic pop
#else
// TODO (Matteo): MSVC Diagnostics
#endif

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

    if (ImGui::BeginCombo(label_buffer, color_name, 0))
    {
        for (Usize i = 0; i < CF_ARRAY_SIZE(colors); ++i)
        {
            bool const selected = i == color_index;
            if (ImGui::Selectable(names[i], selected))
            {
                color_changed = color_index != i;
                *color = colors[i];
            }
            if (selected) ImGui::SetItemDefaultFocus();
        }

        ImGui::EndCombo();
    }

    // Free color editing

    ImFormatString(label_buffer, CF_ARRAY_SIZE(label_buffer), "%s##Picker", label);

    Rgba color4 = rgbaUnpack32(*color);
    I32 edit_flags = ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_AlphaPreviewHalf |
                     ImGuiColorEditFlags_PickerHueWheel;

    if (ImGui::ColorEdit4(label_buffer, color4.channel, edit_flags))
    {
        *color = rgbaPack32(color4);
        color_changed = true;
    }

    return color_changed;
}

void
guiStyleEditor(void)
{
    ImGui::ShowStyleEditor(NULL);
}

//=== File dialogs ===//

// NOTE (Matteo): On windows I use the system dialogs for lazyness (and better experience actually)

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
