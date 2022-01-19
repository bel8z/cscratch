#if !defined(SHADER_DECL)

//===============//
//   Interface   //
//===============//

#    include "foundation/core.h"
#    include "foundation/io.h"

#    if defined(SHADER_STATIC)
#        define SHADER_API static
#    else
#        define SHADER_API extern
#    endif

#    define INVALID_UNIFORM -1

typedef struct CfLog CfLog;

typedef struct Shader
{
    U32 program;
} Shader;

SHADER_API void shaderBind(Shader shader);
SHADER_API void shaderClear(void);

SHADER_API Shader shaderLoadFiles(IoFileContent vtx, IoFileContent pix, CfLog *log);
SHADER_API Shader shaderLoadStrings(Str vtx, Str pix, CfLog *log);
SHADER_API void shaderUnload(Shader *shader);

SHADER_API I32 shaderGetUniform(Shader shader, Cstr uniform_name);

#    define SHADER_DECL
#endif

//====================//
//   Implementation   //
//====================//

#if defined SHADER_IMPL

#    include "foundation/log.h"
#    include "foundation/strings.h"

#    include "gl/gload.h"

#    if defined SHADER_STATIC
CF_DIAGNOSTIC_PUSH()
CF_DIAGNOSTIC_IGNORE_CLANG("-Wunused-function")
#    endif

void
shaderBind(Shader shader)
{
    glUseProgram(shader.program);
}

void
shaderClear(void)
{
    glUseProgram(0);
}

Shader
shaderLoadStrings(Str vtx, Str pix, CfLog *log)
{
    Shader shader = {0};

    I32 success;

    cfLogAppendC(log, "Compiling vertex shader...");

    I32 const vtx_len = (I32)vtx.len;
    U32 const vtx_shader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vtx_shader, 1, &vtx.buf, &vtx_len);
    glCompileShader(vtx_shader);
    glGetShaderiv(vtx_shader, GL_COMPILE_STATUS, &success);
    if (success)
    {
        cfLogAppendC(log, "SUCCEEDED\n");
    }
    else
    {
        cfLogAppendC(log, "FAILED\n");
        Char8 info_log[512];
        glGetShaderInfoLog(vtx_shader, CF_ARRAY_SIZE(info_log), NULL, info_log);
        cfLogAppendF(log, "Vertex shader compilation error: %s\n", info_log);
    }

    cfLogAppendC(log, "Compiling pixel shader...");

    I32 const pix_len = (I32)pix.len;
    U32 const pix_shader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(pix_shader, 1, &pix.buf, &pix_len);
    glCompileShader(pix_shader);
    glGetShaderiv(pix_shader, GL_COMPILE_STATUS, &success);
    if (success)
    {
        cfLogAppendC(log, "SUCCEEDED\n");
    }
    else
    {
        cfLogAppendC(log, "FAILED\n");
        Char8 info_log[512];
        glGetShaderInfoLog(pix_shader, CF_ARRAY_SIZE(info_log), NULL, info_log);
        cfLogAppendF(log, "Pixel shader compilation error: %s\n", info_log);
    }

    cfLogAppendC(log, "Linking shader program...");

    shader.program = glCreateProgram();
    glAttachShader(shader.program, vtx_shader);
    glAttachShader(shader.program, pix_shader);
    glLinkProgram(shader.program);
    glGetProgramiv(shader.program, GL_LINK_STATUS, &success);
    if (success)
    {
        cfLogAppendC(log, "SUCCEEDED\n");
    }
    else
    {
        cfLogAppendC(log, "FAILED\n");
        Char8 info_log[512];
        glGetProgramInfoLog(pix_shader, CF_ARRAY_SIZE(info_log), NULL, info_log);
        cfLogAppendF(log, "Shader program link error: %s\n", info_log);
    }

    glDeleteShader(vtx_shader);
    glDeleteShader(pix_shader);

    return shader;
}

Shader
shaderLoadFiles(IoFileContent vtx, IoFileContent pix, CfLog *log)
{
    return shaderLoadStrings((Str){.buf = (Char8 *)vtx.data, .len = vtx.size},
                             (Str){.buf = (Char8 *)pix.data, .len = pix.size}, //
                             log);
}

void
shaderUnload(Shader *shader)
{
    glDeleteProgram(shader->program);
    shader->program = 0;
}

I32
shaderGetUniform(Shader shader, Cstr uniform_name)
{
    return glGetUniformLocation(shader.program, uniform_name);
}

#    if defined SHADER_STATIC
CF_DIAGNOSTIC_POP()
#    endif

#endif
