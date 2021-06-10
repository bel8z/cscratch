#include "gload.h"

//------------------------------------------------------------------------------
// Internal functions and data
//------------------------------------------------------------------------------

static int gl__ParseVersion(void);
static void gl__OpenLib(void);
static void gl__CloseLib(void);
static void *gl__GetProc(const char *proc);
static void gl__LoadProcs(GlApi *gl);

static GlApi g_api = {0};

static struct
{
    int major, minor;
} version;

//------------------------------------------------------------------------------
// Api
//------------------------------------------------------------------------------

GlApi *gl = &g_api;

int
gloadInit(GlApi *api)
{
    if (api)
    {
        gl = api;
    }
    else
    {
        gl = &g_api;
        gl__OpenLib();
        gl__LoadProcs(gl);
        gl__CloseLib();
    }

    return gl__ParseVersion();
}

int
gloadIsSupported(int major, int minor)
{
    if (major < 3) return 0;
    if (version.major == major) return version.minor >= minor;
    return version.major >= major;
}

void *
gloadGetProc(const char *proc)
{
    return gl__GetProc(proc);
}

//------------------------------------------------------------------------------
// Internal implementation
//------------------------------------------------------------------------------

static int
gl__ParseVersion(void)
{
    if (!glGetIntegerv) return 0;

    glGetIntegerv(GL_MAJOR_VERSION, &version.major);
    glGetIntegerv(GL_MINOR_VERSION, &version.minor);

    if (version.major < 3) return 0;

    return 1;
}

static void
gl__LoadProcs(GlApi *api)
{
    api->CullFace = (PFNGLCULLFACEPROC)gl__GetProc("glCullFace");
    api->FrontFace = (PFNGLFRONTFACEPROC)gl__GetProc("glFrontFace");
    api->Hint = (PFNGLHINTPROC)gl__GetProc("glHint");
    api->LineWidth = (PFNGLLINEWIDTHPROC)gl__GetProc("glLineWidth");
    api->PointSize = (PFNGLPOINTSIZEPROC)gl__GetProc("glPointSize");
    api->PolygonMode = (PFNGLPOLYGONMODEPROC)gl__GetProc("glPolygonMode");
    api->Scissor = (PFNGLSCISSORPROC)gl__GetProc("glScissor");
    api->TexParameterf = (PFNGLTEXPARAMETERFPROC)gl__GetProc("glTexParameterf");
    api->TexParameterfv = (PFNGLTEXPARAMETERFVPROC)gl__GetProc("glTexParameterfv");
    api->TexParameteri = (PFNGLTEXPARAMETERIPROC)gl__GetProc("glTexParameteri");
    api->TexParameteriv = (PFNGLTEXPARAMETERIVPROC)gl__GetProc("glTexParameteriv");
    api->TexImage1D = (PFNGLTEXIMAGE1DPROC)gl__GetProc("glTexImage1D");
    api->TexImage2D = (PFNGLTEXIMAGE2DPROC)gl__GetProc("glTexImage2D");
    api->DrawBuffer = (PFNGLDRAWBUFFERPROC)gl__GetProc("glDrawBuffer");
    api->Clear = (PFNGLCLEARPROC)gl__GetProc("glClear");
    api->ClearColor = (PFNGLCLEARCOLORPROC)gl__GetProc("glClearColor");
    api->ClearStencil = (PFNGLCLEARSTENCILPROC)gl__GetProc("glClearStencil");
    api->ClearDepth = (PFNGLCLEARDEPTHPROC)gl__GetProc("glClearDepth");
    api->StencilMask = (PFNGLSTENCILMASKPROC)gl__GetProc("glStencilMask");
    api->ColorMask = (PFNGLCOLORMASKPROC)gl__GetProc("glColorMask");
    api->DepthMask = (PFNGLDEPTHMASKPROC)gl__GetProc("glDepthMask");
    api->Disable = (PFNGLDISABLEPROC)gl__GetProc("glDisable");
    api->Enable = (PFNGLENABLEPROC)gl__GetProc("glEnable");
    api->Finish = (PFNGLFINISHPROC)gl__GetProc("glFinish");
    api->Flush = (PFNGLFLUSHPROC)gl__GetProc("glFlush");
    api->BlendFunc = (PFNGLBLENDFUNCPROC)gl__GetProc("glBlendFunc");
    api->LogicOp = (PFNGLLOGICOPPROC)gl__GetProc("glLogicOp");
    api->StencilFunc = (PFNGLSTENCILFUNCPROC)gl__GetProc("glStencilFunc");
    api->StencilOp = (PFNGLSTENCILOPPROC)gl__GetProc("glStencilOp");
    api->DepthFunc = (PFNGLDEPTHFUNCPROC)gl__GetProc("glDepthFunc");
    api->PixelStoref = (PFNGLPIXELSTOREFPROC)gl__GetProc("glPixelStoref");
    api->PixelStorei = (PFNGLPIXELSTOREIPROC)gl__GetProc("glPixelStorei");
    api->ReadBuffer = (PFNGLREADBUFFERPROC)gl__GetProc("glReadBuffer");
    api->ReadPixels = (PFNGLREADPIXELSPROC)gl__GetProc("glReadPixels");
    api->GetBooleanv = (PFNGLGETBOOLEANVPROC)gl__GetProc("glGetBooleanv");
    api->GetDoublev = (PFNGLGETDOUBLEVPROC)gl__GetProc("glGetDoublev");
    api->GetError = (PFNGLGETERRORPROC)gl__GetProc("glGetError");
    api->GetFloatv = (PFNGLGETFLOATVPROC)gl__GetProc("glGetFloatv");
    api->GetIntegerv = (PFNGLGETINTEGERVPROC)gl__GetProc("glGetIntegerv");
    api->GetString = (PFNGLGETSTRINGPROC)gl__GetProc("glGetString");
    api->GetTexImage = (PFNGLGETTEXIMAGEPROC)gl__GetProc("glGetTexImage");
    api->GetTexParameterfv = (PFNGLGETTEXPARAMETERFVPROC)gl__GetProc("glGetTexParameterfv");
    api->GetTexParameteriv = (PFNGLGETTEXPARAMETERIVPROC)gl__GetProc("glGetTexParameteriv");
    api->GetTexLevelParameterfv =
        (PFNGLGETTEXLEVELPARAMETERFVPROC)gl__GetProc("glGetTexLevelParameterfv");
    api->GetTexLevelParameteriv =
        (PFNGLGETTEXLEVELPARAMETERIVPROC)gl__GetProc("glGetTexLevelParameteriv");
    api->IsEnabled = (PFNGLISENABLEDPROC)gl__GetProc("glIsEnabled");
    api->DepthRange = (PFNGLDEPTHRANGEPROC)gl__GetProc("glDepthRange");
    api->Viewport = (PFNGLVIEWPORTPROC)gl__GetProc("glViewport");
    api->DrawArrays = (PFNGLDRAWARRAYSPROC)gl__GetProc("glDrawArrays");
    api->DrawElements = (PFNGLDRAWELEMENTSPROC)gl__GetProc("glDrawElements");
    api->GetPointerv = (PFNGLGETPOINTERVPROC)gl__GetProc("glGetPointerv");
    api->PolygonOffset = (PFNGLPOLYGONOFFSETPROC)gl__GetProc("glPolygonOffset");
    api->CopyTexImage1D = (PFNGLCOPYTEXIMAGE1DPROC)gl__GetProc("glCopyTexImage1D");
    api->CopyTexImage2D = (PFNGLCOPYTEXIMAGE2DPROC)gl__GetProc("glCopyTexImage2D");
    api->CopyTexSubImage1D = (PFNGLCOPYTEXSUBIMAGE1DPROC)gl__GetProc("glCopyTexSubImage1D");
    api->CopyTexSubImage2D = (PFNGLCOPYTEXSUBIMAGE2DPROC)gl__GetProc("glCopyTexSubImage2D");
    api->TexSubImage1D = (PFNGLTEXSUBIMAGE1DPROC)gl__GetProc("glTexSubImage1D");
    api->TexSubImage2D = (PFNGLTEXSUBIMAGE2DPROC)gl__GetProc("glTexSubImage2D");
    api->BindTexture = (PFNGLBINDTEXTUREPROC)gl__GetProc("glBindTexture");
    api->DeleteTextures = (PFNGLDELETETEXTURESPROC)gl__GetProc("glDeleteTextures");
    api->GenTextures = (PFNGLGENTEXTURESPROC)gl__GetProc("glGenTextures");
    api->IsTexture = (PFNGLISTEXTUREPROC)gl__GetProc("glIsTexture");
    api->BlendColor = (PFNGLBLENDCOLORPROC)gl__GetProc("glBlendColor");
    api->BlendEquation = (PFNGLBLENDEQUATIONPROC)gl__GetProc("glBlendEquation");
    api->DrawRangeElements = (PFNGLDRAWRANGEELEMENTSPROC)gl__GetProc("glDrawRangeElements");
    api->TexImage3D = (PFNGLTEXIMAGE3DPROC)gl__GetProc("glTexImage3D");
    api->TexSubImage3D = (PFNGLTEXSUBIMAGE3DPROC)gl__GetProc("glTexSubImage3D");
    api->CopyTexSubImage3D = (PFNGLCOPYTEXSUBIMAGE3DPROC)gl__GetProc("glCopyTexSubImage3D");
    api->ActiveTexture = (PFNGLACTIVETEXTUREPROC)gl__GetProc("glActiveTexture");
    api->SampleCoverage = (PFNGLSAMPLECOVERAGEPROC)gl__GetProc("glSampleCoverage");
    api->CompressedTexImage3D =
        (PFNGLCOMPRESSEDTEXIMAGE3DPROC)gl__GetProc("glCompressedTexImage3D");
    api->CompressedTexImage2D =
        (PFNGLCOMPRESSEDTEXIMAGE2DPROC)gl__GetProc("glCompressedTexImage2D");
    api->CompressedTexImage1D =
        (PFNGLCOMPRESSEDTEXIMAGE1DPROC)gl__GetProc("glCompressedTexImage1D");
    api->CompressedTexSubImage3D =
        (PFNGLCOMPRESSEDTEXSUBIMAGE3DPROC)gl__GetProc("glCompressedTexSubImage3D");
    api->CompressedTexSubImage2D =
        (PFNGLCOMPRESSEDTEXSUBIMAGE2DPROC)gl__GetProc("glCompressedTexSubImage2D");
    api->CompressedTexSubImage1D =
        (PFNGLCOMPRESSEDTEXSUBIMAGE1DPROC)gl__GetProc("glCompressedTexSubImage1D");
    api->GetCompressedTexImage =
        (PFNGLGETCOMPRESSEDTEXIMAGEPROC)gl__GetProc("glGetCompressedTexImage");
    api->BlendFuncSeparate = (PFNGLBLENDFUNCSEPARATEPROC)gl__GetProc("glBlendFuncSeparate");
    api->MultiDrawArrays = (PFNGLMULTIDRAWARRAYSPROC)gl__GetProc("glMultiDrawArrays");
    api->MultiDrawElements = (PFNGLMULTIDRAWELEMENTSPROC)gl__GetProc("glMultiDrawElements");
    api->PointParameterf = (PFNGLPOINTPARAMETERFPROC)gl__GetProc("glPointParameterf");
    api->PointParameterfv = (PFNGLPOINTPARAMETERFVPROC)gl__GetProc("glPointParameterfv");
    api->PointParameteri = (PFNGLPOINTPARAMETERIPROC)gl__GetProc("glPointParameteri");
    api->PointParameteriv = (PFNGLPOINTPARAMETERIVPROC)gl__GetProc("glPointParameteriv");
    api->GenQueries = (PFNGLGENQUERIESPROC)gl__GetProc("glGenQueries");
    api->DeleteQueries = (PFNGLDELETEQUERIESPROC)gl__GetProc("glDeleteQueries");
    api->IsQuery = (PFNGLISQUERYPROC)gl__GetProc("glIsQuery");
    api->BeginQuery = (PFNGLBEGINQUERYPROC)gl__GetProc("glBeginQuery");
    api->EndQuery = (PFNGLENDQUERYPROC)gl__GetProc("glEndQuery");
    api->GetQueryiv = (PFNGLGETQUERYIVPROC)gl__GetProc("glGetQueryiv");
    api->GetQueryObjectiv = (PFNGLGETQUERYOBJECTIVPROC)gl__GetProc("glGetQueryObjectiv");
    api->GetQueryObjectuiv = (PFNGLGETQUERYOBJECTUIVPROC)gl__GetProc("glGetQueryObjectuiv");
    api->BindBuffer = (PFNGLBINDBUFFERPROC)gl__GetProc("glBindBuffer");
    api->DeleteBuffers = (PFNGLDELETEBUFFERSPROC)gl__GetProc("glDeleteBuffers");
    api->GenBuffers = (PFNGLGENBUFFERSPROC)gl__GetProc("glGenBuffers");
    api->IsBuffer = (PFNGLISBUFFERPROC)gl__GetProc("glIsBuffer");
    api->BufferData = (PFNGLBUFFERDATAPROC)gl__GetProc("glBufferData");
    api->BufferSubData = (PFNGLBUFFERSUBDATAPROC)gl__GetProc("glBufferSubData");
    api->GetBufferSubData = (PFNGLGETBUFFERSUBDATAPROC)gl__GetProc("glGetBufferSubData");
    api->MapBuffer = (PFNGLMAPBUFFERPROC)gl__GetProc("glMapBuffer");
    api->UnmapBuffer = (PFNGLUNMAPBUFFERPROC)gl__GetProc("glUnmapBuffer");
    api->GetBufferParameteriv =
        (PFNGLGETBUFFERPARAMETERIVPROC)gl__GetProc("glGetBufferParameteriv");
    api->GetBufferPointerv = (PFNGLGETBUFFERPOINTERVPROC)gl__GetProc("glGetBufferPointerv");
    api->BlendEquationSeparate =
        (PFNGLBLENDEQUATIONSEPARATEPROC)gl__GetProc("glBlendEquationSeparate");
    api->DrawBuffers = (PFNGLDRAWBUFFERSPROC)gl__GetProc("glDrawBuffers");
    api->StencilOpSeparate = (PFNGLSTENCILOPSEPARATEPROC)gl__GetProc("glStencilOpSeparate");
    api->StencilFuncSeparate = (PFNGLSTENCILFUNCSEPARATEPROC)gl__GetProc("glStencilFuncSeparate");
    api->StencilMaskSeparate = (PFNGLSTENCILMASKSEPARATEPROC)gl__GetProc("glStencilMaskSeparate");
    api->AttachShader = (PFNGLATTACHSHADERPROC)gl__GetProc("glAttachShader");
    api->BindAttribLocation = (PFNGLBINDATTRIBLOCATIONPROC)gl__GetProc("glBindAttribLocation");
    api->CompileShader = (PFNGLCOMPILESHADERPROC)gl__GetProc("glCompileShader");
    api->CreateProgram = (PFNGLCREATEPROGRAMPROC)gl__GetProc("glCreateProgram");
    api->CreateShader = (PFNGLCREATESHADERPROC)gl__GetProc("glCreateShader");
    api->DeleteProgram = (PFNGLDELETEPROGRAMPROC)gl__GetProc("glDeleteProgram");
    api->DeleteShader = (PFNGLDELETESHADERPROC)gl__GetProc("glDeleteShader");
    api->DetachShader = (PFNGLDETACHSHADERPROC)gl__GetProc("glDetachShader");
    api->DisableVertexAttribArray =
        (PFNGLDISABLEVERTEXATTRIBARRAYPROC)gl__GetProc("glDisableVertexAttribArray");
    api->EnableVertexAttribArray =
        (PFNGLENABLEVERTEXATTRIBARRAYPROC)gl__GetProc("glEnableVertexAttribArray");
    api->GetActiveAttrib = (PFNGLGETACTIVEATTRIBPROC)gl__GetProc("glGetActiveAttrib");
    api->GetActiveUniform = (PFNGLGETACTIVEUNIFORMPROC)gl__GetProc("glGetActiveUniform");
    api->GetAttachedShaders = (PFNGLGETATTACHEDSHADERSPROC)gl__GetProc("glGetAttachedShaders");
    api->GetAttribLocation = (PFNGLGETATTRIBLOCATIONPROC)gl__GetProc("glGetAttribLocation");
    api->GetProgramiv = (PFNGLGETPROGRAMIVPROC)gl__GetProc("glGetProgramiv");
    api->GetProgramInfoLog = (PFNGLGETPROGRAMINFOLOGPROC)gl__GetProc("glGetProgramInfoLog");
    api->GetShaderiv = (PFNGLGETSHADERIVPROC)gl__GetProc("glGetShaderiv");
    api->GetShaderInfoLog = (PFNGLGETSHADERINFOLOGPROC)gl__GetProc("glGetShaderInfoLog");
    api->GetShaderSource = (PFNGLGETSHADERSOURCEPROC)gl__GetProc("glGetShaderSource");
    api->GetUniformLocation = (PFNGLGETUNIFORMLOCATIONPROC)gl__GetProc("glGetUniformLocation");
    api->GetUniformfv = (PFNGLGETUNIFORMFVPROC)gl__GetProc("glGetUniformfv");
    api->GetUniformiv = (PFNGLGETUNIFORMIVPROC)gl__GetProc("glGetUniformiv");
    api->GetVertexAttribdv = (PFNGLGETVERTEXATTRIBDVPROC)gl__GetProc("glGetVertexAttribdv");
    api->GetVertexAttribfv = (PFNGLGETVERTEXATTRIBFVPROC)gl__GetProc("glGetVertexAttribfv");
    api->GetVertexAttribiv = (PFNGLGETVERTEXATTRIBIVPROC)gl__GetProc("glGetVertexAttribiv");
    api->GetVertexAttribPointerv =
        (PFNGLGETVERTEXATTRIBPOINTERVPROC)gl__GetProc("glGetVertexAttribPointerv");
    api->IsProgram = (PFNGLISPROGRAMPROC)gl__GetProc("glIsProgram");
    api->IsShader = (PFNGLISSHADERPROC)gl__GetProc("glIsShader");
    api->LinkProgram = (PFNGLLINKPROGRAMPROC)gl__GetProc("glLinkProgram");
    api->ShaderSource = (PFNGLSHADERSOURCEPROC)gl__GetProc("glShaderSource");
    api->UseProgram = (PFNGLUSEPROGRAMPROC)gl__GetProc("glUseProgram");
    api->Uniform1f = (PFNGLUNIFORM1FPROC)gl__GetProc("glUniform1f");
    api->Uniform2f = (PFNGLUNIFORM2FPROC)gl__GetProc("glUniform2f");
    api->Uniform3f = (PFNGLUNIFORM3FPROC)gl__GetProc("glUniform3f");
    api->Uniform4f = (PFNGLUNIFORM4FPROC)gl__GetProc("glUniform4f");
    api->Uniform1i = (PFNGLUNIFORM1IPROC)gl__GetProc("glUniform1i");
    api->Uniform2i = (PFNGLUNIFORM2IPROC)gl__GetProc("glUniform2i");
    api->Uniform3i = (PFNGLUNIFORM3IPROC)gl__GetProc("glUniform3i");
    api->Uniform4i = (PFNGLUNIFORM4IPROC)gl__GetProc("glUniform4i");
    api->Uniform1fv = (PFNGLUNIFORM1FVPROC)gl__GetProc("glUniform1fv");
    api->Uniform2fv = (PFNGLUNIFORM2FVPROC)gl__GetProc("glUniform2fv");
    api->Uniform3fv = (PFNGLUNIFORM3FVPROC)gl__GetProc("glUniform3fv");
    api->Uniform4fv = (PFNGLUNIFORM4FVPROC)gl__GetProc("glUniform4fv");
    api->Uniform1iv = (PFNGLUNIFORM1IVPROC)gl__GetProc("glUniform1iv");
    api->Uniform2iv = (PFNGLUNIFORM2IVPROC)gl__GetProc("glUniform2iv");
    api->Uniform3iv = (PFNGLUNIFORM3IVPROC)gl__GetProc("glUniform3iv");
    api->Uniform4iv = (PFNGLUNIFORM4IVPROC)gl__GetProc("glUniform4iv");
    api->UniformMatrix2fv = (PFNGLUNIFORMMATRIX2FVPROC)gl__GetProc("glUniformMatrix2fv");
    api->UniformMatrix3fv = (PFNGLUNIFORMMATRIX3FVPROC)gl__GetProc("glUniformMatrix3fv");
    api->UniformMatrix4fv = (PFNGLUNIFORMMATRIX4FVPROC)gl__GetProc("glUniformMatrix4fv");
    api->ValidateProgram = (PFNGLVALIDATEPROGRAMPROC)gl__GetProc("glValidateProgram");
    api->VertexAttrib1d = (PFNGLVERTEXATTRIB1DPROC)gl__GetProc("glVertexAttrib1d");
    api->VertexAttrib1dv = (PFNGLVERTEXATTRIB1DVPROC)gl__GetProc("glVertexAttrib1dv");
    api->VertexAttrib1f = (PFNGLVERTEXATTRIB1FPROC)gl__GetProc("glVertexAttrib1f");
    api->VertexAttrib1fv = (PFNGLVERTEXATTRIB1FVPROC)gl__GetProc("glVertexAttrib1fv");
    api->VertexAttrib1s = (PFNGLVERTEXATTRIB1SPROC)gl__GetProc("glVertexAttrib1s");
    api->VertexAttrib1sv = (PFNGLVERTEXATTRIB1SVPROC)gl__GetProc("glVertexAttrib1sv");
    api->VertexAttrib2d = (PFNGLVERTEXATTRIB2DPROC)gl__GetProc("glVertexAttrib2d");
    api->VertexAttrib2dv = (PFNGLVERTEXATTRIB2DVPROC)gl__GetProc("glVertexAttrib2dv");
    api->VertexAttrib2f = (PFNGLVERTEXATTRIB2FPROC)gl__GetProc("glVertexAttrib2f");
    api->VertexAttrib2fv = (PFNGLVERTEXATTRIB2FVPROC)gl__GetProc("glVertexAttrib2fv");
    api->VertexAttrib2s = (PFNGLVERTEXATTRIB2SPROC)gl__GetProc("glVertexAttrib2s");
    api->VertexAttrib2sv = (PFNGLVERTEXATTRIB2SVPROC)gl__GetProc("glVertexAttrib2sv");
    api->VertexAttrib3d = (PFNGLVERTEXATTRIB3DPROC)gl__GetProc("glVertexAttrib3d");
    api->VertexAttrib3dv = (PFNGLVERTEXATTRIB3DVPROC)gl__GetProc("glVertexAttrib3dv");
    api->VertexAttrib3f = (PFNGLVERTEXATTRIB3FPROC)gl__GetProc("glVertexAttrib3f");
    api->VertexAttrib3fv = (PFNGLVERTEXATTRIB3FVPROC)gl__GetProc("glVertexAttrib3fv");
    api->VertexAttrib3s = (PFNGLVERTEXATTRIB3SPROC)gl__GetProc("glVertexAttrib3s");
    api->VertexAttrib3sv = (PFNGLVERTEXATTRIB3SVPROC)gl__GetProc("glVertexAttrib3sv");
    api->VertexAttrib4Nbv = (PFNGLVERTEXATTRIB4NBVPROC)gl__GetProc("glVertexAttrib4Nbv");
    api->VertexAttrib4Niv = (PFNGLVERTEXATTRIB4NIVPROC)gl__GetProc("glVertexAttrib4Niv");
    api->VertexAttrib4Nsv = (PFNGLVERTEXATTRIB4NSVPROC)gl__GetProc("glVertexAttrib4Nsv");
    api->VertexAttrib4Nub = (PFNGLVERTEXATTRIB4NUBPROC)gl__GetProc("glVertexAttrib4Nub");
    api->VertexAttrib4Nubv = (PFNGLVERTEXATTRIB4NUBVPROC)gl__GetProc("glVertexAttrib4Nubv");
    api->VertexAttrib4Nuiv = (PFNGLVERTEXATTRIB4NUIVPROC)gl__GetProc("glVertexAttrib4Nuiv");
    api->VertexAttrib4Nusv = (PFNGLVERTEXATTRIB4NUSVPROC)gl__GetProc("glVertexAttrib4Nusv");
    api->VertexAttrib4bv = (PFNGLVERTEXATTRIB4BVPROC)gl__GetProc("glVertexAttrib4bv");
    api->VertexAttrib4d = (PFNGLVERTEXATTRIB4DPROC)gl__GetProc("glVertexAttrib4d");
    api->VertexAttrib4dv = (PFNGLVERTEXATTRIB4DVPROC)gl__GetProc("glVertexAttrib4dv");
    api->VertexAttrib4f = (PFNGLVERTEXATTRIB4FPROC)gl__GetProc("glVertexAttrib4f");
    api->VertexAttrib4fv = (PFNGLVERTEXATTRIB4FVPROC)gl__GetProc("glVertexAttrib4fv");
    api->VertexAttrib4iv = (PFNGLVERTEXATTRIB4IVPROC)gl__GetProc("glVertexAttrib4iv");
    api->VertexAttrib4s = (PFNGLVERTEXATTRIB4SPROC)gl__GetProc("glVertexAttrib4s");
    api->VertexAttrib4sv = (PFNGLVERTEXATTRIB4SVPROC)gl__GetProc("glVertexAttrib4sv");
    api->VertexAttrib4ubv = (PFNGLVERTEXATTRIB4UBVPROC)gl__GetProc("glVertexAttrib4ubv");
    api->VertexAttrib4uiv = (PFNGLVERTEXATTRIB4UIVPROC)gl__GetProc("glVertexAttrib4uiv");
    api->VertexAttrib4usv = (PFNGLVERTEXATTRIB4USVPROC)gl__GetProc("glVertexAttrib4usv");
    api->VertexAttribPointer = (PFNGLVERTEXATTRIBPOINTERPROC)gl__GetProc("glVertexAttribPointer");
    api->UniformMatrix2x3fv = (PFNGLUNIFORMMATRIX2X3FVPROC)gl__GetProc("glUniformMatrix2x3fv");
    api->UniformMatrix3x2fv = (PFNGLUNIFORMMATRIX3X2FVPROC)gl__GetProc("glUniformMatrix3x2fv");
    api->UniformMatrix2x4fv = (PFNGLUNIFORMMATRIX2X4FVPROC)gl__GetProc("glUniformMatrix2x4fv");
    api->UniformMatrix4x2fv = (PFNGLUNIFORMMATRIX4X2FVPROC)gl__GetProc("glUniformMatrix4x2fv");
    api->UniformMatrix3x4fv = (PFNGLUNIFORMMATRIX3X4FVPROC)gl__GetProc("glUniformMatrix3x4fv");
    api->UniformMatrix4x3fv = (PFNGLUNIFORMMATRIX4X3FVPROC)gl__GetProc("glUniformMatrix4x3fv");
    api->ColorMaski = (PFNGLCOLORMASKIPROC)gl__GetProc("glColorMaski");
    api->GetBooleani_v = (PFNGLGETBOOLEANI_VPROC)gl__GetProc("glGetBooleani_v");
    api->GetIntegeri_v = (PFNGLGETINTEGERI_VPROC)gl__GetProc("glGetIntegeri_v");
    api->Enablei = (PFNGLENABLEIPROC)gl__GetProc("glEnablei");
    api->Disablei = (PFNGLDISABLEIPROC)gl__GetProc("glDisablei");
    api->IsEnabledi = (PFNGLISENABLEDIPROC)gl__GetProc("glIsEnabledi");
    api->BeginTransformFeedback =
        (PFNGLBEGINTRANSFORMFEEDBACKPROC)gl__GetProc("glBeginTransformFeedback");
    api->EndTransformFeedback =
        (PFNGLENDTRANSFORMFEEDBACKPROC)gl__GetProc("glEndTransformFeedback");
    api->BindBufferRange = (PFNGLBINDBUFFERRANGEPROC)gl__GetProc("glBindBufferRange");
    api->BindBufferBase = (PFNGLBINDBUFFERBASEPROC)gl__GetProc("glBindBufferBase");
    api->TransformFeedbackVaryings =
        (PFNGLTRANSFORMFEEDBACKVARYINGSPROC)gl__GetProc("glTransformFeedbackVaryings");
    api->GetTransformFeedbackVarying =
        (PFNGLGETTRANSFORMFEEDBACKVARYINGPROC)gl__GetProc("glGetTransformFeedbackVarying");
    api->ClampColor = (PFNGLCLAMPCOLORPROC)gl__GetProc("glClampColor");
    api->BeginConditionalRender =
        (PFNGLBEGINCONDITIONALRENDERPROC)gl__GetProc("glBeginConditionalRender");
    api->EndConditionalRender =
        (PFNGLENDCONDITIONALRENDERPROC)gl__GetProc("glEndConditionalRender");
    api->VertexAttribIPointer =
        (PFNGLVERTEXATTRIBIPOINTERPROC)gl__GetProc("glVertexAttribIPointer");
    api->GetVertexAttribIiv = (PFNGLGETVERTEXATTRIBIIVPROC)gl__GetProc("glGetVertexAttribIiv");
    api->GetVertexAttribIuiv = (PFNGLGETVERTEXATTRIBIUIVPROC)gl__GetProc("glGetVertexAttribIuiv");
    api->VertexAttribI1i = (PFNGLVERTEXATTRIBI1IPROC)gl__GetProc("glVertexAttribI1i");
    api->VertexAttribI2i = (PFNGLVERTEXATTRIBI2IPROC)gl__GetProc("glVertexAttribI2i");
    api->VertexAttribI3i = (PFNGLVERTEXATTRIBI3IPROC)gl__GetProc("glVertexAttribI3i");
    api->VertexAttribI4i = (PFNGLVERTEXATTRIBI4IPROC)gl__GetProc("glVertexAttribI4i");
    api->VertexAttribI1ui = (PFNGLVERTEXATTRIBI1UIPROC)gl__GetProc("glVertexAttribI1ui");
    api->VertexAttribI2ui = (PFNGLVERTEXATTRIBI2UIPROC)gl__GetProc("glVertexAttribI2ui");
    api->VertexAttribI3ui = (PFNGLVERTEXATTRIBI3UIPROC)gl__GetProc("glVertexAttribI3ui");
    api->VertexAttribI4ui = (PFNGLVERTEXATTRIBI4UIPROC)gl__GetProc("glVertexAttribI4ui");
    api->VertexAttribI1iv = (PFNGLVERTEXATTRIBI1IVPROC)gl__GetProc("glVertexAttribI1iv");
    api->VertexAttribI2iv = (PFNGLVERTEXATTRIBI2IVPROC)gl__GetProc("glVertexAttribI2iv");
    api->VertexAttribI3iv = (PFNGLVERTEXATTRIBI3IVPROC)gl__GetProc("glVertexAttribI3iv");
    api->VertexAttribI4iv = (PFNGLVERTEXATTRIBI4IVPROC)gl__GetProc("glVertexAttribI4iv");
    api->VertexAttribI1uiv = (PFNGLVERTEXATTRIBI1UIVPROC)gl__GetProc("glVertexAttribI1uiv");
    api->VertexAttribI2uiv = (PFNGLVERTEXATTRIBI2UIVPROC)gl__GetProc("glVertexAttribI2uiv");
    api->VertexAttribI3uiv = (PFNGLVERTEXATTRIBI3UIVPROC)gl__GetProc("glVertexAttribI3uiv");
    api->VertexAttribI4uiv = (PFNGLVERTEXATTRIBI4UIVPROC)gl__GetProc("glVertexAttribI4uiv");
    api->VertexAttribI4bv = (PFNGLVERTEXATTRIBI4BVPROC)gl__GetProc("glVertexAttribI4bv");
    api->VertexAttribI4sv = (PFNGLVERTEXATTRIBI4SVPROC)gl__GetProc("glVertexAttribI4sv");
    api->VertexAttribI4ubv = (PFNGLVERTEXATTRIBI4UBVPROC)gl__GetProc("glVertexAttribI4ubv");
    api->VertexAttribI4usv = (PFNGLVERTEXATTRIBI4USVPROC)gl__GetProc("glVertexAttribI4usv");
    api->GetUniformuiv = (PFNGLGETUNIFORMUIVPROC)gl__GetProc("glGetUniformuiv");
    api->BindFragDataLocation =
        (PFNGLBINDFRAGDATALOCATIONPROC)gl__GetProc("glBindFragDataLocation");
    api->GetFragDataLocation = (PFNGLGETFRAGDATALOCATIONPROC)gl__GetProc("glGetFragDataLocation");
    api->Uniform1ui = (PFNGLUNIFORM1UIPROC)gl__GetProc("glUniform1ui");
    api->Uniform2ui = (PFNGLUNIFORM2UIPROC)gl__GetProc("glUniform2ui");
    api->Uniform3ui = (PFNGLUNIFORM3UIPROC)gl__GetProc("glUniform3ui");
    api->Uniform4ui = (PFNGLUNIFORM4UIPROC)gl__GetProc("glUniform4ui");
    api->Uniform1uiv = (PFNGLUNIFORM1UIVPROC)gl__GetProc("glUniform1uiv");
    api->Uniform2uiv = (PFNGLUNIFORM2UIVPROC)gl__GetProc("glUniform2uiv");
    api->Uniform3uiv = (PFNGLUNIFORM3UIVPROC)gl__GetProc("glUniform3uiv");
    api->Uniform4uiv = (PFNGLUNIFORM4UIVPROC)gl__GetProc("glUniform4uiv");
    api->TexParameterIiv = (PFNGLTEXPARAMETERIIVPROC)gl__GetProc("glTexParameterIiv");
    api->TexParameterIuiv = (PFNGLTEXPARAMETERIUIVPROC)gl__GetProc("glTexParameterIuiv");
    api->GetTexParameterIiv = (PFNGLGETTEXPARAMETERIIVPROC)gl__GetProc("glGetTexParameterIiv");
    api->GetTexParameterIuiv = (PFNGLGETTEXPARAMETERIUIVPROC)gl__GetProc("glGetTexParameterIuiv");
    api->ClearBufferiv = (PFNGLCLEARBUFFERIVPROC)gl__GetProc("glClearBufferiv");
    api->ClearBufferuiv = (PFNGLCLEARBUFFERUIVPROC)gl__GetProc("glClearBufferuiv");
    api->ClearBufferfv = (PFNGLCLEARBUFFERFVPROC)gl__GetProc("glClearBufferfv");
    api->ClearBufferfi = (PFNGLCLEARBUFFERFIPROC)gl__GetProc("glClearBufferfi");
    api->GetStringi = (PFNGLGETSTRINGIPROC)gl__GetProc("glGetStringi");
    api->DrawArraysInstanced = (PFNGLDRAWARRAYSINSTANCEDPROC)gl__GetProc("glDrawArraysInstanced");
    api->DrawElementsInstanced =
        (PFNGLDRAWELEMENTSINSTANCEDPROC)gl__GetProc("glDrawElementsInstanced");
    api->TexBuffer = (PFNGLTEXBUFFERPROC)gl__GetProc("glTexBuffer");
    api->PrimitiveRestartIndex =
        (PFNGLPRIMITIVERESTARTINDEXPROC)gl__GetProc("glPrimitiveRestartIndex");
    api->GetInteger64i_v = (PFNGLGETINTEGER64I_VPROC)gl__GetProc("glGetInteger64i_v");
    api->GetBufferParameteri64v =
        (PFNGLGETBUFFERPARAMETERI64VPROC)gl__GetProc("glGetBufferParameteri64v");
    api->FramebufferTexture = (PFNGLFRAMEBUFFERTEXTUREPROC)gl__GetProc("glFramebufferTexture");
    api->VertexAttribDivisor = (PFNGLVERTEXATTRIBDIVISORPROC)gl__GetProc("glVertexAttribDivisor");
    api->MinSampleShading = (PFNGLMINSAMPLESHADINGPROC)gl__GetProc("glMinSampleShading");
    api->BlendEquationi = (PFNGLBLENDEQUATIONIPROC)gl__GetProc("glBlendEquationi");
    api->BlendEquationSeparatei =
        (PFNGLBLENDEQUATIONSEPARATEIPROC)gl__GetProc("glBlendEquationSeparatei");
    api->BlendFunci = (PFNGLBLENDFUNCIPROC)gl__GetProc("glBlendFunci");
    api->BlendFuncSeparatei = (PFNGLBLENDFUNCSEPARATEIPROC)gl__GetProc("glBlendFuncSeparatei");
    api->IsRenderbuffer = (PFNGLISRENDERBUFFERPROC)gl__GetProc("glIsRenderbuffer");
    api->BindRenderbuffer = (PFNGLBINDRENDERBUFFERPROC)gl__GetProc("glBindRenderbuffer");
    api->DeleteRenderbuffers = (PFNGLDELETERENDERBUFFERSPROC)gl__GetProc("glDeleteRenderbuffers");
    api->GenRenderbuffers = (PFNGLGENRENDERBUFFERSPROC)gl__GetProc("glGenRenderbuffers");
    api->RenderbufferStorage = (PFNGLRENDERBUFFERSTORAGEPROC)gl__GetProc("glRenderbufferStorage");
    api->GetRenderbufferParameteriv =
        (PFNGLGETRENDERBUFFERPARAMETERIVPROC)gl__GetProc("glGetRenderbufferParameteriv");
    api->IsFramebuffer = (PFNGLISFRAMEBUFFERPROC)gl__GetProc("glIsFramebuffer");
    api->BindFramebuffer = (PFNGLBINDFRAMEBUFFERPROC)gl__GetProc("glBindFramebuffer");
    api->DeleteFramebuffers = (PFNGLDELETEFRAMEBUFFERSPROC)gl__GetProc("glDeleteFramebuffers");
    api->GenFramebuffers = (PFNGLGENFRAMEBUFFERSPROC)gl__GetProc("glGenFramebuffers");
    api->CheckFramebufferStatus =
        (PFNGLCHECKFRAMEBUFFERSTATUSPROC)gl__GetProc("glCheckFramebufferStatus");
    api->FramebufferTexture1D =
        (PFNGLFRAMEBUFFERTEXTURE1DPROC)gl__GetProc("glFramebufferTexture1D");
    api->FramebufferTexture2D =
        (PFNGLFRAMEBUFFERTEXTURE2DPROC)gl__GetProc("glFramebufferTexture2D");
    api->FramebufferTexture3D =
        (PFNGLFRAMEBUFFERTEXTURE3DPROC)gl__GetProc("glFramebufferTexture3D");
    api->FramebufferRenderbuffer =
        (PFNGLFRAMEBUFFERRENDERBUFFERPROC)gl__GetProc("glFramebufferRenderbuffer");
    api->GetFramebufferAttachmentParameteriv =
        (PFNGLGETFRAMEBUFFERATTACHMENTPARAMETERIVPROC)gl__GetProc(
            "glGetFramebufferAttachmentParameteriv");
    api->GenerateMipmap = (PFNGLGENERATEMIPMAPPROC)gl__GetProc("glGenerateMipmap");
    api->BlitFramebuffer = (PFNGLBLITFRAMEBUFFERPROC)gl__GetProc("glBlitFramebuffer");
    api->RenderbufferStorageMultisample =
        (PFNGLRENDERBUFFERSTORAGEMULTISAMPLEPROC)gl__GetProc("glRenderbufferStorageMultisample");
    api->FramebufferTextureLayer =
        (PFNGLFRAMEBUFFERTEXTURELAYERPROC)gl__GetProc("glFramebufferTextureLayer");
    api->MapBufferRange = (PFNGLMAPBUFFERRANGEPROC)gl__GetProc("glMapBufferRange");
    api->FlushMappedBufferRange =
        (PFNGLFLUSHMAPPEDBUFFERRANGEPROC)gl__GetProc("glFlushMappedBufferRange");
    api->BindVertexArray = (PFNGLBINDVERTEXARRAYPROC)gl__GetProc("glBindVertexArray");
    api->DeleteVertexArrays = (PFNGLDELETEVERTEXARRAYSPROC)gl__GetProc("glDeleteVertexArrays");
    api->GenVertexArrays = (PFNGLGENVERTEXARRAYSPROC)gl__GetProc("glGenVertexArrays");
    api->IsVertexArray = (PFNGLISVERTEXARRAYPROC)gl__GetProc("glIsVertexArray");
    api->GetUniformIndices = (PFNGLGETUNIFORMINDICESPROC)gl__GetProc("glGetUniformIndices");
    api->GetActiveUniformsiv = (PFNGLGETACTIVEUNIFORMSIVPROC)gl__GetProc("glGetActiveUniformsiv");
    api->GetActiveUniformName =
        (PFNGLGETACTIVEUNIFORMNAMEPROC)gl__GetProc("glGetActiveUniformName");
    api->GetUniformBlockIndex =
        (PFNGLGETUNIFORMBLOCKINDEXPROC)gl__GetProc("glGetUniformBlockIndex");
    api->GetActiveUniformBlockiv =
        (PFNGLGETACTIVEUNIFORMBLOCKIVPROC)gl__GetProc("glGetActiveUniformBlockiv");
    api->GetActiveUniformBlockName =
        (PFNGLGETACTIVEUNIFORMBLOCKNAMEPROC)gl__GetProc("glGetActiveUniformBlockName");
    api->UniformBlockBinding = (PFNGLUNIFORMBLOCKBINDINGPROC)gl__GetProc("glUniformBlockBinding");
    api->CopyBufferSubData = (PFNGLCOPYBUFFERSUBDATAPROC)gl__GetProc("glCopyBufferSubData");
    api->DrawElementsBaseVertex =
        (PFNGLDRAWELEMENTSBASEVERTEXPROC)gl__GetProc("glDrawElementsBaseVertex");
    api->DrawRangeElementsBaseVertex =
        (PFNGLDRAWRANGEELEMENTSBASEVERTEXPROC)gl__GetProc("glDrawRangeElementsBaseVertex");
    api->DrawElementsInstancedBaseVertex =
        (PFNGLDRAWELEMENTSINSTANCEDBASEVERTEXPROC)gl__GetProc("glDrawElementsInstancedBaseVertex");
    api->MultiDrawElementsBaseVertex =
        (PFNGLMULTIDRAWELEMENTSBASEVERTEXPROC)gl__GetProc("glMultiDrawElementsBaseVertex");
    api->ProvokingVertex = (PFNGLPROVOKINGVERTEXPROC)gl__GetProc("glProvokingVertex");
    api->FenceSync = (PFNGLFENCESYNCPROC)gl__GetProc("glFenceSync");
    api->IsSync = (PFNGLISSYNCPROC)gl__GetProc("glIsSync");
    api->DeleteSync = (PFNGLDELETESYNCPROC)gl__GetProc("glDeleteSync");
    api->ClientWaitSync = (PFNGLCLIENTWAITSYNCPROC)gl__GetProc("glClientWaitSync");
    api->WaitSync = (PFNGLWAITSYNCPROC)gl__GetProc("glWaitSync");
    api->GetInteger64v = (PFNGLGETINTEGER64VPROC)gl__GetProc("glGetInteger64v");
    api->GetSynciv = (PFNGLGETSYNCIVPROC)gl__GetProc("glGetSynciv");
    api->TexImage2DMultisample =
        (PFNGLTEXIMAGE2DMULTISAMPLEPROC)gl__GetProc("glTexImage2DMultisample");
    api->TexImage3DMultisample =
        (PFNGLTEXIMAGE3DMULTISAMPLEPROC)gl__GetProc("glTexImage3DMultisample");
    api->GetMultisamplefv = (PFNGLGETMULTISAMPLEFVPROC)gl__GetProc("glGetMultisamplefv");
    api->SampleMaski = (PFNGLSAMPLEMASKIPROC)gl__GetProc("glSampleMaski");
    api->BlendEquationiARB = (PFNGLBLENDEQUATIONIARBPROC)gl__GetProc("glBlendEquationiARB");
    api->BlendEquationSeparateiARB =
        (PFNGLBLENDEQUATIONSEPARATEIARBPROC)gl__GetProc("glBlendEquationSeparateiARB");
    api->BlendFunciARB = (PFNGLBLENDFUNCIARBPROC)gl__GetProc("glBlendFunciARB");
    api->BlendFuncSeparateiARB =
        (PFNGLBLENDFUNCSEPARATEIARBPROC)gl__GetProc("glBlendFuncSeparateiARB");
    api->MinSampleShadingARB = (PFNGLMINSAMPLESHADINGARBPROC)gl__GetProc("glMinSampleShadingARB");
    api->NamedStringARB = (PFNGLNAMEDSTRINGARBPROC)gl__GetProc("glNamedStringARB");
    api->DeleteNamedStringARB =
        (PFNGLDELETENAMEDSTRINGARBPROC)gl__GetProc("glDeleteNamedStringARB");
    api->CompileShaderIncludeARB =
        (PFNGLCOMPILESHADERINCLUDEARBPROC)gl__GetProc("glCompileShaderIncludeARB");
    api->IsNamedStringARB = (PFNGLISNAMEDSTRINGARBPROC)gl__GetProc("glIsNamedStringARB");
    api->GetNamedStringARB = (PFNGLGETNAMEDSTRINGARBPROC)gl__GetProc("glGetNamedStringARB");
    api->GetNamedStringivARB = (PFNGLGETNAMEDSTRINGIVARBPROC)gl__GetProc("glGetNamedStringivARB");
    api->BindFragDataLocationIndexed =
        (PFNGLBINDFRAGDATALOCATIONINDEXEDPROC)gl__GetProc("glBindFragDataLocationIndexed");
    api->GetFragDataIndex = (PFNGLGETFRAGDATAINDEXPROC)gl__GetProc("glGetFragDataIndex");
    api->GenSamplers = (PFNGLGENSAMPLERSPROC)gl__GetProc("glGenSamplers");
    api->DeleteSamplers = (PFNGLDELETESAMPLERSPROC)gl__GetProc("glDeleteSamplers");
    api->IsSampler = (PFNGLISSAMPLERPROC)gl__GetProc("glIsSampler");
    api->BindSampler = (PFNGLBINDSAMPLERPROC)gl__GetProc("glBindSampler");
    api->SamplerParameteri = (PFNGLSAMPLERPARAMETERIPROC)gl__GetProc("glSamplerParameteri");
    api->SamplerParameteriv = (PFNGLSAMPLERPARAMETERIVPROC)gl__GetProc("glSamplerParameteriv");
    api->SamplerParameterf = (PFNGLSAMPLERPARAMETERFPROC)gl__GetProc("glSamplerParameterf");
    api->SamplerParameterfv = (PFNGLSAMPLERPARAMETERFVPROC)gl__GetProc("glSamplerParameterfv");
    api->SamplerParameterIiv = (PFNGLSAMPLERPARAMETERIIVPROC)gl__GetProc("glSamplerParameterIiv");
    api->SamplerParameterIuiv =
        (PFNGLSAMPLERPARAMETERIUIVPROC)gl__GetProc("glSamplerParameterIuiv");
    api->GetSamplerParameteriv =
        (PFNGLGETSAMPLERPARAMETERIVPROC)gl__GetProc("glGetSamplerParameteriv");
    api->GetSamplerParameterIiv =
        (PFNGLGETSAMPLERPARAMETERIIVPROC)gl__GetProc("glGetSamplerParameterIiv");
    api->GetSamplerParameterfv =
        (PFNGLGETSAMPLERPARAMETERFVPROC)gl__GetProc("glGetSamplerParameterfv");
    api->GetSamplerParameterIuiv =
        (PFNGLGETSAMPLERPARAMETERIUIVPROC)gl__GetProc("glGetSamplerParameterIuiv");
    api->QueryCounter = (PFNGLQUERYCOUNTERPROC)gl__GetProc("glQueryCounter");
    api->GetQueryObjecti64v = (PFNGLGETQUERYOBJECTI64VPROC)gl__GetProc("glGetQueryObjecti64v");
    api->GetQueryObjectui64v = (PFNGLGETQUERYOBJECTUI64VPROC)gl__GetProc("glGetQueryObjectui64v");
    api->VertexP2ui = (PFNGLVERTEXP2UIPROC)gl__GetProc("glVertexP2ui");
    api->VertexP2uiv = (PFNGLVERTEXP2UIVPROC)gl__GetProc("glVertexP2uiv");
    api->VertexP3ui = (PFNGLVERTEXP3UIPROC)gl__GetProc("glVertexP3ui");
    api->VertexP3uiv = (PFNGLVERTEXP3UIVPROC)gl__GetProc("glVertexP3uiv");
    api->VertexP4ui = (PFNGLVERTEXP4UIPROC)gl__GetProc("glVertexP4ui");
    api->VertexP4uiv = (PFNGLVERTEXP4UIVPROC)gl__GetProc("glVertexP4uiv");
    api->TexCoordP1ui = (PFNGLTEXCOORDP1UIPROC)gl__GetProc("glTexCoordP1ui");
    api->TexCoordP1uiv = (PFNGLTEXCOORDP1UIVPROC)gl__GetProc("glTexCoordP1uiv");
    api->TexCoordP2ui = (PFNGLTEXCOORDP2UIPROC)gl__GetProc("glTexCoordP2ui");
    api->TexCoordP2uiv = (PFNGLTEXCOORDP2UIVPROC)gl__GetProc("glTexCoordP2uiv");
    api->TexCoordP3ui = (PFNGLTEXCOORDP3UIPROC)gl__GetProc("glTexCoordP3ui");
    api->TexCoordP3uiv = (PFNGLTEXCOORDP3UIVPROC)gl__GetProc("glTexCoordP3uiv");
    api->TexCoordP4ui = (PFNGLTEXCOORDP4UIPROC)gl__GetProc("glTexCoordP4ui");
    api->TexCoordP4uiv = (PFNGLTEXCOORDP4UIVPROC)gl__GetProc("glTexCoordP4uiv");
    api->MultiTexCoordP1ui = (PFNGLMULTITEXCOORDP1UIPROC)gl__GetProc("glMultiTexCoordP1ui");
    api->MultiTexCoordP1uiv = (PFNGLMULTITEXCOORDP1UIVPROC)gl__GetProc("glMultiTexCoordP1uiv");
    api->MultiTexCoordP2ui = (PFNGLMULTITEXCOORDP2UIPROC)gl__GetProc("glMultiTexCoordP2ui");
    api->MultiTexCoordP2uiv = (PFNGLMULTITEXCOORDP2UIVPROC)gl__GetProc("glMultiTexCoordP2uiv");
    api->MultiTexCoordP3ui = (PFNGLMULTITEXCOORDP3UIPROC)gl__GetProc("glMultiTexCoordP3ui");
    api->MultiTexCoordP3uiv = (PFNGLMULTITEXCOORDP3UIVPROC)gl__GetProc("glMultiTexCoordP3uiv");
    api->MultiTexCoordP4ui = (PFNGLMULTITEXCOORDP4UIPROC)gl__GetProc("glMultiTexCoordP4ui");
    api->MultiTexCoordP4uiv = (PFNGLMULTITEXCOORDP4UIVPROC)gl__GetProc("glMultiTexCoordP4uiv");
    api->NormalP3ui = (PFNGLNORMALP3UIPROC)gl__GetProc("glNormalP3ui");
    api->NormalP3uiv = (PFNGLNORMALP3UIVPROC)gl__GetProc("glNormalP3uiv");
    api->ColorP3ui = (PFNGLCOLORP3UIPROC)gl__GetProc("glColorP3ui");
    api->ColorP3uiv = (PFNGLCOLORP3UIVPROC)gl__GetProc("glColorP3uiv");
    api->ColorP4ui = (PFNGLCOLORP4UIPROC)gl__GetProc("glColorP4ui");
    api->ColorP4uiv = (PFNGLCOLORP4UIVPROC)gl__GetProc("glColorP4uiv");
    api->SecondaryColorP3ui = (PFNGLSECONDARYCOLORP3UIPROC)gl__GetProc("glSecondaryColorP3ui");
    api->SecondaryColorP3uiv = (PFNGLSECONDARYCOLORP3UIVPROC)gl__GetProc("glSecondaryColorP3uiv");
    api->VertexAttribP1ui = (PFNGLVERTEXATTRIBP1UIPROC)gl__GetProc("glVertexAttribP1ui");
    api->VertexAttribP1uiv = (PFNGLVERTEXATTRIBP1UIVPROC)gl__GetProc("glVertexAttribP1uiv");
    api->VertexAttribP2ui = (PFNGLVERTEXATTRIBP2UIPROC)gl__GetProc("glVertexAttribP2ui");
    api->VertexAttribP2uiv = (PFNGLVERTEXATTRIBP2UIVPROC)gl__GetProc("glVertexAttribP2uiv");
    api->VertexAttribP3ui = (PFNGLVERTEXATTRIBP3UIPROC)gl__GetProc("glVertexAttribP3ui");
    api->VertexAttribP3uiv = (PFNGLVERTEXATTRIBP3UIVPROC)gl__GetProc("glVertexAttribP3uiv");
    api->VertexAttribP4ui = (PFNGLVERTEXATTRIBP4UIPROC)gl__GetProc("glVertexAttribP4ui");
    api->VertexAttribP4uiv = (PFNGLVERTEXATTRIBP4UIVPROC)gl__GetProc("glVertexAttribP4uiv");
    api->DrawArraysIndirect = (PFNGLDRAWARRAYSINDIRECTPROC)gl__GetProc("glDrawArraysIndirect");
    api->DrawElementsIndirect =
        (PFNGLDRAWELEMENTSINDIRECTPROC)gl__GetProc("glDrawElementsIndirect");
    api->Uniform1d = (PFNGLUNIFORM1DPROC)gl__GetProc("glUniform1d");
    api->Uniform2d = (PFNGLUNIFORM2DPROC)gl__GetProc("glUniform2d");
    api->Uniform3d = (PFNGLUNIFORM3DPROC)gl__GetProc("glUniform3d");
    api->Uniform4d = (PFNGLUNIFORM4DPROC)gl__GetProc("glUniform4d");
    api->Uniform1dv = (PFNGLUNIFORM1DVPROC)gl__GetProc("glUniform1dv");
    api->Uniform2dv = (PFNGLUNIFORM2DVPROC)gl__GetProc("glUniform2dv");
    api->Uniform3dv = (PFNGLUNIFORM3DVPROC)gl__GetProc("glUniform3dv");
    api->Uniform4dv = (PFNGLUNIFORM4DVPROC)gl__GetProc("glUniform4dv");
    api->UniformMatrix2dv = (PFNGLUNIFORMMATRIX2DVPROC)gl__GetProc("glUniformMatrix2dv");
    api->UniformMatrix3dv = (PFNGLUNIFORMMATRIX3DVPROC)gl__GetProc("glUniformMatrix3dv");
    api->UniformMatrix4dv = (PFNGLUNIFORMMATRIX4DVPROC)gl__GetProc("glUniformMatrix4dv");
    api->UniformMatrix2x3dv = (PFNGLUNIFORMMATRIX2X3DVPROC)gl__GetProc("glUniformMatrix2x3dv");
    api->UniformMatrix2x4dv = (PFNGLUNIFORMMATRIX2X4DVPROC)gl__GetProc("glUniformMatrix2x4dv");
    api->UniformMatrix3x2dv = (PFNGLUNIFORMMATRIX3X2DVPROC)gl__GetProc("glUniformMatrix3x2dv");
    api->UniformMatrix3x4dv = (PFNGLUNIFORMMATRIX3X4DVPROC)gl__GetProc("glUniformMatrix3x4dv");
    api->UniformMatrix4x2dv = (PFNGLUNIFORMMATRIX4X2DVPROC)gl__GetProc("glUniformMatrix4x2dv");
    api->UniformMatrix4x3dv = (PFNGLUNIFORMMATRIX4X3DVPROC)gl__GetProc("glUniformMatrix4x3dv");
    api->GetUniformdv = (PFNGLGETUNIFORMDVPROC)gl__GetProc("glGetUniformdv");
    api->GetSubroutineUniformLocation =
        (PFNGLGETSUBROUTINEUNIFORMLOCATIONPROC)gl__GetProc("glGetSubroutineUniformLocation");
    api->GetSubroutineIndex = (PFNGLGETSUBROUTINEINDEXPROC)gl__GetProc("glGetSubroutineIndex");
    api->GetActiveSubroutineUniformiv =
        (PFNGLGETACTIVESUBROUTINEUNIFORMIVPROC)gl__GetProc("glGetActiveSubroutineUniformiv");
    api->GetActiveSubroutineUniformName =
        (PFNGLGETACTIVESUBROUTINEUNIFORMNAMEPROC)gl__GetProc("glGetActiveSubroutineUniformName");
    api->GetActiveSubroutineName =
        (PFNGLGETACTIVESUBROUTINENAMEPROC)gl__GetProc("glGetActiveSubroutineName");
    api->UniformSubroutinesuiv =
        (PFNGLUNIFORMSUBROUTINESUIVPROC)gl__GetProc("glUniformSubroutinesuiv");
    api->GetUniformSubroutineuiv =
        (PFNGLGETUNIFORMSUBROUTINEUIVPROC)gl__GetProc("glGetUniformSubroutineuiv");
    api->GetProgramStageiv = (PFNGLGETPROGRAMSTAGEIVPROC)gl__GetProc("glGetProgramStageiv");
    api->PatchParameteri = (PFNGLPATCHPARAMETERIPROC)gl__GetProc("glPatchParameteri");
    api->PatchParameterfv = (PFNGLPATCHPARAMETERFVPROC)gl__GetProc("glPatchParameterfv");
    api->BindTransformFeedback =
        (PFNGLBINDTRANSFORMFEEDBACKPROC)gl__GetProc("glBindTransformFeedback");
    api->DeleteTransformFeedbacks =
        (PFNGLDELETETRANSFORMFEEDBACKSPROC)gl__GetProc("glDeleteTransformFeedbacks");
    api->GenTransformFeedbacks =
        (PFNGLGENTRANSFORMFEEDBACKSPROC)gl__GetProc("glGenTransformFeedbacks");
    api->IsTransformFeedback = (PFNGLISTRANSFORMFEEDBACKPROC)gl__GetProc("glIsTransformFeedback");
    api->PauseTransformFeedback =
        (PFNGLPAUSETRANSFORMFEEDBACKPROC)gl__GetProc("glPauseTransformFeedback");
    api->ResumeTransformFeedback =
        (PFNGLRESUMETRANSFORMFEEDBACKPROC)gl__GetProc("glResumeTransformFeedback");
    api->DrawTransformFeedback =
        (PFNGLDRAWTRANSFORMFEEDBACKPROC)gl__GetProc("glDrawTransformFeedback");
    api->DrawTransformFeedbackStream =
        (PFNGLDRAWTRANSFORMFEEDBACKSTREAMPROC)gl__GetProc("glDrawTransformFeedbackStream");
    api->BeginQueryIndexed = (PFNGLBEGINQUERYINDEXEDPROC)gl__GetProc("glBeginQueryIndexed");
    api->EndQueryIndexed = (PFNGLENDQUERYINDEXEDPROC)gl__GetProc("glEndQueryIndexed");
    api->GetQueryIndexediv = (PFNGLGETQUERYINDEXEDIVPROC)gl__GetProc("glGetQueryIndexediv");
    api->ReleaseShaderCompiler =
        (PFNGLRELEASESHADERCOMPILERPROC)gl__GetProc("glReleaseShaderCompiler");
    api->ShaderBinary = (PFNGLSHADERBINARYPROC)gl__GetProc("glShaderBinary");
    api->GetShaderPrecisionFormat =
        (PFNGLGETSHADERPRECISIONFORMATPROC)gl__GetProc("glGetShaderPrecisionFormat");
    api->DepthRangef = (PFNGLDEPTHRANGEFPROC)gl__GetProc("glDepthRangef");
    api->ClearDepthf = (PFNGLCLEARDEPTHFPROC)gl__GetProc("glClearDepthf");
    api->GetProgramBinary = (PFNGLGETPROGRAMBINARYPROC)gl__GetProc("glGetProgramBinary");
    api->ProgramBinary = (PFNGLPROGRAMBINARYPROC)gl__GetProc("glProgramBinary");
    api->ProgramParameteri = (PFNGLPROGRAMPARAMETERIPROC)gl__GetProc("glProgramParameteri");
    api->UseProgramStages = (PFNGLUSEPROGRAMSTAGESPROC)gl__GetProc("glUseProgramStages");
    api->ActiveShaderProgram = (PFNGLACTIVESHADERPROGRAMPROC)gl__GetProc("glActiveShaderProgram");
    api->CreateShaderProgramv =
        (PFNGLCREATESHADERPROGRAMVPROC)gl__GetProc("glCreateShaderProgramv");
    api->BindProgramPipeline = (PFNGLBINDPROGRAMPIPELINEPROC)gl__GetProc("glBindProgramPipeline");
    api->DeleteProgramPipelines =
        (PFNGLDELETEPROGRAMPIPELINESPROC)gl__GetProc("glDeleteProgramPipelines");
    api->GenProgramPipelines = (PFNGLGENPROGRAMPIPELINESPROC)gl__GetProc("glGenProgramPipelines");
    api->IsProgramPipeline = (PFNGLISPROGRAMPIPELINEPROC)gl__GetProc("glIsProgramPipeline");
    api->GetProgramPipelineiv =
        (PFNGLGETPROGRAMPIPELINEIVPROC)gl__GetProc("glGetProgramPipelineiv");
    api->ProgramUniform1i = (PFNGLPROGRAMUNIFORM1IPROC)gl__GetProc("glProgramUniform1i");
    api->ProgramUniform1iv = (PFNGLPROGRAMUNIFORM1IVPROC)gl__GetProc("glProgramUniform1iv");
    api->ProgramUniform1f = (PFNGLPROGRAMUNIFORM1FPROC)gl__GetProc("glProgramUniform1f");
    api->ProgramUniform1fv = (PFNGLPROGRAMUNIFORM1FVPROC)gl__GetProc("glProgramUniform1fv");
    api->ProgramUniform1d = (PFNGLPROGRAMUNIFORM1DPROC)gl__GetProc("glProgramUniform1d");
    api->ProgramUniform1dv = (PFNGLPROGRAMUNIFORM1DVPROC)gl__GetProc("glProgramUniform1dv");
    api->ProgramUniform1ui = (PFNGLPROGRAMUNIFORM1UIPROC)gl__GetProc("glProgramUniform1ui");
    api->ProgramUniform1uiv = (PFNGLPROGRAMUNIFORM1UIVPROC)gl__GetProc("glProgramUniform1uiv");
    api->ProgramUniform2i = (PFNGLPROGRAMUNIFORM2IPROC)gl__GetProc("glProgramUniform2i");
    api->ProgramUniform2iv = (PFNGLPROGRAMUNIFORM2IVPROC)gl__GetProc("glProgramUniform2iv");
    api->ProgramUniform2f = (PFNGLPROGRAMUNIFORM2FPROC)gl__GetProc("glProgramUniform2f");
    api->ProgramUniform2fv = (PFNGLPROGRAMUNIFORM2FVPROC)gl__GetProc("glProgramUniform2fv");
    api->ProgramUniform2d = (PFNGLPROGRAMUNIFORM2DPROC)gl__GetProc("glProgramUniform2d");
    api->ProgramUniform2dv = (PFNGLPROGRAMUNIFORM2DVPROC)gl__GetProc("glProgramUniform2dv");
    api->ProgramUniform2ui = (PFNGLPROGRAMUNIFORM2UIPROC)gl__GetProc("glProgramUniform2ui");
    api->ProgramUniform2uiv = (PFNGLPROGRAMUNIFORM2UIVPROC)gl__GetProc("glProgramUniform2uiv");
    api->ProgramUniform3i = (PFNGLPROGRAMUNIFORM3IPROC)gl__GetProc("glProgramUniform3i");
    api->ProgramUniform3iv = (PFNGLPROGRAMUNIFORM3IVPROC)gl__GetProc("glProgramUniform3iv");
    api->ProgramUniform3f = (PFNGLPROGRAMUNIFORM3FPROC)gl__GetProc("glProgramUniform3f");
    api->ProgramUniform3fv = (PFNGLPROGRAMUNIFORM3FVPROC)gl__GetProc("glProgramUniform3fv");
    api->ProgramUniform3d = (PFNGLPROGRAMUNIFORM3DPROC)gl__GetProc("glProgramUniform3d");
    api->ProgramUniform3dv = (PFNGLPROGRAMUNIFORM3DVPROC)gl__GetProc("glProgramUniform3dv");
    api->ProgramUniform3ui = (PFNGLPROGRAMUNIFORM3UIPROC)gl__GetProc("glProgramUniform3ui");
    api->ProgramUniform3uiv = (PFNGLPROGRAMUNIFORM3UIVPROC)gl__GetProc("glProgramUniform3uiv");
    api->ProgramUniform4i = (PFNGLPROGRAMUNIFORM4IPROC)gl__GetProc("glProgramUniform4i");
    api->ProgramUniform4iv = (PFNGLPROGRAMUNIFORM4IVPROC)gl__GetProc("glProgramUniform4iv");
    api->ProgramUniform4f = (PFNGLPROGRAMUNIFORM4FPROC)gl__GetProc("glProgramUniform4f");
    api->ProgramUniform4fv = (PFNGLPROGRAMUNIFORM4FVPROC)gl__GetProc("glProgramUniform4fv");
    api->ProgramUniform4d = (PFNGLPROGRAMUNIFORM4DPROC)gl__GetProc("glProgramUniform4d");
    api->ProgramUniform4dv = (PFNGLPROGRAMUNIFORM4DVPROC)gl__GetProc("glProgramUniform4dv");
    api->ProgramUniform4ui = (PFNGLPROGRAMUNIFORM4UIPROC)gl__GetProc("glProgramUniform4ui");
    api->ProgramUniform4uiv = (PFNGLPROGRAMUNIFORM4UIVPROC)gl__GetProc("glProgramUniform4uiv");
    api->ProgramUniformMatrix2fv =
        (PFNGLPROGRAMUNIFORMMATRIX2FVPROC)gl__GetProc("glProgramUniformMatrix2fv");
    api->ProgramUniformMatrix3fv =
        (PFNGLPROGRAMUNIFORMMATRIX3FVPROC)gl__GetProc("glProgramUniformMatrix3fv");
    api->ProgramUniformMatrix4fv =
        (PFNGLPROGRAMUNIFORMMATRIX4FVPROC)gl__GetProc("glProgramUniformMatrix4fv");
    api->ProgramUniformMatrix2dv =
        (PFNGLPROGRAMUNIFORMMATRIX2DVPROC)gl__GetProc("glProgramUniformMatrix2dv");
    api->ProgramUniformMatrix3dv =
        (PFNGLPROGRAMUNIFORMMATRIX3DVPROC)gl__GetProc("glProgramUniformMatrix3dv");
    api->ProgramUniformMatrix4dv =
        (PFNGLPROGRAMUNIFORMMATRIX4DVPROC)gl__GetProc("glProgramUniformMatrix4dv");
    api->ProgramUniformMatrix2x3fv =
        (PFNGLPROGRAMUNIFORMMATRIX2X3FVPROC)gl__GetProc("glProgramUniformMatrix2x3fv");
    api->ProgramUniformMatrix3x2fv =
        (PFNGLPROGRAMUNIFORMMATRIX3X2FVPROC)gl__GetProc("glProgramUniformMatrix3x2fv");
    api->ProgramUniformMatrix2x4fv =
        (PFNGLPROGRAMUNIFORMMATRIX2X4FVPROC)gl__GetProc("glProgramUniformMatrix2x4fv");
    api->ProgramUniformMatrix4x2fv =
        (PFNGLPROGRAMUNIFORMMATRIX4X2FVPROC)gl__GetProc("glProgramUniformMatrix4x2fv");
    api->ProgramUniformMatrix3x4fv =
        (PFNGLPROGRAMUNIFORMMATRIX3X4FVPROC)gl__GetProc("glProgramUniformMatrix3x4fv");
    api->ProgramUniformMatrix4x3fv =
        (PFNGLPROGRAMUNIFORMMATRIX4X3FVPROC)gl__GetProc("glProgramUniformMatrix4x3fv");
    api->ProgramUniformMatrix2x3dv =
        (PFNGLPROGRAMUNIFORMMATRIX2X3DVPROC)gl__GetProc("glProgramUniformMatrix2x3dv");
    api->ProgramUniformMatrix3x2dv =
        (PFNGLPROGRAMUNIFORMMATRIX3X2DVPROC)gl__GetProc("glProgramUniformMatrix3x2dv");
    api->ProgramUniformMatrix2x4dv =
        (PFNGLPROGRAMUNIFORMMATRIX2X4DVPROC)gl__GetProc("glProgramUniformMatrix2x4dv");
    api->ProgramUniformMatrix4x2dv =
        (PFNGLPROGRAMUNIFORMMATRIX4X2DVPROC)gl__GetProc("glProgramUniformMatrix4x2dv");
    api->ProgramUniformMatrix3x4dv =
        (PFNGLPROGRAMUNIFORMMATRIX3X4DVPROC)gl__GetProc("glProgramUniformMatrix3x4dv");
    api->ProgramUniformMatrix4x3dv =
        (PFNGLPROGRAMUNIFORMMATRIX4X3DVPROC)gl__GetProc("glProgramUniformMatrix4x3dv");
    api->ValidateProgramPipeline =
        (PFNGLVALIDATEPROGRAMPIPELINEPROC)gl__GetProc("glValidateProgramPipeline");
    api->GetProgramPipelineInfoLog =
        (PFNGLGETPROGRAMPIPELINEINFOLOGPROC)gl__GetProc("glGetProgramPipelineInfoLog");
    api->VertexAttribL1d = (PFNGLVERTEXATTRIBL1DPROC)gl__GetProc("glVertexAttribL1d");
    api->VertexAttribL2d = (PFNGLVERTEXATTRIBL2DPROC)gl__GetProc("glVertexAttribL2d");
    api->VertexAttribL3d = (PFNGLVERTEXATTRIBL3DPROC)gl__GetProc("glVertexAttribL3d");
    api->VertexAttribL4d = (PFNGLVERTEXATTRIBL4DPROC)gl__GetProc("glVertexAttribL4d");
    api->VertexAttribL1dv = (PFNGLVERTEXATTRIBL1DVPROC)gl__GetProc("glVertexAttribL1dv");
    api->VertexAttribL2dv = (PFNGLVERTEXATTRIBL2DVPROC)gl__GetProc("glVertexAttribL2dv");
    api->VertexAttribL3dv = (PFNGLVERTEXATTRIBL3DVPROC)gl__GetProc("glVertexAttribL3dv");
    api->VertexAttribL4dv = (PFNGLVERTEXATTRIBL4DVPROC)gl__GetProc("glVertexAttribL4dv");
    api->VertexAttribLPointer =
        (PFNGLVERTEXATTRIBLPOINTERPROC)gl__GetProc("glVertexAttribLPointer");
    api->GetVertexAttribLdv = (PFNGLGETVERTEXATTRIBLDVPROC)gl__GetProc("glGetVertexAttribLdv");
    api->ViewportArrayv = (PFNGLVIEWPORTARRAYVPROC)gl__GetProc("glViewportArrayv");
    api->ViewportIndexedf = (PFNGLVIEWPORTINDEXEDFPROC)gl__GetProc("glViewportIndexedf");
    api->ViewportIndexedfv = (PFNGLVIEWPORTINDEXEDFVPROC)gl__GetProc("glViewportIndexedfv");
    api->ScissorArrayv = (PFNGLSCISSORARRAYVPROC)gl__GetProc("glScissorArrayv");
    api->ScissorIndexed = (PFNGLSCISSORINDEXEDPROC)gl__GetProc("glScissorIndexed");
    api->ScissorIndexedv = (PFNGLSCISSORINDEXEDVPROC)gl__GetProc("glScissorIndexedv");
    api->DepthRangeArrayv = (PFNGLDEPTHRANGEARRAYVPROC)gl__GetProc("glDepthRangeArrayv");
    api->DepthRangeIndexed = (PFNGLDEPTHRANGEINDEXEDPROC)gl__GetProc("glDepthRangeIndexed");
    api->GetFloati_v = (PFNGLGETFLOATI_VPROC)gl__GetProc("glGetFloati_v");
    api->GetDoublei_v = (PFNGLGETDOUBLEI_VPROC)gl__GetProc("glGetDoublei_v");
    api->CreateSyncFromCLeventARB =
        (PFNGLCREATESYNCFROMCLEVENTARBPROC)gl__GetProc("glCreateSyncFromCLeventARB");
    api->DebugMessageControlARB =
        (PFNGLDEBUGMESSAGECONTROLARBPROC)gl__GetProc("glDebugMessageControlARB");
    api->DebugMessageInsertARB =
        (PFNGLDEBUGMESSAGEINSERTARBPROC)gl__GetProc("glDebugMessageInsertARB");
    api->DebugMessageCallbackARB =
        (PFNGLDEBUGMESSAGECALLBACKARBPROC)gl__GetProc("glDebugMessageCallbackARB");
    api->GetDebugMessageLogARB =
        (PFNGLGETDEBUGMESSAGELOGARBPROC)gl__GetProc("glGetDebugMessageLogARB");
    api->GetGraphicsResetStatusARB =
        (PFNGLGETGRAPHICSRESETSTATUSARBPROC)gl__GetProc("glGetGraphicsResetStatusARB");
    api->GetnTexImageARB = (PFNGLGETNTEXIMAGEARBPROC)gl__GetProc("glGetnTexImageARB");
    api->ReadnPixelsARB = (PFNGLREADNPIXELSARBPROC)gl__GetProc("glReadnPixelsARB");
    api->GetnCompressedTexImageARB =
        (PFNGLGETNCOMPRESSEDTEXIMAGEARBPROC)gl__GetProc("glGetnCompressedTexImageARB");
    api->GetnUniformfvARB = (PFNGLGETNUNIFORMFVARBPROC)gl__GetProc("glGetnUniformfvARB");
    api->GetnUniformivARB = (PFNGLGETNUNIFORMIVARBPROC)gl__GetProc("glGetnUniformivARB");
    api->GetnUniformuivARB = (PFNGLGETNUNIFORMUIVARBPROC)gl__GetProc("glGetnUniformuivARB");
    api->GetnUniformdvARB = (PFNGLGETNUNIFORMDVARBPROC)gl__GetProc("glGetnUniformdvARB");
    api->DrawArraysInstancedBaseInstance =
        (PFNGLDRAWARRAYSINSTANCEDBASEINSTANCEPROC)gl__GetProc("glDrawArraysInstancedBaseInstance");
    api->DrawElementsInstancedBaseInstance =
        (PFNGLDRAWELEMENTSINSTANCEDBASEINSTANCEPROC)gl__GetProc(
            "glDrawElementsInstancedBaseInstance");
    api->DrawElementsInstancedBaseVertexBaseInstance =
        (PFNGLDRAWELEMENTSINSTANCEDBASEVERTEXBASEINSTANCEPROC)gl__GetProc(
            "glDrawElementsInstancedBaseVertexBaseInstance");
    api->DrawTransformFeedbackInstanced =
        (PFNGLDRAWTRANSFORMFEEDBACKINSTANCEDPROC)gl__GetProc("glDrawTransformFeedbackInstanced");
    api->DrawTransformFeedbackStreamInstanced =
        (PFNGLDRAWTRANSFORMFEEDBACKSTREAMINSTANCEDPROC)gl__GetProc(
            "glDrawTransformFeedbackStreamInstanced");
    api->GetInternalformativ = (PFNGLGETINTERNALFORMATIVPROC)gl__GetProc("glGetInternalformativ");
    api->GetActiveAtomicCounterBufferiv =
        (PFNGLGETACTIVEATOMICCOUNTERBUFFERIVPROC)gl__GetProc("glGetActiveAtomicCounterBufferiv");
    api->BindImageTexture = (PFNGLBINDIMAGETEXTUREPROC)gl__GetProc("glBindImageTexture");
    api->MemoryBarrier = (PFNGLMEMORYBARRIERPROC)gl__GetProc("glMemoryBarrier");
    api->TexStorage1D = (PFNGLTEXSTORAGE1DPROC)gl__GetProc("glTexStorage1D");
    api->TexStorage2D = (PFNGLTEXSTORAGE2DPROC)gl__GetProc("glTexStorage2D");
    api->TexStorage3D = (PFNGLTEXSTORAGE3DPROC)gl__GetProc("glTexStorage3D");
    api->TextureStorage1DEXT = (PFNGLTEXTURESTORAGE1DEXTPROC)gl__GetProc("glTextureStorage1DEXT");
    api->TextureStorage2DEXT = (PFNGLTEXTURESTORAGE2DEXTPROC)gl__GetProc("glTextureStorage2DEXT");
    api->TextureStorage3DEXT = (PFNGLTEXTURESTORAGE3DEXTPROC)gl__GetProc("glTextureStorage3DEXT");
    api->DebugMessageControl = (PFNGLDEBUGMESSAGECONTROLPROC)gl__GetProc("glDebugMessageControl");
    api->DebugMessageInsert = (PFNGLDEBUGMESSAGEINSERTPROC)gl__GetProc("glDebugMessageInsert");
    api->DebugMessageCallback =
        (PFNGLDEBUGMESSAGECALLBACKPROC)gl__GetProc("glDebugMessageCallback");
    api->GetDebugMessageLog = (PFNGLGETDEBUGMESSAGELOGPROC)gl__GetProc("glGetDebugMessageLog");
    api->PushDebugGroup = (PFNGLPUSHDEBUGGROUPPROC)gl__GetProc("glPushDebugGroup");
    api->PopDebugGroup = (PFNGLPOPDEBUGGROUPPROC)gl__GetProc("glPopDebugGroup");
    api->ObjectLabel = (PFNGLOBJECTLABELPROC)gl__GetProc("glObjectLabel");
    api->GetObjectLabel = (PFNGLGETOBJECTLABELPROC)gl__GetProc("glGetObjectLabel");
    api->ObjectPtrLabel = (PFNGLOBJECTPTRLABELPROC)gl__GetProc("glObjectPtrLabel");
    api->GetObjectPtrLabel = (PFNGLGETOBJECTPTRLABELPROC)gl__GetProc("glGetObjectPtrLabel");
    api->ClearBufferData = (PFNGLCLEARBUFFERDATAPROC)gl__GetProc("glClearBufferData");
    api->ClearBufferSubData = (PFNGLCLEARBUFFERSUBDATAPROC)gl__GetProc("glClearBufferSubData");
    api->ClearNamedBufferDataEXT =
        (PFNGLCLEARNAMEDBUFFERDATAEXTPROC)gl__GetProc("glClearNamedBufferDataEXT");
    api->ClearNamedBufferSubDataEXT =
        (PFNGLCLEARNAMEDBUFFERSUBDATAEXTPROC)gl__GetProc("glClearNamedBufferSubDataEXT");
    api->DispatchCompute = (PFNGLDISPATCHCOMPUTEPROC)gl__GetProc("glDispatchCompute");
    api->DispatchComputeIndirect =
        (PFNGLDISPATCHCOMPUTEINDIRECTPROC)gl__GetProc("glDispatchComputeIndirect");
    api->CopyImageSubData = (PFNGLCOPYIMAGESUBDATAPROC)gl__GetProc("glCopyImageSubData");
    api->TextureView = (PFNGLTEXTUREVIEWPROC)gl__GetProc("glTextureView");
    api->BindVertexBuffer = (PFNGLBINDVERTEXBUFFERPROC)gl__GetProc("glBindVertexBuffer");
    api->VertexAttribFormat = (PFNGLVERTEXATTRIBFORMATPROC)gl__GetProc("glVertexAttribFormat");
    api->VertexAttribIFormat = (PFNGLVERTEXATTRIBIFORMATPROC)gl__GetProc("glVertexAttribIFormat");
    api->VertexAttribLFormat = (PFNGLVERTEXATTRIBLFORMATPROC)gl__GetProc("glVertexAttribLFormat");
    api->VertexAttribBinding = (PFNGLVERTEXATTRIBBINDINGPROC)gl__GetProc("glVertexAttribBinding");
    api->VertexBindingDivisor =
        (PFNGLVERTEXBINDINGDIVISORPROC)gl__GetProc("glVertexBindingDivisor");
    api->VertexArrayBindVertexBufferEXT =
        (PFNGLVERTEXARRAYBINDVERTEXBUFFEREXTPROC)gl__GetProc("glVertexArrayBindVertexBufferEXT");
    api->VertexArrayVertexAttribFormatEXT = (PFNGLVERTEXARRAYVERTEXATTRIBFORMATEXTPROC)gl__GetProc(
        "glVertexArrayVertexAttribFormatEXT");
    api->VertexArrayVertexAttribIFormatEXT =
        (PFNGLVERTEXARRAYVERTEXATTRIBIFORMATEXTPROC)gl__GetProc(
            "glVertexArrayVertexAttribIFormatEXT");
    api->VertexArrayVertexAttribLFormatEXT =
        (PFNGLVERTEXARRAYVERTEXATTRIBLFORMATEXTPROC)gl__GetProc(
            "glVertexArrayVertexAttribLFormatEXT");
    api->VertexArrayVertexAttribBindingEXT =
        (PFNGLVERTEXARRAYVERTEXATTRIBBINDINGEXTPROC)gl__GetProc(
            "glVertexArrayVertexAttribBindingEXT");
    api->VertexArrayVertexBindingDivisorEXT =
        (PFNGLVERTEXARRAYVERTEXBINDINGDIVISOREXTPROC)gl__GetProc(
            "glVertexArrayVertexBindingDivisorEXT");
    api->FramebufferParameteri =
        (PFNGLFRAMEBUFFERPARAMETERIPROC)gl__GetProc("glFramebufferParameteri");
    api->GetFramebufferParameteriv =
        (PFNGLGETFRAMEBUFFERPARAMETERIVPROC)gl__GetProc("glGetFramebufferParameteriv");
    api->NamedFramebufferParameteriEXT =
        (PFNGLNAMEDFRAMEBUFFERPARAMETERIEXTPROC)gl__GetProc("glNamedFramebufferParameteriEXT");
    api->GetNamedFramebufferParameterivEXT =
        (PFNGLGETNAMEDFRAMEBUFFERPARAMETERIVEXTPROC)gl__GetProc(
            "glGetNamedFramebufferParameterivEXT");
    api->GetInternalformati64v =
        (PFNGLGETINTERNALFORMATI64VPROC)gl__GetProc("glGetInternalformati64v");
    api->InvalidateTexSubImage =
        (PFNGLINVALIDATETEXSUBIMAGEPROC)gl__GetProc("glInvalidateTexSubImage");
    api->InvalidateTexImage = (PFNGLINVALIDATETEXIMAGEPROC)gl__GetProc("glInvalidateTexImage");
    api->InvalidateBufferSubData =
        (PFNGLINVALIDATEBUFFERSUBDATAPROC)gl__GetProc("glInvalidateBufferSubData");
    api->InvalidateBufferData =
        (PFNGLINVALIDATEBUFFERDATAPROC)gl__GetProc("glInvalidateBufferData");
    api->InvalidateFramebuffer =
        (PFNGLINVALIDATEFRAMEBUFFERPROC)gl__GetProc("glInvalidateFramebuffer");
    api->InvalidateSubFramebuffer =
        (PFNGLINVALIDATESUBFRAMEBUFFERPROC)gl__GetProc("glInvalidateSubFramebuffer");
    api->MultiDrawArraysIndirect =
        (PFNGLMULTIDRAWARRAYSINDIRECTPROC)gl__GetProc("glMultiDrawArraysIndirect");
    api->MultiDrawElementsIndirect =
        (PFNGLMULTIDRAWELEMENTSINDIRECTPROC)gl__GetProc("glMultiDrawElementsIndirect");
    api->GetProgramInterfaceiv =
        (PFNGLGETPROGRAMINTERFACEIVPROC)gl__GetProc("glGetProgramInterfaceiv");
    api->GetProgramResourceIndex =
        (PFNGLGETPROGRAMRESOURCEINDEXPROC)gl__GetProc("glGetProgramResourceIndex");
    api->GetProgramResourceName =
        (PFNGLGETPROGRAMRESOURCENAMEPROC)gl__GetProc("glGetProgramResourceName");
    api->GetProgramResourceiv =
        (PFNGLGETPROGRAMRESOURCEIVPROC)gl__GetProc("glGetProgramResourceiv");
    api->GetProgramResourceLocation =
        (PFNGLGETPROGRAMRESOURCELOCATIONPROC)gl__GetProc("glGetProgramResourceLocation");
    api->GetProgramResourceLocationIndex =
        (PFNGLGETPROGRAMRESOURCELOCATIONINDEXPROC)gl__GetProc("glGetProgramResourceLocationIndex");
    api->ShaderStorageBlockBinding =
        (PFNGLSHADERSTORAGEBLOCKBINDINGPROC)gl__GetProc("glShaderStorageBlockBinding");
    api->TexBufferRange = (PFNGLTEXBUFFERRANGEPROC)gl__GetProc("glTexBufferRange");
    api->TextureBufferRangeEXT =
        (PFNGLTEXTUREBUFFERRANGEEXTPROC)gl__GetProc("glTextureBufferRangeEXT");
    api->TexStorage2DMultisample =
        (PFNGLTEXSTORAGE2DMULTISAMPLEPROC)gl__GetProc("glTexStorage2DMultisample");
    api->TexStorage3DMultisample =
        (PFNGLTEXSTORAGE3DMULTISAMPLEPROC)gl__GetProc("glTexStorage3DMultisample");
    api->TextureStorage2DMultisampleEXT =
        (PFNGLTEXTURESTORAGE2DMULTISAMPLEEXTPROC)gl__GetProc("glTextureStorage2DMultisampleEXT");
    api->TextureStorage3DMultisampleEXT =
        (PFNGLTEXTURESTORAGE3DMULTISAMPLEEXTPROC)gl__GetProc("glTextureStorage3DMultisampleEXT");
}

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN 1
#include <windows.h>

static HMODULE libgl;

static void
gl__OpenLib(void)
{
    libgl = LoadLibraryA("opengl32.dll");
}

static void
gl__CloseLib(void)
{
    FreeLibrary(libgl);
}

static void *
gl__GetProc(const char *proc)
{
    void *res = (void *)wglGetProcAddress(proc);
    if (!res) res = (void *)GetProcAddress(libgl, proc);
    return res;
}
#elif defined(__APPLE__) || defined(__APPLE_CC__)
#include <Carbon/Carbon.h>

CFBundleRef bundle;
CFURLRef bundleURL;

static void
gl__OpenLib(void)
{
    bundleURL = CFURLCreateWithFileSystemPath(kCFAllocatorDefault,
                                              CFSTR("/System/Library/Frameworks/OpenGL.framework"),
                                              kCFURLPOSIXPathStyle, true);

    bundle = CFBundleCreate(kCFAllocatorDefault, bundleURL);
    assert(bundle != NULL);
}

static void
gl__CloseLib(void)
{
    CFRelease(bundle);
    CFRelease(bundleURL);
}

static void *
gl__GetProc(const char *proc)
{
    void *res;

    CFStringRef procname =
        CFStringCreateWithCString(kCFAllocatorDefault, proc, kCFStringEncodingASCII);
    res = CFBundleGetFunctionPointerForName(bundle, procname);
    CFRelease(procname);
    return res;
}
#else
#include <GL/glx.h>
#include <dlfcn.h>

static void *libgl;

static void
gl__OpenLib(void)
{
    libgl = dlopen("libGL.so.1", RTLD_LAZY | RTLD_GLOBAL);
}

static void
gl__CloseLib(void)
{
    dlclose(libgl);
}

static void *
gl__GetProc(const char *proc)
{
    void *res;

    res = (void *)glXGetProcAddress((const GLubyte *)proc);
    if (!res) res = dlsym(libgl, proc);
    return res;
}
#endif
