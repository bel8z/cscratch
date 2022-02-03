// Platform Backend for GLFW derived from imgui_impl_glfw

// Issues:
//  [ ] Platform: Multi-viewport support: ParentViewportID not honored, and so
//  io.ConfigViewportsNoDefaultParent has no effect (minor).

#include "gui.h"

#include "imgui.h"

#include "foundation/memory.h"

// Clang warnings with -Weverything
CF_DIAGNOSTIC_PUSH()
CF_DIAGNOSTIC_IGNORE_CLANG("-Wold-style-cast")
CF_DIAGNOSTIC_IGNORE_CLANG("-Wzero-as-null-pointer-constant")

// GLFW
#include <GLFW/glfw3.h>
#if CF_OS_WIN32
#    undef APIENTRY
#    define GLFW_EXPOSE_NATIVE_WIN32
#    include <GLFW/glfw3native.h> // for glfwGetWin32Window
#endif

CF_STATIC_ASSERT(GLFW_VERSION_MAJOR >= 3 && GLFW_VERSION_MINOR >= 3 && GLFW_VERSION_REVISION >= 6,
                 "Minimum supported GLFW version is 3.3.6");

// GLFW data

struct GuiGlfwData
{
    GLFWwindow *main_window;
    GLFWwindow *mouse_window;
    GLFWcursor *mouse_cursors[ImGuiMouseCursor_COUNT];
    GLFWwindow *key_owners[GLFW_KEY_LAST];
    double time;
    bool gl_context;
    bool update_monitors;

    GLFWwindowfocusfun focus_callback;
    GLFWcursorenterfun enter_callback;
    GLFWcursorposfun pos_callback;
    GLFWmousebuttonfun mouse_callback;
    GLFWscrollfun scroll_callback;
    GLFWkeyfun key_callback;
    GLFWcharfun char_callback;
    GLFWmonitorfun monitor_callback;

    GuiGlfwData() { memClearStruct(this); }
};

// Backend data stored in io.BackendPlatformUserData to allow support for multiple Dear ImGui
// contexts.
// It is STRONGLY preferred that you use docking branch with multi-viewports (== single
// Dear ImGui context + multiple windows) instead of multiple Dear ImGui contexts.
// FIXME: multi-context support is not well tested and probably dysfunctional in this backend.
// - Because glfwPollEvents() process all windows and some events may be called outside of it, you
// will need to register your own callbacks
//   (passing install_callbacks=false in guiGlfw_InitXXX functions), set the current dear
//   imgui context and then call our callbacks.
// - Otherwise we may need to store a GLFWWindow* -> ImGuiContext* map and handle this in the
// backend, adding a little bit of extra complexity to it.
// FIXME: some shared resources (mouse cursor shape, gamepad) are mishandled when using
// multi-context.
static GuiGlfwData *
guiGlfw_GetBackendData()
{
    return ImGui::GetCurrentContext() ? (GuiGlfwData *)ImGui::GetIO().BackendPlatformUserData
                                      : NULL;
}

// Forward Declarations
static void guiGlfw_UpdateMonitors();
static void guiGlfw_InitPlatformInterface();

// Functions
static inline const char *
guiGlfw_GetClipboardText(void *user_data)
{
    return glfwGetClipboardString((GLFWwindow *)user_data);
}

static inline void
guiGlfw_SetClipboardText(void *user_data, const char *text)
{
    glfwSetClipboardString((GLFWwindow *)user_data, text);
}

static ImGuiKey
guiGlfw_KeyToImGuiKey(int key)
{
    switch (key)
    {
        case GLFW_KEY_TAB: return ImGuiKey_Tab;
        case GLFW_KEY_LEFT: return ImGuiKey_LeftArrow;
        case GLFW_KEY_RIGHT: return ImGuiKey_RightArrow;
        case GLFW_KEY_UP: return ImGuiKey_UpArrow;
        case GLFW_KEY_DOWN: return ImGuiKey_DownArrow;
        case GLFW_KEY_PAGE_UP: return ImGuiKey_PageUp;
        case GLFW_KEY_PAGE_DOWN: return ImGuiKey_PageDown;
        case GLFW_KEY_HOME: return ImGuiKey_Home;
        case GLFW_KEY_END: return ImGuiKey_End;
        case GLFW_KEY_INSERT: return ImGuiKey_Insert;
        case GLFW_KEY_DELETE: return ImGuiKey_Delete;
        case GLFW_KEY_BACKSPACE: return ImGuiKey_Backspace;
        case GLFW_KEY_SPACE: return ImGuiKey_Space;
        case GLFW_KEY_ENTER: return ImGuiKey_Enter;
        case GLFW_KEY_ESCAPE: return ImGuiKey_Escape;
        case GLFW_KEY_APOSTROPHE: return ImGuiKey_Apostrophe;
        case GLFW_KEY_COMMA: return ImGuiKey_Comma;
        case GLFW_KEY_MINUS: return ImGuiKey_Minus;
        case GLFW_KEY_PERIOD: return ImGuiKey_Period;
        case GLFW_KEY_SLASH: return ImGuiKey_Slash;
        case GLFW_KEY_SEMICOLON: return ImGuiKey_Semicolon;
        case GLFW_KEY_EQUAL: return ImGuiKey_Equal;
        case GLFW_KEY_LEFT_BRACKET: return ImGuiKey_LeftBracket;
        case GLFW_KEY_BACKSLASH: return ImGuiKey_Backslash;
        case GLFW_KEY_RIGHT_BRACKET: return ImGuiKey_RightBracket;
        case GLFW_KEY_GRAVE_ACCENT: return ImGuiKey_GraveAccent;
        case GLFW_KEY_CAPS_LOCK: return ImGuiKey_CapsLock;
        case GLFW_KEY_SCROLL_LOCK: return ImGuiKey_ScrollLock;
        case GLFW_KEY_NUM_LOCK: return ImGuiKey_NumLock;
        case GLFW_KEY_PRINT_SCREEN: return ImGuiKey_PrintScreen;
        case GLFW_KEY_PAUSE: return ImGuiKey_Pause;
        case GLFW_KEY_KP_0: return ImGuiKey_Keypad0;
        case GLFW_KEY_KP_1: return ImGuiKey_Keypad1;
        case GLFW_KEY_KP_2: return ImGuiKey_Keypad2;
        case GLFW_KEY_KP_3: return ImGuiKey_Keypad3;
        case GLFW_KEY_KP_4: return ImGuiKey_Keypad4;
        case GLFW_KEY_KP_5: return ImGuiKey_Keypad5;
        case GLFW_KEY_KP_6: return ImGuiKey_Keypad6;
        case GLFW_KEY_KP_7: return ImGuiKey_Keypad7;
        case GLFW_KEY_KP_8: return ImGuiKey_Keypad8;
        case GLFW_KEY_KP_9: return ImGuiKey_Keypad9;
        case GLFW_KEY_KP_DECIMAL: return ImGuiKey_KeypadDecimal;
        case GLFW_KEY_KP_DIVIDE: return ImGuiKey_KeypadDivide;
        case GLFW_KEY_KP_MULTIPLY: return ImGuiKey_KeypadMultiply;
        case GLFW_KEY_KP_SUBTRACT: return ImGuiKey_KeypadSubtract;
        case GLFW_KEY_KP_ADD: return ImGuiKey_KeypadAdd;
        case GLFW_KEY_KP_ENTER: return ImGuiKey_KeypadEnter;
        case GLFW_KEY_KP_EQUAL: return ImGuiKey_KeypadEqual;
        case GLFW_KEY_LEFT_SHIFT: return ImGuiKey_LeftShift;
        case GLFW_KEY_LEFT_CONTROL: return ImGuiKey_LeftCtrl;
        case GLFW_KEY_LEFT_ALT: return ImGuiKey_LeftAlt;
        case GLFW_KEY_LEFT_SUPER: return ImGuiKey_LeftSuper;
        case GLFW_KEY_RIGHT_SHIFT: return ImGuiKey_RightShift;
        case GLFW_KEY_RIGHT_CONTROL: return ImGuiKey_RightCtrl;
        case GLFW_KEY_RIGHT_ALT: return ImGuiKey_RightAlt;
        case GLFW_KEY_RIGHT_SUPER: return ImGuiKey_RightSuper;
        case GLFW_KEY_MENU: return ImGuiKey_Menu;
        case GLFW_KEY_0: return ImGuiKey_0;
        case GLFW_KEY_1: return ImGuiKey_1;
        case GLFW_KEY_2: return ImGuiKey_2;
        case GLFW_KEY_3: return ImGuiKey_3;
        case GLFW_KEY_4: return ImGuiKey_4;
        case GLFW_KEY_5: return ImGuiKey_5;
        case GLFW_KEY_6: return ImGuiKey_6;
        case GLFW_KEY_7: return ImGuiKey_7;
        case GLFW_KEY_8: return ImGuiKey_8;
        case GLFW_KEY_9: return ImGuiKey_9;
        case GLFW_KEY_A: return ImGuiKey_A;
        case GLFW_KEY_B: return ImGuiKey_B;
        case GLFW_KEY_C: return ImGuiKey_C;
        case GLFW_KEY_D: return ImGuiKey_D;
        case GLFW_KEY_E: return ImGuiKey_E;
        case GLFW_KEY_F: return ImGuiKey_F;
        case GLFW_KEY_G: return ImGuiKey_G;
        case GLFW_KEY_H: return ImGuiKey_H;
        case GLFW_KEY_I: return ImGuiKey_I;
        case GLFW_KEY_J: return ImGuiKey_J;
        case GLFW_KEY_K: return ImGuiKey_K;
        case GLFW_KEY_L: return ImGuiKey_L;
        case GLFW_KEY_M: return ImGuiKey_M;
        case GLFW_KEY_N: return ImGuiKey_N;
        case GLFW_KEY_O: return ImGuiKey_O;
        case GLFW_KEY_P: return ImGuiKey_P;
        case GLFW_KEY_Q: return ImGuiKey_Q;
        case GLFW_KEY_R: return ImGuiKey_R;
        case GLFW_KEY_S: return ImGuiKey_S;
        case GLFW_KEY_T: return ImGuiKey_T;
        case GLFW_KEY_U: return ImGuiKey_U;
        case GLFW_KEY_V: return ImGuiKey_V;
        case GLFW_KEY_W: return ImGuiKey_W;
        case GLFW_KEY_X: return ImGuiKey_X;
        case GLFW_KEY_Y: return ImGuiKey_Y;
        case GLFW_KEY_Z: return ImGuiKey_Z;
        case GLFW_KEY_F1: return ImGuiKey_F1;
        case GLFW_KEY_F2: return ImGuiKey_F2;
        case GLFW_KEY_F3: return ImGuiKey_F3;
        case GLFW_KEY_F4: return ImGuiKey_F4;
        case GLFW_KEY_F5: return ImGuiKey_F5;
        case GLFW_KEY_F6: return ImGuiKey_F6;
        case GLFW_KEY_F7: return ImGuiKey_F7;
        case GLFW_KEY_F8: return ImGuiKey_F8;
        case GLFW_KEY_F9: return ImGuiKey_F9;
        case GLFW_KEY_F10: return ImGuiKey_F10;
        case GLFW_KEY_F11: return ImGuiKey_F11;
        case GLFW_KEY_F12: return ImGuiKey_F12;
        default: return ImGuiKey_None;
    }
}

static void
guiGlfw_UpdateKeyModifiers(int mods)
{
    ImGuiIO &io = ImGui::GetIO();
    io.AddKeyEvent(ImGuiKey_ModCtrl, (mods & GLFW_MOD_CONTROL));
    io.AddKeyEvent(ImGuiKey_ModShift, (mods & GLFW_MOD_SHIFT));
    io.AddKeyEvent(ImGuiKey_ModAlt, (mods & GLFW_MOD_ALT));
    io.AddKeyEvent(ImGuiKey_ModSuper, (mods & GLFW_MOD_SUPER));
}

void
guiGlfw_MouseButtonCallback(GLFWwindow *window, int button, int action, int mods)
{
    GuiGlfwData *bd = guiGlfw_GetBackendData();
    if (bd->mouse_callback && bd->main_window == window)
    {
        bd->mouse_callback(window, button, action, mods);
    }

    guiGlfw_UpdateKeyModifiers(mods);

    if (button >= 0 && button < ImGuiMouseButton_COUNT)
    {
        ImGuiIO &io = ImGui::GetIO();
        io.AddMouseButtonEvent(button, action == GLFW_PRESS);
    }
}

void
guiGlfw_ScrollCallback(GLFWwindow *window, double xoffset, double yoffset)
{
    GuiGlfwData *bd = guiGlfw_GetBackendData();
    if (bd->scroll_callback && bd->main_window == window)
    {
        bd->scroll_callback(window, xoffset, yoffset);
    }

    ImGuiIO &io = ImGui::GetIO();
    io.AddMouseWheelEvent((F32)xoffset, (F32)yoffset);
}

static int
guiGlfw_TranslateUntranslatedKey(int key, int scancode)
{
#if !defined(__EMSCRIPTEN__)
    // GLFW 3.1+ attempts to "untranslate" keys, which goes the opposite of what every other
    // framework does, making using lettered shortcuts difficult. (It had reasons to do so: namely
    // GLFW is/was more likely to be used for WASD-type game controls rather than lettered
    // shortcuts, but IHMO the 3.1 change could have been done differently) See
    // https://github.com/glfw/glfw/issues/1502 for details. Adding a workaround to undo this (so
    // our keys are translated->untranslated->translated, likely a lossy process). This won't cover
    // edge cases but this is at least going to cover common cases.
    const char *key_name = glfwGetKeyName(key, scancode);

    if (key_name && key_name[0] != 0 && key_name[1] == 0)
    {
        const char char_names[] = "`-=[]\\,;\'./";
        const int char_keys[] = {GLFW_KEY_GRAVE_ACCENT, GLFW_KEY_MINUS,         GLFW_KEY_EQUAL,
                                 GLFW_KEY_LEFT_BRACKET, GLFW_KEY_RIGHT_BRACKET, GLFW_KEY_BACKSLASH,
                                 GLFW_KEY_COMMA,        GLFW_KEY_SEMICOLON,     GLFW_KEY_APOSTROPHE,
                                 GLFW_KEY_PERIOD,       GLFW_KEY_SLASH,         0};

        CF_STATIC_ASSERT(CF_ARRAY_SIZE(char_names) == CF_ARRAY_SIZE(char_keys), "Check arrays");

        if (key_name[0] >= '0' && key_name[0] <= '9')
        {
            key = GLFW_KEY_0 + (key_name[0] - '0');
        }
        else if (key_name[0] >= 'A' && key_name[0] <= 'Z')
        {
            key = GLFW_KEY_A + (key_name[0] - 'A');
        }
        else if (const char *p = strchr(char_names, key_name[0]))
        {
            key = char_keys[p - char_names];
        }
    }
#else
    CF_UNUSED(scancode);
#endif
    return key;
}

void
guiGlfw_KeyCallback(GLFWwindow *window, int keycode, int scancode, int action, int mods)
{
    GuiGlfwData *bd = guiGlfw_GetBackendData();
    if (bd->key_callback && bd->main_window == window)
    {
        bd->key_callback(window, keycode, scancode, action, mods);
    }

    if (action != GLFW_PRESS && action != GLFW_RELEASE) return;

    bool pressed = (action == GLFW_PRESS);

    guiGlfw_UpdateKeyModifiers(mods);

    if (keycode >= 0 && (Usize)keycode < CF_ARRAY_SIZE(bd->key_owners))
    {
        bd->key_owners[keycode] = pressed ? window : NULL;
    }

    keycode = guiGlfw_TranslateUntranslatedKey(keycode, scancode);

    ImGuiIO &io = ImGui::GetIO();
    ImGuiKey imgui_key = guiGlfw_KeyToImGuiKey(keycode);
    io.AddKeyEvent(imgui_key, pressed);
    // To support legacy indexing (<1.87 user code)
    io.SetKeyEventNativeData(imgui_key, keycode, scancode);
}

void
guiGlfw_WindowFocusCallback(GLFWwindow *window, int focused)
{
    GuiGlfwData *bd = guiGlfw_GetBackendData();
    if (bd->focus_callback && bd->main_window == window) bd->focus_callback(window, focused);

    ImGuiIO &io = ImGui::GetIO();
    io.AddFocusEvent(focused != 0);
}

void
guiGlfw_CursorPosCallback(GLFWwindow *window, double x, double y)
{
    GuiGlfwData *bd = guiGlfw_GetBackendData();
    if (bd->pos_callback && bd->main_window == window) bd->pos_callback(window, x, y);

    ImGuiIO &io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        int window_x, window_y;
        glfwGetWindowPos(window, &window_x, &window_y);
        x += window_x;
        y += window_y;
    }
    io.AddMousePosEvent((F32)x, (F32)y);
}

void
guiGlfw_CursorEnterCallback(GLFWwindow *window, int entered)
{
    GuiGlfwData *bd = guiGlfw_GetBackendData();
    if (bd->enter_callback && window == bd->main_window) bd->enter_callback(window, entered);

    if (entered)
    {
        bd->mouse_window = window;
    }
    else if (bd->mouse_window == window)
    {
        bd->mouse_window = NULL;

        ImGuiIO &io = ImGui::GetIO();
        io.AddMousePosEvent(-FLT_MAX, -FLT_MAX);
    }
}

void
guiGlfw_CharCallback(GLFWwindow *window, unsigned int c)
{
    GuiGlfwData *bd = guiGlfw_GetBackendData();
    if (bd->char_callback && window == bd->main_window) bd->char_callback(window, c);

    ImGuiIO &io = ImGui::GetIO();
    io.AddInputCharacter(c);
}

void
guiGlfw_MonitorCallback(GLFWmonitor *monitor, int event)
{
    GuiGlfwData *bd = guiGlfw_GetBackendData();
    if (bd->monitor_callback) bd->monitor_callback(monitor, event);
    bd->update_monitors = true;
}

bool
guiGlfwInit(GLFWwindow *window, bool gl_context)
{
    ImGuiIO &io = ImGui::GetIO();
    CF_ASSERT(io.BackendPlatformUserData == NULL, "Already initialized a platform backend!");

    // Setup backend capabilities flags
    GuiGlfwData *bd = IM_NEW(GuiGlfwData)();
    io.BackendPlatformUserData = (void *)bd;
    io.BackendPlatformName = "gui_backend_glfw";
    // We can honor GetMouseCursor() values (optional)
    io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;
    // We can honor io.WantSetMousePos requests (optional, rarely used)
    io.BackendFlags |= ImGuiBackendFlags_HasSetMousePos;
    // We can create multi-viewports on the Platform side (optional)
    io.BackendFlags |= ImGuiBackendFlags_PlatformHasViewports;
#if CF_OS_WIN32
    // We can call io.AddMouseViewportEvent() with correct data (optional)
    io.BackendFlags |= ImGuiBackendFlags_HasMouseHoveredViewport;
#endif

    bd->main_window = window;
    bd->time = 0.0;
    bd->update_monitors = true;

    io.SetClipboardTextFn = guiGlfw_SetClipboardText;
    io.GetClipboardTextFn = guiGlfw_GetClipboardText;
    io.ClipboardUserData = bd->main_window;

    // Create mouse cursors
    // (By design, on X11 cursors are user configurable and some cursors may be missing. When a
    // cursor doesn't exist, GLFW will emit an error which will often be printed by the app, so we
    // temporarily disable error reporting. Missing cursors will return NULL and our
    // _UpdateMouseCursor() function will use the Arrow cursor instead.)
    GLFWerrorfun prev_error_callback = glfwSetErrorCallback(NULL);
    bd->mouse_cursors[ImGuiMouseCursor_Arrow] = glfwCreateStandardCursor(GLFW_ARROW_CURSOR);
    bd->mouse_cursors[ImGuiMouseCursor_TextInput] = glfwCreateStandardCursor(GLFW_IBEAM_CURSOR);
    bd->mouse_cursors[ImGuiMouseCursor_ResizeNS] = glfwCreateStandardCursor(GLFW_VRESIZE_CURSOR);
    bd->mouse_cursors[ImGuiMouseCursor_ResizeEW] = glfwCreateStandardCursor(GLFW_HRESIZE_CURSOR);
    bd->mouse_cursors[ImGuiMouseCursor_Hand] = glfwCreateStandardCursor(GLFW_HAND_CURSOR);
    bd->mouse_cursors[ImGuiMouseCursor_ResizeAll] = glfwCreateStandardCursor(GLFW_ARROW_CURSOR);
    bd->mouse_cursors[ImGuiMouseCursor_ResizeNESW] = glfwCreateStandardCursor(GLFW_ARROW_CURSOR);
    bd->mouse_cursors[ImGuiMouseCursor_ResizeNWSE] = glfwCreateStandardCursor(GLFW_ARROW_CURSOR);
    bd->mouse_cursors[ImGuiMouseCursor_NotAllowed] = glfwCreateStandardCursor(GLFW_ARROW_CURSOR);

    glfwSetErrorCallback(prev_error_callback);
    bd->focus_callback = glfwSetWindowFocusCallback(window, guiGlfw_WindowFocusCallback);
    bd->enter_callback = glfwSetCursorEnterCallback(window, guiGlfw_CursorEnterCallback);
    bd->pos_callback = glfwSetCursorPosCallback(window, guiGlfw_CursorPosCallback);
    bd->mouse_callback = glfwSetMouseButtonCallback(window, guiGlfw_MouseButtonCallback);
    bd->scroll_callback = glfwSetScrollCallback(window, guiGlfw_ScrollCallback);
    bd->key_callback = glfwSetKeyCallback(window, guiGlfw_KeyCallback);
    bd->char_callback = glfwSetCharCallback(window, guiGlfw_CharCallback);

    // Update monitors the first time (note: monitor callback are broken in GLFW 3.2 and earlier,
    // see github.com/glfw/glfw/issues/784)
    guiGlfw_UpdateMonitors();
    bd->monitor_callback = glfwSetMonitorCallback(guiGlfw_MonitorCallback);

    // Our mouse update function expect PlatformHandle to be filled for the main viewport
    ImGuiViewport *main_viewport = ImGui::GetMainViewport();
    main_viewport->PlatformHandle = (void *)bd->main_window;
#if CF_OS_WIN32
    main_viewport->PlatformHandleRaw = glfwGetWin32Window(bd->main_window);
#endif
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) guiGlfw_InitPlatformInterface();

    bd->gl_context = gl_context;
    return true;
}

void
guiGlfwShutdown()
{
    GuiGlfwData *bd = guiGlfw_GetBackendData();
    CF_ASSERT(bd != NULL, "No platform backend to shutdown, or already shutdown?");
    ImGuiIO &io = ImGui::GetIO();

    ImGui::DestroyPlatformWindows();

    for (ImGuiMouseCursor cursor_n = 0; cursor_n < ImGuiMouseCursor_COUNT; cursor_n++)
        glfwDestroyCursor(bd->mouse_cursors[cursor_n]);

    io.BackendPlatformName = NULL;
    io.BackendPlatformUserData = NULL;
    IM_DELETE(bd);
}

static void
guiGlfw_UpdateMouseData()
{
    GuiGlfwData *bd = guiGlfw_GetBackendData();
    ImGuiIO &io = ImGui::GetIO();
    ImGuiPlatformIO &platform_io = ImGui::GetPlatformIO();

    ImGuiID mouse_viewport_id = 0;
    const ImVec2 mouse_pos_prev = io.MousePos;
    for (int n = 0; n < platform_io.Viewports.Size; n++)
    {
        ImGuiViewport *viewport = platform_io.Viewports[n];
        GLFWwindow *window = (GLFWwindow *)viewport->PlatformHandle;

#ifdef __EMSCRIPTEN__
        const bool is_window_focused = true;
#else
        const bool is_window_focused = glfwGetWindowAttrib(window, GLFW_FOCUSED) != 0;
#endif
        if (is_window_focused)
        {
            // (Optional) Set OS mouse position from Dear ImGui if requested (rarely used, only when
            // ImGuiConfigFlags_NavEnableSetMousePos is enabled by user) When multi-viewports are
            // enabled, all Dear ImGui positions are same as OS positions.
            if (io.WantSetMousePos)
                glfwSetCursorPos(window, (double)(mouse_pos_prev.x - viewport->Pos.x),
                                 (double)(mouse_pos_prev.y - viewport->Pos.y));

            // (Optional) Fallback to provide mouse position when focused
            // (guiGlfw_CursorPosCallback already provides this when hovered or captured)
            if (bd->mouse_window == NULL)
            {
                double mouse_x, mouse_y;
                glfwGetCursorPos(window, &mouse_x, &mouse_y);
                if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
                {
                    // Single viewport mode: mouse position in client window coordinates
                    // (io.MousePos is (0,0) when the mouse is on the upper-left corner of the app
                    // window) Multi-viewport mode: mouse position in OS absolute coordinates
                    // (io.MousePos is (0,0) when the mouse is on the upper-left of the primary
                    // monitor)
                    int window_x, window_y;
                    glfwGetWindowPos(window, &window_x, &window_y);
                    mouse_x += window_x;
                    mouse_y += window_y;
                }
                io.AddMousePosEvent((F32)mouse_x, (F32)mouse_y);
            }
        }

        // (Optional) When using multiple viewports: call io.AddMouseViewportEvent() with the
        // viewport the OS mouse cursor is hovering. If ImGuiBackendFlags_HasMouseHoveredViewport is
        // not set by the backend, Dear imGui will ignore this field and infer the information using
        // its flawed heuristic.
        // - [X] GLFW >= 3.3 backend ON WINDOWS ONLY does correctly ignore viewports with the
        // _NoInputs flag.
        // - [!] GLFW <= 3.2 backend CANNOT correctly ignore viewports with the _NoInputs flag, and
        // CANNOT reported Hovered Viewport because of mouse capture.
        //       Some backend are not able to handle that correctly. If a backend report an hovered
        //       viewport that has the _NoInputs flag (e.g. when dragging a window for docking, the
        //       viewport has the _NoInputs flag in order to allow us to find the viewport under),
        //       then Dear ImGui is forced to ignore the value reported by the backend, and use its
        //       flawed heuristic to guess the viewport behind.
        // - [X] GLFW backend correctly reports this regardless of another viewport behind focused
        // and dragged from (we need this to find a useful drag and drop target).
        // FIXME: This is currently only correct on Win32. See what we do below with the
        // WM_NCHITTEST, missing an equivalent for other systems. See
        // https://github.com/glfw/glfw/issues/1236 if you want to help in making this a GLFW
        // feature.
#if CF_OS_WIN32
        const bool window_no_input = (viewport->Flags & ImGuiViewportFlags_NoInputs) != 0;
        if (glfwGetWindowAttrib(window, GLFW_HOVERED) && !window_no_input)
            mouse_viewport_id = viewport->ID;
#else
        // We cannot use bd->mouse_window maintained from CursorEnter/Leave callbacks, because it is
        // locked to the window capturing mouse.
#endif
    }

    if (io.BackendFlags & ImGuiBackendFlags_HasMouseHoveredViewport)
        io.AddMouseViewportEvent(mouse_viewport_id);
}

static void
guiGlfw_UpdateMouseCursor()
{
    ImGuiIO &io = ImGui::GetIO();
    GuiGlfwData *bd = guiGlfw_GetBackendData();
    if ((io.ConfigFlags & ImGuiConfigFlags_NoMouseCursorChange) ||
        glfwGetInputMode(bd->main_window, GLFW_CURSOR) == GLFW_CURSOR_DISABLED)
        return;

    ImGuiMouseCursor imgui_cursor = ImGui::GetMouseCursor();
    ImGuiPlatformIO &platform_io = ImGui::GetPlatformIO();
    for (int n = 0; n < platform_io.Viewports.Size; n++)
    {
        GLFWwindow *window = (GLFWwindow *)platform_io.Viewports[n]->PlatformHandle;
        if (imgui_cursor == ImGuiMouseCursor_None || io.MouseDrawCursor)
        {
            // Hide OS mouse cursor if imgui is drawing it or if it wants no cursor
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
        }
        else
        {
            // Show OS mouse cursor
            // FIXME-PLATFORM: Unfocused windows seems to fail changing the mouse cursor with
            // GLFW 3.2, but 3.3 works here.
            glfwSetCursor(window, bd->mouse_cursors[imgui_cursor]
                                      ? bd->mouse_cursors[imgui_cursor]
                                      : bd->mouse_cursors[ImGuiMouseCursor_Arrow]);
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        }
    }
}

// Update gamepad inputs

static inline F32
Saturate(F32 v)
{
    return v < 0.0f ? 0.0f : v > 1.0f ? 1.0f : v;
}

static void
guiGlfw_UpdateGamepads()
{
    ImGuiIO &io = ImGui::GetIO();
    if ((io.ConfigFlags & ImGuiConfigFlags_NavEnableGamepad) == 0) return;

    io.BackendFlags &= ~ImGuiBackendFlags_HasGamepad;

    GLFWgamepadstate gamepad;
    if (!glfwGetGamepadState(GLFW_JOYSTICK_1, &gamepad)) return;

#define MAP_BUTTON(KEY_NO, BUTTON_NO, _UNUSED)                   \
    do                                                           \
    {                                                            \
        io.AddKeyEvent(KEY_NO, gamepad.buttons[BUTTON_NO] != 0); \
    } while (0)

#define MAP_ANALOG(KEY_NO, AXIS_NO, _UNUSED, V0, V1)          \
    do                                                        \
    {                                                         \
        F32 v = gamepad.axes[AXIS_NO];                        \
        v = (v - V0) / (V1 - V0);                             \
        io.AddKeyAnalogEvent(KEY_NO, v > 0.10f, Saturate(v)); \
    } while (0)

    io.BackendFlags |= ImGuiBackendFlags_HasGamepad;
    MAP_BUTTON(ImGuiKey_GamepadStart, GLFW_GAMEPAD_BUTTON_START, 7);
    MAP_BUTTON(ImGuiKey_GamepadBack, GLFW_GAMEPAD_BUTTON_BACK, 6);
    MAP_BUTTON(ImGuiKey_GamepadFaceDown, GLFW_GAMEPAD_BUTTON_A, 0);  // Xbox A, PS Cross
    MAP_BUTTON(ImGuiKey_GamepadFaceRight, GLFW_GAMEPAD_BUTTON_B, 1); // Xbox B, PS Circle
    MAP_BUTTON(ImGuiKey_GamepadFaceLeft, GLFW_GAMEPAD_BUTTON_X, 2);  // Xbox X, PS Square
    MAP_BUTTON(ImGuiKey_GamepadFaceUp, GLFW_GAMEPAD_BUTTON_Y, 3);    // Xbox Y, PS Triangle
    MAP_BUTTON(ImGuiKey_GamepadDpadLeft, GLFW_GAMEPAD_BUTTON_DPAD_LEFT, 13);
    MAP_BUTTON(ImGuiKey_GamepadDpadRight, GLFW_GAMEPAD_BUTTON_DPAD_RIGHT, 11);
    MAP_BUTTON(ImGuiKey_GamepadDpadUp, GLFW_GAMEPAD_BUTTON_DPAD_UP, 10);
    MAP_BUTTON(ImGuiKey_GamepadDpadDown, GLFW_GAMEPAD_BUTTON_DPAD_DOWN, 12);
    MAP_BUTTON(ImGuiKey_GamepadL1, GLFW_GAMEPAD_BUTTON_LEFT_BUMPER, 4);
    MAP_BUTTON(ImGuiKey_GamepadR1, GLFW_GAMEPAD_BUTTON_RIGHT_BUMPER, 5);
    MAP_ANALOG(ImGuiKey_GamepadL2, GLFW_GAMEPAD_AXIS_LEFT_TRIGGER, 4, -0.75f, +1.0f);
    MAP_ANALOG(ImGuiKey_GamepadR2, GLFW_GAMEPAD_AXIS_RIGHT_TRIGGER, 5, -0.75f, +1.0f);
    MAP_BUTTON(ImGuiKey_GamepadL3, GLFW_GAMEPAD_BUTTON_LEFT_THUMB, 8);
    MAP_BUTTON(ImGuiKey_GamepadR3, GLFW_GAMEPAD_BUTTON_RIGHT_THUMB, 9);
    MAP_ANALOG(ImGuiKey_GamepadLStickLeft, GLFW_GAMEPAD_AXIS_LEFT_X, 0, -0.25f, -1.0f);
    MAP_ANALOG(ImGuiKey_GamepadLStickRight, GLFW_GAMEPAD_AXIS_LEFT_X, 0, +0.25f, +1.0f);
    MAP_ANALOG(ImGuiKey_GamepadLStickUp, GLFW_GAMEPAD_AXIS_LEFT_Y, 1, -0.25f, -1.0f);
    MAP_ANALOG(ImGuiKey_GamepadLStickDown, GLFW_GAMEPAD_AXIS_LEFT_Y, 1, +0.25f, +1.0f);
    MAP_ANALOG(ImGuiKey_GamepadRStickLeft, GLFW_GAMEPAD_AXIS_RIGHT_X, 2, -0.25f, -1.0f);
    MAP_ANALOG(ImGuiKey_GamepadRStickRight, GLFW_GAMEPAD_AXIS_RIGHT_X, 2, +0.25f, +1.0f);
    MAP_ANALOG(ImGuiKey_GamepadRStickUp, GLFW_GAMEPAD_AXIS_RIGHT_Y, 3, -0.25f, -1.0f);
    MAP_ANALOG(ImGuiKey_GamepadRStickDown, GLFW_GAMEPAD_AXIS_RIGHT_Y, 3, +0.25f, +1.0f);

#undef MAP_BUTTON
#undef MAP_ANALOG
}

static void
guiGlfw_UpdateMonitors()
{
    int monitors_count = 0;
    GLFWmonitor **glfw_monitors = glfwGetMonitors(&monitors_count);

    ImGuiPlatformIO &platform_io = ImGui::GetPlatformIO();
    platform_io.Monitors.resize(0);

    for (int n = 0; n < monitors_count; ++n)
    {
        int x, y;
        glfwGetMonitorPos(glfw_monitors[n], &x, &y);

        const GLFWvidmode *vid_mode = glfwGetVideoMode(glfw_monitors[n]);

        ImGuiPlatformMonitor monitor;
        monitor.MainPos = monitor.WorkPos = ImVec2((F32)x, (F32)y);
        monitor.MainSize = monitor.WorkSize = ImVec2((F32)vid_mode->width, (F32)vid_mode->height);

        int w, h;
        glfwGetMonitorWorkarea(glfw_monitors[n], &x, &y, &w, &h);
        if (w > 0 && h > 0)
        {
            // Workaround a small GLFW issue reporting zero on monitor changes:
            // https://github.com/glfw/glfw/pull/1761
            monitor.WorkPos = ImVec2((F32)x, (F32)y);
            monitor.WorkSize = ImVec2((F32)w, (F32)h);
        }

        // Warning: the validity of monitor DPI information on Windows depends on the application
        // DPI awareness settings, which generally needs to be set in the manifest or at runtime.
        F32 x_scale, y_scale;
        glfwGetMonitorContentScale(glfw_monitors[n], &x_scale, &y_scale);
        monitor.DpiScale = x_scale;
        platform_io.Monitors.push_back(monitor);
    }

    GuiGlfwData *bd = guiGlfw_GetBackendData();
    bd->update_monitors = false;
}

void
guiGlfwNewFrame()
{
    ImGuiIO &io = ImGui::GetIO();
    GuiGlfwData *bd = guiGlfw_GetBackendData();
    CF_ASSERT(bd != NULL, "Did you call guiGlfw_InitForXXX()?");

    // Setup display size (every frame to accommodate for window resizing)
    int w, h;
    int display_w, display_h;
    glfwGetWindowSize(bd->main_window, &w, &h);
    glfwGetFramebufferSize(bd->main_window, &display_w, &display_h);
    io.DisplaySize = ImVec2((F32)w, (F32)h);
    if (w > 0 && h > 0)
        io.DisplayFramebufferScale = ImVec2((F32)display_w / (F32)w, (F32)display_h / (F32)h);
    if (bd->update_monitors) guiGlfw_UpdateMonitors();

    // TODO (Matteo): Use platform clock functionality
    // Setup time step
    double current_time = glfwGetTime();
    io.DeltaTime = bd->time > 0.0 ? (F32)(current_time - bd->time) : (F32)(1.0f / 60.0f);
    bd->time = current_time;

    guiGlfw_UpdateMouseData();
    guiGlfw_UpdateMouseCursor();

    // Update game controllers (if enabled and available)
    guiGlfw_UpdateGamepads();
}

//--------------------------------------------------------------------------------------------------------
// MULTI-VIEWPORT / PLATFORM INTERFACE SUPPORT
// This is an _advanced_ and _optional_ feature, allowing the backend to create and handle multiple
// viewports simultaneously. If you are new to dear imgui or creating a new binding for dear imgui,
// it is recommended that you completely ignore this section first..
//--------------------------------------------------------------------------------------------------------

// Helper structure we store in the void* RenderUserData field of each ImGuiViewport to easily
// retrieve our backend data.
struct GuiGlfwViewportData
{
    GLFWwindow *Window;
    bool WindowOwned;
    int IgnoreWindowPosEventFrame;
    int IgnoreWindowSizeEventFrame;

    GuiGlfwViewportData()
    {
        Window = NULL;
        WindowOwned = false;
        IgnoreWindowSizeEventFrame = IgnoreWindowPosEventFrame = -1;
    }
    ~GuiGlfwViewportData() { CF_ASSERT(Window == NULL, "Window pointer must be null"); }
};

static void
guiGlfw_WindowCloseCallback(GLFWwindow *window)
{
    if (ImGuiViewport *viewport = ImGui::FindViewportByPlatformHandle(window))
        viewport->PlatformRequestClose = true;
}

// GLFW may dispatch window pos/size events after calling glfwSetWindowPos()/glfwSetWindowSize().
// However: depending on the platform the callback may be invoked at different time:
// - on Windows it appears to be called within the glfwSetWindowPos()/glfwSetWindowSize() call
// - on Linux it is queued and invoked during glfwPollEvents()
// Because the event doesn't always fire on glfwSetWindowXXX() we use a frame counter tag to only
// ignore recent glfwSetWindowXXX() calls.
static void
guiGlfw_WindowPosCallback(GLFWwindow *window, int, int)
{
    if (ImGuiViewport *viewport = ImGui::FindViewportByPlatformHandle(window))
    {
        if (GuiGlfwViewportData *vd = (GuiGlfwViewportData *)viewport->PlatformUserData)
        {
            if (ImGui::GetFrameCount() <= vd->IgnoreWindowPosEventFrame + 1) return;
        }
        viewport->PlatformRequestMove = true;
    }
}

static void
guiGlfw_WindowSizeCallback(GLFWwindow *window, int, int)
{
    if (ImGuiViewport *viewport = ImGui::FindViewportByPlatformHandle(window))
    {
        if (GuiGlfwViewportData *vd = (GuiGlfwViewportData *)viewport->PlatformUserData)
        {
            if (ImGui::GetFrameCount() <= vd->IgnoreWindowSizeEventFrame + 1) return;
        }
        viewport->PlatformRequestResize = true;
    }
}

static void
guiGlfw_CreateWindow(ImGuiViewport *viewport)
{
    GuiGlfwData *bd = guiGlfw_GetBackendData();
    GuiGlfwViewportData *vd = IM_NEW(GuiGlfwViewportData)();
    viewport->PlatformUserData = vd;

    // GLFW 3.2 unfortunately always set focus on glfwCreateWindow() if GLFW_VISIBLE is set,
    // regardless of GLFW_FOCUSED With GLFW 3.3, the hint GLFW_FOCUS_ON_SHOW fixes this problem
    glfwWindowHint(GLFW_VISIBLE, false);
    glfwWindowHint(GLFW_FOCUSED, false);
    glfwWindowHint(GLFW_FOCUS_ON_SHOW, false);
    glfwWindowHint(GLFW_DECORATED,
                   (viewport->Flags & ImGuiViewportFlags_NoDecoration) ? false : true);
    glfwWindowHint(GLFW_FLOATING, (viewport->Flags & ImGuiViewportFlags_TopMost) ? true : false);

    GLFWwindow *share_window = (bd->gl_context) ? bd->main_window : NULL;
    vd->Window = glfwCreateWindow((int)viewport->Size.x, (int)viewport->Size.y, "No Title Yet",
                                  NULL, share_window);
    vd->WindowOwned = true;
    viewport->PlatformHandle = (void *)vd->Window;
#if CF_OS_WIN32
    viewport->PlatformHandleRaw = glfwGetWin32Window(vd->Window);
#endif

    glfwSetWindowPos(vd->Window, (int)viewport->Pos.x, (int)viewport->Pos.y);

    // Install GLFW callbacks for secondary viewports
    glfwSetWindowFocusCallback(vd->Window, guiGlfw_WindowFocusCallback);
    glfwSetCursorEnterCallback(vd->Window, guiGlfw_CursorEnterCallback);
    glfwSetCursorPosCallback(vd->Window, guiGlfw_CursorPosCallback);
    glfwSetMouseButtonCallback(vd->Window, guiGlfw_MouseButtonCallback);
    glfwSetScrollCallback(vd->Window, guiGlfw_ScrollCallback);
    glfwSetKeyCallback(vd->Window, guiGlfw_KeyCallback);
    glfwSetCharCallback(vd->Window, guiGlfw_CharCallback);
    glfwSetWindowCloseCallback(vd->Window, guiGlfw_WindowCloseCallback);
    glfwSetWindowPosCallback(vd->Window, guiGlfw_WindowPosCallback);
    glfwSetWindowSizeCallback(vd->Window, guiGlfw_WindowSizeCallback);

    if (bd->gl_context)
    {
        glfwMakeContextCurrent(vd->Window);
        glfwSwapInterval(0);
    }
}

static void
guiGlfw_DestroyWindow(ImGuiViewport *viewport)
{
    GuiGlfwData *bd = guiGlfw_GetBackendData();
    if (GuiGlfwViewportData *vd = (GuiGlfwViewportData *)viewport->PlatformUserData)
    {
        if (vd->WindowOwned)
        {
#if CF_OS_WIN32
            HWND hwnd = (HWND)viewport->PlatformHandleRaw;
            ::RemovePropA(hwnd, "IMGUI_VIEWPORT");
#endif

            // Release any keys that were pressed in the window being destroyed and are still held
            // down, because we will not receive any release events after window is destroyed.
            for (Usize i = 0; i < CF_ARRAY_SIZE(bd->key_owners); ++i)
            {
                if (bd->key_owners[i] == vd->Window)
                {
                    // Later params are only used for main viewport,
                    // on which this function is never called.
                    guiGlfw_KeyCallback(vd->Window, (int)i, 0, GLFW_RELEASE, 0);
                }
            }

            glfwDestroyWindow(vd->Window);
        }
        vd->Window = NULL;
        IM_DELETE(vd);
    }
    viewport->PlatformUserData = viewport->PlatformHandle = NULL;
}

// We have submitted https://github.com/glfw/glfw/pull/1568 to allow GLFW to support "transparent
// inputs". In the meanwhile we implement custom per-platform workarounds here (FIXME-VIEWPORT:
// Implement same work-around for Linux/OSX!)
#if CF_OS_WIN32

static WNDPROC g_GlfwWndProc = NULL;

static LRESULT CALLBACK
WndProcNoInputs(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (msg == WM_NCHITTEST)
    {
        // Let mouse pass-through the window. This will allow the backend to call
        // io.AddMouseViewportEvent() properly (which is OPTIONAL). The ImGuiViewportFlags_NoInputs
        // flag is set while dragging a viewport, as want to detect the window behind the one we are
        // dragging. If you cannot easily access those viewport flags from your windowing/event
        // code: you may manually synchronize its state e.g. in your main loop after calling
        // UpdatePlatformWindows(). Iterate all viewports/platform windows and pass the flag to your
        // windowing system.
        ImGuiViewport *viewport = (ImGuiViewport *)::GetPropA(hWnd, "IMGUI_VIEWPORT");
        if (viewport->Flags & ImGuiViewportFlags_NoInputs) return HTTRANSPARENT;
    }
    return ::CallWindowProc(g_GlfwWndProc, hWnd, msg, wParam, lParam);
}
#endif

static void
guiGlfw_ShowWindow(ImGuiViewport *viewport)
{
    GuiGlfwViewportData *vd = (GuiGlfwViewportData *)viewport->PlatformUserData;

#if CF_OS_WIN32
    // GLFW hack: Hide icon from task bar
    HWND hwnd = (HWND)viewport->PlatformHandleRaw;
    if (viewport->Flags & ImGuiViewportFlags_NoTaskBarIcon)
    {
        LONG ex_style = ::GetWindowLong(hwnd, GWL_EXSTYLE);
        ex_style &= ~WS_EX_APPWINDOW;
        ex_style |= WS_EX_TOOLWINDOW;
        ::SetWindowLong(hwnd, GWL_EXSTYLE, ex_style);
    }

    // GLFW hack: install hook for WM_NCHITTEST message handler
#    if CF_OS_WIN32
    ::SetPropA(hwnd, "IMGUI_VIEWPORT", viewport);
    if (g_GlfwWndProc == NULL) g_GlfwWndProc = (WNDPROC)::GetWindowLongPtr(hwnd, GWLP_WNDPROC);
    ::SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)WndProcNoInputs);
#    endif
#endif

    glfwShowWindow(vd->Window);
}

static ImVec2
guiGlfw_GetWindowPos(ImGuiViewport *viewport)
{
    GuiGlfwViewportData *vd = (GuiGlfwViewportData *)viewport->PlatformUserData;
    int x = 0, y = 0;
    glfwGetWindowPos(vd->Window, &x, &y);
    return ImVec2((F32)x, (F32)y);
}

static void
guiGlfw_SetWindowPos(ImGuiViewport *viewport, ImVec2 pos)
{
    GuiGlfwViewportData *vd = (GuiGlfwViewportData *)viewport->PlatformUserData;
    vd->IgnoreWindowPosEventFrame = ImGui::GetFrameCount();
    glfwSetWindowPos(vd->Window, (int)pos.x, (int)pos.y);
}

static ImVec2
guiGlfw_GetWindowSize(ImGuiViewport *viewport)
{
    GuiGlfwViewportData *vd = (GuiGlfwViewportData *)viewport->PlatformUserData;
    int w = 0, h = 0;
    glfwGetWindowSize(vd->Window, &w, &h);
    return {(F32)w, (F32)h};
}

static void
guiGlfw_SetWindowSize(ImGuiViewport *viewport, ImVec2 size)
{
    GuiGlfwViewportData *vd = (GuiGlfwViewportData *)viewport->PlatformUserData;
    vd->IgnoreWindowSizeEventFrame = ImGui::GetFrameCount();
    glfwSetWindowSize(vd->Window, (int)size.x, (int)size.y);
}

static void
guiGlfw_SetWindowTitle(ImGuiViewport *viewport, const char *title)
{
    GuiGlfwViewportData *vd = (GuiGlfwViewportData *)viewport->PlatformUserData;
    glfwSetWindowTitle(vd->Window, title);
}

static void
guiGlfw_SetWindowFocus(ImGuiViewport *viewport)
{
    GuiGlfwViewportData *vd = (GuiGlfwViewportData *)viewport->PlatformUserData;
    glfwFocusWindow(vd->Window);
}

static bool
guiGlfw_GetWindowFocus(ImGuiViewport *viewport)
{
    GuiGlfwViewportData *vd = (GuiGlfwViewportData *)viewport->PlatformUserData;
    return glfwGetWindowAttrib(vd->Window, GLFW_FOCUSED) != 0;
}

static bool
guiGlfw_GetWindowMinimized(ImGuiViewport *viewport)
{
    GuiGlfwViewportData *vd = (GuiGlfwViewportData *)viewport->PlatformUserData;
    return glfwGetWindowAttrib(vd->Window, GLFW_ICONIFIED) != 0;
}

static void
guiGlfw_SetWindowAlpha(ImGuiViewport *viewport, F32 alpha)
{
    GuiGlfwViewportData *vd = (GuiGlfwViewportData *)viewport->PlatformUserData;
    glfwSetWindowOpacity(vd->Window, alpha);
}

static void
guiGlfw_RenderWindow(ImGuiViewport *viewport, void *)
{
    GuiGlfwData *bd = guiGlfw_GetBackendData();
    GuiGlfwViewportData *vd = (GuiGlfwViewportData *)viewport->PlatformUserData;
    if (bd->gl_context) glfwMakeContextCurrent(vd->Window);
}

static void
guiGlfw_SwapBuffers(ImGuiViewport *viewport, void *)
{
    GuiGlfwData *bd = guiGlfw_GetBackendData();
    GuiGlfwViewportData *vd = (GuiGlfwViewportData *)viewport->PlatformUserData;
    if (bd->gl_context)
    {
        glfwMakeContextCurrent(vd->Window);
        glfwSwapBuffers(vd->Window);
    }
}

//--------------------------------------------------------------------------------------------------------
// Vulkan support (the Vulkan renderer needs to call a platform-side support function to create the
// surface)
//--------------------------------------------------------------------------------------------------------

CF_EXTERN_C GLFWAPI I32 glfwCreateWindowSurface(VkInstance instance, GLFWwindow *window,
                                                const VkAllocationCallbacks *allocator,
                                                VkSurfaceKHR *surface);

static I32
guiGlfw_CreateVkSurface(ImGuiViewport *viewport, ImU64 vk_instance, const void *vk_allocator,
                        ImU64 *out_vk_surface)
{
    GuiGlfwData *bd = guiGlfw_GetBackendData();
    CF_UNUSED(bd);
    CF_ASSERT(!bd->gl_context, "Unexpected API");

    GuiGlfwViewportData *vd = (GuiGlfwViewportData *)viewport->PlatformUserData;
    return glfwCreateWindowSurface((VkInstance)vk_instance, vd->Window,
                                   (const VkAllocationCallbacks *)vk_allocator,
                                   (VkSurfaceKHR *)out_vk_surface);
}

static void
guiGlfw_InitPlatformInterface()
{
    // Register platform interface (will be coupled with a renderer interface)
    GuiGlfwData *bd = guiGlfw_GetBackendData();
    ImGuiPlatformIO &platform_io = ImGui::GetPlatformIO();
    platform_io.Platform_CreateWindow = guiGlfw_CreateWindow;
    platform_io.Platform_DestroyWindow = guiGlfw_DestroyWindow;
    platform_io.Platform_ShowWindow = guiGlfw_ShowWindow;
    platform_io.Platform_SetWindowPos = guiGlfw_SetWindowPos;
    platform_io.Platform_GetWindowPos = guiGlfw_GetWindowPos;
    platform_io.Platform_SetWindowSize = guiGlfw_SetWindowSize;
    platform_io.Platform_GetWindowSize = guiGlfw_GetWindowSize;
    platform_io.Platform_SetWindowFocus = guiGlfw_SetWindowFocus;
    platform_io.Platform_GetWindowFocus = guiGlfw_GetWindowFocus;
    platform_io.Platform_GetWindowMinimized = guiGlfw_GetWindowMinimized;
    platform_io.Platform_SetWindowTitle = guiGlfw_SetWindowTitle;
    platform_io.Platform_RenderWindow = guiGlfw_RenderWindow;
    platform_io.Platform_SwapBuffers = guiGlfw_SwapBuffers;
    platform_io.Platform_SetWindowAlpha = guiGlfw_SetWindowAlpha;
    platform_io.Platform_CreateVkSurface = guiGlfw_CreateVkSurface;

    // Register main window handle (which is owned by the main application, not by us)
    // This is mostly for simplicity and consistency, so that our code (e.g. mouse handling etc.)
    // can use same logic for main and secondary viewports.
    ImGuiViewport *main_viewport = ImGui::GetMainViewport();
    GuiGlfwViewportData *vd = IM_NEW(GuiGlfwViewportData)();
    vd->Window = bd->main_window;
    vd->WindowOwned = false;
    main_viewport->PlatformUserData = vd;
    main_viewport->PlatformHandle = bd->main_window;
#if CF_OS_WIN32
    main_viewport->PlatformHandleRaw = glfwGetWin32Window(bd->main_window);
#endif
}

CF_DIAGNOSTIC_POP()
