// Forked from Dear ImGui Renderer Backend for modern OpenGL with shaders / programmatic pipeline

#if !defined(GUI_BACKEND_FONT_TEXTURE_RGBA)
#    define GUI_BACKEND_FONT_TEXTURE_RGBA 1
#endif

#if !defined(GUI_BACKEND_CUSTOM)
#    define GUI_BACKEND_CUSTOM 1
#endif

//--------------------------------------------------------------------------------------------------
// OpenGL    GLSL      GLSL
// version   version   string
//--------------------------------------------------------------------------------------------------
//  2.0       110       "#version 110"
//  2.1       120       "#version 120"
//  3.0       130       "#version 130"
//  3.1       140       "#version 140"
//  3.2       150       "#version 150"
//  3.3       330       "#version 330 core"
//  4.0       400       "#version 400 core"
//  4.1       410       "#version 410 core"
//  4.2       420       "#version 410 core"
//  4.3       430       "#version 430 core"
//  ES 2.0    100       "#version 100"      = WebGL 1.0
//  ES 3.0    300       "#version 300 es"   = WebGL 2.0
//--------------------------------------------------------------------------------------------------

#include "gui_backend_gl3.h"

#if !GUI_BACKEND_CUSTOM || defined(IMGUI_IMPL_OPENGL_ES2) || defined(IMGUI_IMPL_OPENGL_ES3)

// NOTE (Matteo): Use DearImgui's own backend for opengl ES

CF_DIAGNOSTIC_PUSH()
CF_DIAGNOSTIC_IGNORE_CLANG("-Wdeprecated-declarations")
CF_DIAGNOSTIC_IGNORE_CLANG("-Wsign-conversion")
#    include <backends/imgui_impl_opengl3.cpp>
CF_DIAGNOSTIC_POP()

// Backend API
bool
guiGl3Init(GuiOpenGLVersion version)
{
    CF_UNUSED(version);
    return ImGui_ImplOpenGL3_Init(NULL);
}

void
guiGl3Shutdown()
{
    ImGui_ImplOpenGL3_Shutdown();
}

void
guiGl3NewFrame()
{
    ImGui_ImplOpenGL3_NewFrame();
}

void
guiGl3Render(GuiDrawData *draw_data)
{
    ImGui_ImplOpenGL3_RenderDrawData(draw_data);
}

void
guiGl3UpdateFontsTexture()
{
    ImGui_ImplOpenGL3_DestroyFontsTexture();
    ImGui_ImplOpenGL3_CreateFontsTexture();
}

#else

#    include "imgui.h"

#    include "foundation/memory.h"

#    include <stdio.h>

CF_DIAGNOSTIC_PUSH()
CF_DIAGNOSTIC_IGNORE_CLANG("-Wold-style-cast")
CF_DIAGNOSTIC_IGNORE_CLANG("-Wzero-as-null-pointer-constant")

// GL includes
#    if !defined(IMGUI_IMPL_OPENGL_LOADER_CUSTOM)
// Modern desktop OpenGL doesn't have a standard portable header file to load OpenGL function
// pointers. Helper libraries are often used for this purpose! Here we are using our own minimal
// custom loader based on gl3w. In the rest of your app/engine, you can use another loader of your
// choice (gl3w, glew, glad, glbinding, glext, glLoadGen, etc.). If you happen to be developing a
// new feature for this backend (imgui_impl_opengl3.cpp):
// - You may need to regenerate imgui_impl_opengl3_loader.h to add new symbols. See
// https://github.com/dearimgui/gl3w_stripped
// - You can temporarily use an unstripped version. See
// https://github.com/dearimgui/gl3w_stripped/releases Changes to this backend using new APIs should
// be accompanied by a regenerated stripped loader version.
#        define IMGL3W_IMPL
#        include "imgui_impl_opengl3_loader.h"
#    endif

// OpenGL Data
struct GuiGl3BackendData
{
    // Extracted at runtime using GL_MAJOR_VERSION, GL_MINOR_VERSION queries (e.g.
    // 320 for GL 3.2)
    GLuint GlVersion;
    // Given by user
    GLuint GlslVersion;

    GLuint font_tex;
    GLuint shader;

    // Uniforms location
    GLint uniform_font;
    GLint uniform_text;
    GLint uniform_proj;
    // Vertex attributes location
    GLuint attr_pos;
    GLuint attr_uv;
    GLuint attr_color;

    GLuint vbo, ebo;
    GLsizeiptr vbo_size;
    GLsizeiptr ebo_size;

    bool has_clip_origin;

    GuiGl3BackendData() { memClearStruct(this); }
};

// Backend data stored in io.BackendRendererUserData to allow support for multiple Dear ImGui
// contexts It is STRONGLY preferred that you use docking branch with multi-viewports (== single
// Dear ImGui context + multiple windows) instead of multiple Dear ImGui contexts.
CF_INTERNAL GuiGl3BackendData *
guiGl3_BackendData()
{
    return ImGui::GetCurrentContext() ? (GuiGl3BackendData *)ImGui::GetIO().BackendRendererUserData
                                      : NULL;
}

// Functions

CF_INTERNAL void
guiGl3_SetupRenderState(ImDrawData *draw_data, GLsizei fb_width, GLsizei fb_height,
                        GLuint vertex_array_object)
{
    GuiGl3BackendData *bd = guiGl3_BackendData();

    // Setup render state: alpha-blending enabled, no face culling, no depth testing, scissor
    // enabled, polygon fill
    gloadToggle(GL_BLEND, true);
    glBlendEquation(GL_FUNC_ADD);
    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    gloadToggle(GL_CULL_FACE, false);
    gloadToggle(GL_DEPTH_TEST, false);
    gloadToggle(GL_STENCIL_TEST, false);
    gloadToggle(GL_SCISSOR_TEST, true);
    if (bd->GlVersion >= 310) gloadToggle(GL_PRIMITIVE_RESTART, false);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    // Support for GL 4.5 rarely used glClipControl(GL_UPPER_LEFT)
#    if defined(GL_CLIP_ORIGIN)
    bool clip_origin_lower_left = true;
    if (bd->has_clip_origin)
    {
        GLenum current_clip_origin = 0;
        glGetIntegerv(GL_CLIP_ORIGIN, (GLint *)&current_clip_origin);
        if (current_clip_origin == GL_UPPER_LEFT) clip_origin_lower_left = false;
    }
#    endif

    // Setup viewport, orthographic projection matrix
    // Our visible imgui space lies from draw_data->DisplayPos (top left) to
    // draw_data->DisplayPos+data_data->DisplaySize (bottom right). DisplayPos is (0,0) for single
    // viewport apps.
    glViewport(0, 0, fb_width, fb_height);
    F32 L = draw_data->DisplayPos.x;
    F32 R = draw_data->DisplayPos.x + draw_data->DisplaySize.x;
    F32 T = draw_data->DisplayPos.y;
    F32 B = draw_data->DisplayPos.y + draw_data->DisplaySize.y;
#    if defined(GL_CLIP_ORIGIN)
    // Swap top and bottom if origin is upper left
    if (!clip_origin_lower_left)
    {
        F32 tmp = T;
        T = B;
        B = tmp;
    }
#    endif
    const F32 ortho_projection[4][4] = {
        {2.0f / (R - L), 0.0f, 0.0f, 0.0f},
        {0.0f, 2.0f / (T - B), 0.0f, 0.0f},
        {0.0f, 0.0f, -1.0f, 0.0f},
        {(R + L) / (L - R), (T + B) / (B - T), 0.0f, 1.0f},
    };
    glUseProgram(bd->shader);
    glUniform1i(bd->uniform_text, 0);
    glUniform1i(bd->uniform_font, 1);
    glUniformMatrix4fv(bd->uniform_proj, 1, GL_FALSE, &ortho_projection[0][0]);

    // We use combined texture/sampler state. Applications using GL 3.3 may set that otherwise.
    if (bd->GlVersion >= 330) glBindSampler(0, 0);

    glBindVertexArray(vertex_array_object);

    // Bind vertex/index buffers and setup attributes for ImDrawVert
    glBindBuffer(GL_ARRAY_BUFFER, bd->vbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, bd->ebo);
    glEnableVertexAttribArray(bd->attr_pos);
    glEnableVertexAttribArray(bd->attr_uv);
    glEnableVertexAttribArray(bd->attr_color);
    glVertexAttribPointer(bd->attr_pos, 2, GL_FLOAT, GL_FALSE, sizeof(ImDrawVert),
                          (GLvoid *)offsetof(ImDrawVert, pos));
    glVertexAttribPointer(bd->attr_uv, 2, GL_FLOAT, GL_FALSE, sizeof(ImDrawVert),
                          (GLvoid *)offsetof(ImDrawVert, uv));
    glVertexAttribPointer(bd->attr_color, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(ImDrawVert),
                          (GLvoid *)offsetof(ImDrawVert, col));
}

// If you get an error please report on github. You may try different GL context version or GLSL
// version. See GL<>GLSL version table at the top of this file.
CF_INTERNAL bool
guiGl3_CheckShader(GLuint handle, const char *desc)
{
    GuiGl3BackendData *bd = guiGl3_BackendData();
    GLint status = 0, log_length = 0;
    glGetShaderiv(handle, GL_COMPILE_STATUS, &status);
    glGetShaderiv(handle, GL_INFO_LOG_LENGTH, &log_length);

    if (status == GL_FALSE)
    {
        fprintf(stderr, "ERROR: Gui OpenGL 3 backend: failed to compile %s! With GLSL: %u\n", desc,
                bd->GlslVersion);
    }

    if (log_length > 1)
    {
        ImVector<char> buf;
        buf.resize((int)(log_length + 1));
        glGetShaderInfoLog(handle, log_length, NULL, (GLchar *)buf.begin());
        fprintf(stderr, "%s\n", buf.begin());
    }

    return (status == GL_TRUE);
}

// If you get an error please report on GitHub. You may try different GL context version or GLSL
// version.
CF_INTERNAL bool
guiGl3_CheckProgram(GLuint handle, const char *desc)
{
    GuiGl3BackendData *bd = guiGl3_BackendData();
    GLint status = 0, log_length = 0;
    glGetProgramiv(handle, GL_LINK_STATUS, &status);
    glGetProgramiv(handle, GL_INFO_LOG_LENGTH, &log_length);

    if (status == GL_FALSE)
    {
        fprintf(stderr, "ERROR: Gui OpenGL 3 backend: failed to link %s! With GLSL %u\n", desc,
                bd->GlslVersion);
    }

    if (log_length > 1)
    {
        ImVector<char> buf;
        buf.resize((int)(log_length + 1));
        glGetProgramInfoLog(handle, log_length, NULL, (GLchar *)buf.begin());
        fprintf(stderr, "%s\n", buf.begin());
    }

    return (status == GL_TRUE);
}

CF_INTERNAL void
guiGl3_CreateFontsTexture()
{
    ImGuiIO &io = ImGui::GetIO();
    GuiGl3BackendData *bd = guiGl3_BackendData();

    // Build texture atlas
    // Load as RGBA 32-bit (75% of the memory is wasted, but default font is so small)
    // because it is more likely to be compatible with user's existing shaders. If
    // your ImTextureId represent a higher-level concept than just a GL texture id,
    // consider calling GetTexDataAsAlpha8() instead to save on GPU memory.
    unsigned char *pixels;
    GLint width, height;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

    // Upload texture to graphics system
    GLint last_texture;
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &last_texture);
    glGenTextures(1, &bd->font_tex);
    glBindTexture(GL_TEXTURE_2D, bd->font_tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
#    ifdef GL_UNPACK_ROW_LENGTH // Not on WebGL/ES
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
#    endif
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);

    // Store our identifier
    io.Fonts->SetTexID((ImTextureID)(Iptr)bd->font_tex);

    // Restore state
    glBindTexture(GL_TEXTURE_2D, (GLuint)last_texture);
}

CF_INTERNAL void
guiGl3_DeleteFontsTexture()
{
    ImGuiIO &io = ImGui::GetIO();
    GuiGl3BackendData *bd = guiGl3_BackendData();
    if (bd->font_tex)
    {
        glDeleteTextures(1, &bd->font_tex);
        io.Fonts->SetTexID(0);
        bd->font_tex = 0;
    }
}

CF_INTERNAL bool
guiGl3_CreateDeviceObjects()
{
    GuiGl3BackendData *bd = guiGl3_BackendData();

    // Backup GL state
    GLint last_texture, last_array_buffer;
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &last_texture);
    glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &last_array_buffer);
    GLint last_vertex_array;
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &last_vertex_array);

    // Parse GLSL version string

    const GLchar *vertex_shader_glsl_300_es = //
        "#version 300 es\n"
        "precision highp float;\n"
        "layout (location = 0) in vec2 Position;\n"
        "layout (location = 1) in vec2 UV;\n"
        "layout (location = 2) in vec4 Color;\n"
        "uniform mat4 ProjMtx;\n"
        "out vec2 Frag_UV;\n"
        "out vec4 Frag_Color;\n"
        "void main()\n"
        "{\n"
        "    Frag_UV = UV;\n"
        "    Frag_Color = Color;\n"
        "    gl_Position = ProjMtx * vec4(Position.xy,0,1);\n"
        "}\n";

    const GLchar *vertex_shader_glsl_410_core = //
        "#version 410 core\n"
        "layout (location = 0) in vec2 Position;\n"
        "layout (location = 1) in vec2 UV;\n"
        "layout (location = 2) in vec4 Color;\n"
        "uniform mat4 ProjMtx;\n"
        "out vec2 Frag_UV;\n"
        "out vec4 Frag_Color;\n"
        "void main()\n"
        "{\n"
        "    Frag_UV = UV;\n"
        "    Frag_Color = Color;\n"
        "    gl_Position = ProjMtx * vec4(Position.xy,0,1);\n"
        "}\n";

    const GLchar *fragment_shader_glsl_300_es =
        "#version 300 es\n"
        "precision mediump float;\n"
        "uniform sampler2D Texture;\n"
        "in vec2 Frag_UV;\n"
        "in vec4 Frag_Color;\n"
        "layout (location = 0) out vec4 Out_Color;\n"
        "void main()\n"
        "{\n"
        "    Out_Color = Frag_Color * texture(Texture, Frag_UV.st);\n"
        "}\n";

    const GLchar *fragment_shader_glsl_410_core = //
        "#version 410 core\n"
        "in vec2 Frag_UV;\n"
        "in vec4 Frag_Color;\n"
        "uniform sampler2D Texture;\n"
        "layout (location = 0) out vec4 Out_Color;\n"
        "void main()\n"
        "{\n"
        "    Out_Color = Frag_Color * texture(Texture, Frag_UV.st);\n"
        "}\n";

    // Select shaders matching our GLSL versions
    const GLchar *vertex_shader = NULL;
    const GLchar *fragment_shader = NULL;

    if (bd->GlslVersion >= 410)
    {
        vertex_shader = vertex_shader_glsl_410_core;
        fragment_shader = fragment_shader_glsl_410_core;
    }
    else
    {
        vertex_shader = vertex_shader_glsl_300_es;
        fragment_shader = fragment_shader_glsl_300_es;
    }

    // Create shaders
    GLuint vert_handle = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vert_handle, 1, &vertex_shader, NULL);
    glCompileShader(vert_handle);
    guiGl3_CheckShader(vert_handle, "vertex shader");

    GLuint frag_handle = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(frag_handle, 1, &fragment_shader, NULL);
    glCompileShader(frag_handle);
    guiGl3_CheckShader(frag_handle, "fragment shader");

    // Link
    bd->shader = glCreateProgram();
    glAttachShader(bd->shader, vert_handle);
    glAttachShader(bd->shader, frag_handle);
    glLinkProgram(bd->shader);
    guiGl3_CheckProgram(bd->shader, "shader program");

    glDetachShader(bd->shader, vert_handle);
    glDetachShader(bd->shader, frag_handle);
    glDeleteShader(vert_handle);
    glDeleteShader(frag_handle);

    bd->uniform_font = glGetUniformLocation(bd->shader, "Fonts");
    bd->uniform_text = glGetUniformLocation(bd->shader, "Texture");
    bd->uniform_proj = glGetUniformLocation(bd->shader, "ProjMtx");
    bd->attr_pos = (GLuint)glGetAttribLocation(bd->shader, "Position");
    bd->attr_uv = (GLuint)glGetAttribLocation(bd->shader, "UV");
    bd->attr_color = (GLuint)glGetAttribLocation(bd->shader, "Color");

    // Create buffers
    glGenBuffers(1, &bd->vbo);
    glGenBuffers(1, &bd->ebo);

    guiGl3_CreateFontsTexture();

    // Restore modified GL state
    glBindTexture(GL_TEXTURE_2D, (GLuint)last_texture);
    glBindBuffer(GL_ARRAY_BUFFER, (GLuint)last_array_buffer);
    glBindVertexArray((GLuint)last_vertex_array);

    return true;
}

CF_INTERNAL void
guiGl3_DeleteDeviceObjects()
{
    GuiGl3BackendData *bd = guiGl3_BackendData();

    if (bd->vbo)
    {
        glDeleteBuffers(1, &bd->vbo);
        bd->vbo = 0;
    }

    if (bd->ebo)
    {
        glDeleteBuffers(1, &bd->ebo);
        bd->ebo = 0;
    }

    if (bd->shader)
    {
        glDeleteProgram(bd->shader);
        bd->shader = 0;
    }

    guiGl3_DeleteFontsTexture();
}

//--------------------------------------------------------------------------------------------------
// MULTI-VIEWPORT / PLATFORM INTERFACE SUPPORT
// This is an _advanced_ and _optional_ feature, allowing the backend to create and handle
// multiple viewports simultaneously. If you are new to dear imgui or creating a new binding for
// dear imgui, it is recommended that you completely ignore this section first..
//--------------------------------------------------------------------------------------------------

CF_INTERNAL void
guiGl3_RenderWindow(ImGuiViewport *viewport, void *)
{
    if (!(viewport->Flags & ImGuiViewportFlags_NoRendererClear))
    {
        ImVec4 clear_color = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
        glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
    }
    guiGl3Render(viewport->DrawData);
}

CF_DIAGNOSTIC_POP()

//--------------------------------------------------------------------------------------------------------
// Backend API
//--------------------------------------------------------------------------------------------------------

bool
guiGl3Init(GuiOpenGLVersion version)
{
    ImGuiIO &io = ImGui::GetIO();
    IM_ASSERT(io.BackendRendererUserData == NULL && "Already initialized a renderer backend!");

    // Initialize our loader
#    if !defined(IMGUI_IMPL_OPENGL_LOADER_CUSTOM)
    if (imgl3wInit() != 0)
    {
        fprintf(stderr, "Failed to initialize OpenGL loader!\n");
        return false;
    }
#    endif

    // Setup backend capabilities flags
    GuiGl3BackendData *bd = IM_NEW(GuiGl3BackendData)();
    io.BackendRendererUserData = (void *)bd;
    io.BackendRendererName = "imgui_impl_opengl3";

    // Query for GL version (e.g. 320 for GL 3.2)
    GLint major = 0;
    GLint minor = 0;
    glGetIntegerv(GL_MAJOR_VERSION, &major);
    glGetIntegerv(GL_MINOR_VERSION, &minor);
    if (major == 0 && minor == 0)
    {
        CF_ASSERT_FAIL("Failed to query OpenGL version");
    }

    CF_ASSERT((GLint)version.major == major && (GLint)version.minor == minor,
              "Invalid OpenGL version");

    bd->GlVersion = (GLuint)(major * 100 + minor * 10);
    bd->GlslVersion = version.glsl;

    // We can honor the ImDrawCmd::VtxOffset field, allowing for large meshes.
    if (bd->GlVersion >= 320) io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;

    // We can create multi-viewports on the Renderer side (optional)
    io.BackendFlags |= ImGuiBackendFlags_RendererHasViewports;

    // Make an arbitrary GL call (we don't actually need the result)
    // IF YOU GET A CRASH HERE: it probably means the OpenGL function loader didn't do its job. Let
    // us know!
    GLint current_texture;
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &current_texture);

    // Detect extensions we support
    bd->has_clip_origin = (bd->GlVersion >= 450);

    GLint num_extensions = 0;
    glGetIntegerv(GL_NUM_EXTENSIONS, &num_extensions);
    for (GLint i = 0; i < num_extensions; i++)
    {
        const char *extension = (const char *)glGetStringi(GL_EXTENSIONS, (GLuint)i);
        if (extension != NULL && strcmp(extension, "GL_ARB_clip_control") == 0)
            bd->has_clip_origin = true;
    }

    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        ImGuiPlatformIO &platform_io = ImGui::GetPlatformIO();
        platform_io.Renderer_RenderWindow = guiGl3_RenderWindow;
    }

    return true;
}

void
guiGl3Shutdown()
{
    GuiGl3BackendData *bd = guiGl3_BackendData();
    IM_ASSERT(bd != NULL && "No renderer backend to shutdown, or already shutdown?");
    ImGuiIO &io = ImGui::GetIO();

    ImGui::DestroyPlatformWindows();
    guiGl3_DeleteDeviceObjects();
    io.BackendRendererName = NULL;
    io.BackendRendererUserData = NULL;
    IM_DELETE(bd);
}

void
guiGl3NewFrame()
{
    GuiGl3BackendData *bd = guiGl3_BackendData();
    IM_ASSERT(bd != NULL && "Did you call guiGl3Init()?");

    if (!bd->shader) guiGl3_CreateDeviceObjects();
}

void
guiGl3Render(GuiDrawData *draw_data)
{
    GuiGl3BackendData *bd = guiGl3_BackendData();

    // NOTE (Matteo): This is required both for backup and rendering
    glActiveTexture(GL_TEXTURE0);

    // Avoid rendering when minimized, scale coordinates for retina displays (screen coordinates !=
    // framebuffer coordinates)
    GLsizei fb_width = (GLsizei)(draw_data->DisplaySize.x * draw_data->FramebufferScale.x);
    GLsizei fb_height = (GLsizei)(draw_data->DisplaySize.y * draw_data->FramebufferScale.y);
    if (fb_width <= 0 || fb_height <= 0) return;

    // Setup desired GL state
    // Recreate the VAO every time (this is to easily allow multiple GL contexts to be rendered to.
    // VAO are not shared among GL contexts) The renderer would actually work without any VAO bound,
    // but then our VertexAttrib calls would overwrite the default one currently bound.
    GLuint vertex_array_object = 0;
    glGenVertexArrays(1, &vertex_array_object);
    guiGl3_SetupRenderState(draw_data, fb_width, fb_height, vertex_array_object);

    // Will project scissor/clipping rectangles into framebuffer space
    // (0,0) unless using multi-viewports
    ImVec2 clip_off = draw_data->DisplayPos;
    // (1,1) unless using retina display which are often (2,2)
    ImVec2 clip_scale = draw_data->FramebufferScale;

    // Render command lists
    for (int n = 0; n < draw_data->CmdListsCount; n++)
    {
        const ImDrawList *cmd_list = draw_data->CmdLists[n];

        // Upload vertex/index buffers
        GLsizeiptr vtx_buffer_size =
            (GLsizeiptr)cmd_list->VtxBuffer.Size * (GLint)sizeof(ImDrawVert);
        GLsizeiptr idx_buffer_size =
            (GLsizeiptr)cmd_list->IdxBuffer.Size * (GLint)sizeof(ImDrawIdx);
        if (bd->vbo_size < vtx_buffer_size)
        {
            bd->vbo_size = vtx_buffer_size;
            glBufferData(GL_ARRAY_BUFFER, bd->vbo_size, NULL, GL_STREAM_DRAW);
        }
        if (bd->ebo_size < idx_buffer_size)
        {
            bd->ebo_size = idx_buffer_size;
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, bd->ebo_size, NULL, GL_STREAM_DRAW);
        }
        glBufferSubData(GL_ARRAY_BUFFER, 0, vtx_buffer_size,
                        (const GLvoid *)cmd_list->VtxBuffer.Data);
        glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, idx_buffer_size,
                        (const GLvoid *)cmd_list->IdxBuffer.Data);

        for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++)
        {
            const ImDrawCmd *pcmd = &cmd_list->CmdBuffer[cmd_i];

            if (pcmd->UserCallback == ImDrawCallback_ResetRenderState)
            {
                // (ImDrawCallback_ResetRenderState is a special callback value used by the user to
                // request the renderer to reset render state.)
                guiGl3_SetupRenderState(draw_data, fb_width, fb_height, vertex_array_object);
            }
            else if (pcmd->UserCallback)
            {
                // User callback, registered via ImDrawList::AddCallback()
                pcmd->UserCallback(cmd_list, pcmd);
            }
            else
            {
                // Project scissor/clipping rectangles into framebuffer space
                ImVec2 clip_min((pcmd->ClipRect.x - clip_off.x) * clip_scale.x,
                                (pcmd->ClipRect.y - clip_off.y) * clip_scale.y);
                ImVec2 clip_max((pcmd->ClipRect.z - clip_off.x) * clip_scale.x,
                                (pcmd->ClipRect.w - clip_off.y) * clip_scale.y);
                if (clip_max.x <= clip_min.x || clip_max.y <= clip_min.y) continue;

                // Apply scissor/clipping rectangle (Y is inverted in OpenGL)
                glScissor((GLint)clip_min.x, (GLint)((F32)fb_height - clip_max.y),
                          (GLint)(clip_max.x - clip_min.x), (GLint)(clip_max.y - clip_min.y));

                // Bind texture, Draw
                Usize const idx_size = sizeof(ImDrawIdx);
                GLuint const tex = (GLuint)(Iptr)pcmd->GetTexID();

                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, tex);

                if (bd->GlVersion >= 320)
                {
                    glDrawElementsBaseVertex(GL_TRIANGLES, (GLsizei)pcmd->ElemCount,
                                             idx_size == 2 ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT,
                                             (void *)(Iptr)(pcmd->IdxOffset * idx_size),
                                             (GLint)pcmd->VtxOffset);
                }
                else
                {
                    glDrawElements(GL_TRIANGLES, (GLsizei)pcmd->ElemCount,
                                   idx_size == 2 ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT,
                                   (void *)(Iptr)(pcmd->IdxOffset * idx_size));
                }
            }
        }
    }

    // Destroy the temporary VAO
    glDeleteVertexArrays(1, &vertex_array_object);

    // Restore modified GL state
    // NOTE (Matteo): Unbind resources and disable ONLY explicitly enabled stuff
    gloadToggle(GL_BLEND, false);
    gloadToggle(GL_SCISSOR_TEST, false);
    glUseProgram(0);
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void
guiGl3UpdateFontsTexture()
{
    GuiGl3BackendData *bd = guiGl3_BackendData();

    if (bd->shader)
    {
        guiGl3_DeleteFontsTexture();
        guiGl3_CreateFontsTexture();
    }
}

#endif
