// ******************************
//  Image viewer application
// ******************************
//
// TODO (Matteo): missing features
// - Drag after zoom
// - Animated GIF support
// - Bounded tool windows ?
// - Better memory allocation strategy based on actual usage patterns
//   (i.e. less usage of the heap allocator)
// - Cleanup loading code
// - Cache texture for image loading (create a new texture only if bigger size is required)
//   This will prevent some flickering
// - Display transparent images properly
//
// ******************************

#include "api.h"

#include "image.h"

#include "gui/gui.h"

#include "gl/gload.h"

#include "foundation/allocator.h"
#include "foundation/color.h"
#include "foundation/common.h"
#include "foundation/fs.h"
#include "foundation/maths.h"
#include "foundation/path.h"
#include "foundation/strings.h"
#include "foundation/threading.h"
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
    FILENAME_SIZE = 256,
};

typedef struct ImageView
{
    bool advanced;
    F32 zoom;
    I32 filter;
    U32 texture;
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
    I32 state;
} ImageFile;

/// Array of image file info backed by a large VM allocation (no waste since
/// memory is committed only when required)
typedef struct ImageList
{
    cfVirtualMemory *vm;
    Usize bytes_reserved;
    Usize bytes_committed;

    ImageFile *files;
    U32 num_files;
} ImageList;

enum
{
    QUEUE_SIZE = 16
};

typedef struct LoadQueue
{
    // Dependencies
    Threading *api;
    cfFileSystem *fs;
    cfAllocator *alloc;

    // Sync
    Mutex mutex;
    ConditionVariable wake;

    // Data
    ImageFile *buf[QUEUE_SIZE];
    U16 pos;
    U16 len;
    bool stop;
} LoadQueue;

struct AppState
{
    cfPlatform *plat;
    cfAllocator *alloc;

    FileDlgFilter filter;
    U32 curr_file;
    ImageList images;

    ImageView iv;

    LoadQueue queue;

    AppWindows windows;
};

//------------------------------------------------------------------------------

static void appLoadFromFile(AppState *state, char const *filename);
static bool appLoadImage(ImageFile *file, cfFileSystem *fs, cfAllocator *alloc);

static THREAD_PROC(loadProc);

//------------------------------------------------------------------------------

// Simple helper functions to load an image into a OpenGL texture with common settings

U32
textureLoadFromImage(Image *image)
{
    // Create a OpenGL texture identifier
    U32 texture = 0;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    // TODO (Matteo): Separate texture parameterization from loading

    // Setup filtering parameters for display
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    // These are required on WebGL for non power-of-two textures
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // Upload pixels into texture
#if defined(GL_UNPACK_ROW_LENGTH) && !defined(__EMSCRIPTEN__)
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
#endif
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image->width, image->height, 0, GL_RGBA,
                 GL_UNSIGNED_BYTE, image->data);

    return texture;
}

void
textureSetFilter(U32 texture, ImageFilter filter)
{
    glBindTexture(GL_TEXTURE_2D, texture);

    I32 value = (filter == ImageFilter_Linear) ? GL_LINEAR : GL_NEAREST;

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, value);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, value);
}

void
textureUnload(U32 *texture)
{
    glDeleteTextures(1, texture);
    *texture = 0;
}

//------------------------------------------------------------------------------
// Application creation/destruction

APP_API AppState *
appCreate(cfPlatform *plat, char const *argv[], I32 argc)
{
    // NOTE (Matteo): Memory comes cleared to 0
    AppState *app = cfAlloc(plat->heap, sizeof(*app));

    app->plat = plat;
    app->alloc = plat->heap;

    app->iv = (ImageView){
        .zoom = 1.0f,
        .filter = ImageFilter_Nearest,
    };

    // Init file list management
    app->filter.name = "Image files";
    app->filter.extensions = g_supported_ext;
    app->filter.num_extensions = CF_ARRAY_SIZE(g_supported_ext);
    app->curr_file = U32_MAX;

    Usize const images_vm = CF_GB(1);
    app->images.vm = plat->vm;
    app->images.bytes_reserved = images_vm;
    app->images.files = cfVmReserve(plat->vm, images_vm);

    // Init Dear Imgui
    guiInit(plat->gui);

    // Init image loading
    gloadInit(plat->gl);

    Threading *threading = app->plat->threading;
    app->queue.alloc = app->alloc;
    app->queue.fs = app->plat->fs;
    app->queue.api = threading;

    threading->cvInit(&app->queue.wake);
    threading->mutexInit(&app->queue.mutex);

    Thread load_thread =
        threading->threadCreate(&(ThreadParms){.proc = loadProc, .args = &app->queue});
    // TODO (Matteo): Detach or monitor thread?
    CF_UNUSED(load_thread);

    if (argc > 1)
    {
        appLoadFromFile(app, argv[1]);
    }

    return app;
}

APP_API void
appDestroy(AppState *app)
{
    for (U32 i = 0; i < app->images.num_files; ++i)
    {
        imageUnload(&app->images.files[i].image, app->alloc);
    }

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

    for (Usize i = 0; i < CF_ARRAY_SIZE(g_supported_ext); ++i)
    {
        if (strEqualInsensitive(g_supported_ext[i], ext)) return true;
    }

    return false;
}

static bool
appLoadImage(ImageFile *file, cfFileSystem *fs, cfAllocator *alloc)
{
    bool result = false;

    switch (file->state)
    {
        case ImageFileState_Loading: break;
        case ImageFileState_Loaded: result = true; break;
        default:
            file->state = ImageFileState_Loading;

            FileContent fc = fs->read_file(file->filename, alloc);

            if (fc.data)
            {
                file->state = ImageFileState_Loaded;
                result = imageLoadFromMemory(&file->image, fc.data, fc.size, alloc);
                cfFree(alloc, fc.data, fc.size);
            }
            else
            {
                file->state = ImageFileState_Failed;
            }
            break;
    }

    return result;
}

static ImageFile *
appPushImageFile(ImageList *images)
{
    ImageFile *file = images->files + images->num_files++;

    Usize commit_size = sizeof(*file) * images->num_files;

    CF_ASSERT(commit_size < images->bytes_reserved, "Out of memory");

    if (commit_size > images->bytes_committed)
    {
        // Round up committed memory as a multiple of the page size
        Usize page_size = images->vm->page_size;
        images->bytes_committed += page_size * ((commit_size + page_size - 1) / page_size);
        cfVmCommit(images->vm, images->files, images->bytes_committed);
    }

    file->state = ImageFileState_Idle;

    return file;
}

static void
appLoadFromFile(AppState *state, char const *full_name)
{
    cfFileSystem const *fs = state->plat->fs;
    ImageList *images = &state->images;
    ImageView *iv = &state->iv;

    for (U32 i = 0; i < images->num_files; ++i)
    {
        imageUnload(&images->files[i].image, state->alloc);
    }

    images->num_files = 0;
    state->curr_file = U32_MAX;

    if (full_name)
    {
        Usize full_size = strSize(full_name);
        CF_ASSERT(full_size <= FILENAME_SIZE, "filename is too long!");

        char const *name = pathSplitName(full_name);
        char dir_name[FILENAME_SIZE] = {0};

        cfMemCopy(full_name, dir_name, (Usize)(name - full_name));

        ImageFile file = {0};

        cfMemCopy(full_name, file.filename, full_size);

        if (appLoadImage(&file, state->plat->fs, state->alloc))
        {
            iv->zoom = 1.0f;
            iv->texture = textureLoadFromImage(&file.image);
            textureSetFilter(iv->texture, iv->filter);
        }
        else
        {
            state->windows.unsupported = true;
        }

        DirIter *it = fs->dir_iter_start(dir_name, state->alloc);

        if (it)
        {
            char const *path = NULL;

            // NOTE (Matteo): Explicit test against NULL is required for compiling with /W4 on MSVC
            while ((path = fs->dir_iter_next(it)) != NULL)
            {
                if (appIsFileSupported(path))
                {
                    ImageFile *tmp = appPushImageFile(images);
                    bool ok = strPrintf(tmp->filename, FILENAME_SIZE, "%s%s", dir_name, path);
                    CF_ASSERT(ok, "path is too long!");
                }
            }

            fs->dir_iter_close(it);
        }

        for (U32 index = 0; index < images->num_files; ++index)
        {
            if (strEqualInsensitive(full_name, state->images.files[index].filename))
            {
                state->curr_file = index;
                state->images.files[index] = file;
                break;
            }
        }
    }
}

static THREAD_PROC(loadProc)
{
    LoadQueue *queue = args;
    Threading *api = queue->api;

    while (true)
    {
        api->mutexAcquire(&queue->mutex);

        while (!queue->stop && queue->len == 0)
        {
            api->cvWaitMutex(&queue->wake, &queue->mutex, TIME_INFINITE);
        }

        if (queue->stop)
        {
            api->mutexRelease(&queue->mutex);
            break;
        }

        ImageFile *file = queue->buf[queue->pos];
        if (++queue->pos == QUEUE_SIZE) queue->pos = 0;
        queue->len--;

        api->mutexRelease(&queue->mutex);

        appLoadImage(file, queue->fs, queue->alloc);
    }
}

static void
loadQueueItem(LoadQueue *queue, ImageFile *file)
{
    Threading *api = queue->api;

    api->mutexAcquire(&queue->mutex);
    {
        CF_ASSERT(queue->len < QUEUE_SIZE, "Queue is full!");

        U16 write_pos = (queue->pos + queue->len) % QUEUE_SIZE;
        queue->buf[write_pos] = file;
        queue->len++;

        api->cvSignalOne(&queue->wake);
    }
    api->mutexRelease(&queue->mutex);
}

static void
appImageView(AppState *state)
{
    F32 const min_zoom = 1.0f;
    F32 const max_zoom = 10.0f;

    ImageView *iv = &state->iv;

    bool update_filter = false;

    // Image scaling settings

    // TODO (Matteo): Maybe this can get cleaner?
    if (iv->advanced)
    {
        if (igRadioButtonIntPtr("Nearest", &iv->filter, ImageFilter_Nearest))
        {
            update_filter = true;
        }
        guiSameLine();
        if (igRadioButtonIntPtr("Linear", &iv->filter, ImageFilter_Linear))
        {
            update_filter = true;
        }
        guiSameLine();
        igSliderFloat("zoom", &iv->zoom, min_zoom, max_zoom, "%.3f", 0);
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
        ImageFile *curr_file = state->images.files + state->curr_file;

        if (curr_file->state != ImageFileState_Loading)
        {
            U32 next = state->curr_file;

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
                loadQueueItem(&state->queue, state->images.files + next);
                textureUnload(&iv->texture);

                // NOTE (Matteo): improve browsing performance by pre-loading previous and next file

                U32 n = (next == state->images.num_files - 1) ? 0 : next + 1;
                U32 p = (next == 0) ? state->images.num_files - 1 : next - 1;

                if (n != next) loadQueueItem(&state->queue, state->images.files + n);
                if (p != n) loadQueueItem(&state->queue, state->images.files + p);
            }
        }
        else if (curr_file->state == ImageFileState_Loaded && iv->texture == 0)
        {
            iv->texture = textureLoadFromImage(&curr_file->image);
            update_filter = true;
        }

        ImGuiIO *io = igGetIO();

        if (igIsItemHovered(0) && io->KeyCtrl)
        {
            iv->zoom = cfClamp(iv->zoom + io->MouseWheel, min_zoom, max_zoom);
        }

        // 3. Draw the image properly scaled to fit the view
        // TODO (Matteo): Fix zoom behavior

        // NOTE (Matteo): in case of more precision required
        // I32 v = (I32)(1000 / *zoom);
        // I32 d = (1000 - v) / 2;
        // F32 uv = (F32)d * 0.001f;

        // NOTE (Matteo): the image is resized in order to adapt to the viewport, keeping the aspect
        // ratio at zoom level == 1; then zoom is applied

        F32 image_w = (F32)curr_file->image.width;
        F32 image_h = (F32)curr_file->image.height;
        F32 image_aspect = image_w / image_h;

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

        // NOTE (Matteo): Apply filtering if required
        if (update_filter) textureSetFilter(iv->texture, iv->filter);

        ImDrawList *draw_list = igGetWindowDrawList();
        ImDrawList_PushClipRect(draw_list, view_min, view_max, true);
        ImDrawList_AddImage(draw_list, (void *)(Iptr)iv->texture, image_min, image_max,
                            (ImVec2){0.0f, 0.0f}, (ImVec2){1.0f, 1.0f},
                            igGetColorU32U32(RGBA32_WHITE));

        if (iv->advanced)
        {
            // DEBUG (Matteo): Draw view and image bounds - remove when zoom is fixed
            ImU32 debug_color = igGetColorU32Vec4((ImVec4){1, 0, 1, 1});
            ImDrawList_AddRect(draw_list, image_min, image_max, debug_color, 0.0f, 0, 1.0f);
            ImDrawList_AddRect(draw_list, view_min, view_max, debug_color, 0.0f, 0, 1.0f);
        }
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

static bool
appMenuBar(AppState *state)
{
    bool quit = false;
    bool open_file_error = false;

    if (igBeginMainMenuBar())
    {
        if (igBeginMenu("File", true))
        {
            if (igMenuItemBool("Open", NULL, false, true))
            {
                open_file_error = !appOpenFile(state);
            }
            igSeparator();
            quit = igMenuItemBool("Quit", NULL, false, true);
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

    return quit;
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

APP_API AppUpdateResult
appUpdate(AppState *state, FontOptions *font_opts)
{
    cfPlatform *plat = state->plat;

    AppUpdateResult result = {
        .flags = AppUpdateFlags_None,
        .back_color = igGetColorU32Col(ImGuiCol_WindowBg, 1.0f),
    };

    //==== Main UI ====

    if (appMenuBar(state))
    {
        result.flags |= AppUpdateFlags_Quit;
    }

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
        F64 framerate = (F64)igGetIO()->Framerate;

        igBegin("Application stats", &state->windows.stats, 0);
        igText("Average %.3f ms/frame (%.1f FPS)", 1000.0 / framerate, framerate);
        igText("Allocated %.3fkb in %zu blocks", (F64)plat->heap_size / 1024, plat->heap_blocks);
        igText("Virtual memory reserved %.3fkb - committed %.3fkb", (F64)plat->reserved_size / 1024,
               (F64)plat->committed_size / 1024);
        igSeparator();
        igText("App base path:%s", state->plat->paths->base);
        igText("App data path:%s", state->plat->paths->data);
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
