#include "api.h"

#include "image.h"
#include "string_buff.h"

#include "gui.h"

#include "foundation/allocator.h"
#include "foundation/common.h"
#include "foundation/path.h"
#include "foundation/strings.h"
#include "foundation/util.h"
#include "foundation/vec.h"

static char const *g_supported_ext[] = {".jpg", ".jpeg", ".bmp", ".png", ".gif"};

typedef struct AppWindows
{
    bool demo;
    bool metrics;
    bool stats;
    bool fonts;
    bool style;
    bool unsupported;
} AppWindows;

enum
{
    CURR_DIR_SIZE = 256,
};

struct AppState
{
    cfPlatform *plat;
    cfAllocator *alloc;

    AppPaths paths;

    AppWindows windows;
    ImVec4 clear_color;

    Image image;
    bool image_adv;

    FileDlgFilter filter;
    StringBuff filenames;
    u32 curr_file;
    char curr_dir[CURR_DIR_SIZE];
};

//------------------------------------------------------------------------------

static void appLoadFromFile(AppState *state, char const *filename);

AppState *
appCreate(cfPlatform *plat, AppPaths paths, char *argv[], i32 argc)
{
    // NOTE (Matteo): Memory comes cleared to 0
    AppState *app = cfAlloc(&plat->heap, sizeof(*app));

    app->plat = plat;
    app->alloc = &plat->heap;

    app->clear_color = (ImVec4){0.45f, 0.55f, 0.60f, 1.00f};
    app->image = (Image){.zoom = 1.0};

    app->paths = paths;

    // Init file list management
    app->filter.name = "Image files";
    app->filter.extensions = g_supported_ext;
    app->filter.num_extensions = CF_ARRAY_SIZE(g_supported_ext);
    app->curr_file = StringBuff_MaxCount;
    cfMemClear(app->curr_dir, CURR_DIR_SIZE);

    if (argc > 1)
    {
        appLoadFromFile(app, argv[1]);
    }
    else
    {
        // TODO (Matteo): Remove - kind of demo, but should not be kept
        char buffer[1024];
        snprintf(buffer, 1024, "%sOpaque.png", paths.data);
        imageLoadFromFile(&app->image, buffer, app->alloc);
    }

    return app;
}

void
appDestroy(AppState *app)
{
    imageUnload(&app->image);
    cfFree(app->alloc, app, sizeof(*app));
}

//------------------------------------------------------------------------------

static bool
guiFileSupported(char const *path)
{
    char const *ext = pathSplitExt(path);
    if (!ext) return false;

    for (usize i = 0; i < CF_ARRAY_SIZE(g_supported_ext); ++i)
    {
        if (!strcmp(g_supported_ext[i], ext)) return true;
    }

    return false;
}

static void
appLoadFromFile(AppState *state, char const *filename)
{
    cfFileSystem *fs = &state->plat->fs;

    imageUnload(&state->image);

    sbClear(&state->filenames);
    state->curr_file = StringBuff_MaxCount;

    if (filename)
    {

        if (!imageLoadFromFile(&state->image, filename, state->alloc))
        {
            state->windows.unsupported = true;
        }

        char const *name = pathSplitName(filename);
        isize dir_size = name - filename;

        CF_ASSERT(0 <= dir_size && dir_size < CURR_DIR_SIZE, "Directory path too big");

        cfMemCopy(filename, state->curr_dir, (usize)dir_size);

        DirIter *it = fs->dir_iter_start(state->curr_dir, state->alloc);

        if (it)
        {
            char const *path = NULL;

            // NOTE (Matteo): Explicit test against NULL is required for compiling with /W4 on MSVC
            while ((path = fs->dir_iter_next(it)) != NULL)
            {
                if (guiFileSupported(path)) sbPush(&state->filenames, path);
            }

            fs->dir_iter_close(it);
        }

        for (u32 index = 0; index < state->filenames.count; ++index)
        {
            if (!_strcmpi(name, sbAt(&state->filenames, index)))
            {
                state->curr_file = index;
                break;
            }
        }
    }
}

static inline bool
guiKeyPressed(i32 key)
{
    return igIsKeyPressed(igGetIO()->KeyMap[key], true);
}

static void
guiImageViewer(AppState *state)
{
    igBegin("Image viewer", NULL, ImGuiWindowFlags_MenuBar);

    Image *image = &state->image;

    f32 const min_zoom = 1.0f;
    f32 const max_zoom = 10.0f;

    bool open_file_error = false;

    if (igBeginMenuBar())
    {
        if (igBeginMenu("File", true))
        {
            if (igMenuItemBool("Open", NULL, false, true))
            {
                char const *hint = (state->curr_file != StringBuff_MaxCount
                                        ? sbAt(&state->filenames, state->curr_file)
                                        : NULL);

                FileDlgResult dlg_result =
                    state->plat->fs.open_file_dlg(hint, &state->filter, 1, state->alloc);

                switch (dlg_result.code)
                {
                    case FileDlgResult_Ok: appLoadFromFile(state, dlg_result.filename); break;
                    case FileDlgResult_Error: open_file_error = true; break;
                    default: break;
                }

                cfFree(state->alloc, dlg_result.filename, dlg_result.filename_size);
            }
            igEndMenu();
        }

        if (igBeginMenu("View", true))
        {
            igMenuItemBoolPtr("Advanced", NULL, &state->image_adv, true);
            igEndMenu();
        }

        igEndMenuBar();
    }

    if (open_file_error) igOpenPopup("Open file error", 0);

    if (igBeginPopupModal("Open file error", NULL, ImGuiWindowFlags_AlwaysAutoResize))
    {
        igText("Error opening file");
        // TODO (Matteo): Find a way to center button inside the popup window
        if (guiButton("Ok"))
        {
            igCloseCurrentPopup();
        }
        igEndPopup();
    }

    // 1. Image scaling settings

    // TODO (Matteo): Maybe this can get cleaner?
    if (state->image_adv)
    {
        i32 filter = image->filter;

        igRadioButtonIntPtr("Nearest", &filter, ImageFilter_Nearest);
        guiSameLine();
        igRadioButtonIntPtr("Linear", &filter, ImageFilter_Linear);
        guiSameLine();
        igSliderFloat("zoom", &image->zoom, min_zoom, max_zoom, "%.3f", 0);

        imageSetFilter(image, filter);
    }

    // 2. Use the available content area as the image view; an invisible button
    // is used in order to catch input.

    ImVec2 view_size, view_min, view_max;
    igGetContentRegionAvail(&view_size);
    if (view_size.x < 50.0f) view_size.x = 50.0f;
    if (view_size.y < 50.0f) view_size.y = 50.0f;

    igInvisibleButton("Image viewer##Area", view_size, 0);
    igGetItemRectMin(&view_min);
    igGetItemRectMax(&view_max);

    if (state->curr_file != StringBuff_MaxCount)
    {
        u32 next = state->curr_file;

        if (guiKeyPressed(ImGuiKey_LeftArrow))
        {
            next = (next - 1) % state->filenames.count;
        }

        if (guiKeyPressed(ImGuiKey_RightArrow))
        {
            next = (next + 1) % state->filenames.count;
        }

        if (state->curr_file != next)
        {
            state->curr_file = next;
            imageUnload(image);
            imageLoadFromFile(image, sbAt(&state->filenames, next), state->alloc);
        }
    }

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
    vecAddN((f32 *)&image_min, (f32 *)&image_size, 2, (f32 *)&image_max);

    ImDrawList *dl = igGetWindowDrawList();
    ImDrawList_AddImage(dl, (void *)(iptr)image->texture, image_min, image_max, uv0, uv1,
                        igGetColorU32Vec4((ImVec4){1, 1, 1, 1}));

    if (state->image_adv)
    {
        // DEBUG (Matteo): Draw view and image bounds - remove when zoom is fixed
        ImU32 debug_color = igGetColorU32Vec4((ImVec4){1, 0, 1, 1});
        ImDrawList_AddRect(dl, image_min, image_max, debug_color, 0.0f, 0, 1.0f);
        ImDrawList_AddRect(dl, view_min, view_max, debug_color, 0.0f, 0, 1.0f);
    }

    igEnd();

    // // Edit bools storing our window open/close state
    // igCheckbox("Demo Window", &state->windows.demo);
    // // Edit 1 float using a slider from 0.0f to 1.0f
    // igSliderFloat("float", &f, 0.0f, 1.0f, "%.3f", 0);
    // // Edit 3 floats representing a color
    // igColorEdit3("clear color", (float *)&state->clear_color, 0);
}

AppUpdateFlags
appUpdate(AppState *state, FontOptions *font_opts)
{
    AppUpdateFlags result = AppUpdateFlags_None;

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

    guiImageViewer(state);

    if (state->windows.demo) igShowDemoWindow(&state->windows.demo);

    if (state->windows.fonts)
    {
        igBegin("Font Options", &state->windows.fonts, 0);

        if (guiFontOptions(font_opts))
        {
            result |= AppUpdateFlags_RebuildFonts;
        }

        igEnd();
    }

    if (state->windows.stats)
    {
        cfAllocatorStats stats = cfAllocStats(state->alloc);
        f64 framerate = (f64)igGetIO()->Framerate;

        igBegin("Application stats stats", &state->windows.stats, 0);
        igText("Average %.3f ms/frame (%.1f FPS)", 1000.0 / framerate, framerate);
        igText("Allocated %.3fkb in %zu blocks", (f64)stats.size / 1024, stats.count);
        igEnd();
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

    if (state->windows.unsupported)
    {
        state->windows.unsupported = false;
        igOpenPopup("Warning", 0);
    }

    if (igBeginPopupModal("Warning", NULL, ImGuiWindowFlags_AlwaysAutoResize))
    {
        igText("Unsupported file format");
        // TODO (Matteo): Find a way to center button inside the popup window
        if (guiButton("Ok"))
        {
            state->windows.unsupported = false;
            igCloseCurrentPopup();
        }
        igEndPopup();
    }

    return result;
}
