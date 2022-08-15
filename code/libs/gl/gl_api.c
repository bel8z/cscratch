#include "gl_api.h"

#include "foundation/core.h"
#include "foundation/error.h"

//------------------------------------------------------------------------------
// Internal functions and data
//------------------------------------------------------------------------------

CF_INTERNAL int gl__ParseVersion(void);
CF_INTERNAL void gl__LoadProcs(GlApi *api, GlApiLoadFn *fn);

static GlApi g_api = {0};

static struct
{
    GLint major, minor;
} version;

//------------------------------------------------------------------------------
// Api
//------------------------------------------------------------------------------

GlApi *gl = &g_api;

bool
glApiLoad(GlApiLoadFn *fn)
{
    if (!fn) return false;

    gl = &g_api;
    gl__LoadProcs(gl, fn);

    // NOTE (Matteo): Reference for Win32
    // void *res = (void *)wglGetProcAddress(proc);
    // if (!res) res = (void *)GetProcAddress(libgl, proc);
    // return res;

    return gl__ParseVersion();
}

bool
glApiSet(GlApi *api)
{
    if (!api) return false;
    gl = api;
    return gl__ParseVersion();
}

bool
glApiIsSupported(U32 major, U32 minor)
{
    if (major < 3) return 0;
    if ((U32)version.major == major) return (U32)version.minor >= minor;
    return (U32)version.major >= major;
}

void
glApiToggle(GLenum cap, bool enable)
{
    if (enable)
    {
        glEnable(cap);
    }
    else
    {
        glDisable(cap);
    }
}

//------------------------------------------------------------------------------
// Internal implementation
//------------------------------------------------------------------------------

int
gl__ParseVersion(void)
{
    if (!glGetIntegerv) return 0;

    glGetIntegerv(GL_MAJOR_VERSION, &version.major);
    glGetIntegerv(GL_MINOR_VERSION, &version.minor);

    if (version.major < 3) return 0;

    return 1;
}

void
gl__LoadProcs(GlApi *api, GlApiLoadFn *fn)
{
    api->CullFace = (PFNGLCULLFACEPROC)fn("glCullFace");
    api->FrontFace = (PFNGLFRONTFACEPROC)fn("glFrontFace");
    api->Hint = (PFNGLHINTPROC)fn("glHint");
    api->LineWidth = (PFNGLLINEWIDTHPROC)fn("glLineWidth");
    api->PointSize = (PFNGLPOINTSIZEPROC)fn("glPointSize");
    api->PolygonMode = (PFNGLPOLYGONMODEPROC)fn("glPolygonMode");
    api->Scissor = (PFNGLSCISSORPROC)fn("glScissor");
    api->TexParameterf = (PFNGLTEXPARAMETERFPROC)fn("glTexParameterf");
    api->TexParameterfv = (PFNGLTEXPARAMETERFVPROC)fn("glTexParameterfv");
    api->TexParameteri = (PFNGLTEXPARAMETERIPROC)fn("glTexParameteri");
    api->TexParameteriv = (PFNGLTEXPARAMETERIVPROC)fn("glTexParameteriv");
    api->TexImage1D = (PFNGLTEXIMAGE1DPROC)fn("glTexImage1D");
    api->TexImage2D = (PFNGLTEXIMAGE2DPROC)fn("glTexImage2D");
    api->DrawBuffer = (PFNGLDRAWBUFFERPROC)fn("glDrawBuffer");
    api->Clear = (PFNGLCLEARPROC)fn("glClear");
    api->ClearColor = (PFNGLCLEARCOLORPROC)fn("glClearColor");
    api->ClearStencil = (PFNGLCLEARSTENCILPROC)fn("glClearStencil");
    api->ClearDepth = (PFNGLCLEARDEPTHPROC)fn("glClearDepth");
    api->StencilMask = (PFNGLSTENCILMASKPROC)fn("glStencilMask");
    api->ColorMask = (PFNGLCOLORMASKPROC)fn("glColorMask");
    api->DepthMask = (PFNGLDEPTHMASKPROC)fn("glDepthMask");
    api->Disable = (PFNGLDISABLEPROC)fn("glDisable");
    api->Enable = (PFNGLENABLEPROC)fn("glEnable");
    api->Finish = (PFNGLFINISHPROC)fn("glFinish");
    api->Flush = (PFNGLFLUSHPROC)fn("glFlush");
    api->BlendFunc = (PFNGLBLENDFUNCPROC)fn("glBlendFunc");
    api->LogicOp = (PFNGLLOGICOPPROC)fn("glLogicOp");
    api->StencilFunc = (PFNGLSTENCILFUNCPROC)fn("glStencilFunc");
    api->StencilOp = (PFNGLSTENCILOPPROC)fn("glStencilOp");
    api->DepthFunc = (PFNGLDEPTHFUNCPROC)fn("glDepthFunc");
    api->PixelStoref = (PFNGLPIXELSTOREFPROC)fn("glPixelStoref");
    api->PixelStorei = (PFNGLPIXELSTOREIPROC)fn("glPixelStorei");
    api->ReadBuffer = (PFNGLREADBUFFERPROC)fn("glReadBuffer");
    api->ReadPixels = (PFNGLREADPIXELSPROC)fn("glReadPixels");
    api->GetBooleanv = (PFNGLGETBOOLEANVPROC)fn("glGetBooleanv");
    api->GetDoublev = (PFNGLGETDOUBLEVPROC)fn("glGetDoublev");
    api->GetError = (PFNGLGETERRORPROC)fn("glGetError");
    api->GetFloatv = (PFNGLGETFLOATVPROC)fn("glGetFloatv");
    api->GetIntegerv = (PFNGLGETINTEGERVPROC)fn("glGetIntegerv");
    api->GetString = (PFNGLGETSTRINGPROC)fn("glGetString");
    api->GetTexImage = (PFNGLGETTEXIMAGEPROC)fn("glGetTexImage");
    api->GetTexParameterfv = (PFNGLGETTEXPARAMETERFVPROC)fn("glGetTexParameterfv");
    api->GetTexParameteriv = (PFNGLGETTEXPARAMETERIVPROC)fn("glGetTexParameteriv");
    api->GetTexLevelParameterfv = (PFNGLGETTEXLEVELPARAMETERFVPROC)fn("glGetTexLevelParameterfv");
    api->GetTexLevelParameteriv = (PFNGLGETTEXLEVELPARAMETERIVPROC)fn("glGetTexLevelParameteriv");
    api->IsEnabled = (PFNGLISENABLEDPROC)fn("glIsEnabled");
    api->DepthRange = (PFNGLDEPTHRANGEPROC)fn("glDepthRange");
    api->Viewport = (PFNGLVIEWPORTPROC)fn("glViewport");
    api->DrawArrays = (PFNGLDRAWARRAYSPROC)fn("glDrawArrays");
    api->DrawElements = (PFNGLDRAWELEMENTSPROC)fn("glDrawElements");
    api->GetPointerv = (PFNGLGETPOINTERVPROC)fn("glGetPointerv");
    api->PolygonOffset = (PFNGLPOLYGONOFFSETPROC)fn("glPolygonOffset");
    api->CopyTexImage1D = (PFNGLCOPYTEXIMAGE1DPROC)fn("glCopyTexImage1D");
    api->CopyTexImage2D = (PFNGLCOPYTEXIMAGE2DPROC)fn("glCopyTexImage2D");
    api->CopyTexSubImage1D = (PFNGLCOPYTEXSUBIMAGE1DPROC)fn("glCopyTexSubImage1D");
    api->CopyTexSubImage2D = (PFNGLCOPYTEXSUBIMAGE2DPROC)fn("glCopyTexSubImage2D");
    api->TexSubImage1D = (PFNGLTEXSUBIMAGE1DPROC)fn("glTexSubImage1D");
    api->TexSubImage2D = (PFNGLTEXSUBIMAGE2DPROC)fn("glTexSubImage2D");
    api->BindTexture = (PFNGLBINDTEXTUREPROC)fn("glBindTexture");
    api->DeleteTextures = (PFNGLDELETETEXTURESPROC)fn("glDeleteTextures");
    api->GenTextures = (PFNGLGENTEXTURESPROC)fn("glGenTextures");
    api->IsTexture = (PFNGLISTEXTUREPROC)fn("glIsTexture");
    api->BlendColor = (PFNGLBLENDCOLORPROC)fn("glBlendColor");
    api->BlendEquation = (PFNGLBLENDEQUATIONPROC)fn("glBlendEquation");
    api->DrawRangeElements = (PFNGLDRAWRANGEELEMENTSPROC)fn("glDrawRangeElements");
    api->TexImage3D = (PFNGLTEXIMAGE3DPROC)fn("glTexImage3D");
    api->TexSubImage3D = (PFNGLTEXSUBIMAGE3DPROC)fn("glTexSubImage3D");
    api->CopyTexSubImage3D = (PFNGLCOPYTEXSUBIMAGE3DPROC)fn("glCopyTexSubImage3D");
    api->ActiveTexture = (PFNGLACTIVETEXTUREPROC)fn("glActiveTexture");
    api->SampleCoverage = (PFNGLSAMPLECOVERAGEPROC)fn("glSampleCoverage");
    api->CompressedTexImage3D = (PFNGLCOMPRESSEDTEXIMAGE3DPROC)fn("glCompressedTexImage3D");
    api->CompressedTexImage2D = (PFNGLCOMPRESSEDTEXIMAGE2DPROC)fn("glCompressedTexImage2D");
    api->CompressedTexImage1D = (PFNGLCOMPRESSEDTEXIMAGE1DPROC)fn("glCompressedTexImage1D");
    api->CompressedTexSubImage3D =
        (PFNGLCOMPRESSEDTEXSUBIMAGE3DPROC)fn("glCompressedTexSubImage3D");
    api->CompressedTexSubImage2D =
        (PFNGLCOMPRESSEDTEXSUBIMAGE2DPROC)fn("glCompressedTexSubImage2D");
    api->CompressedTexSubImage1D =
        (PFNGLCOMPRESSEDTEXSUBIMAGE1DPROC)fn("glCompressedTexSubImage1D");
    api->GetCompressedTexImage = (PFNGLGETCOMPRESSEDTEXIMAGEPROC)fn("glGetCompressedTexImage");
    api->BlendFuncSeparate = (PFNGLBLENDFUNCSEPARATEPROC)fn("glBlendFuncSeparate");
    api->MultiDrawArrays = (PFNGLMULTIDRAWARRAYSPROC)fn("glMultiDrawArrays");
    api->MultiDrawElements = (PFNGLMULTIDRAWELEMENTSPROC)fn("glMultiDrawElements");
    api->PointParameterf = (PFNGLPOINTPARAMETERFPROC)fn("glPointParameterf");
    api->PointParameterfv = (PFNGLPOINTPARAMETERFVPROC)fn("glPointParameterfv");
    api->PointParameteri = (PFNGLPOINTPARAMETERIPROC)fn("glPointParameteri");
    api->PointParameteriv = (PFNGLPOINTPARAMETERIVPROC)fn("glPointParameteriv");
    api->GenQueries = (PFNGLGENQUERIESPROC)fn("glGenQueries");
    api->DeleteQueries = (PFNGLDELETEQUERIESPROC)fn("glDeleteQueries");
    api->IsQuery = (PFNGLISQUERYPROC)fn("glIsQuery");
    api->BeginQuery = (PFNGLBEGINQUERYPROC)fn("glBeginQuery");
    api->EndQuery = (PFNGLENDQUERYPROC)fn("glEndQuery");
    api->GetQueryiv = (PFNGLGETQUERYIVPROC)fn("glGetQueryiv");
    api->GetQueryObjectiv = (PFNGLGETQUERYOBJECTIVPROC)fn("glGetQueryObjectiv");
    api->GetQueryObjectuiv = (PFNGLGETQUERYOBJECTUIVPROC)fn("glGetQueryObjectuiv");
    api->BindBuffer = (PFNGLBINDBUFFERPROC)fn("glBindBuffer");
    api->DeleteBuffers = (PFNGLDELETEBUFFERSPROC)fn("glDeleteBuffers");
    api->GenBuffers = (PFNGLGENBUFFERSPROC)fn("glGenBuffers");
    api->IsBuffer = (PFNGLISBUFFERPROC)fn("glIsBuffer");
    api->BufferData = (PFNGLBUFFERDATAPROC)fn("glBufferData");
    api->BufferSubData = (PFNGLBUFFERSUBDATAPROC)fn("glBufferSubData");
    api->GetBufferSubData = (PFNGLGETBUFFERSUBDATAPROC)fn("glGetBufferSubData");
    api->MapBuffer = (PFNGLMAPBUFFERPROC)fn("glMapBuffer");
    api->UnmapBuffer = (PFNGLUNMAPBUFFERPROC)fn("glUnmapBuffer");
    api->GetBufferParameteriv = (PFNGLGETBUFFERPARAMETERIVPROC)fn("glGetBufferParameteriv");
    api->GetBufferPointerv = (PFNGLGETBUFFERPOINTERVPROC)fn("glGetBufferPointerv");
    api->BlendEquationSeparate = (PFNGLBLENDEQUATIONSEPARATEPROC)fn("glBlendEquationSeparate");
    api->DrawBuffers = (PFNGLDRAWBUFFERSPROC)fn("glDrawBuffers");
    api->StencilOpSeparate = (PFNGLSTENCILOPSEPARATEPROC)fn("glStencilOpSeparate");
    api->StencilFuncSeparate = (PFNGLSTENCILFUNCSEPARATEPROC)fn("glStencilFuncSeparate");
    api->StencilMaskSeparate = (PFNGLSTENCILMASKSEPARATEPROC)fn("glStencilMaskSeparate");
    api->AttachShader = (PFNGLATTACHSHADERPROC)fn("glAttachShader");
    api->BindAttribLocation = (PFNGLBINDATTRIBLOCATIONPROC)fn("glBindAttribLocation");
    api->CompileShader = (PFNGLCOMPILESHADERPROC)fn("glCompileShader");
    api->CreateProgram = (PFNGLCREATEPROGRAMPROC)fn("glCreateProgram");
    api->CreateShader = (PFNGLCREATESHADERPROC)fn("glCreateShader");
    api->DeleteProgram = (PFNGLDELETEPROGRAMPROC)fn("glDeleteProgram");
    api->DeleteShader = (PFNGLDELETESHADERPROC)fn("glDeleteShader");
    api->DetachShader = (PFNGLDETACHSHADERPROC)fn("glDetachShader");
    api->DisableVertexAttribArray =
        (PFNGLDISABLEVERTEXATTRIBARRAYPROC)fn("glDisableVertexAttribArray");
    api->EnableVertexAttribArray =
        (PFNGLENABLEVERTEXATTRIBARRAYPROC)fn("glEnableVertexAttribArray");
    api->GetActiveAttrib = (PFNGLGETACTIVEATTRIBPROC)fn("glGetActiveAttrib");
    api->GetActiveUniform = (PFNGLGETACTIVEUNIFORMPROC)fn("glGetActiveUniform");
    api->GetAttachedShaders = (PFNGLGETATTACHEDSHADERSPROC)fn("glGetAttachedShaders");
    api->GetAttribLocation = (PFNGLGETATTRIBLOCATIONPROC)fn("glGetAttribLocation");
    api->GetProgramiv = (PFNGLGETPROGRAMIVPROC)fn("glGetProgramiv");
    api->GetProgramInfoLog = (PFNGLGETPROGRAMINFOLOGPROC)fn("glGetProgramInfoLog");
    api->GetShaderiv = (PFNGLGETSHADERIVPROC)fn("glGetShaderiv");
    api->GetShaderInfoLog = (PFNGLGETSHADERINFOLOGPROC)fn("glGetShaderInfoLog");
    api->GetShaderSource = (PFNGLGETSHADERSOURCEPROC)fn("glGetShaderSource");
    api->GetUniformLocation = (PFNGLGETUNIFORMLOCATIONPROC)fn("glGetUniformLocation");
    api->GetUniformfv = (PFNGLGETUNIFORMFVPROC)fn("glGetUniformfv");
    api->GetUniformiv = (PFNGLGETUNIFORMIVPROC)fn("glGetUniformiv");
    api->GetVertexAttribdv = (PFNGLGETVERTEXATTRIBDVPROC)fn("glGetVertexAttribdv");
    api->GetVertexAttribfv = (PFNGLGETVERTEXATTRIBFVPROC)fn("glGetVertexAttribfv");
    api->GetVertexAttribiv = (PFNGLGETVERTEXATTRIBIVPROC)fn("glGetVertexAttribiv");
    api->GetVertexAttribPointerv =
        (PFNGLGETVERTEXATTRIBPOINTERVPROC)fn("glGetVertexAttribPointerv");
    api->IsProgram = (PFNGLISPROGRAMPROC)fn("glIsProgram");
    api->IsShader = (PFNGLISSHADERPROC)fn("glIsShader");
    api->LinkProgram = (PFNGLLINKPROGRAMPROC)fn("glLinkProgram");
    api->ShaderSource = (PFNGLSHADERSOURCEPROC)fn("glShaderSource");
    api->UseProgram = (PFNGLUSEPROGRAMPROC)fn("glUseProgram");
    api->Uniform1f = (PFNGLUNIFORM1FPROC)fn("glUniform1f");
    api->Uniform2f = (PFNGLUNIFORM2FPROC)fn("glUniform2f");
    api->Uniform3f = (PFNGLUNIFORM3FPROC)fn("glUniform3f");
    api->Uniform4f = (PFNGLUNIFORM4FPROC)fn("glUniform4f");
    api->Uniform1i = (PFNGLUNIFORM1IPROC)fn("glUniform1i");
    api->Uniform2i = (PFNGLUNIFORM2IPROC)fn("glUniform2i");
    api->Uniform3i = (PFNGLUNIFORM3IPROC)fn("glUniform3i");
    api->Uniform4i = (PFNGLUNIFORM4IPROC)fn("glUniform4i");
    api->Uniform1fv = (PFNGLUNIFORM1FVPROC)fn("glUniform1fv");
    api->Uniform2fv = (PFNGLUNIFORM2FVPROC)fn("glUniform2fv");
    api->Uniform3fv = (PFNGLUNIFORM3FVPROC)fn("glUniform3fv");
    api->Uniform4fv = (PFNGLUNIFORM4FVPROC)fn("glUniform4fv");
    api->Uniform1iv = (PFNGLUNIFORM1IVPROC)fn("glUniform1iv");
    api->Uniform2iv = (PFNGLUNIFORM2IVPROC)fn("glUniform2iv");
    api->Uniform3iv = (PFNGLUNIFORM3IVPROC)fn("glUniform3iv");
    api->Uniform4iv = (PFNGLUNIFORM4IVPROC)fn("glUniform4iv");
    api->UniformMatrix2fv = (PFNGLUNIFORMMATRIX2FVPROC)fn("glUniformMatrix2fv");
    api->UniformMatrix3fv = (PFNGLUNIFORMMATRIX3FVPROC)fn("glUniformMatrix3fv");
    api->UniformMatrix4fv = (PFNGLUNIFORMMATRIX4FVPROC)fn("glUniformMatrix4fv");
    api->ValidateProgram = (PFNGLVALIDATEPROGRAMPROC)fn("glValidateProgram");
    api->VertexAttrib1d = (PFNGLVERTEXATTRIB1DPROC)fn("glVertexAttrib1d");
    api->VertexAttrib1dv = (PFNGLVERTEXATTRIB1DVPROC)fn("glVertexAttrib1dv");
    api->VertexAttrib1f = (PFNGLVERTEXATTRIB1FPROC)fn("glVertexAttrib1f");
    api->VertexAttrib1fv = (PFNGLVERTEXATTRIB1FVPROC)fn("glVertexAttrib1fv");
    api->VertexAttrib1s = (PFNGLVERTEXATTRIB1SPROC)fn("glVertexAttrib1s");
    api->VertexAttrib1sv = (PFNGLVERTEXATTRIB1SVPROC)fn("glVertexAttrib1sv");
    api->VertexAttrib2d = (PFNGLVERTEXATTRIB2DPROC)fn("glVertexAttrib2d");
    api->VertexAttrib2dv = (PFNGLVERTEXATTRIB2DVPROC)fn("glVertexAttrib2dv");
    api->VertexAttrib2f = (PFNGLVERTEXATTRIB2FPROC)fn("glVertexAttrib2f");
    api->VertexAttrib2fv = (PFNGLVERTEXATTRIB2FVPROC)fn("glVertexAttrib2fv");
    api->VertexAttrib2s = (PFNGLVERTEXATTRIB2SPROC)fn("glVertexAttrib2s");
    api->VertexAttrib2sv = (PFNGLVERTEXATTRIB2SVPROC)fn("glVertexAttrib2sv");
    api->VertexAttrib3d = (PFNGLVERTEXATTRIB3DPROC)fn("glVertexAttrib3d");
    api->VertexAttrib3dv = (PFNGLVERTEXATTRIB3DVPROC)fn("glVertexAttrib3dv");
    api->VertexAttrib3f = (PFNGLVERTEXATTRIB3FPROC)fn("glVertexAttrib3f");
    api->VertexAttrib3fv = (PFNGLVERTEXATTRIB3FVPROC)fn("glVertexAttrib3fv");
    api->VertexAttrib3s = (PFNGLVERTEXATTRIB3SPROC)fn("glVertexAttrib3s");
    api->VertexAttrib3sv = (PFNGLVERTEXATTRIB3SVPROC)fn("glVertexAttrib3sv");
    api->VertexAttrib4Nbv = (PFNGLVERTEXATTRIB4NBVPROC)fn("glVertexAttrib4Nbv");
    api->VertexAttrib4Niv = (PFNGLVERTEXATTRIB4NIVPROC)fn("glVertexAttrib4Niv");
    api->VertexAttrib4Nsv = (PFNGLVERTEXATTRIB4NSVPROC)fn("glVertexAttrib4Nsv");
    api->VertexAttrib4Nub = (PFNGLVERTEXATTRIB4NUBPROC)fn("glVertexAttrib4Nub");
    api->VertexAttrib4Nubv = (PFNGLVERTEXATTRIB4NUBVPROC)fn("glVertexAttrib4Nubv");
    api->VertexAttrib4Nuiv = (PFNGLVERTEXATTRIB4NUIVPROC)fn("glVertexAttrib4Nuiv");
    api->VertexAttrib4Nusv = (PFNGLVERTEXATTRIB4NUSVPROC)fn("glVertexAttrib4Nusv");
    api->VertexAttrib4bv = (PFNGLVERTEXATTRIB4BVPROC)fn("glVertexAttrib4bv");
    api->VertexAttrib4d = (PFNGLVERTEXATTRIB4DPROC)fn("glVertexAttrib4d");
    api->VertexAttrib4dv = (PFNGLVERTEXATTRIB4DVPROC)fn("glVertexAttrib4dv");
    api->VertexAttrib4f = (PFNGLVERTEXATTRIB4FPROC)fn("glVertexAttrib4f");
    api->VertexAttrib4fv = (PFNGLVERTEXATTRIB4FVPROC)fn("glVertexAttrib4fv");
    api->VertexAttrib4iv = (PFNGLVERTEXATTRIB4IVPROC)fn("glVertexAttrib4iv");
    api->VertexAttrib4s = (PFNGLVERTEXATTRIB4SPROC)fn("glVertexAttrib4s");
    api->VertexAttrib4sv = (PFNGLVERTEXATTRIB4SVPROC)fn("glVertexAttrib4sv");
    api->VertexAttrib4ubv = (PFNGLVERTEXATTRIB4UBVPROC)fn("glVertexAttrib4ubv");
    api->VertexAttrib4uiv = (PFNGLVERTEXATTRIB4UIVPROC)fn("glVertexAttrib4uiv");
    api->VertexAttrib4usv = (PFNGLVERTEXATTRIB4USVPROC)fn("glVertexAttrib4usv");
    api->VertexAttribPointer = (PFNGLVERTEXATTRIBPOINTERPROC)fn("glVertexAttribPointer");
    api->UniformMatrix2x3fv = (PFNGLUNIFORMMATRIX2X3FVPROC)fn("glUniformMatrix2x3fv");
    api->UniformMatrix3x2fv = (PFNGLUNIFORMMATRIX3X2FVPROC)fn("glUniformMatrix3x2fv");
    api->UniformMatrix2x4fv = (PFNGLUNIFORMMATRIX2X4FVPROC)fn("glUniformMatrix2x4fv");
    api->UniformMatrix4x2fv = (PFNGLUNIFORMMATRIX4X2FVPROC)fn("glUniformMatrix4x2fv");
    api->UniformMatrix3x4fv = (PFNGLUNIFORMMATRIX3X4FVPROC)fn("glUniformMatrix3x4fv");
    api->UniformMatrix4x3fv = (PFNGLUNIFORMMATRIX4X3FVPROC)fn("glUniformMatrix4x3fv");
    api->ColorMaski = (PFNGLCOLORMASKIPROC)fn("glColorMaski");
    api->GetBooleani_v = (PFNGLGETBOOLEANI_VPROC)fn("glGetBooleani_v");
    api->GetIntegeri_v = (PFNGLGETINTEGERI_VPROC)fn("glGetIntegeri_v");
    api->Enablei = (PFNGLENABLEIPROC)fn("glEnablei");
    api->Disablei = (PFNGLDISABLEIPROC)fn("glDisablei");
    api->IsEnabledi = (PFNGLISENABLEDIPROC)fn("glIsEnabledi");
    api->BeginTransformFeedback = (PFNGLBEGINTRANSFORMFEEDBACKPROC)fn("glBeginTransformFeedback");
    api->EndTransformFeedback = (PFNGLENDTRANSFORMFEEDBACKPROC)fn("glEndTransformFeedback");
    api->BindBufferRange = (PFNGLBINDBUFFERRANGEPROC)fn("glBindBufferRange");
    api->BindBufferBase = (PFNGLBINDBUFFERBASEPROC)fn("glBindBufferBase");
    api->TransformFeedbackVaryings =
        (PFNGLTRANSFORMFEEDBACKVARYINGSPROC)fn("glTransformFeedbackVaryings");
    api->GetTransformFeedbackVarying =
        (PFNGLGETTRANSFORMFEEDBACKVARYINGPROC)fn("glGetTransformFeedbackVarying");
    api->ClampColor = (PFNGLCLAMPCOLORPROC)fn("glClampColor");
    api->BeginConditionalRender = (PFNGLBEGINCONDITIONALRENDERPROC)fn("glBeginConditionalRender");
    api->EndConditionalRender = (PFNGLENDCONDITIONALRENDERPROC)fn("glEndConditionalRender");
    api->VertexAttribIPointer = (PFNGLVERTEXATTRIBIPOINTERPROC)fn("glVertexAttribIPointer");
    api->GetVertexAttribIiv = (PFNGLGETVERTEXATTRIBIIVPROC)fn("glGetVertexAttribIiv");
    api->GetVertexAttribIuiv = (PFNGLGETVERTEXATTRIBIUIVPROC)fn("glGetVertexAttribIuiv");
    api->VertexAttribI1i = (PFNGLVERTEXATTRIBI1IPROC)fn("glVertexAttribI1i");
    api->VertexAttribI2i = (PFNGLVERTEXATTRIBI2IPROC)fn("glVertexAttribI2i");
    api->VertexAttribI3i = (PFNGLVERTEXATTRIBI3IPROC)fn("glVertexAttribI3i");
    api->VertexAttribI4i = (PFNGLVERTEXATTRIBI4IPROC)fn("glVertexAttribI4i");
    api->VertexAttribI1ui = (PFNGLVERTEXATTRIBI1UIPROC)fn("glVertexAttribI1ui");
    api->VertexAttribI2ui = (PFNGLVERTEXATTRIBI2UIPROC)fn("glVertexAttribI2ui");
    api->VertexAttribI3ui = (PFNGLVERTEXATTRIBI3UIPROC)fn("glVertexAttribI3ui");
    api->VertexAttribI4ui = (PFNGLVERTEXATTRIBI4UIPROC)fn("glVertexAttribI4ui");
    api->VertexAttribI1iv = (PFNGLVERTEXATTRIBI1IVPROC)fn("glVertexAttribI1iv");
    api->VertexAttribI2iv = (PFNGLVERTEXATTRIBI2IVPROC)fn("glVertexAttribI2iv");
    api->VertexAttribI3iv = (PFNGLVERTEXATTRIBI3IVPROC)fn("glVertexAttribI3iv");
    api->VertexAttribI4iv = (PFNGLVERTEXATTRIBI4IVPROC)fn("glVertexAttribI4iv");
    api->VertexAttribI1uiv = (PFNGLVERTEXATTRIBI1UIVPROC)fn("glVertexAttribI1uiv");
    api->VertexAttribI2uiv = (PFNGLVERTEXATTRIBI2UIVPROC)fn("glVertexAttribI2uiv");
    api->VertexAttribI3uiv = (PFNGLVERTEXATTRIBI3UIVPROC)fn("glVertexAttribI3uiv");
    api->VertexAttribI4uiv = (PFNGLVERTEXATTRIBI4UIVPROC)fn("glVertexAttribI4uiv");
    api->VertexAttribI4bv = (PFNGLVERTEXATTRIBI4BVPROC)fn("glVertexAttribI4bv");
    api->VertexAttribI4sv = (PFNGLVERTEXATTRIBI4SVPROC)fn("glVertexAttribI4sv");
    api->VertexAttribI4ubv = (PFNGLVERTEXATTRIBI4UBVPROC)fn("glVertexAttribI4ubv");
    api->VertexAttribI4usv = (PFNGLVERTEXATTRIBI4USVPROC)fn("glVertexAttribI4usv");
    api->GetUniformuiv = (PFNGLGETUNIFORMUIVPROC)fn("glGetUniformuiv");
    api->BindFragDataLocation = (PFNGLBINDFRAGDATALOCATIONPROC)fn("glBindFragDataLocation");
    api->GetFragDataLocation = (PFNGLGETFRAGDATALOCATIONPROC)fn("glGetFragDataLocation");
    api->Uniform1ui = (PFNGLUNIFORM1UIPROC)fn("glUniform1ui");
    api->Uniform2ui = (PFNGLUNIFORM2UIPROC)fn("glUniform2ui");
    api->Uniform3ui = (PFNGLUNIFORM3UIPROC)fn("glUniform3ui");
    api->Uniform4ui = (PFNGLUNIFORM4UIPROC)fn("glUniform4ui");
    api->Uniform1uiv = (PFNGLUNIFORM1UIVPROC)fn("glUniform1uiv");
    api->Uniform2uiv = (PFNGLUNIFORM2UIVPROC)fn("glUniform2uiv");
    api->Uniform3uiv = (PFNGLUNIFORM3UIVPROC)fn("glUniform3uiv");
    api->Uniform4uiv = (PFNGLUNIFORM4UIVPROC)fn("glUniform4uiv");
    api->TexParameterIiv = (PFNGLTEXPARAMETERIIVPROC)fn("glTexParameterIiv");
    api->TexParameterIuiv = (PFNGLTEXPARAMETERIUIVPROC)fn("glTexParameterIuiv");
    api->GetTexParameterIiv = (PFNGLGETTEXPARAMETERIIVPROC)fn("glGetTexParameterIiv");
    api->GetTexParameterIuiv = (PFNGLGETTEXPARAMETERIUIVPROC)fn("glGetTexParameterIuiv");
    api->ClearBufferiv = (PFNGLCLEARBUFFERIVPROC)fn("glClearBufferiv");
    api->ClearBufferuiv = (PFNGLCLEARBUFFERUIVPROC)fn("glClearBufferuiv");
    api->ClearBufferfv = (PFNGLCLEARBUFFERFVPROC)fn("glClearBufferfv");
    api->ClearBufferfi = (PFNGLCLEARBUFFERFIPROC)fn("glClearBufferfi");
    api->GetStringi = (PFNGLGETSTRINGIPROC)fn("glGetStringi");
    api->DrawArraysInstanced = (PFNGLDRAWARRAYSINSTANCEDPROC)fn("glDrawArraysInstanced");
    api->DrawElementsInstanced = (PFNGLDRAWELEMENTSINSTANCEDPROC)fn("glDrawElementsInstanced");
    api->TexBuffer = (PFNGLTEXBUFFERPROC)fn("glTexBuffer");
    api->PrimitiveRestartIndex = (PFNGLPRIMITIVERESTARTINDEXPROC)fn("glPrimitiveRestartIndex");
    api->GetInteger64i_v = (PFNGLGETINTEGER64I_VPROC)fn("glGetInteger64i_v");
    api->GetBufferParameteri64v = (PFNGLGETBUFFERPARAMETERI64VPROC)fn("glGetBufferParameteri64v");
    api->FramebufferTexture = (PFNGLFRAMEBUFFERTEXTUREPROC)fn("glFramebufferTexture");
    api->VertexAttribDivisor = (PFNGLVERTEXATTRIBDIVISORPROC)fn("glVertexAttribDivisor");
    api->MinSampleShading = (PFNGLMINSAMPLESHADINGPROC)fn("glMinSampleShading");
    api->BlendEquationi = (PFNGLBLENDEQUATIONIPROC)fn("glBlendEquationi");
    api->BlendEquationSeparatei = (PFNGLBLENDEQUATIONSEPARATEIPROC)fn("glBlendEquationSeparatei");
    api->BlendFunci = (PFNGLBLENDFUNCIPROC)fn("glBlendFunci");
    api->BlendFuncSeparatei = (PFNGLBLENDFUNCSEPARATEIPROC)fn("glBlendFuncSeparatei");
    api->IsRenderbuffer = (PFNGLISRENDERBUFFERPROC)fn("glIsRenderbuffer");
    api->BindRenderbuffer = (PFNGLBINDRENDERBUFFERPROC)fn("glBindRenderbuffer");
    api->DeleteRenderbuffers = (PFNGLDELETERENDERBUFFERSPROC)fn("glDeleteRenderbuffers");
    api->GenRenderbuffers = (PFNGLGENRENDERBUFFERSPROC)fn("glGenRenderbuffers");
    api->RenderbufferStorage = (PFNGLRENDERBUFFERSTORAGEPROC)fn("glRenderbufferStorage");
    api->GetRenderbufferParameteriv =
        (PFNGLGETRENDERBUFFERPARAMETERIVPROC)fn("glGetRenderbufferParameteriv");
    api->IsFramebuffer = (PFNGLISFRAMEBUFFERPROC)fn("glIsFramebuffer");
    api->BindFramebuffer = (PFNGLBINDFRAMEBUFFERPROC)fn("glBindFramebuffer");
    api->DeleteFramebuffers = (PFNGLDELETEFRAMEBUFFERSPROC)fn("glDeleteFramebuffers");
    api->GenFramebuffers = (PFNGLGENFRAMEBUFFERSPROC)fn("glGenFramebuffers");
    api->CheckFramebufferStatus = (PFNGLCHECKFRAMEBUFFERSTATUSPROC)fn("glCheckFramebufferStatus");
    api->FramebufferTexture1D = (PFNGLFRAMEBUFFERTEXTURE1DPROC)fn("glFramebufferTexture1D");
    api->FramebufferTexture2D = (PFNGLFRAMEBUFFERTEXTURE2DPROC)fn("glFramebufferTexture2D");
    api->FramebufferTexture3D = (PFNGLFRAMEBUFFERTEXTURE3DPROC)fn("glFramebufferTexture3D");
    api->FramebufferRenderbuffer =
        (PFNGLFRAMEBUFFERRENDERBUFFERPROC)fn("glFramebufferRenderbuffer");
    api->GetFramebufferAttachmentParameteriv =
        (PFNGLGETFRAMEBUFFERATTACHMENTPARAMETERIVPROC)fn("glGetFramebufferAttachmentParameteriv");
    api->GenerateMipmap = (PFNGLGENERATEMIPMAPPROC)fn("glGenerateMipmap");
    api->BlitFramebuffer = (PFNGLBLITFRAMEBUFFERPROC)fn("glBlitFramebuffer");
    api->RenderbufferStorageMultisample =
        (PFNGLRENDERBUFFERSTORAGEMULTISAMPLEPROC)fn("glRenderbufferStorageMultisample");
    api->FramebufferTextureLayer =
        (PFNGLFRAMEBUFFERTEXTURELAYERPROC)fn("glFramebufferTextureLayer");
    api->MapBufferRange = (PFNGLMAPBUFFERRANGEPROC)fn("glMapBufferRange");
    api->FlushMappedBufferRange = (PFNGLFLUSHMAPPEDBUFFERRANGEPROC)fn("glFlushMappedBufferRange");
    api->BindVertexArray = (PFNGLBINDVERTEXARRAYPROC)fn("glBindVertexArray");
    api->DeleteVertexArrays = (PFNGLDELETEVERTEXARRAYSPROC)fn("glDeleteVertexArrays");
    api->GenVertexArrays = (PFNGLGENVERTEXARRAYSPROC)fn("glGenVertexArrays");
    api->IsVertexArray = (PFNGLISVERTEXARRAYPROC)fn("glIsVertexArray");
    api->GetUniformIndices = (PFNGLGETUNIFORMINDICESPROC)fn("glGetUniformIndices");
    api->GetActiveUniformsiv = (PFNGLGETACTIVEUNIFORMSIVPROC)fn("glGetActiveUniformsiv");
    api->GetActiveUniformName = (PFNGLGETACTIVEUNIFORMNAMEPROC)fn("glGetActiveUniformName");
    api->GetUniformBlockIndex = (PFNGLGETUNIFORMBLOCKINDEXPROC)fn("glGetUniformBlockIndex");
    api->GetActiveUniformBlockiv =
        (PFNGLGETACTIVEUNIFORMBLOCKIVPROC)fn("glGetActiveUniformBlockiv");
    api->GetActiveUniformBlockName =
        (PFNGLGETACTIVEUNIFORMBLOCKNAMEPROC)fn("glGetActiveUniformBlockName");
    api->UniformBlockBinding = (PFNGLUNIFORMBLOCKBINDINGPROC)fn("glUniformBlockBinding");
    api->CopyBufferSubData = (PFNGLCOPYBUFFERSUBDATAPROC)fn("glCopyBufferSubData");
    api->DrawElementsBaseVertex = (PFNGLDRAWELEMENTSBASEVERTEXPROC)fn("glDrawElementsBaseVertex");
    api->DrawRangeElementsBaseVertex =
        (PFNGLDRAWRANGEELEMENTSBASEVERTEXPROC)fn("glDrawRangeElementsBaseVertex");
    api->DrawElementsInstancedBaseVertex =
        (PFNGLDRAWELEMENTSINSTANCEDBASEVERTEXPROC)fn("glDrawElementsInstancedBaseVertex");
    api->MultiDrawElementsBaseVertex =
        (PFNGLMULTIDRAWELEMENTSBASEVERTEXPROC)fn("glMultiDrawElementsBaseVertex");
    api->ProvokingVertex = (PFNGLPROVOKINGVERTEXPROC)fn("glProvokingVertex");
    api->FenceSync = (PFNGLFENCESYNCPROC)fn("glFenceSync");
    api->IsSync = (PFNGLISSYNCPROC)fn("glIsSync");
    api->DeleteSync = (PFNGLDELETESYNCPROC)fn("glDeleteSync");
    api->ClientWaitSync = (PFNGLCLIENTWAITSYNCPROC)fn("glClientWaitSync");
    api->WaitSync = (PFNGLWAITSYNCPROC)fn("glWaitSync");
    api->GetInteger64v = (PFNGLGETINTEGER64VPROC)fn("glGetInteger64v");
    api->GetSynciv = (PFNGLGETSYNCIVPROC)fn("glGetSynciv");
    api->TexImage2DMultisample = (PFNGLTEXIMAGE2DMULTISAMPLEPROC)fn("glTexImage2DMultisample");
    api->TexImage3DMultisample = (PFNGLTEXIMAGE3DMULTISAMPLEPROC)fn("glTexImage3DMultisample");
    api->GetMultisamplefv = (PFNGLGETMULTISAMPLEFVPROC)fn("glGetMultisamplefv");
    api->SampleMaski = (PFNGLSAMPLEMASKIPROC)fn("glSampleMaski");
    api->BlendEquationiARB = (PFNGLBLENDEQUATIONIARBPROC)fn("glBlendEquationiARB");
    api->BlendEquationSeparateiARB =
        (PFNGLBLENDEQUATIONSEPARATEIARBPROC)fn("glBlendEquationSeparateiARB");
    api->BlendFunciARB = (PFNGLBLENDFUNCIARBPROC)fn("glBlendFunciARB");
    api->BlendFuncSeparateiARB = (PFNGLBLENDFUNCSEPARATEIARBPROC)fn("glBlendFuncSeparateiARB");
    api->MinSampleShadingARB = (PFNGLMINSAMPLESHADINGARBPROC)fn("glMinSampleShadingARB");
    api->NamedStringARB = (PFNGLNAMEDSTRINGARBPROC)fn("glNamedStringARB");
    api->DeleteNamedStringARB = (PFNGLDELETENAMEDSTRINGARBPROC)fn("glDeleteNamedStringARB");
    api->CompileShaderIncludeARB =
        (PFNGLCOMPILESHADERINCLUDEARBPROC)fn("glCompileShaderIncludeARB");
    api->IsNamedStringARB = (PFNGLISNAMEDSTRINGARBPROC)fn("glIsNamedStringARB");
    api->GetNamedStringARB = (PFNGLGETNAMEDSTRINGARBPROC)fn("glGetNamedStringARB");
    api->GetNamedStringivARB = (PFNGLGETNAMEDSTRINGIVARBPROC)fn("glGetNamedStringivARB");
    api->BindFragDataLocationIndexed =
        (PFNGLBINDFRAGDATALOCATIONINDEXEDPROC)fn("glBindFragDataLocationIndexed");
    api->GetFragDataIndex = (PFNGLGETFRAGDATAINDEXPROC)fn("glGetFragDataIndex");
    api->GenSamplers = (PFNGLGENSAMPLERSPROC)fn("glGenSamplers");
    api->DeleteSamplers = (PFNGLDELETESAMPLERSPROC)fn("glDeleteSamplers");
    api->IsSampler = (PFNGLISSAMPLERPROC)fn("glIsSampler");
    api->BindSampler = (PFNGLBINDSAMPLERPROC)fn("glBindSampler");
    api->SamplerParameteri = (PFNGLSAMPLERPARAMETERIPROC)fn("glSamplerParameteri");
    api->SamplerParameteriv = (PFNGLSAMPLERPARAMETERIVPROC)fn("glSamplerParameteriv");
    api->SamplerParameterf = (PFNGLSAMPLERPARAMETERFPROC)fn("glSamplerParameterf");
    api->SamplerParameterfv = (PFNGLSAMPLERPARAMETERFVPROC)fn("glSamplerParameterfv");
    api->SamplerParameterIiv = (PFNGLSAMPLERPARAMETERIIVPROC)fn("glSamplerParameterIiv");
    api->SamplerParameterIuiv = (PFNGLSAMPLERPARAMETERIUIVPROC)fn("glSamplerParameterIuiv");
    api->GetSamplerParameteriv = (PFNGLGETSAMPLERPARAMETERIVPROC)fn("glGetSamplerParameteriv");
    api->GetSamplerParameterIiv = (PFNGLGETSAMPLERPARAMETERIIVPROC)fn("glGetSamplerParameterIiv");
    api->GetSamplerParameterfv = (PFNGLGETSAMPLERPARAMETERFVPROC)fn("glGetSamplerParameterfv");
    api->GetSamplerParameterIuiv =
        (PFNGLGETSAMPLERPARAMETERIUIVPROC)fn("glGetSamplerParameterIuiv");
    api->QueryCounter = (PFNGLQUERYCOUNTERPROC)fn("glQueryCounter");
    api->GetQueryObjecti64v = (PFNGLGETQUERYOBJECTI64VPROC)fn("glGetQueryObjecti64v");
    api->GetQueryObjectui64v = (PFNGLGETQUERYOBJECTUI64VPROC)fn("glGetQueryObjectui64v");
    api->VertexP2ui = (PFNGLVERTEXP2UIPROC)fn("glVertexP2ui");
    api->VertexP2uiv = (PFNGLVERTEXP2UIVPROC)fn("glVertexP2uiv");
    api->VertexP3ui = (PFNGLVERTEXP3UIPROC)fn("glVertexP3ui");
    api->VertexP3uiv = (PFNGLVERTEXP3UIVPROC)fn("glVertexP3uiv");
    api->VertexP4ui = (PFNGLVERTEXP4UIPROC)fn("glVertexP4ui");
    api->VertexP4uiv = (PFNGLVERTEXP4UIVPROC)fn("glVertexP4uiv");
    api->TexCoordP1ui = (PFNGLTEXCOORDP1UIPROC)fn("glTexCoordP1ui");
    api->TexCoordP1uiv = (PFNGLTEXCOORDP1UIVPROC)fn("glTexCoordP1uiv");
    api->TexCoordP2ui = (PFNGLTEXCOORDP2UIPROC)fn("glTexCoordP2ui");
    api->TexCoordP2uiv = (PFNGLTEXCOORDP2UIVPROC)fn("glTexCoordP2uiv");
    api->TexCoordP3ui = (PFNGLTEXCOORDP3UIPROC)fn("glTexCoordP3ui");
    api->TexCoordP3uiv = (PFNGLTEXCOORDP3UIVPROC)fn("glTexCoordP3uiv");
    api->TexCoordP4ui = (PFNGLTEXCOORDP4UIPROC)fn("glTexCoordP4ui");
    api->TexCoordP4uiv = (PFNGLTEXCOORDP4UIVPROC)fn("glTexCoordP4uiv");
    api->MultiTexCoordP1ui = (PFNGLMULTITEXCOORDP1UIPROC)fn("glMultiTexCoordP1ui");
    api->MultiTexCoordP1uiv = (PFNGLMULTITEXCOORDP1UIVPROC)fn("glMultiTexCoordP1uiv");
    api->MultiTexCoordP2ui = (PFNGLMULTITEXCOORDP2UIPROC)fn("glMultiTexCoordP2ui");
    api->MultiTexCoordP2uiv = (PFNGLMULTITEXCOORDP2UIVPROC)fn("glMultiTexCoordP2uiv");
    api->MultiTexCoordP3ui = (PFNGLMULTITEXCOORDP3UIPROC)fn("glMultiTexCoordP3ui");
    api->MultiTexCoordP3uiv = (PFNGLMULTITEXCOORDP3UIVPROC)fn("glMultiTexCoordP3uiv");
    api->MultiTexCoordP4ui = (PFNGLMULTITEXCOORDP4UIPROC)fn("glMultiTexCoordP4ui");
    api->MultiTexCoordP4uiv = (PFNGLMULTITEXCOORDP4UIVPROC)fn("glMultiTexCoordP4uiv");
    api->NormalP3ui = (PFNGLNORMALP3UIPROC)fn("glNormalP3ui");
    api->NormalP3uiv = (PFNGLNORMALP3UIVPROC)fn("glNormalP3uiv");
    api->ColorP3ui = (PFNGLCOLORP3UIPROC)fn("glColorP3ui");
    api->ColorP3uiv = (PFNGLCOLORP3UIVPROC)fn("glColorP3uiv");
    api->ColorP4ui = (PFNGLCOLORP4UIPROC)fn("glColorP4ui");
    api->ColorP4uiv = (PFNGLCOLORP4UIVPROC)fn("glColorP4uiv");
    api->SecondaryColorP3ui = (PFNGLSECONDARYCOLORP3UIPROC)fn("glSecondaryColorP3ui");
    api->SecondaryColorP3uiv = (PFNGLSECONDARYCOLORP3UIVPROC)fn("glSecondaryColorP3uiv");
    api->VertexAttribP1ui = (PFNGLVERTEXATTRIBP1UIPROC)fn("glVertexAttribP1ui");
    api->VertexAttribP1uiv = (PFNGLVERTEXATTRIBP1UIVPROC)fn("glVertexAttribP1uiv");
    api->VertexAttribP2ui = (PFNGLVERTEXATTRIBP2UIPROC)fn("glVertexAttribP2ui");
    api->VertexAttribP2uiv = (PFNGLVERTEXATTRIBP2UIVPROC)fn("glVertexAttribP2uiv");
    api->VertexAttribP3ui = (PFNGLVERTEXATTRIBP3UIPROC)fn("glVertexAttribP3ui");
    api->VertexAttribP3uiv = (PFNGLVERTEXATTRIBP3UIVPROC)fn("glVertexAttribP3uiv");
    api->VertexAttribP4ui = (PFNGLVERTEXATTRIBP4UIPROC)fn("glVertexAttribP4ui");
    api->VertexAttribP4uiv = (PFNGLVERTEXATTRIBP4UIVPROC)fn("glVertexAttribP4uiv");
    api->DrawArraysIndirect = (PFNGLDRAWARRAYSINDIRECTPROC)fn("glDrawArraysIndirect");
    api->DrawElementsIndirect = (PFNGLDRAWELEMENTSINDIRECTPROC)fn("glDrawElementsIndirect");
    api->Uniform1d = (PFNGLUNIFORM1DPROC)fn("glUniform1d");
    api->Uniform2d = (PFNGLUNIFORM2DPROC)fn("glUniform2d");
    api->Uniform3d = (PFNGLUNIFORM3DPROC)fn("glUniform3d");
    api->Uniform4d = (PFNGLUNIFORM4DPROC)fn("glUniform4d");
    api->Uniform1dv = (PFNGLUNIFORM1DVPROC)fn("glUniform1dv");
    api->Uniform2dv = (PFNGLUNIFORM2DVPROC)fn("glUniform2dv");
    api->Uniform3dv = (PFNGLUNIFORM3DVPROC)fn("glUniform3dv");
    api->Uniform4dv = (PFNGLUNIFORM4DVPROC)fn("glUniform4dv");
    api->UniformMatrix2dv = (PFNGLUNIFORMMATRIX2DVPROC)fn("glUniformMatrix2dv");
    api->UniformMatrix3dv = (PFNGLUNIFORMMATRIX3DVPROC)fn("glUniformMatrix3dv");
    api->UniformMatrix4dv = (PFNGLUNIFORMMATRIX4DVPROC)fn("glUniformMatrix4dv");
    api->UniformMatrix2x3dv = (PFNGLUNIFORMMATRIX2X3DVPROC)fn("glUniformMatrix2x3dv");
    api->UniformMatrix2x4dv = (PFNGLUNIFORMMATRIX2X4DVPROC)fn("glUniformMatrix2x4dv");
    api->UniformMatrix3x2dv = (PFNGLUNIFORMMATRIX3X2DVPROC)fn("glUniformMatrix3x2dv");
    api->UniformMatrix3x4dv = (PFNGLUNIFORMMATRIX3X4DVPROC)fn("glUniformMatrix3x4dv");
    api->UniformMatrix4x2dv = (PFNGLUNIFORMMATRIX4X2DVPROC)fn("glUniformMatrix4x2dv");
    api->UniformMatrix4x3dv = (PFNGLUNIFORMMATRIX4X3DVPROC)fn("glUniformMatrix4x3dv");
    api->GetUniformdv = (PFNGLGETUNIFORMDVPROC)fn("glGetUniformdv");
    api->GetSubroutineUniformLocation =
        (PFNGLGETSUBROUTINEUNIFORMLOCATIONPROC)fn("glGetSubroutineUniformLocation");
    api->GetSubroutineIndex = (PFNGLGETSUBROUTINEINDEXPROC)fn("glGetSubroutineIndex");
    api->GetActiveSubroutineUniformiv =
        (PFNGLGETACTIVESUBROUTINEUNIFORMIVPROC)fn("glGetActiveSubroutineUniformiv");
    api->GetActiveSubroutineUniformName =
        (PFNGLGETACTIVESUBROUTINEUNIFORMNAMEPROC)fn("glGetActiveSubroutineUniformName");
    api->GetActiveSubroutineName =
        (PFNGLGETACTIVESUBROUTINENAMEPROC)fn("glGetActiveSubroutineName");
    api->UniformSubroutinesuiv = (PFNGLUNIFORMSUBROUTINESUIVPROC)fn("glUniformSubroutinesuiv");
    api->GetUniformSubroutineuiv =
        (PFNGLGETUNIFORMSUBROUTINEUIVPROC)fn("glGetUniformSubroutineuiv");
    api->GetProgramStageiv = (PFNGLGETPROGRAMSTAGEIVPROC)fn("glGetProgramStageiv");
    api->PatchParameteri = (PFNGLPATCHPARAMETERIPROC)fn("glPatchParameteri");
    api->PatchParameterfv = (PFNGLPATCHPARAMETERFVPROC)fn("glPatchParameterfv");
    api->BindTransformFeedback = (PFNGLBINDTRANSFORMFEEDBACKPROC)fn("glBindTransformFeedback");
    api->DeleteTransformFeedbacks =
        (PFNGLDELETETRANSFORMFEEDBACKSPROC)fn("glDeleteTransformFeedbacks");
    api->GenTransformFeedbacks = (PFNGLGENTRANSFORMFEEDBACKSPROC)fn("glGenTransformFeedbacks");
    api->IsTransformFeedback = (PFNGLISTRANSFORMFEEDBACKPROC)fn("glIsTransformFeedback");
    api->PauseTransformFeedback = (PFNGLPAUSETRANSFORMFEEDBACKPROC)fn("glPauseTransformFeedback");
    api->ResumeTransformFeedback =
        (PFNGLRESUMETRANSFORMFEEDBACKPROC)fn("glResumeTransformFeedback");
    api->DrawTransformFeedback = (PFNGLDRAWTRANSFORMFEEDBACKPROC)fn("glDrawTransformFeedback");
    api->DrawTransformFeedbackStream =
        (PFNGLDRAWTRANSFORMFEEDBACKSTREAMPROC)fn("glDrawTransformFeedbackStream");
    api->BeginQueryIndexed = (PFNGLBEGINQUERYINDEXEDPROC)fn("glBeginQueryIndexed");
    api->EndQueryIndexed = (PFNGLENDQUERYINDEXEDPROC)fn("glEndQueryIndexed");
    api->GetQueryIndexediv = (PFNGLGETQUERYINDEXEDIVPROC)fn("glGetQueryIndexediv");
    api->ReleaseShaderCompiler = (PFNGLRELEASESHADERCOMPILERPROC)fn("glReleaseShaderCompiler");
    api->ShaderBinary = (PFNGLSHADERBINARYPROC)fn("glShaderBinary");
    api->GetShaderPrecisionFormat =
        (PFNGLGETSHADERPRECISIONFORMATPROC)fn("glGetShaderPrecisionFormat");
    api->DepthRangef = (PFNGLDEPTHRANGEFPROC)fn("glDepthRangef");
    api->ClearDepthf = (PFNGLCLEARDEPTHFPROC)fn("glClearDepthf");
    api->GetProgramBinary = (PFNGLGETPROGRAMBINARYPROC)fn("glGetProgramBinary");
    api->ProgramBinary = (PFNGLPROGRAMBINARYPROC)fn("glProgramBinary");
    api->ProgramParameteri = (PFNGLPROGRAMPARAMETERIPROC)fn("glProgramParameteri");
    api->UseProgramStages = (PFNGLUSEPROGRAMSTAGESPROC)fn("glUseProgramStages");
    api->ActiveShaderProgram = (PFNGLACTIVESHADERPROGRAMPROC)fn("glActiveShaderProgram");
    api->CreateShaderProgramv = (PFNGLCREATESHADERPROGRAMVPROC)fn("glCreateShaderProgramv");
    api->BindProgramPipeline = (PFNGLBINDPROGRAMPIPELINEPROC)fn("glBindProgramPipeline");
    api->DeleteProgramPipelines = (PFNGLDELETEPROGRAMPIPELINESPROC)fn("glDeleteProgramPipelines");
    api->GenProgramPipelines = (PFNGLGENPROGRAMPIPELINESPROC)fn("glGenProgramPipelines");
    api->IsProgramPipeline = (PFNGLISPROGRAMPIPELINEPROC)fn("glIsProgramPipeline");
    api->GetProgramPipelineiv = (PFNGLGETPROGRAMPIPELINEIVPROC)fn("glGetProgramPipelineiv");
    api->ProgramUniform1i = (PFNGLPROGRAMUNIFORM1IPROC)fn("glProgramUniform1i");
    api->ProgramUniform1iv = (PFNGLPROGRAMUNIFORM1IVPROC)fn("glProgramUniform1iv");
    api->ProgramUniform1f = (PFNGLPROGRAMUNIFORM1FPROC)fn("glProgramUniform1f");
    api->ProgramUniform1fv = (PFNGLPROGRAMUNIFORM1FVPROC)fn("glProgramUniform1fv");
    api->ProgramUniform1d = (PFNGLPROGRAMUNIFORM1DPROC)fn("glProgramUniform1d");
    api->ProgramUniform1dv = (PFNGLPROGRAMUNIFORM1DVPROC)fn("glProgramUniform1dv");
    api->ProgramUniform1ui = (PFNGLPROGRAMUNIFORM1UIPROC)fn("glProgramUniform1ui");
    api->ProgramUniform1uiv = (PFNGLPROGRAMUNIFORM1UIVPROC)fn("glProgramUniform1uiv");
    api->ProgramUniform2i = (PFNGLPROGRAMUNIFORM2IPROC)fn("glProgramUniform2i");
    api->ProgramUniform2iv = (PFNGLPROGRAMUNIFORM2IVPROC)fn("glProgramUniform2iv");
    api->ProgramUniform2f = (PFNGLPROGRAMUNIFORM2FPROC)fn("glProgramUniform2f");
    api->ProgramUniform2fv = (PFNGLPROGRAMUNIFORM2FVPROC)fn("glProgramUniform2fv");
    api->ProgramUniform2d = (PFNGLPROGRAMUNIFORM2DPROC)fn("glProgramUniform2d");
    api->ProgramUniform2dv = (PFNGLPROGRAMUNIFORM2DVPROC)fn("glProgramUniform2dv");
    api->ProgramUniform2ui = (PFNGLPROGRAMUNIFORM2UIPROC)fn("glProgramUniform2ui");
    api->ProgramUniform2uiv = (PFNGLPROGRAMUNIFORM2UIVPROC)fn("glProgramUniform2uiv");
    api->ProgramUniform3i = (PFNGLPROGRAMUNIFORM3IPROC)fn("glProgramUniform3i");
    api->ProgramUniform3iv = (PFNGLPROGRAMUNIFORM3IVPROC)fn("glProgramUniform3iv");
    api->ProgramUniform3f = (PFNGLPROGRAMUNIFORM3FPROC)fn("glProgramUniform3f");
    api->ProgramUniform3fv = (PFNGLPROGRAMUNIFORM3FVPROC)fn("glProgramUniform3fv");
    api->ProgramUniform3d = (PFNGLPROGRAMUNIFORM3DPROC)fn("glProgramUniform3d");
    api->ProgramUniform3dv = (PFNGLPROGRAMUNIFORM3DVPROC)fn("glProgramUniform3dv");
    api->ProgramUniform3ui = (PFNGLPROGRAMUNIFORM3UIPROC)fn("glProgramUniform3ui");
    api->ProgramUniform3uiv = (PFNGLPROGRAMUNIFORM3UIVPROC)fn("glProgramUniform3uiv");
    api->ProgramUniform4i = (PFNGLPROGRAMUNIFORM4IPROC)fn("glProgramUniform4i");
    api->ProgramUniform4iv = (PFNGLPROGRAMUNIFORM4IVPROC)fn("glProgramUniform4iv");
    api->ProgramUniform4f = (PFNGLPROGRAMUNIFORM4FPROC)fn("glProgramUniform4f");
    api->ProgramUniform4fv = (PFNGLPROGRAMUNIFORM4FVPROC)fn("glProgramUniform4fv");
    api->ProgramUniform4d = (PFNGLPROGRAMUNIFORM4DPROC)fn("glProgramUniform4d");
    api->ProgramUniform4dv = (PFNGLPROGRAMUNIFORM4DVPROC)fn("glProgramUniform4dv");
    api->ProgramUniform4ui = (PFNGLPROGRAMUNIFORM4UIPROC)fn("glProgramUniform4ui");
    api->ProgramUniform4uiv = (PFNGLPROGRAMUNIFORM4UIVPROC)fn("glProgramUniform4uiv");
    api->ProgramUniformMatrix2fv =
        (PFNGLPROGRAMUNIFORMMATRIX2FVPROC)fn("glProgramUniformMatrix2fv");
    api->ProgramUniformMatrix3fv =
        (PFNGLPROGRAMUNIFORMMATRIX3FVPROC)fn("glProgramUniformMatrix3fv");
    api->ProgramUniformMatrix4fv =
        (PFNGLPROGRAMUNIFORMMATRIX4FVPROC)fn("glProgramUniformMatrix4fv");
    api->ProgramUniformMatrix2dv =
        (PFNGLPROGRAMUNIFORMMATRIX2DVPROC)fn("glProgramUniformMatrix2dv");
    api->ProgramUniformMatrix3dv =
        (PFNGLPROGRAMUNIFORMMATRIX3DVPROC)fn("glProgramUniformMatrix3dv");
    api->ProgramUniformMatrix4dv =
        (PFNGLPROGRAMUNIFORMMATRIX4DVPROC)fn("glProgramUniformMatrix4dv");
    api->ProgramUniformMatrix2x3fv =
        (PFNGLPROGRAMUNIFORMMATRIX2X3FVPROC)fn("glProgramUniformMatrix2x3fv");
    api->ProgramUniformMatrix3x2fv =
        (PFNGLPROGRAMUNIFORMMATRIX3X2FVPROC)fn("glProgramUniformMatrix3x2fv");
    api->ProgramUniformMatrix2x4fv =
        (PFNGLPROGRAMUNIFORMMATRIX2X4FVPROC)fn("glProgramUniformMatrix2x4fv");
    api->ProgramUniformMatrix4x2fv =
        (PFNGLPROGRAMUNIFORMMATRIX4X2FVPROC)fn("glProgramUniformMatrix4x2fv");
    api->ProgramUniformMatrix3x4fv =
        (PFNGLPROGRAMUNIFORMMATRIX3X4FVPROC)fn("glProgramUniformMatrix3x4fv");
    api->ProgramUniformMatrix4x3fv =
        (PFNGLPROGRAMUNIFORMMATRIX4X3FVPROC)fn("glProgramUniformMatrix4x3fv");
    api->ProgramUniformMatrix2x3dv =
        (PFNGLPROGRAMUNIFORMMATRIX2X3DVPROC)fn("glProgramUniformMatrix2x3dv");
    api->ProgramUniformMatrix3x2dv =
        (PFNGLPROGRAMUNIFORMMATRIX3X2DVPROC)fn("glProgramUniformMatrix3x2dv");
    api->ProgramUniformMatrix2x4dv =
        (PFNGLPROGRAMUNIFORMMATRIX2X4DVPROC)fn("glProgramUniformMatrix2x4dv");
    api->ProgramUniformMatrix4x2dv =
        (PFNGLPROGRAMUNIFORMMATRIX4X2DVPROC)fn("glProgramUniformMatrix4x2dv");
    api->ProgramUniformMatrix3x4dv =
        (PFNGLPROGRAMUNIFORMMATRIX3X4DVPROC)fn("glProgramUniformMatrix3x4dv");
    api->ProgramUniformMatrix4x3dv =
        (PFNGLPROGRAMUNIFORMMATRIX4X3DVPROC)fn("glProgramUniformMatrix4x3dv");
    api->ValidateProgramPipeline =
        (PFNGLVALIDATEPROGRAMPIPELINEPROC)fn("glValidateProgramPipeline");
    api->GetProgramPipelineInfoLog =
        (PFNGLGETPROGRAMPIPELINEINFOLOGPROC)fn("glGetProgramPipelineInfoLog");
    api->VertexAttribL1d = (PFNGLVERTEXATTRIBL1DPROC)fn("glVertexAttribL1d");
    api->VertexAttribL2d = (PFNGLVERTEXATTRIBL2DPROC)fn("glVertexAttribL2d");
    api->VertexAttribL3d = (PFNGLVERTEXATTRIBL3DPROC)fn("glVertexAttribL3d");
    api->VertexAttribL4d = (PFNGLVERTEXATTRIBL4DPROC)fn("glVertexAttribL4d");
    api->VertexAttribL1dv = (PFNGLVERTEXATTRIBL1DVPROC)fn("glVertexAttribL1dv");
    api->VertexAttribL2dv = (PFNGLVERTEXATTRIBL2DVPROC)fn("glVertexAttribL2dv");
    api->VertexAttribL3dv = (PFNGLVERTEXATTRIBL3DVPROC)fn("glVertexAttribL3dv");
    api->VertexAttribL4dv = (PFNGLVERTEXATTRIBL4DVPROC)fn("glVertexAttribL4dv");
    api->VertexAttribLPointer = (PFNGLVERTEXATTRIBLPOINTERPROC)fn("glVertexAttribLPointer");
    api->GetVertexAttribLdv = (PFNGLGETVERTEXATTRIBLDVPROC)fn("glGetVertexAttribLdv");
    api->ViewportArrayv = (PFNGLVIEWPORTARRAYVPROC)fn("glViewportArrayv");
    api->ViewportIndexedf = (PFNGLVIEWPORTINDEXEDFPROC)fn("glViewportIndexedf");
    api->ViewportIndexedfv = (PFNGLVIEWPORTINDEXEDFVPROC)fn("glViewportIndexedfv");
    api->ScissorArrayv = (PFNGLSCISSORARRAYVPROC)fn("glScissorArrayv");
    api->ScissorIndexed = (PFNGLSCISSORINDEXEDPROC)fn("glScissorIndexed");
    api->ScissorIndexedv = (PFNGLSCISSORINDEXEDVPROC)fn("glScissorIndexedv");
    api->DepthRangeArrayv = (PFNGLDEPTHRANGEARRAYVPROC)fn("glDepthRangeArrayv");
    api->DepthRangeIndexed = (PFNGLDEPTHRANGEINDEXEDPROC)fn("glDepthRangeIndexed");
    api->GetFloati_v = (PFNGLGETFLOATI_VPROC)fn("glGetFloati_v");
    api->GetDoublei_v = (PFNGLGETDOUBLEI_VPROC)fn("glGetDoublei_v");
    api->CreateSyncFromCLeventARB =
        (PFNGLCREATESYNCFROMCLEVENTARBPROC)fn("glCreateSyncFromCLeventARB");
    api->DebugMessageControlARB = (PFNGLDEBUGMESSAGECONTROLARBPROC)fn("glDebugMessageControlARB");
    api->DebugMessageInsertARB = (PFNGLDEBUGMESSAGEINSERTARBPROC)fn("glDebugMessageInsertARB");
    api->DebugMessageCallbackARB =
        (PFNGLDEBUGMESSAGECALLBACKARBPROC)fn("glDebugMessageCallbackARB");
    api->GetDebugMessageLogARB = (PFNGLGETDEBUGMESSAGELOGARBPROC)fn("glGetDebugMessageLogARB");
    api->GetGraphicsResetStatusARB =
        (PFNGLGETGRAPHICSRESETSTATUSARBPROC)fn("glGetGraphicsResetStatusARB");
    api->GetnTexImageARB = (PFNGLGETNTEXIMAGEARBPROC)fn("glGetnTexImageARB");
    api->ReadnPixelsARB = (PFNGLREADNPIXELSARBPROC)fn("glReadnPixelsARB");
    api->GetnCompressedTexImageARB =
        (PFNGLGETNCOMPRESSEDTEXIMAGEARBPROC)fn("glGetnCompressedTexImageARB");
    api->GetnUniformfvARB = (PFNGLGETNUNIFORMFVARBPROC)fn("glGetnUniformfvARB");
    api->GetnUniformivARB = (PFNGLGETNUNIFORMIVARBPROC)fn("glGetnUniformivARB");
    api->GetnUniformuivARB = (PFNGLGETNUNIFORMUIVARBPROC)fn("glGetnUniformuivARB");
    api->GetnUniformdvARB = (PFNGLGETNUNIFORMDVARBPROC)fn("glGetnUniformdvARB");
    api->DrawArraysInstancedBaseInstance =
        (PFNGLDRAWARRAYSINSTANCEDBASEINSTANCEPROC)fn("glDrawArraysInstancedBaseInstance");
    api->DrawElementsInstancedBaseInstance =
        (PFNGLDRAWELEMENTSINSTANCEDBASEINSTANCEPROC)fn("glDrawElementsInstancedBaseInstance");
    api->DrawElementsInstancedBaseVertexBaseInstance =
        (PFNGLDRAWELEMENTSINSTANCEDBASEVERTEXBASEINSTANCEPROC)fn(
            "glDrawElementsInstancedBaseVertexBaseInstance");
    api->DrawTransformFeedbackInstanced =
        (PFNGLDRAWTRANSFORMFEEDBACKINSTANCEDPROC)fn("glDrawTransformFeedbackInstanced");
    api->DrawTransformFeedbackStreamInstanced =
        (PFNGLDRAWTRANSFORMFEEDBACKSTREAMINSTANCEDPROC)fn("glDrawTransformFeedbackStreamInstanced");
    api->GetInternalformativ = (PFNGLGETINTERNALFORMATIVPROC)fn("glGetInternalformativ");
    api->GetActiveAtomicCounterBufferiv =
        (PFNGLGETACTIVEATOMICCOUNTERBUFFERIVPROC)fn("glGetActiveAtomicCounterBufferiv");
    api->BindImageTexture = (PFNGLBINDIMAGETEXTUREPROC)fn("glBindImageTexture");
    api->MemoryBarrier = (PFNGLMEMORYBARRIERPROC)fn("glMemoryBarrier");
    api->TexStorage1D = (PFNGLTEXSTORAGE1DPROC)fn("glTexStorage1D");
    api->TexStorage2D = (PFNGLTEXSTORAGE2DPROC)fn("glTexStorage2D");
    api->TexStorage3D = (PFNGLTEXSTORAGE3DPROC)fn("glTexStorage3D");
    api->TextureStorage1DEXT = (PFNGLTEXTURESTORAGE1DEXTPROC)fn("glTextureStorage1DEXT");
    api->TextureStorage2DEXT = (PFNGLTEXTURESTORAGE2DEXTPROC)fn("glTextureStorage2DEXT");
    api->TextureStorage3DEXT = (PFNGLTEXTURESTORAGE3DEXTPROC)fn("glTextureStorage3DEXT");
    api->DebugMessageControl = (PFNGLDEBUGMESSAGECONTROLPROC)fn("glDebugMessageControl");
    api->DebugMessageInsert = (PFNGLDEBUGMESSAGEINSERTPROC)fn("glDebugMessageInsert");
    api->DebugMessageCallback = (PFNGLDEBUGMESSAGECALLBACKPROC)fn("glDebugMessageCallback");
    api->GetDebugMessageLog = (PFNGLGETDEBUGMESSAGELOGPROC)fn("glGetDebugMessageLog");
    api->PushDebugGroup = (PFNGLPUSHDEBUGGROUPPROC)fn("glPushDebugGroup");
    api->PopDebugGroup = (PFNGLPOPDEBUGGROUPPROC)fn("glPopDebugGroup");
    api->ObjectLabel = (PFNGLOBJECTLABELPROC)fn("glObjectLabel");
    api->GetObjectLabel = (PFNGLGETOBJECTLABELPROC)fn("glGetObjectLabel");
    api->ObjectPtrLabel = (PFNGLOBJECTPTRLABELPROC)fn("glObjectPtrLabel");
    api->GetObjectPtrLabel = (PFNGLGETOBJECTPTRLABELPROC)fn("glGetObjectPtrLabel");
    api->ClearBufferData = (PFNGLCLEARBUFFERDATAPROC)fn("glClearBufferData");
    api->ClearBufferSubData = (PFNGLCLEARBUFFERSUBDATAPROC)fn("glClearBufferSubData");
    api->ClearNamedBufferDataEXT =
        (PFNGLCLEARNAMEDBUFFERDATAEXTPROC)fn("glClearNamedBufferDataEXT");
    api->ClearNamedBufferSubDataEXT =
        (PFNGLCLEARNAMEDBUFFERSUBDATAEXTPROC)fn("glClearNamedBufferSubDataEXT");
    api->DispatchCompute = (PFNGLDISPATCHCOMPUTEPROC)fn("glDispatchCompute");
    api->DispatchComputeIndirect =
        (PFNGLDISPATCHCOMPUTEINDIRECTPROC)fn("glDispatchComputeIndirect");
    api->CopyImageSubData = (PFNGLCOPYIMAGESUBDATAPROC)fn("glCopyImageSubData");
    api->TextureView = (PFNGLTEXTUREVIEWPROC)fn("glTextureView");
    api->BindVertexBuffer = (PFNGLBINDVERTEXBUFFERPROC)fn("glBindVertexBuffer");
    api->VertexAttribFormat = (PFNGLVERTEXATTRIBFORMATPROC)fn("glVertexAttribFormat");
    api->VertexAttribIFormat = (PFNGLVERTEXATTRIBIFORMATPROC)fn("glVertexAttribIFormat");
    api->VertexAttribLFormat = (PFNGLVERTEXATTRIBLFORMATPROC)fn("glVertexAttribLFormat");
    api->VertexAttribBinding = (PFNGLVERTEXATTRIBBINDINGPROC)fn("glVertexAttribBinding");
    api->VertexBindingDivisor = (PFNGLVERTEXBINDINGDIVISORPROC)fn("glVertexBindingDivisor");
    api->VertexArrayBindVertexBufferEXT =
        (PFNGLVERTEXARRAYBINDVERTEXBUFFEREXTPROC)fn("glVertexArrayBindVertexBufferEXT");
    api->VertexArrayVertexAttribFormatEXT =
        (PFNGLVERTEXARRAYVERTEXATTRIBFORMATEXTPROC)fn("glVertexArrayVertexAttribFormatEXT");
    api->VertexArrayVertexAttribIFormatEXT =
        (PFNGLVERTEXARRAYVERTEXATTRIBIFORMATEXTPROC)fn("glVertexArrayVertexAttribIFormatEXT");
    api->VertexArrayVertexAttribLFormatEXT =
        (PFNGLVERTEXARRAYVERTEXATTRIBLFORMATEXTPROC)fn("glVertexArrayVertexAttribLFormatEXT");
    api->VertexArrayVertexAttribBindingEXT =
        (PFNGLVERTEXARRAYVERTEXATTRIBBINDINGEXTPROC)fn("glVertexArrayVertexAttribBindingEXT");
    api->VertexArrayVertexBindingDivisorEXT =
        (PFNGLVERTEXARRAYVERTEXBINDINGDIVISOREXTPROC)fn("glVertexArrayVertexBindingDivisorEXT");
    api->FramebufferParameteri = (PFNGLFRAMEBUFFERPARAMETERIPROC)fn("glFramebufferParameteri");
    api->GetFramebufferParameteriv =
        (PFNGLGETFRAMEBUFFERPARAMETERIVPROC)fn("glGetFramebufferParameteriv");
    api->NamedFramebufferParameteriEXT =
        (PFNGLNAMEDFRAMEBUFFERPARAMETERIEXTPROC)fn("glNamedFramebufferParameteriEXT");
    api->GetNamedFramebufferParameterivEXT =
        (PFNGLGETNAMEDFRAMEBUFFERPARAMETERIVEXTPROC)fn("glGetNamedFramebufferParameterivEXT");
    api->GetInternalformati64v = (PFNGLGETINTERNALFORMATI64VPROC)fn("glGetInternalformati64v");
    api->InvalidateTexSubImage = (PFNGLINVALIDATETEXSUBIMAGEPROC)fn("glInvalidateTexSubImage");
    api->InvalidateTexImage = (PFNGLINVALIDATETEXIMAGEPROC)fn("glInvalidateTexImage");
    api->InvalidateBufferSubData =
        (PFNGLINVALIDATEBUFFERSUBDATAPROC)fn("glInvalidateBufferSubData");
    api->InvalidateBufferData = (PFNGLINVALIDATEBUFFERDATAPROC)fn("glInvalidateBufferData");
    api->InvalidateFramebuffer = (PFNGLINVALIDATEFRAMEBUFFERPROC)fn("glInvalidateFramebuffer");
    api->InvalidateSubFramebuffer =
        (PFNGLINVALIDATESUBFRAMEBUFFERPROC)fn("glInvalidateSubFramebuffer");
    api->MultiDrawArraysIndirect =
        (PFNGLMULTIDRAWARRAYSINDIRECTPROC)fn("glMultiDrawArraysIndirect");
    api->MultiDrawElementsIndirect =
        (PFNGLMULTIDRAWELEMENTSINDIRECTPROC)fn("glMultiDrawElementsIndirect");
    api->GetProgramInterfaceiv = (PFNGLGETPROGRAMINTERFACEIVPROC)fn("glGetProgramInterfaceiv");
    api->GetProgramResourceIndex =
        (PFNGLGETPROGRAMRESOURCEINDEXPROC)fn("glGetProgramResourceIndex");
    api->GetProgramResourceName = (PFNGLGETPROGRAMRESOURCENAMEPROC)fn("glGetProgramResourceName");
    api->GetProgramResourceiv = (PFNGLGETPROGRAMRESOURCEIVPROC)fn("glGetProgramResourceiv");
    api->GetProgramResourceLocation =
        (PFNGLGETPROGRAMRESOURCELOCATIONPROC)fn("glGetProgramResourceLocation");
    api->GetProgramResourceLocationIndex =
        (PFNGLGETPROGRAMRESOURCELOCATIONINDEXPROC)fn("glGetProgramResourceLocationIndex");
    api->ShaderStorageBlockBinding =
        (PFNGLSHADERSTORAGEBLOCKBINDINGPROC)fn("glShaderStorageBlockBinding");
    api->TexBufferRange = (PFNGLTEXBUFFERRANGEPROC)fn("glTexBufferRange");
    api->TextureBufferRangeEXT = (PFNGLTEXTUREBUFFERRANGEEXTPROC)fn("glTextureBufferRangeEXT");
    api->TexStorage2DMultisample =
        (PFNGLTEXSTORAGE2DMULTISAMPLEPROC)fn("glTexStorage2DMultisample");
    api->TexStorage3DMultisample =
        (PFNGLTEXSTORAGE3DMULTISAMPLEPROC)fn("glTexStorage3DMultisample");
    api->TextureStorage2DMultisampleEXT =
        (PFNGLTEXTURESTORAGE2DMULTISAMPLEEXTPROC)fn("glTextureStorage2DMultisampleEXT");
    api->TextureStorage3DMultisampleEXT =
        (PFNGLTEXTURESTORAGE3DMULTISAMPLEEXTPROC)fn("glTextureStorage3DMultisampleEXT");
}
