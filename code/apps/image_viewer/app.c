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

#include "app.h"
#include "platform.h"
#include "version.h"

#define IMAGE_INTERNAL
#define IMAGE_IMPL
#include "image.h"

#include "gui/gui.h"

#include "gl/gload.h"

#include "foundation/core.h"

#include "foundation/colors.h"
#include "foundation/error.h"
#include "foundation/io.h"
#include "foundation/memory.h"
#include "foundation/paths.h"
#include "foundation/strings.h"
#include "foundation/task.h"

#include "foundation/math.inl"
#include "foundation/mem_buffer.inl"

//-------------------//
//     Constants     //
//-------------------//

static Cstr g_supported_ext[] = {".jpg", ".jpeg", ".bmp", ".png", ".gif"};
static IoFileApi *g_file = NULL;

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

typedef struct ImageTex
{
    U32 id;
    I32 width;
    I32 height;
} ImageTex;

typedef struct ImageView
{
    ImageTex tex[NumTextures];

    Vec2 drag;
    F32 zoom;

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

    /// Array of image file info backed by a large VM allocation (no waste since
    /// memory is committed only when required)
    MemBuffer(ImageFile) files;
    Usize curr_file;
    Usize browse_width;

    //=== Async file loading ===//

    TaskQueue *queue;

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

TASK_QUEUE_FN(loadFileTask)
{
    CF_ASSERT_NOT_NULL(data);
    CF_ASSERT_NOT_NULL(g_file);

    ImageFile *file = data;

    if (file->state == ImageFileState_Queued)
    {
        file->state = ImageFileState_Loading;

        if (!(*canceled) && imageLoadFromFile(&file->image, strFromCstr(file->filename), g_file))
        {
            file->state = ImageFileState_Loaded;
        }
        else
        {
            file->state = ImageFileState_Failed;
        }
    }
}

static void
loadFileEnqueue(TaskQueue *queue, ImageFile *file)
{
    if (file->state == ImageFileState_Idle)
    {
        file->state = ImageFileState_Queued;
        if (!taskEnqueue(queue, loadFileTask, file))
        {
            CF_INVALID_CODE_PATH();
        }
    }
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
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
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
    iv->drag = (Vec2){0};
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

        glBindTexture(GL_TEXTURE_2D, tex->id);
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
    for (U32 i = 0; i < app->files.size; ++i)
    {
        ImageFile *file = app->files.data + i;

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

    memBufferClear(&app->files);
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

    Usize curr = app->curr_file;
    TaskQueue *queue = app->queue;

    loadFileEnqueue(queue, app->files.data + curr);

    if (app->browse_width == app->files.size)
    {
        for (Usize i = curr + 1; i < app->files.size; ++i)
        {
            loadFileEnqueue(queue, app->files.data + i);
        }

        for (Usize i = 0; i < curr; ++i)
        {
            loadFileEnqueue(queue, app->files.data + curr - i - 1);
        }
    }
    else
    {
        CF_ASSERT(app->browse_width > 1, "Window width must be > 1");
        CF_ASSERT(app->browse_width & 1, "Window width must be odd");

        Usize mid = app->browse_width / 2;
        for (Usize i = 0; i < mid; ++i)
        {
            Usize next = cfMod(curr + i + 1, app->files.size);
            Usize prev = cfMod(curr - i - 1, app->files.size);
            CF_ASSERT(next != curr, "");
            CF_ASSERT(next != prev, "");
            CF_ASSERT(prev != curr, "");
            loadFileEnqueue(queue, app->files.data + next);
            loadFileEnqueue(queue, app->files.data + prev);
        }
    }

    // NOTE (Matteo): Set view as dirty
    app->iv.dirty = true;
    app->iv.zoom = 1.0f;
    app->iv.drag = (Vec2){0};
}

static void
appPushFile(AppState *app, Cstr root_name, Str filename)
{
    ImageFile *file = NULL;
    ErrorCode32 err = memBufferExtendAlloc(&app->files, 1, memArenaAllocator(app->main), &file);
    CF_ASSERT(!err, "Push file should not fail");

    file->state = ImageFileState_Idle;

    err = strPrint(file->filename, FILENAME_SIZE, "%s%.*s", root_name, (I32)filename.len,
                   filename.buf);
    CF_ASSERT(!err, "Path is too long!");
}

static void
appLoadFromFile(AppState *state, Str full_name)
{
    appClearImages(state);

    if (strValid(full_name))
    {
        CF_ASSERT(full_name.len < FILENAME_SIZE, "filename is too long!");

        ImageFile file = {0};
        Char8 root_name[FILENAME_SIZE] = {0};
        Str file_name = pathSplitName(full_name);
        memCopy(full_name.buf, root_name, full_name.len - file_name.len);
        strToCstr(full_name, file.filename, FILENAME_SIZE);

        CF_DIAGNOSTIC_PUSH()
        CF_DIAGNOSTIC_IGNORE_MSVC(4221)

        IoFileApi *io = state->plat->file;
        IoDirectory it = {0};
        if (io->dirOpen(&it, strFromCstr(root_name)))
        {
            Str filename = {0};
            while (it.next(&it, &filename, NULL))
            {
                if (appIsFileSupported(filename))
                {
                    appPushFile(state, root_name, filename);
                }
            }

            it.close(&it);
        }

        CF_DIAGNOSTIC_POP()

        for (U32 file_no = 0; file_no < state->files.size; ++file_no)
        {
            Str temp_name = strFromCstr(state->files.data[file_no].filename);
            if (strEqualInsensitive(full_name, temp_name))
            {
                state->curr_file = file_no;
                state->files.data[file_no] = file;
                break;
            }
        }

        state->browse_width = cfMin(BrowseWidth, state->files.size);
        CF_ASSERT(state->curr_file != USIZE_MAX, "At least one image file should be present");
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

    Usize next = cfWrapInc(app->curr_file, app->files.size);

    if (app->browse_width != app->files.size)
    {
        Usize lru = cfMod(app->curr_file - app->browse_width / 2, app->files.size);
        ImageFile *file = app->files.data + lru;
        if (file->state == ImageFileState_Loaded)
        {
            imageUnload(&file->image);
            file->state = ImageFileState_Idle;
        }
    }
    else
    {
        CF_ASSERT(app->files.data[next].state != ImageFileState_Idle, "");
    }

    app->curr_file = next;
    appQueueLoadFiles(app);
}

static void
appBrowsePrev(AppState *app)
{
    CF_ASSERT(app->curr_file != USIZE_MAX, "Invalid browse command");

    Usize prev = cfWrapDec(app->curr_file, app->files.size);

    if (app->browse_width != app->files.size)
    {
        Usize lru = cfMod(app->curr_file + app->browse_width / 2, app->files.size);
        ImageFile *file = app->files.data + lru;
        if (file->state == ImageFileState_Loaded)
        {
            imageUnload(&file->image);
            file->state = ImageFileState_Idle;
        }
    }
    else
    {
        CF_ASSERT(app->files.data[prev].state != ImageFileState_Idle, "");
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
        bool double_buffer = (iv->tex_count == CF_ARRAY_SIZE(iv->tex));
        if (guiCheckbox("Double buffer", &double_buffer))
        {
            iv->dirty = true;
            iv->tex_count = double_buffer ? CF_ARRAY_SIZE(iv->tex) : 1;
        }
        guiSameLine();
        guiSlider("zoom", &curr_zoom, min_zoom, max_zoom);
    }

    // NOTE (Matteo): Use the available content area as the image view; an invisible button
    // is used in order to catch input.

    guiSetNextWindowSize((Vec2){.x = 50, .y = 50}, GuiCond_Once);

    GuiCanvas canvas = {0};
    guiCanvasBegin(&canvas);

    Vec2 view_min = canvas.p0;
    Vec2 view_max = canvas.p1;

    Vec2 view_center = {.x = (view_max.x + view_min.x) / 2, //
                        .y = (view_max.y + view_min.y) / 2};
    Vec2 zoom_location = view_center;

    if (state->curr_file != USIZE_MAX)
    {
        ImageFile *curr_file = state->files.data + state->curr_file;
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
                curr_file->state = ImageFileState_Idle;
                break;
        }

        if (can_browse)
        {
            if (guiKeyPressed(GuiKey_LeftArrow)) appBrowsePrev(state);
            if (guiKeyPressed(GuiKey_RightArrow)) appBrowseNext(state);
        }

        if (guiIsItemHovered())
        {
            if (guiKeyCtrl())
            {
                curr_zoom = cfClamp(curr_zoom + guiGetMouseWheel(), min_zoom, max_zoom);
                zoom_location = guiGetMousePos();
            }

            guiGetMouseDragging(GuiMouseButton_Left, &iv->drag);
        }

        // NOTE (Matteo): Draw the image properly scaled to fit the view
        // TODO (Matteo): Fix zoom behavior

        F32 image_w = (F32)curr_file->image.width;
        F32 image_h = (F32)curr_file->image.height;
        ImageTex *tex = imageViewCurrTex(iv);

        // NOTE (Matteo): Clamp the displayed portion of the texture to the actual image size,
        // since the texture could be larger in order to be reused
        Vec2 clamp_uv = {.x = image_w / (F32)tex->width, //
                         .y = image_h / (F32)tex->height};

        // NOTE (Matteo): the image is resized in order to adapt to the viewport, keeping the
        // aspect ratio at zoom level == 1; then zoom is applied
        if (image_w > canvas.size.width)
        {
            image_h *= canvas.size.width / image_w;
            image_w = canvas.size.width;
        }

        if (image_h > canvas.size.height)
        {
            image_w *= canvas.size.height / image_h;
            image_h = canvas.size.height;
        }

        // NOTE (Matteo): Round image bounds to nearest pixel for stable rendering
        image_w = mRound(image_w * curr_zoom);
        image_h = mRound(image_h * curr_zoom);

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

        Vec2 image_min = {
            .x = iv->drag.x + mRound(view_min.x + 0.5f * (canvas.size.width - image_w)),
            .y = iv->drag.y + mRound(view_min.y + 0.5f * (canvas.size.height - image_h))};

        Vec2 image_max = {.x = image_min.x + image_w, .y = image_min.y + image_h};

        guiCanvasDrawImage(&canvas, tex->id, image_min, image_max, (Vec2){0}, clamp_uv);

        if (iv->advanced)
        {
            // DEBUG (Matteo): Draw view and image bounds - remove when zoom is fixed
            canvas.stroke_color = guiGetStyledColor(SRGB32_FUCHSIA);
            guiCanvasDrawRect(&canvas, image_min, image_max);
            guiCanvasDrawRect(&canvas, view_min, view_max);
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
            dlg_parms.filename_hint = strFromCstr(state->files.data[state->curr_file].filename);
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

    if (guiBeginMainMenuBar())
    {
        if (guiBeginMenu("File", true))
        {
            if (guiMenuItem("Open", NULL))
            {
                open_file_error = !appOpenFile(state);
            }
            guiSeparator();
            quit = guiMenuItem("Quit", NULL);
            guiEndMenu();
        }

        if (guiBeginMenu("View", true))
        {
            guiMenuItem("Advanced", &state->iv.advanced);
            guiSeparator();
            guiMenuItem(STYLE_WINDOW, &state->style);
            guiMenuItem(FONTS_WINDOW, &state->fonts);
            guiSeparator();
            guiMenuItem("Stats", &state->stats);
            guiMenuItem("Metrics", &state->metrics);
            guiEndMenu();
        }

        guiEndMainMenuBar();
    }

    if (open_file_error) guiOpenPopup("Open file error");

    return quit;
}

static void
appMainWindow(AppState *state)
{

    GuiDockLayout layout = guiDockLayout();
    U32 dock_id_right = guiDockSplitRight(&layout, 0.25f);
    U32 dock_id_down = guiDockSplitDown(&layout, 0.25f);

    // Pre-dock application windows in the created nodes
    guiDockWindow(&layout, STATS_WINDOW, dock_id_down);
    guiDockWindow(&layout, STYLE_WINDOW, dock_id_right);
    guiDockWindow(&layout, FONTS_WINDOW, dock_id_right);

    guiBeginLayout(MAIN_WINDOW, &layout);

    appImageView(state);

    guiEnd();
}

//---------------------------------------//
//     Application API implementation    //
//---------------------------------------//

APP_API APP_CREATE_FN(appCreate)
{
    // NOTE (Matteo): Memory comes cleared to 0
    Usize const storage_size = CF_GB(1);
    void *storage = vmemReserve(plat->vmem, storage_size);

    MemArena *main = memArenaBootstrapFromVmem(plat->vmem, storage, storage_size);
    AppState *app = memArenaAllocStruct(main, AppState);

    app->base_pointer = storage;
    app->storage_size = storage_size;

    app->plat = plat;
    app->main = main;

    // NOTE (Matteo): Split scratch storage from main allocation
    app->scratch = memArenaAllocStruct(main, MemArena);
    memArenaSplit(main, app->scratch, memArenaAvailable(main) / 2);

    // Init file list management
    app->filter.name = "Image files";
    app->filter.extensions = g_supported_ext;
    app->filter.num_extensions = CF_ARRAY_SIZE(g_supported_ext);
    app->curr_file = USIZE_MAX;

    TaskQueueConfig cfg = {.buffer_size = 128, .num_workers = 1};
    if (taskConfig(&cfg))
    {
        app->queue = taskInit(&cfg, memArenaAlloc(main, cfg.footprint));
    }
    else
    {
        CF_INVALID_CODE_PATH();
    }

    // NOTE (Matteo): Init global state (same as library re-load)
    appLoad(app);

    imageViewInit(&app->iv);

    if (cmd_line->len > 1) appLoadFromFile(app, strFromCstr(cmd_line->arg[1]));

    return app;
}

APP_API APP_FN(appDestroy)
{
    appUnload(app);
    taskShutdown(app->queue);
    appClearImages(app);
    imageViewShutdown(&app->iv);
    memBufferFree(&app->files, memArenaAllocator(app->main));
    memArenaClear(app->scratch);
    memArenaClear(app->main);

    vmemRelease(app->plat->vmem, app->base_pointer, app->storage_size);
}

APP_API APP_FN(appLoad)
{
    CF_ASSERT_NOT_NULL(app);
    CF_ASSERT_NOT_NULL(app->plat);
    // Init Dear Imgui
    guiSetContext(app->plat->gui);
    // Init image loading
    gloadInit(app->plat->gl);
    imageInit(app->plat->heap);

    g_file = app->plat->file;

    taskStartProcessing(app->queue);
}

APP_API APP_FN(appUnload)
{
    taskStopProcessing(app->queue, true);
}

APP_API APP_UPDATE_FN(appUpdate)
{
    Platform *plat = state->plat;

    io->back_color = guiGetBackColor();
    io->continuous_update = false;

    if (!io->window_title)
    {
        io->window_title = VER_PRODUCTNAME_STR;
        io->window_title_changed = true;
    }
    else if (io->window_title_changed)
    {
        io->window_title_changed = false;
    }

    //==== Main UI ====//

    if (appMenuBar(state)) io->quit = true;

    appMainWindow(state);

    //==== Tool windows ====//

    if (state->style)
    {
        guiBegin(STYLE_WINDOW, &state->style);
        guiThemeSelector("Theme");
        // TODO (Matteo): Should this be really available?
        guiSeparator();
        guiStyleEditor();
        guiEnd();
    }

    if (state->fonts)
    {
        guiBegin(FONTS_WINDOW, &state->fonts);
        io->rebuild_fonts = guiFontOptionsEdit(io->font_opts);
        guiEnd();
    }

    if (state->stats)
    {
        F64 framerate = (F64)guiGetFramerate();

        guiBegin(STATS_WINDOW, &state->stats);
        guiText("Average %.3f ms/frame (%.1f FPS)", 1000.0 / framerate, framerate);
        guiText("Allocated %.3fkb in %zu blocks", (F64)plat->heap_size / 1024, plat->heap_blocks);
        guiText("Virtual memory reserved %.3fkb - committed %.3fkb",
                (F64)plat->reserved_size / 1024, (F64)plat->committed_size / 1024);
        guiSeparator();
        guiText("App base path:%.*s", (I32)state->plat->paths->base.len,
                state->plat->paths->base.buf);
        guiText("App data path:%.*s", (I32)state->plat->paths->data.len,
                state->plat->paths->data.buf);
        guiEnd();
    }

    if (state->metrics)
    {
        guiMetricsWindow(&state->metrics);
    }

    if (state->unsupported)
    {
        state->unsupported = false;
        guiOpenPopup("Warning");
    }

    //==== Popups ====//

    if (guiBeginPopupModal("Open file error", NULL))
    {
        guiText("Error opening file");
        if (guiCenteredButton("Ok"))
        {
            guiClosePopup();
        }
        guiEndPopup();
    }

    if (guiBeginPopupModal("Warning", NULL))
    {
        guiText("Unsupported file format");
        if (guiCenteredButton("Ok"))
        {
            state->unsupported = false;
            guiClosePopup();
        }
        guiEndPopup();
    }
}
