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

#include "gui.h"

#include "foundation/allocator.h"
#include "foundation/color.h"
#include "foundation/common.h"
#include "foundation/fs.h"
#include "foundation/path.h"
#include "foundation/strings.h"
#include "foundation/util.h"
#include "foundation/vec.h"
#include "foundation/vm.h"

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
    FILENAME_SIZE = 256,
};

typedef struct ImageView
{
    Image image;
    bool advanced;
    f32 zoom;
    i32 filter;
} ImageView;

typedef enum ImageFileState
{
    ImageFileState_Idle = 0,
    ImageFileState_Loading,
    ImageFileState_Loaded,
    ImageFileState_Failed,
} ImageFileState;

typedef struct ImageFile
{
    char filename[FILENAME_SIZE];
    Image image;
    i32 state;
} ImageFile;

typedef struct ImageList
{
    cfVirtualMemory *vm;
    usize bytes_reserved;
    usize bytes_committed;

    ImageFile *files;
    u32 num_files;
} ImageList;

struct AppState
{
    cfPlatform *plat;
    cfAllocator *alloc;

    FileDlgFilter filter;
    u32 curr_file;
    char curr_dir[CURR_DIR_SIZE];
    ImageList images;

    ImageView iv;

    AppPaths paths;
    AppWindows windows;
};

//------------------------------------------------------------------------------

static void appLoadFromFile(AppState *state, char const *filename);
static bool appLoadImage(AppState *state, ImageFile *file, cfFileSystem *fs, cfAllocator *alloc);

//------------------------------------------------------------------------------
// Application creation/destruction

AppState *
appCreate(cfPlatform *plat, AppPaths paths, char const *argv[], i32 argc)
{
    // NOTE (Matteo): Memory comes cleared to 0
    AppState *app = cfAlloc(plat->heap, sizeof(*app));

    app->plat = plat;
    app->alloc = plat->heap;

    app->iv = (ImageView){
        .zoom = 1.0f,
        .filter = ImageFilter_Nearest,
    };

    app->paths = paths;

    // Init file list management
    app->filter.name = "Image files";
    app->filter.extensions = g_supported_ext;
    app->filter.num_extensions = CF_ARRAY_SIZE(g_supported_ext);
    app->curr_file = U32_MAX;
    cfMemClear(app->curr_dir, CURR_DIR_SIZE);

    usize const images_vm = CF_GB(1);
    app->images.vm = plat->vm;
    app->images.bytes_reserved = images_vm;
    app->images.files = cfVmReserve(plat->vm, images_vm);

    if (argc > 1)
    {
        appLoadFromFile(app, argv[1]);
    }
    else
    {
        // TODO (Matteo): Remove - kind of demo, but should not be kept

        ImageFile file = {0};
        strPrintf(app->curr_dir, CURR_DIR_SIZE, "%s", paths.data);
        strPrintf(file.filename, FILENAME_SIZE, "Opaque.png");
        if (appLoadImage(app, &file, app->plat->fs, app->alloc))
        {
            app->iv.image = file.image;
            imageSetFilter(&app->iv.image, app->iv.filter);
        }
    }

    return app;
}

void
appDestroy(AppState *app)
{
    imageUnload(&app->iv.image);
    cfVmRelease(app->images.vm, app->images.files, app->images.bytes_reserved);
    cfFree(app->alloc, app, sizeof(*app));
}

//------------------------------------------------------------------------------
// Application update

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
appLoadImage(AppState *state, ImageFile *file, cfFileSystem *fs, cfAllocator *alloc)
{
    bool result = false;

    if (file->state == ImageFileState_Loaded)
    {
        result = true;
    }
    else
    {
        char filename[CURR_DIR_SIZE];
        bool ok = strPrintf(filename, CURR_DIR_SIZE, "%s/%s", state->curr_dir, file->filename);

        CF_ASSERT(ok, "path is too long!");

        FileContent fc = fs->read_file(filename, alloc);

        if (fc.data)
        {
            file->state = ImageFileState_Loaded;
            result = imageLoadFromMemory(&file->image, fc.data, fc.size, alloc);
            cfFree(alloc, fc.data, fc.size);
        }
        else
        {
            file->state = ImageFileState_Idle;
        }
    }

    return result;
}

static void
appLoadFromFile(AppState *state, char const *filename)
{
    cfFileSystem *fs = state->plat->fs;
    ImageList *images = &state->images;

    for (u32 i = 0; i < images->num_files; ++i)
    {
        imageUnload(&images->files[i].image);
    }

    images->num_files = 0;
    state->curr_file = U32_MAX;

    if (filename)
    {
        char const *name = pathSplitName(filename);
        isize dir_size = name - filename;

        CF_ASSERT(0 <= dir_size && dir_size < CURR_DIR_SIZE, "Directory path too big");

        ImageFile file = {0};

        cfMemClear(state->curr_dir, CURR_DIR_SIZE);
        cfMemCopy(filename, state->curr_dir, (usize)dir_size);
        cfMemCopy(name, file.filename, strSize(name));

        if (appLoadImage(state, &file, state->plat->fs, state->alloc))
        {
            state->iv.image = file.image;
            state->iv.zoom = 1.0f;
            imageSetFilter(&state->iv.image, state->iv.filter);
        }
        else
        {
            state->windows.unsupported = true;
        }

        DirIter *it = fs->dir_iter_start(state->curr_dir, state->alloc);

        if (it)
        {
            char const *path = NULL;

            // NOTE (Matteo): Explicit test against NULL is required for compiling with /W4 on MSVC
            while ((path = fs->dir_iter_next(it)) != NULL)
            {
                if (appIsFileSupported(path))
                {
                    ImageFile *f = images->files + images->num_files++;

                    usize commit_size = sizeof(*f) * images->num_files;
                    CF_ASSERT(commit_size < images->bytes_reserved, "Out of memory");
                    if (commit_size > images->bytes_committed)
                    {
                        usize page_size = images->vm->page_size;
                        images->bytes_committed +=
                            page_size * ((commit_size + page_size - 1) / page_size);
                        cfVmCommit(images->vm, images->files, images->bytes_committed);
                    }

                    f->state = ImageFileState_Idle;
                    cfMemCopy(path, f->filename, strSize(path));
                }
            }

            fs->dir_iter_close(it);
        }

        for (u32 index = 0; index < images->num_files; ++index)
        {
            if (strEqualInsensitive(name, state->images.files[index].filename))
            {
                state->curr_file = index;
                state->images.files[index] = file;
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

    ImageView *iv = &state->iv;

    // Image scaling settings

    // TODO (Matteo): Maybe this can get cleaner?
    if (iv->advanced)
    {
        igRadioButtonIntPtr("Nearest", &iv->filter, ImageFilter_Nearest);
        guiSameLine();
        igRadioButtonIntPtr("Linear", &iv->filter, ImageFilter_Linear);
        guiSameLine();
        igSliderFloat("zoom", &iv->zoom, min_zoom, max_zoom, "%.3f", 0);

        imageSetFilter(&iv->image, iv->filter);
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

    if (state->curr_file != U32_MAX)
    {
        u32 next = state->curr_file;

        if (guiKeyPressed(ImGuiKey_LeftArrow))
        {
            if (next-- == 0) next = state->images.num_files - 1;
        }

        if (guiKeyPressed(ImGuiKey_RightArrow))
        {
            if (++next == state->images.num_files) next = 0;
        }

        if (state->curr_file != next)
        {
            state->curr_file = next;

            ImageFile *file = state->images.files + next;

            if (appLoadImage(state, file, state->plat->fs, state->alloc))
            {
                state->iv.image = file->image;
                state->iv.zoom = 1.0f;
                imageSetFilter(&state->iv.image, state->iv.filter);
            }
        }
    }

    ImGuiIO *io = igGetIO();

    if (igIsItemHovered(0) && io->KeyCtrl)
    {
        iv->zoom = cfClamp(iv->zoom + io->MouseWheel, min_zoom, max_zoom);
    }

    // 3. Draw the image properly scaled to fit the view
    // TODO (Matteo): Fix zoom behavior

    // NOTE (Matteo): in case of more precision required
    // i32 v = (i32)(1000 / *zoom);
    // i32 d = (1000 - v) / 2;
    // f32 uv = (f32)d * 0.001f;

    // NOTE (Matteo): the image is resized in order to adapt to the viewport, keeping the aspect
    // ratio at zoom level == 1; then zoom is applied

    f32 image_w = (f32)iv->image.width;
    f32 image_h = (f32)iv->image.height;
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

    image_w = cfRound(image_w * iv->zoom);
    image_h = cfRound(image_h * iv->zoom);

    ImVec2 image_min = {cfRound(view_min.x + 0.5f * (view_size.x - image_w)),
                        cfRound(view_min.y + 0.5f * (view_size.y - image_h))};

    ImVec2 image_max = {image_min.x + image_w, //
                        image_min.y + image_h};

    ImDrawList *draw_list = igGetWindowDrawList();
    ImDrawList_PushClipRect(draw_list, view_min, view_max, true);
    ImDrawList_AddImage(draw_list, (void *)(iptr)iv->image.texture, image_min, image_max,
                        (ImVec2){0.0f, 0.0f}, (ImVec2){1.0f, 1.0f}, igGetColorU32U32(RGBA32_WHITE));

    if (iv->advanced)
    {
        // DEBUG (Matteo): Draw view and image bounds - remove when zoom is fixed
        ImU32 debug_color = igGetColorU32Vec4((ImVec4){1, 0, 1, 1});
        ImDrawList_AddRect(draw_list, image_min, image_max, debug_color, 0.0f, 0, 1.0f);
        ImDrawList_AddRect(draw_list, view_min, view_max, debug_color, 0.0f, 0, 1.0f);
    }
}

static bool
appOpenFile(AppState *state)
{
    cfPlatform *plat = state->plat;
    bool result = true;

    char const *hint =
        (state->curr_file != U32_MAX ? state->images.files[state->curr_file].filename : NULL);

    FileDlgResult dlg_result = plat->fs->open_file_dlg(hint, &state->filter, 1, state->alloc);

    switch (dlg_result.code)
    {
        case FileDlgResult_Ok: appLoadFromFile(state, dlg_result.filename); break;
        case FileDlgResult_Error: result = false; break;
    }

    cfFree(state->alloc, dlg_result.filename, dlg_result.filename_size);

    return result;
}

static void
appMenuBar(AppState *state)
{
    bool open_file_error = false;

    if (igBeginMainMenuBar())
    {
        if (igBeginMenu("File", true))
        {
            if (igMenuItemBool("Open", NULL, false, true))
            {
                open_file_error = !appOpenFile(state);
            }
            igEndMenu();
        }

        if (igBeginMenu("View", true))
        {
            igMenuItemBoolPtr("Advanced", NULL, &state->iv.advanced, true);
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
}

static void
appMainWindow(AppState *state)
{
    // NOTE (Matteo): Layout main window as a fixed dockspace that can host tool windows
    // ImGuiDockNodeFlags_NoDockingInCentralNode is used to prevent tool windows from hiding the
    // image view
    ImGuiWindowFlags const window_flags =
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus |
        ImGuiWindowFlags_NoNavFocus;
    ImGuiViewport const *viewport = igGetMainViewport();
    ImGuiID const dock_id =
        igDockSpaceOverViewport(viewport, ImGuiDockNodeFlags_NoDockingInCentralNode, NULL);

    igSetNextWindowDockID(dock_id, ImGuiCond_None);
    igBegin("Main", 0, window_flags);

    // NOTE (Matteo): Instruct the docking system to consider the window's node always as the
    // central one, thus not using it as a docking target (there's the backing dockspace already)
    ImGuiDockNode *dock_node = igGetWindowDockNode();
    dock_node->LocalFlags |= ImGuiDockNodeFlags_NoTabBar | ImGuiDockNodeFlags_CentralNode;

    appImageView(state);

    igEnd();
}

AppUpdateResult
appUpdate(AppState *state, FontOptions *font_opts)
{
    cfPlatform *plat = state->plat;

    AppUpdateResult result = {
        .flags = AppUpdateFlags_None,
        .back_color = igGetColorU32Col(ImGuiCol_WindowBg, 1.0f),
    };

    //==== Main UI ====

    appMenuBar(state);
    appMainWindow(state);

    //==== Tool windows ====

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

        igBegin("Application stats", &state->windows.stats, 0);
        igText("Average %.3f ms/frame (%.1f FPS)", 1000.0 / framerate, framerate);
        igText("Allocated %.3fkb in %zu blocks", (f64)plat->heap_size / 1024, plat->heap_blocks);
        igText("Virtual memory reserved %.3fkb - committed %.3fkb", (f64)plat->reserved_size / 1024,
               (f64)plat->committed_size / 1024);
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

    //==== Popups ====

    if (igBeginPopupModal("Open file error", NULL, ImGuiWindowFlags_AlwaysAutoResize))
    {
        igText("Error opening file");
        if (guiCenteredButton("Ok"))
        {
            igCloseCurrentPopup();
        }
        igEndPopup();
    }

    if (igBeginPopupModal("Warning", NULL, ImGuiWindowFlags_AlwaysAutoResize))
    {
        igText("Unsupported file format");
        if (guiCenteredButton("Ok"))
        {
            state->windows.unsupported = false;
            igCloseCurrentPopup();
        }
        igEndPopup();
    }

    return result;
}
