// Forked from Dear ImGui Renderer Backend for modern OpenGL with shaders / programmatic pipeline

//----------------------------------------
// OpenGL    GLSL      GLSL
// version   version   string
//----------------------------------------
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
//----------------------------------------

#include "gui_backend_gl3.h"

#include "foundation/core.h"
#include "foundation/memory.h"
#include "foundation/strings.h"

#include "imgui.h"

#include <stdio.h>

#define FONT_TEXTURE_RGBA 0

#if !defined(IMGUI_IMPL_OPENGL_LOADER_CUSTOM)
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
#    define IMGl3W_IMPL
#    include "imgui_impl_opengl3_loader.h"
#endif

// OpenGL Data
struct GuiGl3
{
    // Extracted at runtime using GL_MAJOR_VERSION, GL_MINOR_VERSION queries (e.g. 320 for GL 3.2)
    GLuint GlVersion;

    // Specified by user or detected based on compile time GL settings.
    char GlslVersionString[32];

    GLuint font_tex;
    GLuint shader;
    GLuint vbo;
    GLuint ebo;
    GLint AttribLocationTex; // Uniforms location
    GLint AttribLocationProjMtx;
    GLuint AttribLocationVtxPos; // Vertex attributes location
    GLuint AttribLocationVtxUV;
    GLuint AttribLocationVtxColor;

    bool HasClipOrigin;

    GuiGl3() { memClearStruct(this); }
};

struct GlRenderState
{
    GuiGl3 *bd;

    GLenum active_texture;
    GLuint shader;
    GLuint texture;
    GLuint vbo;
    GLuint vao;
    GLint polygon_mode[2];
    GLint viewport[4];
    GLint scissor_box[4];
    GLenum blend_src_rgb;
    GLenum blend_dst_rgb;
    GLenum blend_src_alpha;
    GLenum blend_dst_alpha;
    GLenum blend_equation_rgb;
    GLenum blend_equation_alpha;
    GLboolean enable_blend;
    GLboolean enable_cull_face;
    GLboolean enable_depth_test;
    GLboolean enable_stencil_test;
    GLboolean enable_scissor_test;
    // Desktop GL 3.1+ has GL_PRIMITIVE_RESTART state
    GLboolean enable_primitive_restart;
    // Desktop GL 3.3+ has glBindSampler()
    GLuint sampler;

    GlRenderState(GuiGl3 *bd)
    {
        glGetIntegerv(GL_ACTIVE_TEXTURE, (GLint *)&active_texture);
        glActiveTexture(GL_TEXTURE0);
        glGetIntegerv(GL_CURRENT_PROGRAM, (GLint *)&shader);
        glGetIntegerv(GL_TEXTURE_BINDING_2D, (GLint *)&texture);
        glGetIntegerv(GL_ARRAY_BUFFER_BINDING, (GLint *)&vbo);
        glGetIntegerv(GL_VERTEX_ARRAY_BINDING, (GLint *)&vao);
        glGetIntegerv(GL_POLYGON_MODE, polygon_mode);
        glGetIntegerv(GL_VIEWPORT, viewport);
        glGetIntegerv(GL_SCISSOR_BOX, scissor_box);
        glGetIntegerv(GL_BLEND_SRC_RGB, (GLint *)&blend_src_rgb);
        glGetIntegerv(GL_BLEND_DST_RGB, (GLint *)&blend_dst_rgb);
        glGetIntegerv(GL_BLEND_SRC_ALPHA, (GLint *)&blend_src_alpha);
        glGetIntegerv(GL_BLEND_DST_ALPHA, (GLint *)&blend_dst_alpha);
        glGetIntegerv(GL_BLEND_EQUATION_RGB, (GLint *)&blend_equation_rgb);
        glGetIntegerv(GL_BLEND_EQUATION_ALPHA, (GLint *)&blend_equation_alpha);
        enable_blend = glIsEnabled(GL_BLEND);
        enable_cull_face = glIsEnabled(GL_CULL_FACE);
        enable_depth_test = glIsEnabled(GL_DEPTH_TEST);
        enable_stencil_test = glIsEnabled(GL_STENCIL_TEST);
        enable_scissor_test = glIsEnabled(GL_SCISSOR_TEST);

#ifdef GL_VERSION_3_1
        enable_primitive_restart =
            (bd->GlVersion >= 310) ? glIsEnabled(GL_PRIMITIVE_RESTART) : GL_FALSE;
#endif

#if defined(GL_VERSION_3_3)
        if (bd->GlVersion >= 330)
        {
            glGetIntegerv(GL_SAMPLER_BINDING, (GLint *)&sampler);
        }
        else
        {
            sampler = 0;
        }
#endif
    }

    ~GlRenderState()
    {
        glUseProgram(shader);
        glBindTexture(GL_TEXTURE_2D, texture);

        glActiveTexture(active_texture);
        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBlendEquationSeparate(blend_equation_rgb, blend_equation_alpha);
        glBlendFuncSeparate(blend_src_rgb, blend_dst_rgb, blend_src_alpha, blend_dst_alpha);
        glPolygonMode(GL_FRONT, (GLenum)polygon_mode[0]);
        glPolygonMode(GL_BACK, (GLenum)polygon_mode[1]);
        glViewport(viewport[0], viewport[1], (GLsizei)viewport[2], (GLsizei)viewport[3]);
        glScissor(scissor_box[0], scissor_box[1], (GLsizei)scissor_box[2], (GLsizei)scissor_box[3]);

        // clang-format off
        if (enable_blend)        glEnable(GL_BLEND);        else glDisable(GL_BLEND);
        if (enable_cull_face)    glEnable(GL_CULL_FACE);    else glDisable(GL_CULL_FACE);
        if (enable_depth_test)   glEnable(GL_DEPTH_TEST);   else glDisable(GL_DEPTH_TEST);
        if (enable_stencil_test) glEnable(GL_STENCIL_TEST); else glDisable(GL_STENCIL_TEST);
        if (enable_scissor_test) glEnable(GL_SCISSOR_TEST); else glDisable(GL_SCISSOR_TEST);
            // clang-format on

#ifdef GL_VERSION_3_1
        if (bd->GlVersion >= 310)
        {
            if (enable_primitive_restart)
                glEnable(GL_PRIMITIVE_RESTART);
            else
                glDisable(GL_PRIMITIVE_RESTART);
        }
#endif

#if defined(GL_VERSION_3_3)
        if (bd->GlVersion >= 330) glBindSampler(0, sampler);
#endif
    }
};

// Backend data stored in io.BackendRendererUserData to allow support for multiple Dear ImGui
// contexts It is STRONGLY preferred that you use docking branch with multi-viewports (== single
// Dear ImGui context + multiple windows) instead of multiple Dear ImGui contexts.
static GuiGl3 *
guiGl3BackendData()
{
    return ImGui::GetCurrentContext() ? (GuiGl3 *)ImGui::GetIO().BackendRendererUserData : NULL;
}

// Functions

static void guiGl3RenderWindow(ImGuiViewport *viewport, void *);

bool
guiGl3Init(GlVersion version)
{
    ImGuiIO &io = ImGui::GetIO();
    CF_ASSERT(io.BackendRendererUserData == NULL, "Already initialized a renderer backend!");

    // Initialize our loader
#if !defined(IMGUI_IMPL_OPENGL_LOADER_CUSTOM)
    if (imgl3wInit() != 0)
    {
        fprintf(stderr, "Failed to initialize OpenGL loader!\n");
        return false;
    }
#endif

    // Setup backend capabilities flags
    GuiGl3 *bd = IM_NEW(GuiGl3)();
    io.BackendRendererUserData = (void *)bd;
    io.BackendRendererName = "guiGl3";

    // Query for GL version (e.g. 320 for GL 3.2)
    GLint major = 0;
    GLint minor = 0;
    glGetIntegerv(GL_MAJOR_VERSION, &major);
    glGetIntegerv(GL_MINOR_VERSION, &minor);
    if (major == 0 && minor == 0)
    {
        // Query GL_VERSION in desktop GL 2.x, the string will start with "<major>.<minor>"
        Cstr gl_version = (Cstr)glGetString(GL_VERSION);
        sscanf_s(gl_version, "%d.%d", &major, &minor);
    }
    bd->GlVersion = (GLuint)(major * 100 + minor * 10);

// Desktop GL 3.2+ has glDrawElementsBaseVertex()
#if defined(GL_VERSION_3_2)
    if (bd->GlVersion >= 320)
        io.BackendFlags |=
            ImGuiBackendFlags_RendererHasVtxOffset; // We can honor the ImDrawCmd::VtxOffset field,
                                                    // allowing for large meshes.
#endif
    io.BackendFlags |= ImGuiBackendFlags_RendererHasViewports; // We can create multi-viewports on
                                                               // the Renderer side (optional)

    // Store GLSL version string so we can refer to it later in case we recreate shaders.
    // Note: GLSL version is NOT the same as GL version. Leave this to NULL if unsure.
    if (version.glsl == NULL)
    {

#if defined(__APPLE__)
        version.glsl = "#version 150";
#else
        version.glsl = "#version 130";
#endif
    }

    Usize l = strLength(version.glsl);
    CF_ASSERT((l + 2) < CF_ARRAY_SIZE(bd->GlslVersionString), "");
    strncpy_s(bd->GlslVersionString, version.glsl, CF_ARRAY_SIZE(bd->GlslVersionString));
    strncat_s(bd->GlslVersionString, "\n", CF_ARRAY_SIZE(bd->GlslVersionString) - l);

    // Make an arbitrary GL call (we don't actually need the result)
    // IF YOU GET A CRASH HERE: it probably means the OpenGL function loader didn't do its job. Let
    // us know!
    GLint current_texture;
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &current_texture);

    // Detect extensions we support
    bd->HasClipOrigin = (bd->GlVersion >= 450);
    GLint num_extensions = 0;
    glGetIntegerv(GL_NUM_EXTENSIONS, &num_extensions);
    for (GLint i = 0; i < num_extensions; i++)
    {
        Cstr extension = (Cstr)glGetStringi(GL_EXTENSIONS, i);
        if (extension != NULL && strcmp(extension, "GL_ARB_clip_control") == 0)
        {
            bd->HasClipOrigin = true;
        }
    }

    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        ImGuiPlatformIO &platform_io = ImGui::GetPlatformIO();
        platform_io.Renderer_RenderWindow = guiGl3RenderWindow;
    }

    return true;
}

void
guiGl3Shutdown()
{
    GuiGl3 *bd = guiGl3BackendData();
    CF_ASSERT(bd != NULL, "No renderer backend to shutdown, or already shutdown?");
    ImGuiIO &io = ImGui::GetIO();

    ImGui::DestroyPlatformWindows();
    guiGl3DeleteDeviceObjects();
    io.BackendRendererName = NULL;
    io.BackendRendererUserData = NULL;
    IM_DELETE(bd);
}

void
guiGl3NewFrame()
{
    GuiGl3 *bd = guiGl3BackendData();
    CF_ASSERT(bd != NULL, "Did you call guiGl3Init()?");

    if (!bd->shader) guiGl3CreateDeviceObjects();
}

static void
guiGl3SetupRenderState(ImDrawData *draw_data, int fb_width, int fb_height, GLuint vao)
{
    GuiGl3 *bd = guiGl3BackendData();

    // Setup render state: alpha-blending enabled, no face culling, no depth testing, scissor
    // enabled, polygon fill
    glEnable(GL_BLEND);
    glBlendEquation(GL_FUNC_ADD);
    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_STENCIL_TEST);
    glEnable(GL_SCISSOR_TEST);
#ifdef GL_VERSION_3_1
    if (bd->GlVersion >= 310) glDisable(GL_PRIMITIVE_RESTART);
#endif
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    // Support for GL 4.5 rarely used glClipControl(GL_UPPER_LEFT)
#if defined(GL_CLIP_ORIGIN)
    bool clip_origin_lower_left = true;
    if (bd->HasClipOrigin)
    {
        GLenum current_clip_origin = 0;
        glGetIntegerv(GL_CLIP_ORIGIN, (GLint *)&current_clip_origin);
        if (current_clip_origin == GL_UPPER_LEFT) clip_origin_lower_left = false;
    }
#endif

    // Setup viewport, orthographic projection matrix
    // Our visible imgui space lies from draw_data->DisplayPos (top left) to
    // draw_data->DisplayPos+data_data->DisplaySize (bottom right). DisplayPos is (0,0) for single
    // viewport apps.
    glViewport(0, 0, (GLsizei)fb_width, (GLsizei)fb_height);
    float L = draw_data->DisplayPos.x;
    float R = draw_data->DisplayPos.x + draw_data->DisplaySize.x;
    float T = draw_data->DisplayPos.y;
    float B = draw_data->DisplayPos.y + draw_data->DisplaySize.y;
#if defined(GL_CLIP_ORIGIN)
    if (!clip_origin_lower_left)
    {
        float tmp = T;
        T = B;
        B = tmp;
    } // Swap top and bottom if origin is upper left
#endif
    const float ortho_projection[4][4] = {
        {2.0f / (R - L), 0.0f, 0.0f, 0.0f},
        {0.0f, 2.0f / (T - B), 0.0f, 0.0f},
        {0.0f, 0.0f, -1.0f, 0.0f},
        {(R + L) / (L - R), (T + B) / (B - T), 0.0f, 1.0f},
    };
    glUseProgram(bd->shader);
    glUniform1i(bd->AttribLocationTex, 0);
    glUniformMatrix4fv(bd->AttribLocationProjMtx, 1, GL_FALSE, &ortho_projection[0][0]);

#ifdef IMGUI_IMPL_OPENGL_MAY_HAVE_BIND_SAMPLER
    if (bd->GlVersion >= 330)
        glBindSampler(0, 0); // We use combined texture/sampler state. Applications using GL 3.3 may
                             // set that otherwise.
#endif

    glBindVertexArray(vao);

    // Bind vertex/index buffers and setup attributes for ImDrawVert
    glBindBuffer(GL_ARRAY_BUFFER, bd->vbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, bd->ebo);
    glEnableVertexAttribArray(bd->AttribLocationVtxPos);
    glEnableVertexAttribArray(bd->AttribLocationVtxUV);
    glEnableVertexAttribArray(bd->AttribLocationVtxColor);
    glVertexAttribPointer(bd->AttribLocationVtxPos, 2, GL_FLOAT, GL_FALSE, sizeof(ImDrawVert),
                          (GLvoid *)offsetof(ImDrawVert, pos));
    glVertexAttribPointer(bd->AttribLocationVtxUV, 2, GL_FLOAT, GL_FALSE, sizeof(ImDrawVert),
                          (GLvoid *)offsetof(ImDrawVert, uv));
    glVertexAttribPointer(bd->AttribLocationVtxColor, 4, GL_UNSIGNED_BYTE, GL_TRUE,
                          sizeof(ImDrawVert), (GLvoid *)offsetof(ImDrawVert, col));
}

// OpenGl3 Render function.
// Note that this implementation is little overcomplicated because we are saving/setting
// up/restoring every OpenGL state explicitly. This is in order to be able to run within an OpenGL
// engine that doesn't do so.
void
guiGl3Render(ImDrawData *draw_data)
{
    auto bd = guiGl3BackendData();

    // Backup and restore modified state at scope exit
    GlRenderState state{bd};

    // Avoid rendering when minimized, scale coordinates for retina displays (screen coordinates !=
    // framebuffer coordinates)
    int fb_width = (int)(draw_data->DisplaySize.x * draw_data->FramebufferScale.x);
    int fb_height = (int)(draw_data->DisplaySize.y * draw_data->FramebufferScale.y);
    if (fb_width <= 0 || fb_height <= 0) return;

    // Setup desired GL state
    // Recreate the VAO every time (this is to easily allow multiple GL contexts to be rendered to.
    // VAO are not shared among GL contexts) The renderer would actually work without any VAO bound,
    // but then our VertexAttrib calls would overwrite the default one currently bound.
    GLuint vao = 0;
    glGenVertexArrays(1, &vao);
    guiGl3SetupRenderState(draw_data, fb_width, fb_height, vao);

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
        glBufferData(GL_ARRAY_BUFFER,
                     (GLsizeiptr)cmd_list->VtxBuffer.Size * (int)sizeof(ImDrawVert),
                     (const GLvoid *)cmd_list->VtxBuffer.Data, GL_STREAM_DRAW);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                     (GLsizeiptr)cmd_list->IdxBuffer.Size * (int)sizeof(ImDrawIdx),
                     (const GLvoid *)cmd_list->IdxBuffer.Data, GL_STREAM_DRAW);

        for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++)
        {
            const ImDrawCmd *pcmd = &cmd_list->CmdBuffer[cmd_i];
            if (pcmd->UserCallback != NULL)
            {
                // User callback, registered via ImDrawList::AddCallback()
                // (ImDrawCallback_ResetRenderState is a special callback value used by the user to
                // request the renderer to reset render state.)
                if (pcmd->UserCallback == ImDrawCallback_ResetRenderState)
                    guiGl3SetupRenderState(draw_data, fb_width, fb_height, vao);
                else
                    pcmd->UserCallback(cmd_list, pcmd);
            }
            else
            {
                // Project scissor/clipping rectangles into framebuffer space
                ImVec2 clip_min((pcmd->ClipRect.x - clip_off.x) * clip_scale.x,
                                (pcmd->ClipRect.y - clip_off.y) * clip_scale.y);
                ImVec2 clip_max((pcmd->ClipRect.z - clip_off.x) * clip_scale.x,
                                (pcmd->ClipRect.w - clip_off.y) * clip_scale.y);
                if (clip_max.x < clip_min.x || clip_max.y < clip_min.y) continue;

                // Apply scissor/clipping rectangle (Y is inverted in OpenGL)
                glScissor((int)clip_min.x, (int)(fb_height - clip_max.y),
                          (int)(clip_max.x - clip_min.x), (int)(clip_max.y - clip_min.y));

                // Bind texture, Draw
                glBindTexture(GL_TEXTURE_2D, (GLuint)(intptr_t)pcmd->GetTexID());

// Desktop GL 3.2+ has glDrawElementsBaseVertex()
#if defined(GL_VERSION_3_2)
                if (bd->GlVersion >= 320)
                    glDrawElementsBaseVertex(
                        GL_TRIANGLES, (GLsizei)pcmd->ElemCount,
                        sizeof(ImDrawIdx) == 2 ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT,
                        (void *)(intptr_t)(pcmd->IdxOffset * sizeof(ImDrawIdx)),
                        (GLint)pcmd->VtxOffset);
                else
#endif
                    glDrawElements(GL_TRIANGLES, (GLsizei)pcmd->ElemCount,
                                   sizeof(ImDrawIdx) == 2 ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT,
                                   (void *)(intptr_t)(pcmd->IdxOffset * sizeof(ImDrawIdx)));
            }
        }
    }

    // Destroy the temporary VAO
    glDeleteVertexArrays(1, &vao);
}

void
guiGl3CreateFontsTexture()
{
    ImGuiIO &io = ImGui::GetIO();
    GuiGl3 *bd = guiGl3BackendData();

    // Upload texture to graphics system
    GLint last_texture;
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &last_texture);
    glGenTextures(1, &bd->font_tex);
    glBindTexture(GL_TEXTURE_2D, bd->font_tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

#ifdef GL_UNPACK_ROW_LENGTH // Not on WebGL/ES
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
#endif

    // Build texture atlas
    U8 *pixels;
    I32 width, height;
#if FONT_TEXTURE_RGBA
    // Load as RGBA 32-bit (75% of the memory is wasted, but default font is so small)
    // because it is more likely to be compatible with user's existing shaders. If
    // your ImTextureId represent a higher-level concept than just a GL texture id,
    // consider calling GetTexDataAsAlpha8() instead to save on GPU memory.
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
#else
    io.Fonts->GetTexDataAsAlpha8(&pixels, &width, &height);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, width, height, 0, GL_RED, GL_UNSIGNED_BYTE, pixels);
#endif

    // Store our identifier
    io.Fonts->SetTexID((ImTextureID)(intptr_t)bd->font_tex);

    // Restore state
    glBindTexture(GL_TEXTURE_2D, last_texture);
}

void
guiGl3DeleteFontsTexture()
{
    ImGuiIO &io = ImGui::GetIO();
    GuiGl3 *bd = guiGl3BackendData();
    if (bd->font_tex)
    {
        glDeleteTextures(1, &bd->font_tex);
        io.Fonts->SetTexID(0);
        bd->font_tex = 0;
    }
}

// If you get an error please report on github. You may try different GL context version or GLSL
// version. See GL<>GLSL version table at the top of this file.
static bool
CheckShader(GLuint handle, Cstr desc)
{
    GuiGl3 *bd = guiGl3BackendData();
    GLint status = 0, log_length = 0;
    glGetShaderiv(handle, GL_COMPILE_STATUS, &status);
    glGetShaderiv(handle, GL_INFO_LOG_LENGTH, &log_length);
    if ((GLboolean)status == GL_FALSE)
        fprintf(stderr, "ERROR: guiGl3CreateDeviceObjects: failed to compile %s! With GLSL: %s\n",
                desc, bd->GlslVersionString);
    if (log_length > 1)
    {
        ImVector<char> buf;
        buf.resize((int)(log_length + 1));
        glGetShaderInfoLog(handle, log_length, NULL, (GLchar *)buf.begin());
        fprintf(stderr, "%s\n", buf.begin());
    }
    return (GLboolean)status == GL_TRUE;
}

// If you get an error please report on GitHub. You may try different GL context version or GLSL
// version.
static bool
guiGl3CheckProgram(GLuint handle, Cstr desc)
{
    GuiGl3 *bd = guiGl3BackendData();
    GLint status = 0, log_length = 0;
    glGetProgramiv(handle, GL_LINK_STATUS, &status);
    glGetProgramiv(handle, GL_INFO_LOG_LENGTH, &log_length);
    if ((GLboolean)status == GL_FALSE)
        fprintf(stderr, "ERROR: guiGl3CreateDeviceObjects: failed to link %s! With GLSL %s\n", desc,
                bd->GlslVersionString);
    if (log_length > 1)
    {
        ImVector<char> buf;
        buf.resize((int)(log_length + 1));
        glGetProgramInfoLog(handle, log_length, NULL, (GLchar *)buf.begin());
        fprintf(stderr, "%s\n", buf.begin());
    }
    return (GLboolean)status == GL_TRUE;
}

bool
guiGl3CreateDeviceObjects()
{
    GuiGl3 *bd = guiGl3BackendData();

    // Backup GL state
    GLint last_texture, last_vbo, last_vao;
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &last_texture);
    glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &last_vbo);
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &last_vao);

    // Parse GLSL version string
    int glsl_version = 130;
    sscanf_s(bd->GlslVersionString, "#version %d", &glsl_version);

    const GLchar *vertex_shader_glsl_130 = //
        "uniform mat4 ProjMtx;\n"
        "in vec2 Position;\n"
        "in vec2 UV;\n"
        "in vec4 Color;\n"
        "out vec2 Frag_UV;\n"
        "out vec4 Frag_Color;\n"
        "void main()\n"
        "{\n"
        "    Frag_UV = UV;\n"
        "    Frag_Color = Color;\n"
        "    gl_Position = ProjMtx * vec4(Position.xy,0,1);\n"
        "}\n";

    const GLchar *vertex_shader_glsl_410_core = //
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

    const GLchar *fragment_shader_glsl_130 = //
        "uniform sampler2D Texture;\n"
        "in vec2 Frag_UV;\n"
        "in vec4 Frag_Color;\n"
        "out vec4 Out_Color;\n"
        "void main()\n"
        "{\n"
#if FONT_TEXTURE_BYTE
        "    vec4 sampled = vec4(1.0, 1.0, 1.0, texture(Texture, Frag_UV.st).r);"
        "    Out_Color = Frag_Color * sampled;\n"
#else
        "    Out_Color = Frag_Color * texture(Texture, Frag_UV.st);\n"
#endif
        "}\n";

    // Select shaders matching our GLSL versions
    const GLchar *vertex_shader = NULL;
    const GLchar *fragment_shader = NULL;
    if (glsl_version < 130)
    {
        CF_ASSERT_FAIL("GLSL version must be >= 130");
    }
    else
    {
        vertex_shader =
            (glsl_version >= 410) ? vertex_shader_glsl_410_core : vertex_shader_glsl_130;
        fragment_shader = fragment_shader_glsl_130;
    }

    // Create shaders
    const GLchar *vertex_shader_with_version[2] = {bd->GlslVersionString, vertex_shader};
    GLuint vert_handle = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vert_handle, 2, vertex_shader_with_version, NULL);
    glCompileShader(vert_handle);
    CheckShader(vert_handle, "vertex shader");

    const GLchar *fragment_shader_with_version[2] = {bd->GlslVersionString, fragment_shader};
    GLuint frag_handle = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(frag_handle, 2, fragment_shader_with_version, NULL);
    glCompileShader(frag_handle);
    CheckShader(frag_handle, "fragment shader");

    // Link
    bd->shader = glCreateProgram();
    glAttachShader(bd->shader, vert_handle);
    glAttachShader(bd->shader, frag_handle);
    glLinkProgram(bd->shader);
    guiGl3CheckProgram(bd->shader, "shader program");

    glDetachShader(bd->shader, vert_handle);
    glDetachShader(bd->shader, frag_handle);
    glDeleteShader(vert_handle);
    glDeleteShader(frag_handle);

    bd->AttribLocationTex = glGetUniformLocation(bd->shader, "Texture");
    bd->AttribLocationProjMtx = glGetUniformLocation(bd->shader, "ProjMtx");
    bd->AttribLocationVtxPos = (GLuint)glGetAttribLocation(bd->shader, "Position");
    bd->AttribLocationVtxUV = (GLuint)glGetAttribLocation(bd->shader, "UV");
    bd->AttribLocationVtxColor = (GLuint)glGetAttribLocation(bd->shader, "Color");

    // Create buffers
    glGenBuffers(1, &bd->vbo);
    glGenBuffers(1, &bd->ebo);

    guiGl3CreateFontsTexture();

    // Restore modified GL state
    glBindTexture(GL_TEXTURE_2D, last_texture);
    glBindBuffer(GL_ARRAY_BUFFER, last_vbo);
    glBindVertexArray(last_vao);

    return true;
}

void
guiGl3DeleteDeviceObjects()
{
    GuiGl3 *bd = guiGl3BackendData();
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
    guiGl3DeleteFontsTexture();
}

//--------------------------------------------------------------------------------------------------------
// MULTI-VIEWPORT / PLATFORM INTERFACE SUPPORT
// This is an _advanced_ and _optional_ feature, allowing the backend to create and handle multiple
// viewports simultaneously. If you are new to dear imgui or creating a new binding for dear imgui,
// it is recommended that you completely ignore this section first..
//--------------------------------------------------------------------------------------------------------

static void
guiGl3RenderWindow(ImGuiViewport *viewport, void *)
{
    if (!(viewport->Flags & ImGuiViewportFlags_NoRendererClear))
    {
        glClearColor(0, 0, 0, 1);
        glClear(GL_COLOR_BUFFER_BIT);
    }
    guiGl3Render(viewport->DrawData);
}
