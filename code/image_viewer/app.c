// ******************************
//  Image viewer application
// ******************************
//
// TODO (Matteo): missing features
// ! Display transparent images properly
// ! Drag after zoom
// ! Properly handle loading files that go out of window
// - Animated GIF support
// - Cleanup loading code
// - Bounded tool windows ?
// - Better memory allocation strategy based on actual usage patterns
//   (i.e. less usage of the heap allocator)
// - Compress browsing code (too much duplication)
//
// ******************************

#include "api.h"

#include "image.h"

#include "gui/gui.h"

#include "gl/gload.h"

#include "foundation/core.h"

#include "foundation/array.h"
#include "foundation/color.h"
#include "foundation/fs.h"
#include "foundation/maths.h"
#include "foundation/memory.h"
#include "foundation/path.h"
#include "foundation/strings.h"
#include "foundation/threading.h"

static char const *g_supported_ext[] = {".jpg", ".jpeg", ".bmp", ".png", ".gif"};

#define MAIN_WINDOW "Main"
#define STYLE_WINDOW "Style Editor"
#define FONTS_WINDOW "Font Options"
#define STATS_WINDOW "Application Statistics"

enum Constants
{
    FILENAME_SIZE = 256,
    LoadQueue_Size = 16,
    BrowseWidth = 5,
};

CF_STATIC_ASSERT(BrowseWidth & 1, "Browse width must be odd");
CF_STATIC_ASSERT(BrowseWidth > 1, "Browse width must be > 1");

typedef struct AppWindows
{
    bool style;
    bool fonts;
    bool stats;
    bool metrics;
    bool unsupported;
} AppWindows;

typedef enum ImageFilter
{
    ImageFilter_Nearest,
    ImageFilter_Linear,
} ImageFilter;

typedef struct ImageTex
{
    U32 id;
    I32 width;
    I32 height;
} ImageTex;

typedef struct ImageView
{
    bool advanced;
    F32 zoom;
    I32 filter;
    ImageTex tex[2];
    struct // FLAGS
    {
        U8 tex_index : 1;
        U8 dirty     : 1;
        U8 _         : 6;
    };
} ImageView;

typedef enum ImageFileState
{
    ImageFileState_Idle = 0,
    ImageFileState_Queued,
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
typedef cfArray(ImageFile) ImageList;

typedef struct LoadQueue
{
    // Dependencies
    cfFileSystem *fs;
    cfAllocator alloc;

    // Sync
    Thread thread;
    Mutex mutex;
    ConditionVariable wake;

    // Data
    ImageFile *buf[LoadQueue_Size];
    U16 pos;
    U16 len;
    bool stop;
} LoadQueue;

struct AppState
{
    Platform *plat;

    Arena *arena;
    cfAllocator heap;

    FileDlgFilter filter;

    Usize browse_width;
    Usize curr_file;
    ImageList images;

    ImageView iv;

    LoadQueue queue;

    AppWindows windows;
};

//------------------------------------------------------------------------------
// Image loading prototypes (those functions are commonly used)

static void appLoadFromFile(AppState *state, char const *filename);

//------------------------------------------------------------------------------
// Async file loading

static void
loadQueuePush(LoadQueue *queue, ImageFile *file)
{
    if (file->state == ImageFileState_Idle)
    {
        mutexAcquire(&queue->mutex);
        {
            CF_ASSERT(queue->len < LoadQueue_Size, "Queue is full!");

            U16 write_pos = (queue->pos + queue->len) % LoadQueue_Size;
            queue->buf[write_pos] = file;
            queue->len++;

            file->state = ImageFileState_Queued;

            cvSignalOne(&queue->wake);
        }
        mutexRelease(&queue->mutex);
    }
}

static THREAD_PROC(loadQueueProc)
{
    LoadQueue *queue = args;
    for (;;)
    {
        ImageFile *file = NULL;

        // Wait and dequeue file
        mutexAcquire(&queue->mutex);
        {
            while (!queue->stop && queue->len == 0)
            {
                cvWaitMutex(&queue->wake, &queue->mutex, TIME_INFINITE);
            }

            if (queue->stop)
            {
                mutexRelease(&queue->mutex);
                break;
            }

            file = queue->buf[queue->pos];
            if (++queue->pos == LoadQueue_Size) queue->pos = 0;
            queue->len--;
        }
        mutexRelease(&queue->mutex);

        // Process file
        CF_ASSERT_NOT_NULL(file);

        if (file->state == ImageFileState_Queued)
        {
            file->state = ImageFileState_Loading;

            if (imageLoadFromFile(&file->image, file->filename, queue->alloc))
            {
                file->state = ImageFileState_Loaded;
            }
            else
            {
                file->state = ImageFileState_Failed;
            }
        }
    }
}

static void
loadQueueStart(LoadQueue *queue)
{
    CF_ASSERT_NOT_NULL(queue);
    CF_ASSERT(!queue->thread.handle, "Load queue thread is running");

    queue->stop = false;
    queue->thread = threadStart(loadQueueProc, .args = queue, .debug_name = "Load queue");
}

static void
loadQueueStop(LoadQueue *queue)
{
    mutexAcquire(&queue->mutex);
    {
        queue->stop = true;
        cvSignalOne(&queue->wake);
    }
    mutexRelease(&queue->mutex);

    threadWait(queue->thread, TIME_INFINITE);
    queue->thread.handle = 0;
}

//------------------------------------------------------------------------------
// Simple helper functions to load an image into a OpenGL texture with common settings

static void
imageTexBuild(ImageTex *tex)
{
    CF_ASSERT(tex->id == 0, "Overwriting an existing texture");

    glGenTextures(1, &tex->id);
    glBindTexture(GL_TEXTURE_2D, tex->id);

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

    // NOTE (Matteo): The following is the official suggested way to create a complete texture with
    // a single mipmap level (multiple levels are not needed for the purpose of these textures and
    // are only a waste of memory and bandwidth)
    // See: https://www.khronos.org/opengl/wiki/Common_Mistakes#Creating_a_complete_texture
    if (gloadIsSupported(4, 2))
    {
        glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, tex->width, tex->height);
    }
    else
    {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, tex->width, tex->height, 0, GL_RGBA,
                     GL_UNSIGNED_BYTE, NULL);
    }
}

static ImageTex
imageTexCreate(I32 width, I32 height)
{
    ImageTex tex = {.width = width, .height = height};
    imageTexBuild(&tex);
    return tex;
}

static void
imageViewInit(ImageView *iv)
{
    iv->advanced = false;
    iv->zoom = 1.0f;
    iv->dirty = true;
    iv->filter = ImageFilter_Nearest;
    iv->tex[0] = imageTexCreate(4920, 3264);
    iv->tex[1] = imageTexCreate(4920, 3264);
    iv->tex_index = 0;
}

static void
imageViewShutdown(ImageView *iv)
{
    glDeleteTextures(1, &iv->tex[0].id);
    glDeleteTextures(1, &iv->tex[1].id);
    iv->tex[0].id = 0;
    iv->tex[1].id = 0;
}

static inline ImageTex *
imageViewCurrTex(ImageView *iv)
{
    return iv->tex + iv->tex_index;
}

static void
imageViewUpdate(ImageView *iv, Image const *image)
{
    if (iv->dirty)
    {
        // NOTE (Matteo): Using a "dirty" flag to avoid redundant GPU uploads.
        // TODO (Matteo): Does this make texture double buffering useless?
        iv->dirty = false;

        // NOTE (Matteo): Swap textures by incrementing index modulo 2 (no. of textures)
        iv->tex_index = (iv->tex_index + 1) & 1;

        ImageTex *tex = iv->tex + iv->tex_index;

        if (image->width > tex->width || image->height > tex->height)
        {
            glDeleteTextures(1, &tex->id);
            tex->id = 0;
            tex->width = cfMax(image->width, tex->width);
            tex->height = cfMax(image->height, tex->height);
            imageTexBuild(tex);
        }

        I32 value = (iv->filter == ImageFilter_Linear) ? GL_LINEAR : GL_NEAREST;
        glBindTexture(GL_TEXTURE_2D, tex->id);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, value);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, value);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, image->width, image->height, GL_RGBA,
                        GL_UNSIGNED_BYTE, image->bytes);
    }
}

//------------------------------------------------------------------------------
// Application creation/destruction

APP_API AppState *
appCreate(Platform *plat, char const *argv[], I32 argc)
{
    // NOTE (Matteo): Memory comes cleared to 0
    Arena *arena = arenaBootstrap(plat->vm, CF_GB(1));
    AppState *app = arenaAllocStruct(arena, AppState);

    app->plat = plat;
    app->heap = plat->heap;
    app->arena = arena;

    // Init file list management
    app->filter.name = "Image files";
    app->filter.extensions = g_supported_ext;
    app->filter.num_extensions = CF_ARRAY_SIZE(g_supported_ext);
    app->curr_file = USIZE_MAX;

    // Usize const images_vm = CF_GB(1);
    cfArrayInit(&app->images, arenaAllocator(arena));

    app->queue.alloc = app->heap;
    app->queue.fs = app->plat->fs;
    cvInit(&app->queue.wake);
    mutexInit(&app->queue.mutex);

    // NOTE (Matteo): Init global state (same as library re-load)
    appLoad(app);

    imageViewInit(&app->iv);

    if (argc > 1)
    {
        appLoadFromFile(app, argv[1]);
    }

    return app;
}

APP_API APP_PROC(appLoad)
{
    CF_ASSERT_NOT_NULL(app);
    CF_ASSERT_NOT_NULL(app->plat);
    // Init Dear Imgui
    guiInit(app->plat->gui);
    // Init image loading
    gloadInit(app->plat->gl);

    loadQueueStart(&app->queue);
}

APP_API APP_PROC(appUnload)
{
    loadQueueStop(&app->queue);
}

static void
appClearImages(AppState *app)
{
    for (U32 i = 0; i < app->images.len; ++i)
    {
        ImageFile *file = app->images.buf + i;

        CF_ASSERT(file->state != ImageFileState_Queued, "Leaking queued files");

        if (file->state == ImageFileState_Loaded)
        {
            imageUnload(&file->image, app->heap);
            file->state = ImageFileState_Idle;
        }
        else
        {
            CF_ASSERT(file->image.bytes == NULL, "Invalid file state");
        }
    }

    cfArrayClear(&app->images);

    app->curr_file = USIZE_MAX;
}

APP_API void
appDestroy(AppState *app)
{
    appUnload(app);
    appClearImages(app);
    imageViewShutdown(&app->iv);
    cfArrayFree(&app->images);
    arenaShutdown(app->arena);
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

static ImageFile *
appPushImageFile(ImageList *images)
{
    cfArrayPush(images, (ImageFile){.state = ImageFileState_Idle});
    return cfArrayLast(images);
}

static void
appQueueLoadFiles(AppState *app)
{
    // NOTE (Matteo): improve browsing performance by pre-loading previous and next files

    ImageFile *files = app->images.buf;
    Usize num_files = app->images.len;
    Usize curr = app->curr_file;

    loadQueuePush(&app->queue, files + curr);

    if (app->browse_width == num_files)
    {
        for (Usize i = app->curr_file + 1; i < num_files; ++i)
        {
            loadQueuePush(&app->queue, files + i);
        }

        for (Usize i = 0; i < app->curr_file; ++i)
        {
            loadQueuePush(&app->queue, files + app->curr_file - 1 - i);
        }
    }
    else
    {
        CF_ASSERT(app->browse_width > 1, "Window width must be > 1");
        CF_ASSERT(app->browse_width & 1, "Window width must be odd");

        Usize mid = app->browse_width / 2;
        for (Usize i = 0; i < mid; ++i)
        {
            Usize next = cfMod(curr + i + 1, num_files);
            Usize prev = cfMod(curr - i - 1, num_files);
            CF_ASSERT(next != curr, "");
            CF_ASSERT(next != prev, "");
            CF_ASSERT(prev != curr, "");
            loadQueuePush(&app->queue, files + next);
            loadQueuePush(&app->queue, files + prev);
        }
    }

    // NOTE (Matteo): Set view as dirty
    app->iv.dirty = true;
    app->iv.zoom = 1.0f;
}

static void
appBrowseNext(AppState *app)
{
    CF_ASSERT(app->curr_file != U32_MAX, "Invalid browse command");

    Usize next = cfWrapInc(app->curr_file, app->images.len);

    if (app->browse_width != app->images.len)
    {
        Usize lru = cfMod(app->curr_file - app->browse_width / 2, app->images.len);
        ImageFile *file = app->images.buf + lru;
        if (file->state == ImageFileState_Loaded)
        {
            imageUnload(&file->image, app->heap);
            file->state = ImageFileState_Idle;
        }
    }
    else
    {
        CF_ASSERT(app->images.buf[next].state != ImageFileState_Idle, "");
    }

    app->curr_file = next;
    appQueueLoadFiles(app);
}

static void
appBrowsePrev(AppState *app)
{
    CF_ASSERT(app->curr_file != USIZE_MAX, "Invalid browse command");

    Usize prev = cfWrapDec(app->curr_file, app->images.len);

    if (app->browse_width != app->images.len)
    {
        Usize lru = cfMod(app->curr_file + app->browse_width / 2, app->images.len);
        ImageFile *file = app->images.buf + lru;
        if (file->state == ImageFileState_Loaded)
        {
            imageUnload(&file->image, app->heap);
            file->state = ImageFileState_Idle;
        }
    }
    else
    {
        CF_ASSERT(app->images.buf[prev].state != ImageFileState_Idle, "");
    }

    app->curr_file = prev;
    appQueueLoadFiles(app);
}

static void
appLoadFromFile(AppState *state, char const *full_name)
{
    cfFileSystem const *fs = state->plat->fs;
    ImageList *images = &state->images;

    appClearImages(state);

    if (full_name)
    {
        Usize full_size = strSize(full_name);
        CF_ASSERT(full_size <= FILENAME_SIZE, "filename is too long!");

        char const *name = pathSplitName(full_name);
        char dir_name[FILENAME_SIZE] = {0};

        cfMemCopy(full_name, dir_name, (Usize)(name - full_name));

        ImageFile file = {0};

        cfMemCopy(full_name, file.filename, full_size);

        DirIter *it = fs->dirIterStart(dir_name, state->heap);

        if (it)
        {
            char const *path = NULL;

            // NOTE (Matteo): Explicit test against NULL is required for compiling with /W4 on MSVC
            while ((path = fs->dirIterNext(it)) != NULL)
            {
                if (appIsFileSupported(path))
                {
                    ImageFile *tmp = appPushImageFile(images);
                    bool ok = strPrintf(tmp->filename, FILENAME_SIZE, "%s%s", dir_name, path);
                    CF_ASSERT(ok, "path is too long!");
                }
            }

            fs->dirIterClose(it);
        }

        for (U32 index = 0; index < images->len; ++index)
        {
            if (strEqualInsensitive(full_name, state->images.buf[index].filename))
            {
                state->curr_file = index;
                state->images.buf[index] = file;
                break;
            }
        }

        state->browse_width = cfMin(BrowseWidth, images->len);
        appQueueLoadFiles(state);
    }
}

static void
appImageView(AppState *state)
{
    F32 const min_zoom = 1.0f;
    F32 const max_zoom = 10.0f;

    ImageView *iv = &state->iv;

    // Image scaling settings

    // TODO (Matteo): Maybe this can get cleaner?
    if (iv->advanced)
    {
        I32 filter = iv->filter;
        igRadioButtonIntPtr("Nearest", &filter, ImageFilter_Nearest);
        guiSameLine();
        igRadioButtonIntPtr("Linear", &filter, ImageFilter_Linear);
        guiSameLine();
        igSliderFloat("zoom", &iv->zoom, min_zoom, max_zoom, "%.3f", 0);
        iv->dirty = filter != iv->filter;
        iv->filter = filter;
    }

    // NOTE (Matteo): Use the available content area as the image view; an invisible button
    // is used in order to catch input.

    ImVec2 view_size, view_min, view_max;
    igGetContentRegionAvail(&view_size);
    if (view_size.x < 50.0f) view_size.x = 50.0f;
    if (view_size.y < 50.0f) view_size.y = 50.0f;

    igInvisibleButton("Image viewer##Area", view_size, 0);
    igGetItemRectMin(&view_min);
    igGetItemRectMax(&view_max);

    if (state->curr_file != USIZE_MAX)
    {
        ImageFile *curr_file = state->images.buf + state->curr_file;
        bool process_input = true;

        switch (curr_file->state)
        {
            case ImageFileState_Loading:
            case ImageFileState_Queued:
                // Do nothing
                process_input = false;
                break;

            case ImageFileState_Loaded:
                // Update texture and proceed with input
                imageViewUpdate(iv, &curr_file->image);
                break;

            case ImageFileState_Failed:
                // Signal error
                state->windows.unsupported = true;
                break;
        }

        if (process_input)
        {
            if (guiKeyPressed(ImGuiKey_LeftArrow)) appBrowsePrev(state);
            if (guiKeyPressed(ImGuiKey_RightArrow)) appBrowseNext(state);
        }

        ImGuiIO *io = igGetIO();

        if (igIsItemHovered(0) && io->KeyCtrl)
        {
            iv->zoom = cfClamp(iv->zoom + io->MouseWheel, min_zoom, max_zoom);
        }

        // NOTE (Matteo): Draw the image properly scaled to fit the view
        // TODO (Matteo): Fix zoom behavior

        F32 image_w = (F32)curr_file->image.width;
        F32 image_h = (F32)curr_file->image.height;
        ImageTex *tex = imageViewCurrTex(iv);

        // NOTE (Matteo): Clamp the displayed portion of the texture to the actual image size, since
        // the texture could be larger in order to be reused
        ImVec2 clamp_uv = {
            image_w / (F32)tex->width,
            image_h / (F32)tex->height,
        };

        // NOTE (Matteo): the image is resized in order to adapt to the viewport, keeping the aspect
        // ratio at zoom level == 1; then zoom is applied
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

        ImDrawList *draw_list = igGetWindowDrawList();
        ImDrawList_PushClipRect(draw_list, view_min, view_max, true);
        ImDrawList_AddImage(draw_list, (ImTextureID)(Iptr)tex->id, image_min, image_max,
                            (ImVec2){0.0f, 0.0f}, clamp_uv, igGetColorU32U32(RGBA32_WHITE));

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
    Platform *plat = state->plat;
    bool result = true;

    char const *hint =
        (state->curr_file != USIZE_MAX ? state->images.buf[state->curr_file].filename : NULL);

    FileDlgResult dlg_result = plat->fs->open_file_dlg(hint, &state->filter, 1, state->heap);

    switch (dlg_result.code)
    {
        case FileDlgResult_Ok: appLoadFromFile(state, dlg_result.filename); break;
        case FileDlgResult_Error: result = false; break;
    }

    cfFree(state->heap, dlg_result.filename, dlg_result.filename_size);

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
            igMenuItemBoolPtr(STYLE_WINDOW, NULL, &state->windows.style, true);
            igMenuItemBoolPtr(FONTS_WINDOW, NULL, &state->windows.fonts, true);
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
    ImGuiDockNodeFlags const dock_flags = ImGuiDockNodeFlags_NoDockingInCentralNode;
    ImGuiViewport const *viewport = igGetMainViewport();
    ImGuiID dock_id = igDockSpaceOverViewport(viewport, dock_flags, NULL);

    // NOTE (Matteo): Setup docking layout on first run (if the dockspace node is already split the
    // layout has been setup and maybe modified by the user).
    // This code is partially copied from github since the DockBuilder API is not documented -
    // understand it better!
    ImGuiDockNode *dockspace_node = igDockBuilderGetNode(dock_id);
    if (!dockspace_node || !ImGuiDockNode_IsSplitNode(dockspace_node))
    {
        ImGuiID dock_id_right =
            igDockBuilderSplitNode(dock_id, ImGuiDir_Right, 0.25f, NULL, &dock_id);
        ImGuiID dock_id_down =
            igDockBuilderSplitNode(dock_id, ImGuiDir_Down, 0.25f, NULL, &dock_id);

        // Pre-dock application windows in the created nodes
        igDockBuilderDockWindow(MAIN_WINDOW, dock_id);
        igDockBuilderDockWindow(STATS_WINDOW, dock_id_down);
        igDockBuilderDockWindow(STYLE_WINDOW, dock_id_right);
        igDockBuilderDockWindow(FONTS_WINDOW, dock_id_right);
        igDockBuilderFinish(dock_id);
    }

    igBegin(MAIN_WINDOW, 0, window_flags);

    // NOTE (Matteo): Instruct the docking system to consider the window's node always as the
    // central one, thus not using it as a docking target (there's the backing dockspace already)
    ImGuiDockNode *main_node = igGetWindowDockNode();
    main_node->LocalFlags |= ImGuiDockNodeFlags_NoTabBar | ImGuiDockNodeFlags_CentralNode;

    appImageView(state);

    igEnd();
}

APP_API AppUpdateResult
appUpdate(AppState *state, FontOptions *font_opts)
{
    Platform *plat = state->plat;

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
        igBegin(FONTS_WINDOW, &state->windows.fonts, 0);

        if (guiFontOptionsEdit(font_opts))
        {
            result.flags |= AppUpdateFlags_RebuildFonts;
        }

        igEnd();
    }

    if (state->windows.stats)
    {
        F64 framerate = (F64)igGetIO()->Framerate;

        igBegin(STATS_WINDOW, &state->windows.stats, 0);
        igText("Average %.3f ms/frame (%.1f FPS)", 1000.0 / framerate, framerate);
        igText("Allocated %.3fkb in %zu blocks", (F64)plat->heap_size / 1024, plat->heap_blocks);
        igText("Virtual memory reserved %.3fkb - committed %.3fkb", (F64)plat->reserved_size / 1024,
               (F64)plat->committed_size / 1024);
        igSeparator();
        igText("App base path:%s", state->plat->paths->base);
        igText("App data path:%s", state->plat->paths->data);
        // igSeparator();
        // igText("Loaded images: %d", imageLoadCount());
        igEnd();
    }

    if (state->windows.style)
    {
        igBegin(STYLE_WINDOW, &state->windows.style, 0);
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
