// ******************************
//  Image viewer application
// ******************************
//
// TODO (Matteo): missing features
// - Drag after zoom
// - Async file load/decode
// - Bounded tool windows
// - Better docking
// - Animated GIF support
//
// ******************************

#include "api.h"

#include "image.h"
#include "string_buff.h"

#include "gui.h"

#include "foundation/allocator.h"
#include "foundation/color.h"
#include "foundation/common.h"
#include "foundation/fs.h"
#include "foundation/path.h"
#include "foundation/strings.h"
#include "foundation/util.h"
#include "foundation/vec.h"

static char const *g_supported_ext[] = {".jpg", ".jpeg", ".bmp", ".png", ".gif"};

typedef struct AppWindows
{
    bool style;
    bool fonts;
    bool stats;
    bool metrics;
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

    FileDlgFilter filter;
    StringBuff filenames;
    u32 curr_file;
    char curr_dir[CURR_DIR_SIZE];

    Image image;
    bool image_adv;

    AppPaths paths;
    AppWindows windows;
};

//------------------------------------------------------------------------------

static void appLoadFromFile(AppState *state, char const *filename);
static bool appLoadImage(AppState *state, char const *filename);

//------------------------------------------------------------------------------

AppState *
appCreate(cfPlatform *plat, AppPaths paths, char const *argv[], i32 argc)
{
    // NOTE (Matteo): Memory comes cleared to 0
    AppState *app = cfAlloc(plat->heap, sizeof(*app));

    app->plat = plat;
    app->alloc = plat->heap;

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
        strPrintf(buffer, 1024, "%sOpaque.png", paths.data);
        appLoadImage(app, buffer);
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
appIsFileSupported(char const *path)
{
    char const *ext = pathSplitExt(path);
    if (!ext) return false;

    for (usize i = 0; i < CF_ARRAY_SIZE(g_supported_ext); ++i)
    {
        if (strEqualInsensitive(g_supported_ext[i], ext)) return true;
    }

    return false;
}

static bool
appLoadImage(AppState *state, char const *filename)
{
    bool result = false;
    FileContent fc = state->plat->fs->read_file(filename, state->alloc);

    if (fc.data)
    {
        result = imageLoadFromMemory(&state->image, fc.data, fc.size, state->alloc);
        cfFree(state->alloc, fc.data, fc.size);
    }

    return result;
}

static void
appLoadFromFile(AppState *state, char const *filename)
{
    cfFileSystem *fs = state->plat->fs;

    imageUnload(&state->image);

    sbClear(&state->filenames);
    state->curr_file = StringBuff_MaxCount;

    if (filename)
    {

        if (!appLoadImage(state, filename))
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
                if (appIsFileSupported(path)) sbPush(&state->filenames, path);
            }

            fs->dir_iter_close(it);
        }

        for (u32 index = 0; index < state->filenames.count; ++index)
        {
            if (strEqualInsensitive(name, sbAt(&state->filenames, index)))
            {
                state->curr_file = index;
                break;
            }
        }
    }
}

static void
appImageView(AppState *state)
{
    f32 const min_zoom = 1.0f;
    f32 const max_zoom = 10.0f;

    Image *image = &state->image;

    // Image scaling settings

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

    // Use the available content area as the image view; an invisible button
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
            if (next-- == 0) next = state->filenames.count - 1;
        }

        if (guiKeyPressed(ImGuiKey_RightArrow))
        {
            if (++next == state->filenames.count) next = 0;
        }

        if (state->curr_file != next)
        {
            state->curr_file = next;
            imageUnload(image);

            char buffer[CURR_DIR_SIZE];
            bool ok = strPrintf(buffer, CURR_DIR_SIZE, "%s/%s", state->curr_dir,
                                sbAt(&state->filenames, next));

            CF_ASSERT(ok, "path is too long!");

            appLoadImage(state, buffer);
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

    // NOTE (Matteo): the image is resized in order to adapt to the viewport, keeping the aspect
    // ratio at zoom level == 1; then zoom is applied

    f32 image_w = (f32)image->width;
    f32 image_h = (f32)image->height;
    f32 image_aspect = image_w / image_h;

    if (image_w > view_size.x)
    {
        image_w = view_size.x;
        image_h = image_w / image_aspect;
    }

    if (image_h > view_size.y)
    {
        image_h = view_size.y;
        image_w = image_h * image_aspect;
    }

    // NOTE (Matteo): Round image bounds to nearest pixel for stable rendering

    image_w = cfRound(image_w * image->zoom);
    image_h = cfRound(image_h * image->zoom);

    ImVec2 image_min = {cfRound(view_min.x + 0.5f * (view_size.x - image_w)),
                        cfRound(view_min.y + 0.5f * (view_size.y - image_h))};

    ImVec2 image_max = {image_min.x + image_w, //
                        image_min.y + image_h};

    ImDrawList *draw_list = igGetWindowDrawList();
    ImDrawList_PushClipRect(draw_list, view_min, view_max, true);
    ImDrawList_AddImage(draw_list, (void *)(iptr)image->texture, image_min, image_max,
                        (ImVec2){0.0f, 0.0f}, (ImVec2){1.0f, 1.0f}, igGetColorU32U32(RGBA32_WHITE));

    if (state->image_adv)
    {
        // DEBUG (Matteo): Draw view and image bounds - remove when zoom is fixed
        ImU32 debug_color = igGetColorU32Vec4((ImVec4){1, 0, 1, 1});
        ImDrawList_AddRect(draw_list, image_min, image_max, debug_color, 0.0f, 0, 1.0f);
        ImDrawList_AddRect(draw_list, view_min, view_max, debug_color, 0.0f, 0, 1.0f);
    }
}

AppUpdateResult
appUpdate(AppState *state, FontOptions *font_opts)
{
    cfPlatform *plat = state->plat;

    AppUpdateResult result = {.flags = AppUpdateFlags_None};
    bool open_file_error = false;

    if (igBeginMainMenuBar())
    {
        if (igBeginMenu("File", true))
        {
            if (igMenuItemBool("Open", NULL, false, true))
            {
                char const *hint = (state->curr_file != StringBuff_MaxCount
                                        ? sbAt(&state->filenames, state->curr_file)
                                        : NULL);

                FileDlgResult dlg_result =
                    plat->fs->open_file_dlg(hint, &state->filter, 1, state->alloc);

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
            igSeparator();
            igMenuItemBoolPtr("Style editor", NULL, &state->windows.style, true);
            igMenuItemBoolPtr("Font options", NULL, &state->windows.fonts, true);
            igSeparator();
            igMenuItemBoolPtr("Stats", NULL, &state->windows.stats, true);
            igMenuItemBoolPtr("Metrics", NULL, &state->windows.metrics, true);
            igEndMenu();
        }

        igEndMainMenuBar();
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

    guiBeginFullScreen("Main", false, false);
    appImageView(state);
    guiEndFullScreen();

    if (state->windows.fonts)
    {
        igBegin("Font Options", &state->windows.fonts, 0);

        if (guiFontOptions(font_opts))
        {
            result.flags |= AppUpdateFlags_RebuildFonts;
        }

        igEnd();
    }

    if (state->windows.stats)
    {
        f64 framerate = (f64)igGetIO()->Framerate;

        igBegin("Application stats stats", &state->windows.stats, 0);
        igText("Average %.3f ms/frame (%.1f FPS)", 1000.0 / framerate, framerate);
        igText("Allocated %.3fkb in %zu blocks", (f64)plat->heap_size / 1024, plat->heap_blocks);
        igSeparator();
        igText("App base path:%s", state->paths.base);
        igText("App data path:%s", state->paths.data);
        igEnd();
    }

    if (state->windows.style)
    {
        igBegin("Style Editor", &state->windows.style, 0);
        igShowStyleEditor(NULL);
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

    result.back_color = igGetColorU32Col(ImGuiCol_WindowBg, 1.0f);

    return result;
}
