#include "app.h"

#include "image.h"

#include "gui.h"

#include "foundation/allocator.h"
#include "foundation/common.h"
#include "foundation/platform.h"
#include "foundation/util.h"
#include "foundation/vec.h"

typedef struct FontOptions
{
    f32 rasterizer_multiply;
    // Freetype only
    bool freetype_enabled;
    u32 freetype_flags;
    // Stb only
    i32 oversample_h;
    i32 oversample_v;
} FontOptions;

typedef struct AppWindows
{
    bool demo;
    bool metrics;
    bool stats;
    bool fonts;
    bool style;

} AppWindows;

struct AppState
{
    cfPlatform *plat;
    cfAllocator *alloc;

    AppPaths paths;

    FontOptions font_opts;
    bool rebuild_fonts;

    AppWindows windows;
    ImVec4 clear_color;

    Image image;
};

AppState *
appCreate(cfPlatform *plat, AppPaths paths)
{
    AppState *app = cfAlloc(&plat->heap, sizeof(*app));

    app->plat = plat;
    app->alloc = &plat->heap;

    app->rebuild_fonts = true;
    app->font_opts = (FontOptions){
        .freetype_enabled = true,
        .freetype_flags = 0,
        .oversample_h = 1,
        .oversample_v = 1,
        .rasterizer_multiply = 1.0f,
    };

    app->windows = (AppWindows){
        .demo = false,
        .fonts = false,
        .metrics = false,
        .stats = false,
        .style = false,
    };

    app->clear_color = (ImVec4){0.45f, 0.55f, 0.60f, 1.00f};
    app->image = (Image){.zoom = 1.0};

    app->paths = paths;

    char buffer[1024];
    snprintf(buffer, 1024, "%sOpaque.png", paths.data);
    imageLoadFromFile(&app->image, buffer, app->alloc);

    return app;
}

void
appDestroy(AppState *app)
{
    imageUnload(&app->image);
    cfFree(app->alloc, app, sizeof(*app));
}

bool
appPrepareUpdate(AppState *state)
{
    if (!state->rebuild_fonts) return false;

    ImGuiIO *io = igGetIO();
    ImFontAtlas *fonts = io->Fonts;
    FontOptions const *font_opts = &state->font_opts;

    for (i32 i = 0; i < fonts->ConfigData.Size; ++i)
    {
        fonts->ConfigData.Data[i].RasterizerMultiply = font_opts->rasterizer_multiply;
        fonts->ConfigData.Data[i].OversampleH = font_opts->oversample_h;
        fonts->ConfigData.Data[i].OversampleV = font_opts->oversample_v;
    }

    if (font_opts->freetype_enabled)
    {
        fonts->FontBuilderIO = ImGuiFreeType_GetBuilderForFreeType();
        fonts->FontBuilderFlags = (u32)font_opts->freetype_flags;
    }
    else
    {
        fonts->FontBuilderIO = igImFontAtlasGetBuilderForStbTruetype();
    }

    ImFontAtlas_Build(fonts);
    state->rebuild_fonts = false;

    return true;
}

static void
guiShowAllocStats(cfAllocatorStats const *stats, bool *p_open)
{
    igBegin("Allocation stats", p_open, 0);
    igLabelText("# of allocations", "%zu", stats->count);
    igLabelText("Size of allocations", "%zu", stats->size);
    igEnd();
}

static bool
guiShowFontOptions(FontOptions *state, bool *p_open)
{
    ImFontAtlas *atlas = igGetIO()->Fonts;
    bool rebuild_fonts = false;

    igBegin("Font Options", p_open, 0);
    igShowFontSelector("Fonts");

    if (igRadioButtonBool("FreeType", state->freetype_enabled))
    {
        state->freetype_enabled = true;
        rebuild_fonts = true;
    }
    guiSameLine();
    if (igRadioButtonBool("Stb (Default)", !state->freetype_enabled))
    {
        state->freetype_enabled = false;
        rebuild_fonts = true;
    }

    rebuild_fonts |= igDragInt("TexGlyphPadding", &atlas->TexGlyphPadding, 0.1f, 1, 16, NULL,
                               ImGuiSliderFlags_None);

    rebuild_fonts |= igDragFloat("RasterizerMultiply", &state->rasterizer_multiply, 0.001f, 0.0f,
                                 2.0f, NULL, ImGuiSliderFlags_None);

    igSeparator();

    if (state->freetype_enabled)
    {

        rebuild_fonts |= igCheckboxFlagsUintPtr("NoHinting", &state->freetype_flags,
                                                ImGuiFreeTypeBuilderFlags_NoHinting);
        rebuild_fonts |= igCheckboxFlagsUintPtr("NoAutoHint", &state->freetype_flags,
                                                ImGuiFreeTypeBuilderFlags_NoAutoHint);
        rebuild_fonts |= igCheckboxFlagsUintPtr("ForceAutoHint", &state->freetype_flags,
                                                ImGuiFreeTypeBuilderFlags_ForceAutoHint);
        rebuild_fonts |= igCheckboxFlagsUintPtr("LightHinting", &state->freetype_flags,
                                                ImGuiFreeTypeBuilderFlags_LightHinting);
        rebuild_fonts |= igCheckboxFlagsUintPtr("MonoHinting", &state->freetype_flags,
                                                ImGuiFreeTypeBuilderFlags_MonoHinting);
        rebuild_fonts |=
            igCheckboxFlagsUintPtr("Bold", &state->freetype_flags, ImGuiFreeTypeBuilderFlags_Bold);
        rebuild_fonts |= igCheckboxFlagsUintPtr("Oblique", &state->freetype_flags,
                                                ImGuiFreeTypeBuilderFlags_Oblique);
        rebuild_fonts |= igCheckboxFlagsUintPtr("Monochrome", &state->freetype_flags,
                                                ImGuiFreeTypeBuilderFlags_Monochrome);
    }
    else
    {
        rebuild_fonts |= igDragInt("Oversample H", &state->oversample_h, 0.1f, 1, 5, NULL,
                                   ImGuiSliderFlags_None);
        rebuild_fonts |= igDragInt("Oversample V", &state->oversample_v, 0.1f, 1, 5, NULL,
                                   ImGuiSliderFlags_None);
    }

    igEnd();

    return rebuild_fonts;
}

static void
guiImageView(Image *image)
{

    f32 const min_zoom = 1.0f;
    f32 const max_zoom = 10.0f;

    // 1. Image scaling settings

    // TODO (Matteo): Maybe this can get cleaner?
    ImageFilter filter = image->filter;
    igRadioButtonIntPtr("Nearest", &filter, ImageFilter_Nearest);
    igRadioButtonIntPtr("Linear", &filter, ImageFilter_Linear);
    imageSetFilter(image, filter);

    igSliderFloat("zoom", &image->zoom, min_zoom, max_zoom, "%.3f", 0);

    // 2. Use the available content area as the image view; an invisible button
    // is used in order to catch input.

    ImVec2 view_size, view_min, view_max;
    igGetContentRegionAvail(&view_size);
    igInvisibleButton("Image view", view_size, 0);
    igGetItemRectMin(&view_min);
    igGetItemRectMax(&view_max);

    ImGuiIO *io = igGetIO();

    if (igIsItemHovered(0) && io->KeyCtrl)
    {
        image->zoom = cfClamp(image->zoom + io->MouseWheel, min_zoom, max_zoom);
    }

    // 3. Draw the image properly scaled to fit the view
    // TODO (Matteo): Fix zoom behavior

    // NOTE (Matteo): in case of more precision required
    // i32 v = (i32)(1000 / *zoom);
    // i32 d = (1000 - v) / 2;
    // f32 uv = (f32)d * 0.001f;

    f32 uv = 0.5f * (1 - 1 / image->zoom);
    ImVec2 uv0 = {uv, uv};
    ImVec2 uv1 = {1 - uv, 1 - uv};

    ImVec2 image_size = {(f32)image->width * image->zoom, (f32)image->height * image->zoom};

    f32 image_aspect = image_size.x / image_size.y;

    if (image_size.x > view_size.x)
    {
        image_size.x = view_size.x;
        image_size.y = image_size.x / image_aspect;
    }

    if (image_size.y > view_size.y)
    {
        image_size.y = view_size.y;
        image_size.x = image_size.y * image_aspect;
    }

    ImVec2 image_min = {view_min.x + 0.5f * (view_size.x - image_size.x),
                        view_min.y + 0.5f * (view_size.y - image_size.y)};
    ImVec2 image_max;
    vecAddN((const f32 *)&image_min, (const f32 *)&image_size, 2, (f32 *)&image_max);

    ImDrawList *dl = igGetWindowDrawList();
    ImDrawList_AddImage(dl, (void *)(iptr)image->texture, image_min, image_max, uv0, uv1,
                        igGetColorU32Vec4((ImVec4){1, 1, 1, 1}));

    // DEBUG (Matteo): Draw view and image bounds - remove when zoom is fixed
    ImU32 debug_color = igGetColorU32Vec4((ImVec4){1, 0, 1, 1});
    ImDrawList_AddRect(dl, image_min, image_max, debug_color, 0.0f, 0, 1.0f);
    ImDrawList_AddRect(dl, view_min, view_max, debug_color, 0.0f, 0, 1.0f);
}

static void
guiTestWindow(AppState *state)
{
    static f32 f = 0.0f;
    static i32 counter = 0;

    // Create a window called "Hello, world!" and append into it.
    igBegin("Test window", NULL, 0);

    // Display some text (you can use a format strings  too)
    igText("This is some useful text.");

    // Edit bools storing our window open/close state
    igCheckbox("Demo Window", &state->windows.demo);

    // Edit 1 float using a slider from 0.0f to 1.0f
    igSliderFloat("float", &f, 0.0f, 1.0f, "%.3f", 0);
    // Edit 3 floats representing a color
    igColorEdit3("clear color", (float *)&state->clear_color, 0);

    // Buttons return true when clicked (most widgets return true
    // when edited/activated)
    if (guiButton("Button")) counter++;

    guiSameLine();
    igText("counter = %d", counter);

    f64 framerate = (f64)igGetIO()->Framerate;
    igText("Application average %.3f ms/frame (%.1f FPS)", 1000.0 / framerate, framerate);

    guiImageView(&state->image);

    igEnd();
}

void
appUpdate(AppState *state)
{
    if (igBeginMainMenuBar())
    {
        if (igBeginMenu("File", true)) igEndMenu();

        if (igBeginMenu("Windows", true))
        {
            igMenuItemBoolPtr("Style editor", NULL, &state->windows.style, true);
            igMenuItemBoolPtr("Font options", NULL, &state->windows.fonts, true);
            igSeparator();
            igMenuItemBoolPtr("Stats", NULL, &state->windows.stats, true);
            igMenuItemBoolPtr("Metrics", NULL, &state->windows.metrics, true);
            igSeparator();
            igMenuItemBoolPtr("Demo window", NULL, &state->windows.demo, true);
            igEndMenu();
        }

        igEndMainMenuBar();
    }

    guiTestWindow(state);

    if (state->windows.demo) igShowDemoWindow(&state->windows.demo);

    if (state->windows.fonts)
    {
        state->rebuild_fonts = guiShowFontOptions(&state->font_opts, &state->windows.fonts);
    }

    if (state->windows.stats)
    {
        cfAllocatorStats stats = cfAllocStats(state->alloc);
        guiShowAllocStats(&stats, &state->windows.stats);
    }

    if (state->windows.style)
    {
        igBegin("Style Editor", &state->windows.style, 0);
        igShowStyleEditor(igGetStyle());
        igEnd();
    }

    if (state->windows.metrics)
    {
        igShowMetricsWindow(&state->windows.metrics);
    }
}
