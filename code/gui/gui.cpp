#include "gui.h"

#include "gui_config.h"

// Restore warnings disabled for DearImgui compilation
CF_DIAGNOSTIC_RESTORE_CLANG("-Wsign-conversion")
CF_DIAGNOSTIC_RESTORE_CLANG("-Wimplicit-int-float-conversion")
CF_DIAGNOSTIC_RESTORE_CLANG("-Wunused-function")
CF_DIAGNOSTIC_RESTORE_CLANG("-Wfloat-conversion")

CF_DIAGNOSTIC_PUSH()
CF_DIAGNOSTIC_IGNORE_CLANG("-Wlanguage-extension-token")
CF_DIAGNOSTIC_IGNORE_MSVC(4201)
CF_DIAGNOSTIC_IGNORE_MSVC(4214)

#include "imgui.h"
#include "imgui_freetype.h"
#include "imgui_internal.h"

CF_DIAGNOSTIC_POP()

#include "foundation/colors.h"
#include "foundation/log.h"
#include "foundation/memory.h"

struct GuiData
{
    MemAllocator allocator;
    GuiMemory memory;

    GuiTheme theme;

    void *user_data;
};

#if !defined GUI_SRGB_COLORS
#    define GUI_SRGB_COLORS 0
#endif // GUI_SRGB_COLORS

#if !defined GUI_VIEWPORTS
#    define GUI_VIEWPORTS 0
#endif // GUI_VIEWPORTS

//=== Type conversions ===//

CF_STATIC_ASSERT(sizeof(Vec2) == sizeof(ImVec2), "Vec2 not compatible with ImVec2");
CF_STATIC_ASSERT(sizeof(LinearColor) == sizeof(ImVec4), "LinearColor not compatible with ImVec4");

//=== Memory management ===//

static void *
guiAlloc(Usize size, void *state)
{
    // TODO (Matteo: Check for misaligned access

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

static void
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

static GuiData &
guiData()
{
    return *(GuiData *)ImGui::GetIO().UserData;
}

void
guiInit(Gui *gui)
{
    CF_ASSERT_NOT_NULL(gui);

    IMGUI_CHECKVERSION();

    if (gui->ctx)
    {
        ImGui::SetCurrentContext(gui->ctx);
        ImGui::SetAllocatorFunctions(guiAlloc, guiFree, &guiData());
    }
    else
    {
        // Configure custom user data
        GuiData *gui_data = (GuiData *)memAlloc(gui->alloc, sizeof(*gui_data));
        gui_data->allocator = gui->alloc;
        gui_data->memory = {0};
        gui_data->user_data = gui->user_data;

        ImGui::SetAllocatorFunctions(guiAlloc, guiFree, gui_data);

        gui->ctx = ImGui::CreateContext(gui->shared_atlas);

        ImGuiIO &io = ImGui::GetIO();

        io.UserData = gui_data;

        io.IniFilename = gui->ini_filename;

        // Enable Keyboard Controls
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        // Enable Docking
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
        // Reduce visual noise while docking, also has a benefit for out-of-sync viewport rendering
        io.ConfigDockingTransparentPayload = true;

#if GUI_VIEWPORTS
        // Enable Multi-Viewport / Platform Windows
        io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
#endif

        guiSetTheme(GuiTheme_Dark);
    }
}

void
guiShutdown(Gui *gui)
{
    GuiData *gui_data = (GuiData *)ImGui::GetIO().UserData;

    ImGui::DestroyContext(gui->ctx);

    CF_ASSERT(gui_data->memory.size == 0, "Possible GUI memory leak");
    CF_ASSERT(gui_data->memory.blocks == 0, "Possible GUI memory leak");

    memFree(gui_data->allocator, gui_data, sizeof(*gui_data));
}

bool
guiViewportsEnabled(void)
{
    return ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable;
}

CF_API GuiMemory
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

ImDrawData *
guiRender(void)
{
    ImGui::Render();
    return ImGui::GetDrawData();
}

void
guiUpdateViewports(bool render)
{
    ImGui::UpdatePlatformWindows();
    if (render) ImGui::RenderPlatformWindowsDefault(NULL, NULL);
}

//=== Themes & styling ===//

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

static ImVec4
guiColor(F32 r, F32 g, F32 b, F32 a)
{
#if GUI_SRGB_COLORS
    return {ImPow(r, 2.2f), ImPow(g, 2.2f), ImPow(b, 2.2f), a};
#else
    return {r, g, b, a};
#endif
}

void
guiSetTheme(GuiTheme theme)
{
    guiData().theme = theme;

    switch (theme)
    {
        case GuiTheme_Dark:
        {
            ImGui::StyleColorsClassic(NULL);
            ImGuiStyle &style = ImGui::GetStyle();
            // clang-format off
            style.Colors[ImGuiCol_Text]                   = guiColor(1.00f, 1.00f, 1.00f, 1.00f);
            style.Colors[ImGuiCol_TextDisabled]           = guiColor(0.50f, 0.50f, 0.50f, 1.00f);
            style.Colors[ImGuiCol_WindowBg]               = guiColor(0.06f, 0.06f, 0.06f, 0.94f);
            style.Colors[ImGuiCol_ChildBg]                = guiColor(0.00f, 0.00f, 0.00f, 0.00f);
            style.Colors[ImGuiCol_PopupBg]                = guiColor(0.08f, 0.08f, 0.08f, 0.94f);
            style.Colors[ImGuiCol_Border]                 = guiColor(0.43f, 0.43f, 0.50f, 0.50f);
            style.Colors[ImGuiCol_BorderShadow]           = guiColor(0.00f, 0.00f, 0.00f, 0.00f);
            style.Colors[ImGuiCol_FrameBg]                = guiColor(0.44f, 0.44f, 0.44f, 0.60f);
            style.Colors[ImGuiCol_FrameBgHovered]         = guiColor(0.57f, 0.57f, 0.57f, 0.70f);
            style.Colors[ImGuiCol_FrameBgActive]          = guiColor(0.76f, 0.76f, 0.76f, 0.80f);
            style.Colors[ImGuiCol_TitleBg]                = guiColor(0.04f, 0.04f, 0.04f, 1.00f);
            style.Colors[ImGuiCol_TitleBgActive]          = guiColor(0.16f, 0.16f, 0.16f, 1.00f);
            style.Colors[ImGuiCol_TitleBgCollapsed]       = guiColor(0.00f, 0.00f, 0.00f, 0.60f);
            style.Colors[ImGuiCol_MenuBarBg]              = guiColor(0.14f, 0.14f, 0.14f, 1.00f);
            style.Colors[ImGuiCol_ScrollbarBg]            = guiColor(0.02f, 0.02f, 0.02f, 0.53f);
            style.Colors[ImGuiCol_ScrollbarGrab]          = guiColor(0.31f, 0.31f, 0.31f, 1.00f);
            style.Colors[ImGuiCol_ScrollbarGrabHovered]   = guiColor(0.41f, 0.41f, 0.41f, 1.00f);
            style.Colors[ImGuiCol_ScrollbarGrabActive]    = guiColor(0.51f, 0.51f, 0.51f, 1.00f);
            style.Colors[ImGuiCol_CheckMark]              = guiColor(0.13f, 0.75f, 0.55f, 0.80f);
            style.Colors[ImGuiCol_SliderGrab]             = guiColor(0.13f, 0.75f, 0.75f, 0.80f);
            style.Colors[ImGuiCol_SliderGrabActive]       = guiColor(0.13f, 0.75f, 1.00f, 0.80f);
            style.Colors[ImGuiCol_Button]                 = guiColor(0.13f, 0.75f, 0.55f, 0.40f);
            style.Colors[ImGuiCol_ButtonHovered]          = guiColor(0.13f, 0.75f, 0.75f, 0.60f);
            style.Colors[ImGuiCol_ButtonActive]           = guiColor(0.13f, 0.75f, 1.00f, 0.80f);
            style.Colors[ImGuiCol_Header]                 = guiColor(0.13f, 0.75f, 0.55f, 0.40f);
            style.Colors[ImGuiCol_HeaderHovered]          = guiColor(0.13f, 0.75f, 0.75f, 0.60f);
            style.Colors[ImGuiCol_HeaderActive]           = guiColor(0.13f, 0.75f, 1.00f, 0.80f);
            style.Colors[ImGuiCol_Separator]              = guiColor(0.13f, 0.75f, 0.55f, 0.40f);
            style.Colors[ImGuiCol_SeparatorHovered]       = guiColor(0.13f, 0.75f, 0.75f, 0.60f);
            style.Colors[ImGuiCol_SeparatorActive]        = guiColor(0.13f, 0.75f, 1.00f, 0.80f);
            style.Colors[ImGuiCol_ResizeGrip]             = guiColor(0.13f, 0.75f, 0.55f, 0.40f);
            style.Colors[ImGuiCol_ResizeGripHovered]      = guiColor(0.13f, 0.75f, 0.75f, 0.60f);
            style.Colors[ImGuiCol_ResizeGripActive]       = guiColor(0.13f, 0.75f, 1.00f, 0.80f);
            style.Colors[ImGuiCol_Tab]                    = guiColor(0.13f, 0.75f, 0.55f, 0.80f);
            style.Colors[ImGuiCol_TabHovered]             = guiColor(0.13f, 0.75f, 0.75f, 0.80f);
            style.Colors[ImGuiCol_TabActive]              = guiColor(0.13f, 0.75f, 1.00f, 0.80f);
            style.Colors[ImGuiCol_TabUnfocused]           = guiColor(0.18f, 0.18f, 0.18f, 1.00f);
            style.Colors[ImGuiCol_TabUnfocusedActive]     = guiColor(0.36f, 0.36f, 0.36f, 0.54f);
            style.Colors[ImGuiCol_DockingPreview]         = guiColor(0.13f, 0.75f, 0.55f, 0.80f);
            style.Colors[ImGuiCol_DockingEmptyBg]         = guiColor(0.13f, 0.13f, 0.13f, 0.80f);
            style.Colors[ImGuiCol_PlotLines]              = guiColor(0.61f, 0.61f, 0.61f, 1.00f);
            style.Colors[ImGuiCol_PlotLinesHovered]       = guiColor(1.00f, 0.43f, 0.35f, 1.00f);
            style.Colors[ImGuiCol_PlotHistogram]          = guiColor(0.90f, 0.70f, 0.00f, 1.00f);
            style.Colors[ImGuiCol_PlotHistogramHovered]   = guiColor(1.00f, 0.60f, 0.00f, 1.00f);
            style.Colors[ImGuiCol_TableHeaderBg]          = guiColor(0.19f, 0.19f, 0.20f, 1.00f);
            style.Colors[ImGuiCol_TableBorderStrong]      = guiColor(0.31f, 0.31f, 0.35f, 1.00f);
            style.Colors[ImGuiCol_TableBorderLight]       = guiColor(0.23f, 0.23f, 0.25f, 1.00f);
            style.Colors[ImGuiCol_TableRowBg]             = guiColor(0.00f, 0.00f, 0.00f, 0.00f);
            style.Colors[ImGuiCol_TableRowBgAlt]          = guiColor(1.00f, 1.00f, 1.00f, 0.07f);
            style.Colors[ImGuiCol_TextSelectedBg]         = guiColor(0.26f, 0.59f, 0.98f, 0.35f);
            style.Colors[ImGuiCol_DragDropTarget]         = guiColor(1.00f, 1.00f, 0.00f, 0.90f);
            style.Colors[ImGuiCol_NavHighlight]           = guiColor(0.26f, 0.59f, 0.98f, 1.00f);
            style.Colors[ImGuiCol_NavWindowingHighlight]  = guiColor(1.00f, 1.00f, 1.00f, 0.70f);
            style.Colors[ImGuiCol_NavWindowingDimBg]      = guiColor(0.80f, 0.80f, 0.80f, 0.20f);
            style.Colors[ImGuiCol_ModalWindowDimBg]       = guiColor(0.80f, 0.80f, 0.80f, 0.35f);
            // clang-format on
        }
        break;

        case GuiTheme_Light:
        {
            ImGui::StyleColorsClassic(NULL);
            ImGuiStyle &style = ImGui::GetStyle();
            // clang-format off
            style.Colors[ImGuiCol_Text]                   = guiColor(0.00f, 0.00f, 0.00f, 1.00f);
            style.Colors[ImGuiCol_TextDisabled]           = guiColor(0.60f, 0.60f, 0.60f, 1.00f);
            style.Colors[ImGuiCol_WindowBg]               = guiColor(0.89f, 0.89f, 0.89f, 1.00f);
            style.Colors[ImGuiCol_ChildBg]                = guiColor(0.00f, 0.00f, 0.00f, 0.00f);
            style.Colors[ImGuiCol_PopupBg]                = guiColor(1.00f, 1.00f, 1.00f, 0.98f);
            style.Colors[ImGuiCol_Border]                 = guiColor(0.00f, 0.00f, 0.00f, 0.30f);
            style.Colors[ImGuiCol_BorderShadow]           = guiColor(0.00f, 0.00f, 0.00f, 0.00f);
            style.Colors[ImGuiCol_FrameBg]                = guiColor(1.00f, 1.00f, 1.00f, 1.00f);
            style.Colors[ImGuiCol_FrameBgHovered]         = guiColor(0.26f, 0.59f, 0.98f, 0.40f);
            style.Colors[ImGuiCol_FrameBgActive]          = guiColor(0.26f, 0.59f, 0.98f, 0.67f);
            style.Colors[ImGuiCol_TitleBg]                = guiColor(0.96f, 0.96f, 0.96f, 1.00f);
            style.Colors[ImGuiCol_TitleBgActive]          = guiColor(0.82f, 0.82f, 0.82f, 1.00f);
            style.Colors[ImGuiCol_TitleBgCollapsed]       = guiColor(1.00f, 1.00f, 1.00f, 0.51f);
            style.Colors[ImGuiCol_MenuBarBg]              = guiColor(0.86f, 0.86f, 0.86f, 1.00f);
            style.Colors[ImGuiCol_ScrollbarBg]            = guiColor(0.98f, 0.98f, 0.98f, 0.53f);
            style.Colors[ImGuiCol_ScrollbarGrab]          = guiColor(0.69f, 0.69f, 0.69f, 0.80f);
            style.Colors[ImGuiCol_ScrollbarGrabHovered]   = guiColor(0.49f, 0.49f, 0.49f, 0.80f);
            style.Colors[ImGuiCol_ScrollbarGrabActive]    = guiColor(0.49f, 0.49f, 0.49f, 1.00f);
            style.Colors[ImGuiCol_CheckMark]              = guiColor(0.26f, 0.59f, 0.98f, 1.00f);
            style.Colors[ImGuiCol_SliderGrab]             = guiColor(0.26f, 0.59f, 0.98f, 0.78f);
            style.Colors[ImGuiCol_SliderGrabActive]       = guiColor(0.46f, 0.54f, 0.80f, 0.60f);
            style.Colors[ImGuiCol_Button]                 = guiColor(0.26f, 0.59f, 0.98f, 0.40f);
            style.Colors[ImGuiCol_ButtonHovered]          = guiColor(0.26f, 0.59f, 0.98f, 1.00f);
            style.Colors[ImGuiCol_ButtonActive]           = guiColor(0.06f, 0.53f, 0.98f, 1.00f);
            style.Colors[ImGuiCol_Header]                 = guiColor(0.26f, 0.59f, 0.98f, 0.31f);
            style.Colors[ImGuiCol_HeaderHovered]          = guiColor(0.26f, 0.59f, 0.98f, 0.80f);
            style.Colors[ImGuiCol_HeaderActive]           = guiColor(0.26f, 0.59f, 0.98f, 1.00f);
            style.Colors[ImGuiCol_Separator]              = guiColor(0.39f, 0.39f, 0.39f, 0.62f);
            style.Colors[ImGuiCol_SeparatorHovered]       = guiColor(0.14f, 0.44f, 0.80f, 0.78f);
            style.Colors[ImGuiCol_SeparatorActive]        = guiColor(0.14f, 0.44f, 0.80f, 1.00f);
            style.Colors[ImGuiCol_ResizeGrip]             = guiColor(0.35f, 0.35f, 0.35f, 0.17f);
            style.Colors[ImGuiCol_ResizeGripHovered]      = guiColor(0.26f, 0.59f, 0.98f, 0.67f);
            style.Colors[ImGuiCol_ResizeGripActive]       = guiColor(0.26f, 0.59f, 0.98f, 0.95f);
            style.Colors[ImGuiCol_Tab]                    = guiColor(0.76f, 0.80f, 0.84f, 0.93f);
            style.Colors[ImGuiCol_TabHovered]             = guiColor(0.26f, 0.59f, 0.98f, 0.80f);
            style.Colors[ImGuiCol_TabActive]              = guiColor(0.60f, 0.73f, 0.88f, 1.00f);
            style.Colors[ImGuiCol_TabUnfocused]           = guiColor(0.92f, 0.93f, 0.94f, 0.99f);
            style.Colors[ImGuiCol_TabUnfocusedActive]     = guiColor(0.74f, 0.82f, 0.91f, 1.00f);
            style.Colors[ImGuiCol_DockingPreview]         = guiColor(0.26f, 0.59f, 0.98f, 0.22f);
            style.Colors[ImGuiCol_DockingEmptyBg]         = guiColor(0.20f, 0.20f, 0.20f, 1.00f);
            style.Colors[ImGuiCol_PlotLines]              = guiColor(0.39f, 0.39f, 0.39f, 1.00f);
            style.Colors[ImGuiCol_PlotLinesHovered]       = guiColor(1.00f, 0.43f, 0.35f, 1.00f);
            style.Colors[ImGuiCol_PlotHistogram]          = guiColor(0.90f, 0.70f, 0.00f, 1.00f);
            style.Colors[ImGuiCol_PlotHistogramHovered]   = guiColor(1.00f, 0.45f, 0.00f, 1.00f);
            style.Colors[ImGuiCol_TableHeaderBg]          = guiColor(0.78f, 0.87f, 0.98f, 1.00f);
            style.Colors[ImGuiCol_TableBorderStrong]      = guiColor(0.57f, 0.57f, 0.64f, 1.00f);
            style.Colors[ImGuiCol_TableBorderLight]       = guiColor(0.68f, 0.68f, 0.74f, 1.00f);
            style.Colors[ImGuiCol_TableRowBg]             = guiColor(0.00f, 0.00f, 0.00f, 0.00f);
            style.Colors[ImGuiCol_TableRowBgAlt]          = guiColor(0.30f, 0.30f, 0.30f, 0.09f);
            style.Colors[ImGuiCol_TextSelectedBg]         = guiColor(0.26f, 0.59f, 0.98f, 0.35f);
            style.Colors[ImGuiCol_DragDropTarget]         = guiColor(0.26f, 0.59f, 0.98f, 0.95f);
            style.Colors[ImGuiCol_NavHighlight]           = guiColor(0.26f, 0.59f, 0.98f, 0.80f);
            style.Colors[ImGuiCol_NavWindowingHighlight]  = guiColor(0.70f, 0.70f, 0.70f, 0.70f);
            style.Colors[ImGuiCol_NavWindowingDimBg]      = guiColor(0.20f, 0.20f, 0.20f, 0.20f);
            style.Colors[ImGuiCol_ModalWindowDimBg]       = guiColor(0.20f, 0.20f, 0.20f, 0.35f);
            // clang-format on
        }
        break;

        case GuiTheme_Dummy:
        {
            ImGui::StyleColorsLight(NULL);
            ImGuiStyle &style = ImGui::GetStyle();
            // clang-format off
            style.Colors[ImGuiCol_Text]                 = guiColor(0.31f, 0.25f, 0.24f, 1.00f);
            style.Colors[ImGuiCol_WindowBg]             = guiColor(0.94f, 0.94f, 0.94f, 1.00f);
            style.Colors[ImGuiCol_MenuBarBg]            = guiColor(0.74f, 0.74f, 0.94f, 1.00f);
            style.Colors[ImGuiCol_ChildBg]              = guiColor(0.68f, 0.68f, 0.68f, 0.00f);
            style.Colors[ImGuiCol_Border]               = guiColor(0.50f, 0.50f, 0.50f, 0.60f);
            style.Colors[ImGuiCol_BorderShadow]         = guiColor(0.00f, 0.00f, 0.00f, 0.00f);
            style.Colors[ImGuiCol_FrameBg]              = guiColor(0.62f, 0.70f, 0.72f, 0.56f);
            style.Colors[ImGuiCol_FrameBgHovered]       = guiColor(0.95f, 0.33f, 0.14f, 0.47f);
            style.Colors[ImGuiCol_FrameBgActive]        = guiColor(0.97f, 0.31f, 0.13f, 0.81f);
            style.Colors[ImGuiCol_TitleBg]              = guiColor(0.42f, 0.75f, 1.00f, 0.53f);
            style.Colors[ImGuiCol_TitleBgCollapsed]     = guiColor(0.40f, 0.65f, 0.80f, 0.20f);
            style.Colors[ImGuiCol_ScrollbarBg]          = guiColor(0.40f, 0.62f, 0.80f, 0.15f);
            style.Colors[ImGuiCol_ScrollbarGrab]        = guiColor(0.39f, 0.64f, 0.80f, 0.30f);
            style.Colors[ImGuiCol_ScrollbarGrabHovered] = guiColor(0.28f, 0.67f, 0.80f, 0.59f);
            style.Colors[ImGuiCol_ScrollbarGrabActive]  = guiColor(0.25f, 0.48f, 0.53f, 0.67f);
            style.Colors[ImGuiCol_CheckMark]            = guiColor(0.48f, 0.47f, 0.47f, 0.71f);
            style.Colors[ImGuiCol_SliderGrabActive]     = guiColor(0.31f, 0.47f, 0.99f, 1.00f);
            style.Colors[ImGuiCol_Button]               = guiColor(1.00f, 0.79f, 0.18f, 0.78f);
            style.Colors[ImGuiCol_ButtonHovered]        = guiColor(0.42f, 0.82f, 1.00f, 0.81f);
            style.Colors[ImGuiCol_ButtonActive]         = guiColor(0.72f, 1.00f, 1.00f, 0.86f);
            style.Colors[ImGuiCol_Header]               = guiColor(0.65f, 0.78f, 0.84f, 0.80f);
            style.Colors[ImGuiCol_HeaderHovered]        = guiColor(0.75f, 0.88f, 0.94f, 0.80f);
            style.Colors[ImGuiCol_HeaderActive]         = guiColor(0.55f, 0.68f, 0.74f, 0.80f);//Rgba(0.46f, 0.84f, 0.90f, 1.00f);
            style.Colors[ImGuiCol_ResizeGrip]           = guiColor(0.60f, 0.60f, 0.80f, 0.30f);
            style.Colors[ImGuiCol_ResizeGripHovered]    = guiColor(1.00f, 1.00f, 1.00f, 0.60f);
            style.Colors[ImGuiCol_ResizeGripActive]     = guiColor(1.00f, 1.00f, 1.00f, 0.90f);
            style.Colors[ImGuiCol_TextSelectedBg]       = guiColor(1.00f, 0.99f, 0.54f, 0.43f);
            style.Colors[ImGuiCol_PopupBg]              = guiColor(0.82f, 0.92f, 1.00f, 0.90f); // LinearColor(0.89f, 0.98f, 1.00f, 0.99f)
            // colors[ImGuiCol_ComboBg] = LinearColor(0.89f, 0.98f, 1.00f, 0.99f);
            // colors[ImGuiCol_CloseButton] = LinearColor(0.41f, 0.75f, 0.98f, 0.50f);
            // colors[ImGuiCol_CloseButtonHovered] = LinearColor(1.00f, 0.47f, 0.41f, 0.60f);
            // colors[ImGuiCol_CloseButtonActive] = LinearColor(1.00f, 0.16f, 0.00f, 1.00f);
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

static U32
gui_DockSpaceOnMainViewport(ImGuiDockNodeFlags dock_flags)
{
    ImGuiViewport const *viewport = ImGui::GetMainViewport();
    return ImGui::DockSpaceOverViewport(viewport, dock_flags, NULL);
}

CF_API void
guiDockSpace(GuiDockStyle style)
{
    CF_STATIC_ASSERT(ImGuiDockNodeFlags_PassthruCentralNode == GuiDockStyle_Transparent,
                     "ImGuiDockNodeFlags changed");
    CF_STATIC_ASSERT(ImGuiDockNodeFlags_NoDockingInCentralNode == GuiDockStyle_CentralViewport,
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
    guiTextV(fmt, args);
    va_end(args);
}

CF_API void
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
