#pragma once

#include "shader.h"

#include "foundation/core.h"
#include "foundation/fs.h"
#include "foundation/strings.h"

#include "gl/gload.h"

typedef struct Shader
{
    U32 program;
} Shader;

Shader shaderLoadFiles(FileContent vtx, FileContent pix);
Shader shaderLoadStrings(Str vtx, Str pix);
void shaderBegin(Shader shader);
void shaderEnd(void);

Shader
shaderLoadStrings(Str vtx, Str pix)
{
    Shader shader = {0};

    I32 success;

    I32 const vtx_len = (I32)vtx.len;
    U32 const vtx_shader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vtx_shader, 1, &vtx.buf, &vtx_len);
    glCompileShader(vtx_shader);
    glGetShaderiv(vtx_shader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        Char8 info_log[512];
        glGetShaderInfoLog(vtx_shader, CF_ARRAY_SIZE(info_log), NULL, info_log);
        appLog("Vertex shader compilation error: %s\n", info_log);
    }

    I32 const pix_len = (I32)pix.len;
    U32 const pix_shader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(pix_shader, 1, &pix.buf, &pix_len);
    glCompileShader(pix_shader);
    glGetShaderiv(pix_shader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        Char8 info_log[512];
        glGetShaderInfoLog(pix_shader, CF_ARRAY_SIZE(info_log), NULL, info_log);
        appLog("Pixel shader compilation error: %s\n", info_log);
    }

    shader.program = glCreateProgram();
    glAttachShader(shader.program, vtx_shader);
    glAttachShader(shader.program, pix_shader);
    glLinkProgram(shader.program);
    glGetProgramiv(shader.program, GL_LINK_STATUS, &success);
    if (!success)
    {
        Char8 info_log[512];
        glGetProgramInfoLog(pix_shader, CF_ARRAY_SIZE(info_log), NULL, info_log);
        appLog("Shader program link error: %s\n", info_log);
    }

    glDeleteShader(vtx_shader);
    glDeleteShader(pix_shader);

    return shader;
}

Shader
shaderLoadFiles(FileContent vtx, FileContent pix)
{
    return shaderLoadStrings((Str){.buf = (Char8 *)vtx.data, .len = vtx.size},
                             (Str){.buf = (Char8 *)pix.data, .len = pix.size});
}

void
shaderBegin(Shader shader)
{
    glUseProgram(shader.program);
}

void
shaderEnd(void)
{
    glUseProgram(0);
}
