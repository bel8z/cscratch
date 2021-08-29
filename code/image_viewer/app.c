//================================//
//    Image viewer application    //
//================================//
//
// TODO (Matteo): missing features
// ! Display transparent images properly
// ! Drag after zoom
// ! Better synchronization between browsing and loading
// - Animated GIF support
// - Cleanup loading code
// - Bounded tool windows ?
// - Better memory allocation strategy based on actual usage patterns
//   (i.e. replace the heap allocator with a custom solution)
// - Compress browsing code (too much duplication)
//
//================================//

#include "api.h"

#include "image.h"

#include "gui/gui.h"

#include "gl/gload.h"

#include "foundation/core.h"

#include "foundation/array.h"
#include "foundation/colors.h"
#include "foundation/fs.h"
#include "foundation/maths.h"
#include "foundation/memory.h"
#include "foundation/paths.h"
#include "foundation/strings.h"
#include "foundation/threading.h"

//-------------------//
//     Constants     //
//-------------------//

static Cstr g_supported_ext[] = {".jpg", ".jpeg", ".bmp", ".png", ".gif"};

#define MAIN_WINDOW "Main"
#define STYLE_WINDOW "Style Editor"
#define FONTS_WINDOW "Font Options"
#define STATS_WINDOW "Application Statistics"

enum Constants
{
    /// Reasonable buffer size to store a file path
    FILENAME_SIZE = 256,
    /// Width of the browsing window (number of images in a folder to keep loaded to reduce browsing
    /// latency). Must be odd because the current image is at the center of the window, and there
    /// are (n-1)/2 loaded images before and after.
    BrowseWidth = 5,
    /// Number of buffered textures to use for image display
    /// 1 texture = no buffering, 2 textures seems reasonable
    NumTextures = 2,
};

CF_STATIC_ASSERT(BrowseWidth & 1, "Browse width must be odd");
CF_STATIC_ASSERT(BrowseWidth > 1, "Browse width must be > 1");

//----------------------//
//     Data structs     //
//----------------------//

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
    ImageTex tex[NumTextures];

    ImVec2 drag;
    F32 zoom;
    I32 filter;

    U32 tex_index;
    U32 tex_count;

    bool dirty;
    bool advanced;
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
    Char8 filename[FILENAME_SIZE];
    Image image;
    I32 state;
} ImageFile;

/// Array of image file info backed by a large VM allocation (no waste since
/// memory is committed only when required)
typedef CfArray(ImageFile) ImageList;

typedef struct LoadQueue
{
    // Dependencies
    CfFileSystem *fs;

    // Sync
    CfThread thread;
    CfMutex mutex;
    CfConditionVariable wake;

    // Data
    ImageFile *buf[16];
    U16 pos;
    U16 len;
    bool stop;
} LoadQueue;

struct AppState
{
    //=== Application memory storage ===//

    void *base_pointer; /// Address of the main VM allocation
    Usize storage_size; /// Size of the main VM allocation
    MemArena *main;     /// Arena for persistent storage (main data structures)
    MemArena *scratch;  /// Arena for temporary storage (one-off allocations)

    //=== Platform services ===//

    Platform *plat;
    GuiFileDialogFilter filter;

    //=== Image brosing ===//

    Usize browse_width;
    Usize curr_file;
    ImageList images;

    //=== Async file loading ===//

    LoadQueue queue;

    //=== GUI ===//

    ImageView iv;
    bool style;
    bool fonts;
    bool stats;
    bool metrics;
    bool unsupported;
};

//------------------------//
//   Async file loading   //
//------------------------//

static void
loadQueuePush(LoadQueue *queue, ImageFile *file)
{
    if (file->state == ImageFileState_Idle)
    {
        cfMutexAcquire(&queue->mutex);
        {
            CF_ASSERT(queue->len < CF_ARRAY_SIZE(queue->buf), "Queue is full!");

            U16 write_pos = (queue->pos + queue->len) % CF_ARRAY_SIZE(queue->buf);
            queue->buf[write_pos] = file;
            queue->len++;

            file->state = ImageFileState_Queued;

            cfCvSignalOne(&queue->wake);
        }
        cfMutexRelease(&queue->mutex);
    }
}

static CF_THREAD_PROC(loadQueueProc)
{
    LoadQueue *queue = args;
    for (;;)
    {
        ImageFile *file = NULL;

        // Wait and dequeue file
        cfMutexAcquire(&queue->mutex);
        {
            while (!queue->stop && queue->len == 0)
            {
                cfCvWaitMutex(&queue->wake, &queue->mutex, DURATION_INFINITE);
            }

            if (queue->stop)
            {
                cfMutexRelease(&queue->mutex);
                break;
            }

            file = queue->buf[queue->pos];
            if (++queue->pos == CF_ARRAY_SIZE(queue->buf)) queue->pos = 0;
            queue->len--;
        }
        cfMutexRelease(&queue->mutex);

        // Process file
        CF_ASSERT_NOT_NULL(file);

        if (file->state == ImageFileState_Queued)
        {
            file->state = ImageFileState_Loading;

            if (imageLoadFromFile(&file->image, file->filename))
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
    queue->thread = cfThreadStart(loadQueueProc, .args = queue, .debug_name = "Load queue");
}

static void
loadQueueStop(LoadQueue *queue)
{
    cfMutexAcquire(&queue->mutex);
    {
        queue->stop = true;
        cfCvSignalOne(&queue->wake);
    }
    cfMutexRelease(&queue->mutex);

    cfThreadWait(queue->thread, DURATION_INFINITE);
    queue->thread.handle = 0;
}

//-----------------------//
//     Image display     //
//-----------------------//

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
    iv->drag = (ImVec2){0};
    iv->filter = ImageFilter_Nearest;
    iv->tex_index = 0;
    iv->tex_count = CF_ARRAY_SIZE(iv->tex);

    for (Usize i = 0; i < CF_ARRAY_SIZE(iv->tex); ++i)
    {
        iv->tex[i] = imageTexCreate(4920, 3264);
    }
}

static void
imageViewShutdown(ImageView *iv)
{
    for (Usize i = 0; i < CF_ARRAY_SIZE(iv->tex); ++i)
    {
        glDeleteTextures(1, &iv->tex[i].id);
        iv->tex[i].id = 0;
    }
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

        // NOTE (Matteo): Switch to next buffered texture
        iv->tex_index = (iv->tex_index + 1) % iv->tex_count;

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

//------------------------------------//
//     Application image handling     //
//------------------------------------//

static void
appClearImages(AppState *app)
{
    for (U32 i = 0; i < app->images.len; ++i)
    {
        ImageFile *file = app->images.buf + i;

        CF_ASSERT(file->state != ImageFileState_Queued, "Leaking queued files");

        if (file->state == ImageFileState_Loaded)
        {
            imageUnload(&file->image);
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

static bool
appIsFileSupported(Str path)
{
    Str ext = pathSplitExt(path);
    if (!strValid(ext)) return false;

    for (Usize i = 0; i < CF_ARRAY_SIZE(g_supported_ext); ++i)
    {
        if (strEqualInsensitive(strFromCstr(g_supported_ext[i]), ext)) return true;
    }

    return false;
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
    app->iv.drag = (ImVec2){0};
}

static void
appLoadFromFile(AppState *state, Str full_name)
{
    CfFileSystem const *fs = state->plat->fs;
    ImageList *images = &state->images;

    appClearImages(state);

    if (strValid(full_name))
    {
        CF_ASSERT(full_name.len < FILENAME_SIZE, "filename is too long!");

        ImageFile file = {0};
        Char8 root_name[FILENAME_SIZE] = {0};
        Str file_name = pathSplitName(full_name);
        memCopy(full_name.buf, root_name, full_name.len - file_name.len);
        strToCstr(full_name, file.filename, FILENAME_SIZE);

#if CF_COMPILER_MSVC
#    pragma warning(push)
#    pragma warning(disable : 4221) // cannot be initialized using address of automatic variable
#endif
        DirIterator it = {0};
        if (fs->dirIterStart(&it, strFromCstr(root_name)))
        {
            Str filename = {0};
            while (fs->dirIterNext(&it, &filename, NULL))
            {
                if (appIsFileSupported(filename))
                {
                    cfArrayPush(images, (ImageFile){.state = ImageFileState_Idle});
                    bool ok = strPrintf(cfArrayLast(images)->filename, FILENAME_SIZE, "%s%.*s",
                                        root_name, (I32)filename.len, filename.buf);
                    CF_ASSERT(ok, "path is too long!");
                }
            }

            fs->dirIterEnd(&it);
        }
#if CF_COMPILER_MSVC
#    pragma warning(pop)
#endif

        for (U32 index = 0; index < images->len; ++index)
        {
            Str temp_name = strFromCstr(state->images.buf[index].filename);
            if (strEqualInsensitive(full_name, temp_name))
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

//------------------------//
//     Application GUI    //
//------------------------//

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
            imageUnload(&file->image);
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
            imageUnload(&file->image);
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
appImageView(AppState *state)
{
    F32 const min_zoom = 1.0f;
    F32 const max_zoom = 10.0f;

    ImageView *iv = &state->iv;
    F32 curr_zoom = iv->zoom;

    // Image scaling settings

    // TODO (Matteo): Maybe this can get cleaner?
    if (iv->advanced)
    {
        I32 filter = iv->filter;
        igRadioButton_IntPtr("Nearest", &filter, ImageFilter_Nearest);
        guiSameLine();
        igRadioButton_IntPtr("Linear", &filter, ImageFilter_Linear);

        bool double_buffer = (iv->tex_count == CF_ARRAY_SIZE(iv->tex));
        guiSameLine();
        if (igCheckbox("Double buffer", &double_buffer))
        {
            iv->dirty = true;
            iv->tex_count = double_buffer ? CF_ARRAY_SIZE(iv->tex) : 1;
        }

        guiSameLine();
        igSliderFloat("zoom", &curr_zoom, min_zoom, max_zoom, "%.3f", 0);

        // NOTE (Matteo): OR dirty flag to keep it set
        iv->dirty |= (filter != iv->filter);
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

    Vec2 view_center = {.x = (view_max.x + view_min.x) / 2, //
                        .y = (view_max.y + view_min.y) / 2};
    Vec2 zoom_location = view_center;

    if (state->curr_file != USIZE_MAX)
    {
        ImageFile *curr_file = state->images.buf + state->curr_file;
        bool can_browse = true;

        switch (curr_file->state)
        {
            case ImageFileState_Loading:
            case ImageFileState_Queued:
                // Do nothing
                can_browse = false;
                break;

            case ImageFileState_Loaded:
                // Update texture and proceed with input
                imageViewUpdate(iv, &curr_file->image);
                break;

            case ImageFileState_Failed:
                // Signal error
                state->unsupported = true;
                break;
        }

        if (can_browse)
        {
            if (guiKeyPressed(ImGuiKey_LeftArrow)) appBrowsePrev(state);
            if (guiKeyPressed(ImGuiKey_RightArrow)) appBrowseNext(state);
        }

        ImGuiIO *io = igGetIO();

        if (igIsItemHovered(0))
        {
            if (io->KeyCtrl)
            {
                curr_zoom = cfClamp(curr_zoom + io->MouseWheel, min_zoom, max_zoom);
                igGetMousePos((ImVec2 *)(&zoom_location));
            }

            if (igIsMouseDragging(ImGuiMouseButton_Left, -1.0f))
            {
                igGetMouseDragDelta(&iv->drag, ImGuiMouseButton_Left, -1.0f);
            }
        }

        // NOTE (Matteo): Draw the image properly scaled to fit the view
        // TODO (Matteo): Fix zoom behavior

        F32 image_w = (F32)curr_file->image.width;
        F32 image_h = (F32)curr_file->image.height;
        ImageTex *tex = imageViewCurrTex(iv);

        // NOTE (Matteo): Clamp the displayed portion of the texture to the actual image size, since
        // the texture could be larger in order to be reused
        ImVec2 clamp_uv = {image_w / (F32)tex->width, //
                           image_h / (F32)tex->height};

        // NOTE (Matteo): the image is resized in order to adapt to the viewport, keeping the aspect
        // ratio at zoom level == 1; then zoom is applied
        if (image_w > view_size.x)
        {
            image_h *= view_size.x / image_w;
            image_w = view_size.x;
        }

        if (image_h > view_size.y)
        {
            image_w *= view_size.y / image_h;
            image_h = view_size.y;
        }

        // NOTE (Matteo): Round image bounds to nearest pixel for stable rendering
        image_w = cfRound(image_w * curr_zoom);
        image_h = cfRound(image_h * curr_zoom);

        // NOTE (Matteo): Handle zoom target
        if (curr_zoom != iv->zoom)
        {
            Vec2 curr_pos = vecSub(zoom_location, view_center);
            Vec2 next_pos = vecMul(curr_pos, curr_zoom);
            Vec2 delta = vecSub(curr_pos, next_pos);

            iv->drag.x = delta.x;
            iv->drag.y = delta.y;

            iv->zoom = curr_zoom;
        }

        ImVec2 image_min = {iv->drag.x + cfRound(view_min.x + 0.5f * (view_size.x - image_w)),
                            iv->drag.y + cfRound(view_min.y + 0.5f * (view_size.y - image_h))};

        ImVec2 image_max = {image_min.x + image_w, image_min.y + image_h};

        ImDrawList *draw_list = igGetWindowDrawList();
        ImDrawList_PushClipRect(draw_list, view_min, view_max, true);
        ImDrawList_AddImage(draw_list, (ImTextureID)(Iptr)tex->id, image_min, image_max,
                            (ImVec2){0.0f, 0.0f}, clamp_uv, igGetColorU32_U32(RGBA32_WHITE));

        if (iv->advanced)
        {
            // DEBUG (Matteo): Draw view and image bounds - remove when zoom is fixed
            ImU32 debug_color = igGetColorU32_U32(RGBA32_FUCHSIA);
            ImDrawList_AddRect(draw_list, image_min, image_max, debug_color, 0.0f, 0, 1.0f);
            ImDrawList_AddRect(draw_list, view_min, view_max, debug_color, 0.0f, 0, 1.0f);
        }
    }
}

static bool
appOpenFile(AppState *state)
{
    bool result = true;

    MEM_ARENA_TEMP_SCOPE(state->scratch)
    {
        GuiFileDialogParms dlg_parms = {
            .type = GuiFileDialog_Open,
            .filters = &state->filter,
            .num_filters = 1,
        };

        if (state->curr_file != USIZE_MAX)
        {
            dlg_parms.filename_hint = strFromCstr(state->images.buf[state->curr_file].filename);
        }

        GuiFileDialogResult dlg_result =
            guiFileDialog(&dlg_parms, memArenaAllocator(state->scratch));

        switch (dlg_result.code)
        {
            case GuiFileDialogResult_Ok: appLoadFromFile(state, dlg_result.filename); break;
            case GuiFileDialogResult_Error: result = false; break;
        }
    }

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
            if (igMenuItem_Bool("Open", NULL, false, true))
            {
                open_file_error = !appOpenFile(state);
            }
            igSeparator();
            quit = igMenuItem_Bool("Quit", NULL, false, true);
            igEndMenu();
        }

        if (igBeginMenu("View", true))
        {
            igMenuItem_BoolPtr("Advanced", NULL, &state->iv.advanced, true);
            igSeparator();
            igMenuItem_BoolPtr(STYLE_WINDOW, NULL, &state->style, true);
            igMenuItem_BoolPtr(FONTS_WINDOW, NULL, &state->fonts, true);
            igSeparator();
            igMenuItem_BoolPtr("Stats", NULL, &state->stats, true);
            igMenuItem_BoolPtr("Metrics", NULL, &state->metrics, true);
            igEndMenu();
        }

        igEndMainMenuBar();
    }

    if (open_file_error) igOpenPopup_Str("Open file error", 0);

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

//---------------------------------------//
//     Application API implementation    //
//---------------------------------------//

APP_API AppState *
appCreate(Platform *plat, Cstr argv[], I32 argc)
{
    // NOTE (Matteo): Memory comes cleared to 0
    Usize const storage_size = CF_GB(1);
    void *storage = vmReserve(plat->vm, storage_size);

    MemArena *main = memArenaBootstrapFromVm(plat->vm, storage, storage_size);
    AppState *app = memArenaAllocStruct(main, AppState);

    app->base_pointer = storage;
    app->storage_size = storage_size;

    app->plat = plat;
    app->main = main;

    // NOTE (Matteo): Split scratch storage from main allocation
    app->scratch = memArenaAllocStruct(main, MemArena);
    memArenaSplit(main, app->scratch, memArenaRemaining(main) / 2);

    // Init file list management
    app->filter.name = "Image files";
    app->filter.extensions = g_supported_ext;
    app->filter.num_extensions = CF_ARRAY_SIZE(g_supported_ext);
    app->curr_file = USIZE_MAX;

    // Usize const images_vm = CF_GB(1);
    cfArrayInit(&app->images, memArenaAllocator(main));

    app->queue.fs = app->plat->fs;
    cfCvInit(&app->queue.wake);
    cfMutexInit(&app->queue.mutex);

    // NOTE (Matteo): Init global state (same as library re-load)
    appLoad(app);

    imageViewInit(&app->iv);

    if (argc > 1) appLoadFromFile(app, strFromCstr(argv[1]));

    return app;
}

APP_API void
appDestroy(AppState *app)
{
    appUnload(app);
    appClearImages(app);
    imageViewShutdown(&app->iv);
    cfArrayFree(&app->images);
    memArenaClear(app->scratch);
    memArenaClear(app->main);

    vmRelease(app->plat->vm, app->base_pointer, app->storage_size);
}

APP_API APP_PROC(appLoad)
{
    CF_ASSERT_NOT_NULL(app);
    CF_ASSERT_NOT_NULL(app->plat);
    // Init Dear Imgui
    guiInit(app->plat->gui);
    // Init image loading
    gloadInit(app->plat->gl);
    imageInit(app->plat->heap);
    loadQueueStart(&app->queue);
}

APP_API APP_PROC(appUnload)
{
    loadQueueStop(&app->queue);
}

APP_API APP_UPDATE_PROC(appUpdate)
{
    Platform *plat = state->plat;

    io->back_color = igGetColorU32_Col(ImGuiCol_WindowBg, 1.0f);
    io->continuous_update = false;

    //==== Main UI ====//

    if (appMenuBar(state)) io->quit = true;

    appMainWindow(state);

    //==== Tool windows ====//

    if (state->fonts)
    {
        igBegin(FONTS_WINDOW, &state->fonts, 0);
        io->rebuild_fonts = guiFontOptionsEdit(io->font_opts);
        igEnd();
    }

    if (state->stats)
    {
        F64 framerate = (F64)igGetIO()->Framerate;

        igBegin(STATS_WINDOW, &state->stats, 0);
        igText("Average %.3f ms/frame (%.1f FPS)", 1000.0 / framerate, framerate);
        igText("Allocated %.3fkb in %zu blocks", (F64)plat->heap_size / 1024, plat->heap_blocks);
        igText("Virtual memory reserved %.3fkb - committed %.3fkb", (F64)plat->reserved_size / 1024,
               (F64)plat->committed_size / 1024);
        igSeparator();
        igText("App base path:%.*s", state->plat->paths->base.len, state->plat->paths->base.buf);
        igText("App data path:%.*s", state->plat->paths->data.len, state->plat->paths->data.buf);
        igEnd();
    }

    if (state->style)
    {
        igBegin(STYLE_WINDOW, &state->style, 0);
        igShowStyleEditor(NULL);
        igEnd();
    }

    if (state->metrics)
    {
        igShowMetricsWindow(&state->metrics);
    }

    if (state->unsupported)
    {
        state->unsupported = false;
        igOpenPopup_Str("Warning", 0);
    }

    //==== Popups ====//

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
            state->unsupported = false;
            igCloseCurrentPopup();
        }
        igEndPopup();
    }
}
