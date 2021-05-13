#include "gload.h"

static struct
{
    int major, minor;
} version;

static int gl__ParseVersion(void);
static void gl__OpenLib(void);
static void gl__CloseLib(void);
static void *gl__GetProc(const char *proc);
static void gl__LoadProcs(Gl *gl);

static Gl g_gl = {0};

Gl *gl = &g_gl;

int
gloadInit(Gl *gl_procs)
{
    if (gl_procs)
    {
        gl = gl_procs;
    }
    else
    {
        gl = &g_gl;
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
gl__LoadProcs(Gl *gl)
{
    gl->CullFace = (PFNGLCULLFACEPROC)gl__GetProc("glCullFace");
    gl->FrontFace = (PFNGLFRONTFACEPROC)gl__GetProc("glFrontFace");
    gl->Hint = (PFNGLHINTPROC)gl__GetProc("glHint");
    gl->LineWidth = (PFNGLLINEWIDTHPROC)gl__GetProc("glLineWidth");
    gl->PointSize = (PFNGLPOINTSIZEPROC)gl__GetProc("glPointSize");
    gl->PolygonMode = (PFNGLPOLYGONMODEPROC)gl__GetProc("glPolygonMode");
    gl->Scissor = (PFNGLSCISSORPROC)gl__GetProc("glScissor");
    gl->TexParameterf = (PFNGLTEXPARAMETERFPROC)gl__GetProc("glTexParameterf");
    gl->TexParameterfv = (PFNGLTEXPARAMETERFVPROC)gl__GetProc("glTexParameterfv");
    gl->TexParameteri = (PFNGLTEXPARAMETERIPROC)gl__GetProc("glTexParameteri");
    gl->TexParameteriv = (PFNGLTEXPARAMETERIVPROC)gl__GetProc("glTexParameteriv");
    gl->TexImage1D = (PFNGLTEXIMAGE1DPROC)gl__GetProc("glTexImage1D");
    gl->TexImage2D = (PFNGLTEXIMAGE2DPROC)gl__GetProc("glTexImage2D");
    gl->DrawBuffer = (PFNGLDRAWBUFFERPROC)gl__GetProc("glDrawBuffer");
    gl->Clear = (PFNGLCLEARPROC)gl__GetProc("glClear");
    gl->ClearColor = (PFNGLCLEARCOLORPROC)gl__GetProc("glClearColor");
    gl->ClearStencil = (PFNGLCLEARSTENCILPROC)gl__GetProc("glClearStencil");
    gl->ClearDepth = (PFNGLCLEARDEPTHPROC)gl__GetProc("glClearDepth");
    gl->StencilMask = (PFNGLSTENCILMASKPROC)gl__GetProc("glStencilMask");
    gl->ColorMask = (PFNGLCOLORMASKPROC)gl__GetProc("glColorMask");
    gl->DepthMask = (PFNGLDEPTHMASKPROC)gl__GetProc("glDepthMask");
    gl->Disable = (PFNGLDISABLEPROC)gl__GetProc("glDisable");
    gl->Enable = (PFNGLENABLEPROC)gl__GetProc("glEnable");
    gl->Finish = (PFNGLFINISHPROC)gl__GetProc("glFinish");
    gl->Flush = (PFNGLFLUSHPROC)gl__GetProc("glFlush");
    gl->BlendFunc = (PFNGLBLENDFUNCPROC)gl__GetProc("glBlendFunc");
    gl->LogicOp = (PFNGLLOGICOPPROC)gl__GetProc("glLogicOp");
    gl->StencilFunc = (PFNGLSTENCILFUNCPROC)gl__GetProc("glStencilFunc");
    gl->StencilOp = (PFNGLSTENCILOPPROC)gl__GetProc("glStencilOp");
    gl->DepthFunc = (PFNGLDEPTHFUNCPROC)gl__GetProc("glDepthFunc");
    gl->PixelStoref = (PFNGLPIXELSTOREFPROC)gl__GetProc("glPixelStoref");
    gl->PixelStorei = (PFNGLPIXELSTOREIPROC)gl__GetProc("glPixelStorei");
    gl->ReadBuffer = (PFNGLREADBUFFERPROC)gl__GetProc("glReadBuffer");
    gl->ReadPixels = (PFNGLREADPIXELSPROC)gl__GetProc("glReadPixels");
    gl->GetBooleanv = (PFNGLGETBOOLEANVPROC)gl__GetProc("glGetBooleanv");
    gl->GetDoublev = (PFNGLGETDOUBLEVPROC)gl__GetProc("glGetDoublev");
    gl->GetError = (PFNGLGETERRORPROC)gl__GetProc("glGetError");
    gl->GetFloatv = (PFNGLGETFLOATVPROC)gl__GetProc("glGetFloatv");
    gl->GetIntegerv = (PFNGLGETINTEGERVPROC)gl__GetProc("glGetIntegerv");
    gl->GetString = (PFNGLGETSTRINGPROC)gl__GetProc("glGetString");
    gl->GetTexImage = (PFNGLGETTEXIMAGEPROC)gl__GetProc("glGetTexImage");
    gl->GetTexParameterfv = (PFNGLGETTEXPARAMETERFVPROC)gl__GetProc("glGetTexParameterfv");
    gl->GetTexParameteriv = (PFNGLGETTEXPARAMETERIVPROC)gl__GetProc("glGetTexParameteriv");
    gl->GetTexLevelParameterfv =
        (PFNGLGETTEXLEVELPARAMETERFVPROC)gl__GetProc("glGetTexLevelParameterfv");
    gl->GetTexLevelParameteriv =
        (PFNGLGETTEXLEVELPARAMETERIVPROC)gl__GetProc("glGetTexLevelParameteriv");
    gl->IsEnabled = (PFNGLISENABLEDPROC)gl__GetProc("glIsEnabled");
    gl->DepthRange = (PFNGLDEPTHRANGEPROC)gl__GetProc("glDepthRange");
    gl->Viewport = (PFNGLVIEWPORTPROC)gl__GetProc("glViewport");
    gl->DrawArrays = (PFNGLDRAWARRAYSPROC)gl__GetProc("glDrawArrays");
    gl->DrawElements = (PFNGLDRAWELEMENTSPROC)gl__GetProc("glDrawElements");
    gl->GetPointerv = (PFNGLGETPOINTERVPROC)gl__GetProc("glGetPointerv");
    gl->PolygonOffset = (PFNGLPOLYGONOFFSETPROC)gl__GetProc("glPolygonOffset");
    gl->CopyTexImage1D = (PFNGLCOPYTEXIMAGE1DPROC)gl__GetProc("glCopyTexImage1D");
    gl->CopyTexImage2D = (PFNGLCOPYTEXIMAGE2DPROC)gl__GetProc("glCopyTexImage2D");
    gl->CopyTexSubImage1D = (PFNGLCOPYTEXSUBIMAGE1DPROC)gl__GetProc("glCopyTexSubImage1D");
    gl->CopyTexSubImage2D = (PFNGLCOPYTEXSUBIMAGE2DPROC)gl__GetProc("glCopyTexSubImage2D");
    gl->TexSubImage1D = (PFNGLTEXSUBIMAGE1DPROC)gl__GetProc("glTexSubImage1D");
    gl->TexSubImage2D = (PFNGLTEXSUBIMAGE2DPROC)gl__GetProc("glTexSubImage2D");
    gl->BindTexture = (PFNGLBINDTEXTUREPROC)gl__GetProc("glBindTexture");
    gl->DeleteTextures = (PFNGLDELETETEXTURESPROC)gl__GetProc("glDeleteTextures");
    gl->GenTextures = (PFNGLGENTEXTURESPROC)gl__GetProc("glGenTextures");
    gl->IsTexture = (PFNGLISTEXTUREPROC)gl__GetProc("glIsTexture");
    gl->BlendColor = (PFNGLBLENDCOLORPROC)gl__GetProc("glBlendColor");
    gl->BlendEquation = (PFNGLBLENDEQUATIONPROC)gl__GetProc("glBlendEquation");
    gl->DrawRangeElements = (PFNGLDRAWRANGEELEMENTSPROC)gl__GetProc("glDrawRangeElements");
    gl->TexImage3D = (PFNGLTEXIMAGE3DPROC)gl__GetProc("glTexImage3D");
    gl->TexSubImage3D = (PFNGLTEXSUBIMAGE3DPROC)gl__GetProc("glTexSubImage3D");
    gl->CopyTexSubImage3D = (PFNGLCOPYTEXSUBIMAGE3DPROC)gl__GetProc("glCopyTexSubImage3D");
    gl->ActiveTexture = (PFNGLACTIVETEXTUREPROC)gl__GetProc("glActiveTexture");
    gl->SampleCoverage = (PFNGLSAMPLECOVERAGEPROC)gl__GetProc("glSampleCoverage");
    gl->CompressedTexImage3D = (PFNGLCOMPRESSEDTEXIMAGE3DPROC)gl__GetProc("glCompressedTexImage3D");
    gl->CompressedTexImage2D = (PFNGLCOMPRESSEDTEXIMAGE2DPROC)gl__GetProc("glCompressedTexImage2D");
    gl->CompressedTexImage1D = (PFNGLCOMPRESSEDTEXIMAGE1DPROC)gl__GetProc("glCompressedTexImage1D");
    gl->CompressedTexSubImage3D =
        (PFNGLCOMPRESSEDTEXSUBIMAGE3DPROC)gl__GetProc("glCompressedTexSubImage3D");
    gl->CompressedTexSubImage2D =
        (PFNGLCOMPRESSEDTEXSUBIMAGE2DPROC)gl__GetProc("glCompressedTexSubImage2D");
    gl->CompressedTexSubImage1D =
        (PFNGLCOMPRESSEDTEXSUBIMAGE1DPROC)gl__GetProc("glCompressedTexSubImage1D");
    gl->GetCompressedTexImage =
        (PFNGLGETCOMPRESSEDTEXIMAGEPROC)gl__GetProc("glGetCompressedTexImage");
    gl->BlendFuncSeparate = (PFNGLBLENDFUNCSEPARATEPROC)gl__GetProc("glBlendFuncSeparate");
    gl->MultiDrawArrays = (PFNGLMULTIDRAWARRAYSPROC)gl__GetProc("glMultiDrawArrays");
    gl->MultiDrawElements = (PFNGLMULTIDRAWELEMENTSPROC)gl__GetProc("glMultiDrawElements");
    gl->PointParameterf = (PFNGLPOINTPARAMETERFPROC)gl__GetProc("glPointParameterf");
    gl->PointParameterfv = (PFNGLPOINTPARAMETERFVPROC)gl__GetProc("glPointParameterfv");
    gl->PointParameteri = (PFNGLPOINTPARAMETERIPROC)gl__GetProc("glPointParameteri");
    gl->PointParameteriv = (PFNGLPOINTPARAMETERIVPROC)gl__GetProc("glPointParameteriv");
    gl->GenQueries = (PFNGLGENQUERIESPROC)gl__GetProc("glGenQueries");
    gl->DeleteQueries = (PFNGLDELETEQUERIESPROC)gl__GetProc("glDeleteQueries");
    gl->IsQuery = (PFNGLISQUERYPROC)gl__GetProc("glIsQuery");
    gl->BeginQuery = (PFNGLBEGINQUERYPROC)gl__GetProc("glBeginQuery");
    gl->EndQuery = (PFNGLENDQUERYPROC)gl__GetProc("glEndQuery");
    gl->GetQueryiv = (PFNGLGETQUERYIVPROC)gl__GetProc("glGetQueryiv");
    gl->GetQueryObjectiv = (PFNGLGETQUERYOBJECTIVPROC)gl__GetProc("glGetQueryObjectiv");
    gl->GetQueryObjectuiv = (PFNGLGETQUERYOBJECTUIVPROC)gl__GetProc("glGetQueryObjectuiv");
    gl->BindBuffer = (PFNGLBINDBUFFERPROC)gl__GetProc("glBindBuffer");
    gl->DeleteBuffers = (PFNGLDELETEBUFFERSPROC)gl__GetProc("glDeleteBuffers");
    gl->GenBuffers = (PFNGLGENBUFFERSPROC)gl__GetProc("glGenBuffers");
    gl->IsBuffer = (PFNGLISBUFFERPROC)gl__GetProc("glIsBuffer");
    gl->BufferData = (PFNGLBUFFERDATAPROC)gl__GetProc("glBufferData");
    gl->BufferSubData = (PFNGLBUFFERSUBDATAPROC)gl__GetProc("glBufferSubData");
    gl->GetBufferSubData = (PFNGLGETBUFFERSUBDATAPROC)gl__GetProc("glGetBufferSubData");
    gl->MapBuffer = (PFNGLMAPBUFFERPROC)gl__GetProc("glMapBuffer");
    gl->UnmapBuffer = (PFNGLUNMAPBUFFERPROC)gl__GetProc("glUnmapBuffer");
    gl->GetBufferParameteriv = (PFNGLGETBUFFERPARAMETERIVPROC)gl__GetProc("glGetBufferParameteriv");
    gl->GetBufferPointerv = (PFNGLGETBUFFERPOINTERVPROC)gl__GetProc("glGetBufferPointerv");
    gl->BlendEquationSeparate =
        (PFNGLBLENDEQUATIONSEPARATEPROC)gl__GetProc("glBlendEquationSeparate");
    gl->DrawBuffers = (PFNGLDRAWBUFFERSPROC)gl__GetProc("glDrawBuffers");
    gl->StencilOpSeparate = (PFNGLSTENCILOPSEPARATEPROC)gl__GetProc("glStencilOpSeparate");
    gl->StencilFuncSeparate = (PFNGLSTENCILFUNCSEPARATEPROC)gl__GetProc("glStencilFuncSeparate");
    gl->StencilMaskSeparate = (PFNGLSTENCILMASKSEPARATEPROC)gl__GetProc("glStencilMaskSeparate");
    gl->AttachShader = (PFNGLATTACHSHADERPROC)gl__GetProc("glAttachShader");
    gl->BindAttribLocation = (PFNGLBINDATTRIBLOCATIONPROC)gl__GetProc("glBindAttribLocation");
    gl->CompileShader = (PFNGLCOMPILESHADERPROC)gl__GetProc("glCompileShader");
    gl->CreateProgram = (PFNGLCREATEPROGRAMPROC)gl__GetProc("glCreateProgram");
    gl->CreateShader = (PFNGLCREATESHADERPROC)gl__GetProc("glCreateShader");
    gl->DeleteProgram = (PFNGLDELETEPROGRAMPROC)gl__GetProc("glDeleteProgram");
    gl->DeleteShader = (PFNGLDELETESHADERPROC)gl__GetProc("glDeleteShader");
    gl->DetachShader = (PFNGLDETACHSHADERPROC)gl__GetProc("glDetachShader");
    gl->DisableVertexAttribArray =
        (PFNGLDISABLEVERTEXATTRIBARRAYPROC)gl__GetProc("glDisableVertexAttribArray");
    gl->EnableVertexAttribArray =
        (PFNGLENABLEVERTEXATTRIBARRAYPROC)gl__GetProc("glEnableVertexAttribArray");
    gl->GetActiveAttrib = (PFNGLGETACTIVEATTRIBPROC)gl__GetProc("glGetActiveAttrib");
    gl->GetActiveUniform = (PFNGLGETACTIVEUNIFORMPROC)gl__GetProc("glGetActiveUniform");
    gl->GetAttachedShaders = (PFNGLGETATTACHEDSHADERSPROC)gl__GetProc("glGetAttachedShaders");
    gl->GetAttribLocation = (PFNGLGETATTRIBLOCATIONPROC)gl__GetProc("glGetAttribLocation");
    gl->GetProgramiv = (PFNGLGETPROGRAMIVPROC)gl__GetProc("glGetProgramiv");
    gl->GetProgramInfoLog = (PFNGLGETPROGRAMINFOLOGPROC)gl__GetProc("glGetProgramInfoLog");
    gl->GetShaderiv = (PFNGLGETSHADERIVPROC)gl__GetProc("glGetShaderiv");
    gl->GetShaderInfoLog = (PFNGLGETSHADERINFOLOGPROC)gl__GetProc("glGetShaderInfoLog");
    gl->GetShaderSource = (PFNGLGETSHADERSOURCEPROC)gl__GetProc("glGetShaderSource");
    gl->GetUniformLocation = (PFNGLGETUNIFORMLOCATIONPROC)gl__GetProc("glGetUniformLocation");
    gl->GetUniformfv = (PFNGLGETUNIFORMFVPROC)gl__GetProc("glGetUniformfv");
    gl->GetUniformiv = (PFNGLGETUNIFORMIVPROC)gl__GetProc("glGetUniformiv");
    gl->GetVertexAttribdv = (PFNGLGETVERTEXATTRIBDVPROC)gl__GetProc("glGetVertexAttribdv");
    gl->GetVertexAttribfv = (PFNGLGETVERTEXATTRIBFVPROC)gl__GetProc("glGetVertexAttribfv");
    gl->GetVertexAttribiv = (PFNGLGETVERTEXATTRIBIVPROC)gl__GetProc("glGetVertexAttribiv");
    gl->GetVertexAttribPointerv =
        (PFNGLGETVERTEXATTRIBPOINTERVPROC)gl__GetProc("glGetVertexAttribPointerv");
    gl->IsProgram = (PFNGLISPROGRAMPROC)gl__GetProc("glIsProgram");
    gl->IsShader = (PFNGLISSHADERPROC)gl__GetProc("glIsShader");
    gl->LinkProgram = (PFNGLLINKPROGRAMPROC)gl__GetProc("glLinkProgram");
    gl->ShaderSource = (PFNGLSHADERSOURCEPROC)gl__GetProc("glShaderSource");
    gl->UseProgram = (PFNGLUSEPROGRAMPROC)gl__GetProc("glUseProgram");
    gl->Uniform1f = (PFNGLUNIFORM1FPROC)gl__GetProc("glUniform1f");
    gl->Uniform2f = (PFNGLUNIFORM2FPROC)gl__GetProc("glUniform2f");
    gl->Uniform3f = (PFNGLUNIFORM3FPROC)gl__GetProc("glUniform3f");
    gl->Uniform4f = (PFNGLUNIFORM4FPROC)gl__GetProc("glUniform4f");
    gl->Uniform1i = (PFNGLUNIFORM1IPROC)gl__GetProc("glUniform1i");
    gl->Uniform2i = (PFNGLUNIFORM2IPROC)gl__GetProc("glUniform2i");
    gl->Uniform3i = (PFNGLUNIFORM3IPROC)gl__GetProc("glUniform3i");
    gl->Uniform4i = (PFNGLUNIFORM4IPROC)gl__GetProc("glUniform4i");
    gl->Uniform1fv = (PFNGLUNIFORM1FVPROC)gl__GetProc("glUniform1fv");
    gl->Uniform2fv = (PFNGLUNIFORM2FVPROC)gl__GetProc("glUniform2fv");
    gl->Uniform3fv = (PFNGLUNIFORM3FVPROC)gl__GetProc("glUniform3fv");
    gl->Uniform4fv = (PFNGLUNIFORM4FVPROC)gl__GetProc("glUniform4fv");
    gl->Uniform1iv = (PFNGLUNIFORM1IVPROC)gl__GetProc("glUniform1iv");
    gl->Uniform2iv = (PFNGLUNIFORM2IVPROC)gl__GetProc("glUniform2iv");
    gl->Uniform3iv = (PFNGLUNIFORM3IVPROC)gl__GetProc("glUniform3iv");
    gl->Uniform4iv = (PFNGLUNIFORM4IVPROC)gl__GetProc("glUniform4iv");
    gl->UniformMatrix2fv = (PFNGLUNIFORMMATRIX2FVPROC)gl__GetProc("glUniformMatrix2fv");
    gl->UniformMatrix3fv = (PFNGLUNIFORMMATRIX3FVPROC)gl__GetProc("glUniformMatrix3fv");
    gl->UniformMatrix4fv = (PFNGLUNIFORMMATRIX4FVPROC)gl__GetProc("glUniformMatrix4fv");
    gl->ValidateProgram = (PFNGLVALIDATEPROGRAMPROC)gl__GetProc("glValidateProgram");
    gl->VertexAttrib1d = (PFNGLVERTEXATTRIB1DPROC)gl__GetProc("glVertexAttrib1d");
    gl->VertexAttrib1dv = (PFNGLVERTEXATTRIB1DVPROC)gl__GetProc("glVertexAttrib1dv");
    gl->VertexAttrib1f = (PFNGLVERTEXATTRIB1FPROC)gl__GetProc("glVertexAttrib1f");
    gl->VertexAttrib1fv = (PFNGLVERTEXATTRIB1FVPROC)gl__GetProc("glVertexAttrib1fv");
    gl->VertexAttrib1s = (PFNGLVERTEXATTRIB1SPROC)gl__GetProc("glVertexAttrib1s");
    gl->VertexAttrib1sv = (PFNGLVERTEXATTRIB1SVPROC)gl__GetProc("glVertexAttrib1sv");
    gl->VertexAttrib2d = (PFNGLVERTEXATTRIB2DPROC)gl__GetProc("glVertexAttrib2d");
    gl->VertexAttrib2dv = (PFNGLVERTEXATTRIB2DVPROC)gl__GetProc("glVertexAttrib2dv");
    gl->VertexAttrib2f = (PFNGLVERTEXATTRIB2FPROC)gl__GetProc("glVertexAttrib2f");
    gl->VertexAttrib2fv = (PFNGLVERTEXATTRIB2FVPROC)gl__GetProc("glVertexAttrib2fv");
    gl->VertexAttrib2s = (PFNGLVERTEXATTRIB2SPROC)gl__GetProc("glVertexAttrib2s");
    gl->VertexAttrib2sv = (PFNGLVERTEXATTRIB2SVPROC)gl__GetProc("glVertexAttrib2sv");
    gl->VertexAttrib3d = (PFNGLVERTEXATTRIB3DPROC)gl__GetProc("glVertexAttrib3d");
    gl->VertexAttrib3dv = (PFNGLVERTEXATTRIB3DVPROC)gl__GetProc("glVertexAttrib3dv");
    gl->VertexAttrib3f = (PFNGLVERTEXATTRIB3FPROC)gl__GetProc("glVertexAttrib3f");
    gl->VertexAttrib3fv = (PFNGLVERTEXATTRIB3FVPROC)gl__GetProc("glVertexAttrib3fv");
    gl->VertexAttrib3s = (PFNGLVERTEXATTRIB3SPROC)gl__GetProc("glVertexAttrib3s");
    gl->VertexAttrib3sv = (PFNGLVERTEXATTRIB3SVPROC)gl__GetProc("glVertexAttrib3sv");
    gl->VertexAttrib4Nbv = (PFNGLVERTEXATTRIB4NBVPROC)gl__GetProc("glVertexAttrib4Nbv");
    gl->VertexAttrib4Niv = (PFNGLVERTEXATTRIB4NIVPROC)gl__GetProc("glVertexAttrib4Niv");
    gl->VertexAttrib4Nsv = (PFNGLVERTEXATTRIB4NSVPROC)gl__GetProc("glVertexAttrib4Nsv");
    gl->VertexAttrib4Nub = (PFNGLVERTEXATTRIB4NUBPROC)gl__GetProc("glVertexAttrib4Nub");
    gl->VertexAttrib4Nubv = (PFNGLVERTEXATTRIB4NUBVPROC)gl__GetProc("glVertexAttrib4Nubv");
    gl->VertexAttrib4Nuiv = (PFNGLVERTEXATTRIB4NUIVPROC)gl__GetProc("glVertexAttrib4Nuiv");
    gl->VertexAttrib4Nusv = (PFNGLVERTEXATTRIB4NUSVPROC)gl__GetProc("glVertexAttrib4Nusv");
    gl->VertexAttrib4bv = (PFNGLVERTEXATTRIB4BVPROC)gl__GetProc("glVertexAttrib4bv");
    gl->VertexAttrib4d = (PFNGLVERTEXATTRIB4DPROC)gl__GetProc("glVertexAttrib4d");
    gl->VertexAttrib4dv = (PFNGLVERTEXATTRIB4DVPROC)gl__GetProc("glVertexAttrib4dv");
    gl->VertexAttrib4f = (PFNGLVERTEXATTRIB4FPROC)gl__GetProc("glVertexAttrib4f");
    gl->VertexAttrib4fv = (PFNGLVERTEXATTRIB4FVPROC)gl__GetProc("glVertexAttrib4fv");
    gl->VertexAttrib4iv = (PFNGLVERTEXATTRIB4IVPROC)gl__GetProc("glVertexAttrib4iv");
    gl->VertexAttrib4s = (PFNGLVERTEXATTRIB4SPROC)gl__GetProc("glVertexAttrib4s");
    gl->VertexAttrib4sv = (PFNGLVERTEXATTRIB4SVPROC)gl__GetProc("glVertexAttrib4sv");
    gl->VertexAttrib4ubv = (PFNGLVERTEXATTRIB4UBVPROC)gl__GetProc("glVertexAttrib4ubv");
    gl->VertexAttrib4uiv = (PFNGLVERTEXATTRIB4UIVPROC)gl__GetProc("glVertexAttrib4uiv");
    gl->VertexAttrib4usv = (PFNGLVERTEXATTRIB4USVPROC)gl__GetProc("glVertexAttrib4usv");
    gl->VertexAttribPointer = (PFNGLVERTEXATTRIBPOINTERPROC)gl__GetProc("glVertexAttribPointer");
    gl->UniformMatrix2x3fv = (PFNGLUNIFORMMATRIX2X3FVPROC)gl__GetProc("glUniformMatrix2x3fv");
    gl->UniformMatrix3x2fv = (PFNGLUNIFORMMATRIX3X2FVPROC)gl__GetProc("glUniformMatrix3x2fv");
    gl->UniformMatrix2x4fv = (PFNGLUNIFORMMATRIX2X4FVPROC)gl__GetProc("glUniformMatrix2x4fv");
    gl->UniformMatrix4x2fv = (PFNGLUNIFORMMATRIX4X2FVPROC)gl__GetProc("glUniformMatrix4x2fv");
    gl->UniformMatrix3x4fv = (PFNGLUNIFORMMATRIX3X4FVPROC)gl__GetProc("glUniformMatrix3x4fv");
    gl->UniformMatrix4x3fv = (PFNGLUNIFORMMATRIX4X3FVPROC)gl__GetProc("glUniformMatrix4x3fv");
    gl->ColorMaski = (PFNGLCOLORMASKIPROC)gl__GetProc("glColorMaski");
    gl->GetBooleani_v = (PFNGLGETBOOLEANI_VPROC)gl__GetProc("glGetBooleani_v");
    gl->GetIntegeri_v = (PFNGLGETINTEGERI_VPROC)gl__GetProc("glGetIntegeri_v");
    gl->Enablei = (PFNGLENABLEIPROC)gl__GetProc("glEnablei");
    gl->Disablei = (PFNGLDISABLEIPROC)gl__GetProc("glDisablei");
    gl->IsEnabledi = (PFNGLISENABLEDIPROC)gl__GetProc("glIsEnabledi");
    gl->BeginTransformFeedback =
        (PFNGLBEGINTRANSFORMFEEDBACKPROC)gl__GetProc("glBeginTransformFeedback");
    gl->EndTransformFeedback = (PFNGLENDTRANSFORMFEEDBACKPROC)gl__GetProc("glEndTransformFeedback");
    gl->BindBufferRange = (PFNGLBINDBUFFERRANGEPROC)gl__GetProc("glBindBufferRange");
    gl->BindBufferBase = (PFNGLBINDBUFFERBASEPROC)gl__GetProc("glBindBufferBase");
    gl->TransformFeedbackVaryings =
        (PFNGLTRANSFORMFEEDBACKVARYINGSPROC)gl__GetProc("glTransformFeedbackVaryings");
    gl->GetTransformFeedbackVarying =
        (PFNGLGETTRANSFORMFEEDBACKVARYINGPROC)gl__GetProc("glGetTransformFeedbackVarying");
    gl->ClampColor = (PFNGLCLAMPCOLORPROC)gl__GetProc("glClampColor");
    gl->BeginConditionalRender =
        (PFNGLBEGINCONDITIONALRENDERPROC)gl__GetProc("glBeginConditionalRender");
    gl->EndConditionalRender = (PFNGLENDCONDITIONALRENDERPROC)gl__GetProc("glEndConditionalRender");
    gl->VertexAttribIPointer = (PFNGLVERTEXATTRIBIPOINTERPROC)gl__GetProc("glVertexAttribIPointer");
    gl->GetVertexAttribIiv = (PFNGLGETVERTEXATTRIBIIVPROC)gl__GetProc("glGetVertexAttribIiv");
    gl->GetVertexAttribIuiv = (PFNGLGETVERTEXATTRIBIUIVPROC)gl__GetProc("glGetVertexAttribIuiv");
    gl->VertexAttribI1i = (PFNGLVERTEXATTRIBI1IPROC)gl__GetProc("glVertexAttribI1i");
    gl->VertexAttribI2i = (PFNGLVERTEXATTRIBI2IPROC)gl__GetProc("glVertexAttribI2i");
    gl->VertexAttribI3i = (PFNGLVERTEXATTRIBI3IPROC)gl__GetProc("glVertexAttribI3i");
    gl->VertexAttribI4i = (PFNGLVERTEXATTRIBI4IPROC)gl__GetProc("glVertexAttribI4i");
    gl->VertexAttribI1ui = (PFNGLVERTEXATTRIBI1UIPROC)gl__GetProc("glVertexAttribI1ui");
    gl->VertexAttribI2ui = (PFNGLVERTEXATTRIBI2UIPROC)gl__GetProc("glVertexAttribI2ui");
    gl->VertexAttribI3ui = (PFNGLVERTEXATTRIBI3UIPROC)gl__GetProc("glVertexAttribI3ui");
    gl->VertexAttribI4ui = (PFNGLVERTEXATTRIBI4UIPROC)gl__GetProc("glVertexAttribI4ui");
    gl->VertexAttribI1iv = (PFNGLVERTEXATTRIBI1IVPROC)gl__GetProc("glVertexAttribI1iv");
    gl->VertexAttribI2iv = (PFNGLVERTEXATTRIBI2IVPROC)gl__GetProc("glVertexAttribI2iv");
    gl->VertexAttribI3iv = (PFNGLVERTEXATTRIBI3IVPROC)gl__GetProc("glVertexAttribI3iv");
    gl->VertexAttribI4iv = (PFNGLVERTEXATTRIBI4IVPROC)gl__GetProc("glVertexAttribI4iv");
    gl->VertexAttribI1uiv = (PFNGLVERTEXATTRIBI1UIVPROC)gl__GetProc("glVertexAttribI1uiv");
    gl->VertexAttribI2uiv = (PFNGLVERTEXATTRIBI2UIVPROC)gl__GetProc("glVertexAttribI2uiv");
    gl->VertexAttribI3uiv = (PFNGLVERTEXATTRIBI3UIVPROC)gl__GetProc("glVertexAttribI3uiv");
    gl->VertexAttribI4uiv = (PFNGLVERTEXATTRIBI4UIVPROC)gl__GetProc("glVertexAttribI4uiv");
    gl->VertexAttribI4bv = (PFNGLVERTEXATTRIBI4BVPROC)gl__GetProc("glVertexAttribI4bv");
    gl->VertexAttribI4sv = (PFNGLVERTEXATTRIBI4SVPROC)gl__GetProc("glVertexAttribI4sv");
    gl->VertexAttribI4ubv = (PFNGLVERTEXATTRIBI4UBVPROC)gl__GetProc("glVertexAttribI4ubv");
    gl->VertexAttribI4usv = (PFNGLVERTEXATTRIBI4USVPROC)gl__GetProc("glVertexAttribI4usv");
    gl->GetUniformuiv = (PFNGLGETUNIFORMUIVPROC)gl__GetProc("glGetUniformuiv");
    gl->BindFragDataLocation = (PFNGLBINDFRAGDATALOCATIONPROC)gl__GetProc("glBindFragDataLocation");
    gl->GetFragDataLocation = (PFNGLGETFRAGDATALOCATIONPROC)gl__GetProc("glGetFragDataLocation");
    gl->Uniform1ui = (PFNGLUNIFORM1UIPROC)gl__GetProc("glUniform1ui");
    gl->Uniform2ui = (PFNGLUNIFORM2UIPROC)gl__GetProc("glUniform2ui");
    gl->Uniform3ui = (PFNGLUNIFORM3UIPROC)gl__GetProc("glUniform3ui");
    gl->Uniform4ui = (PFNGLUNIFORM4UIPROC)gl__GetProc("glUniform4ui");
    gl->Uniform1uiv = (PFNGLUNIFORM1UIVPROC)gl__GetProc("glUniform1uiv");
    gl->Uniform2uiv = (PFNGLUNIFORM2UIVPROC)gl__GetProc("glUniform2uiv");
    gl->Uniform3uiv = (PFNGLUNIFORM3UIVPROC)gl__GetProc("glUniform3uiv");
    gl->Uniform4uiv = (PFNGLUNIFORM4UIVPROC)gl__GetProc("glUniform4uiv");
    gl->TexParameterIiv = (PFNGLTEXPARAMETERIIVPROC)gl__GetProc("glTexParameterIiv");
    gl->TexParameterIuiv = (PFNGLTEXPARAMETERIUIVPROC)gl__GetProc("glTexParameterIuiv");
    gl->GetTexParameterIiv = (PFNGLGETTEXPARAMETERIIVPROC)gl__GetProc("glGetTexParameterIiv");
    gl->GetTexParameterIuiv = (PFNGLGETTEXPARAMETERIUIVPROC)gl__GetProc("glGetTexParameterIuiv");
    gl->ClearBufferiv = (PFNGLCLEARBUFFERIVPROC)gl__GetProc("glClearBufferiv");
    gl->ClearBufferuiv = (PFNGLCLEARBUFFERUIVPROC)gl__GetProc("glClearBufferuiv");
    gl->ClearBufferfv = (PFNGLCLEARBUFFERFVPROC)gl__GetProc("glClearBufferfv");
    gl->ClearBufferfi = (PFNGLCLEARBUFFERFIPROC)gl__GetProc("glClearBufferfi");
    gl->GetStringi = (PFNGLGETSTRINGIPROC)gl__GetProc("glGetStringi");
    gl->DrawArraysInstanced = (PFNGLDRAWARRAYSINSTANCEDPROC)gl__GetProc("glDrawArraysInstanced");
    gl->DrawElementsInstanced =
        (PFNGLDRAWELEMENTSINSTANCEDPROC)gl__GetProc("glDrawElementsInstanced");
    gl->TexBuffer = (PFNGLTEXBUFFERPROC)gl__GetProc("glTexBuffer");
    gl->PrimitiveRestartIndex =
        (PFNGLPRIMITIVERESTARTINDEXPROC)gl__GetProc("glPrimitiveRestartIndex");
    gl->GetInteger64i_v = (PFNGLGETINTEGER64I_VPROC)gl__GetProc("glGetInteger64i_v");
    gl->GetBufferParameteri64v =
        (PFNGLGETBUFFERPARAMETERI64VPROC)gl__GetProc("glGetBufferParameteri64v");
    gl->FramebufferTexture = (PFNGLFRAMEBUFFERTEXTUREPROC)gl__GetProc("glFramebufferTexture");
    gl->VertexAttribDivisor = (PFNGLVERTEXATTRIBDIVISORPROC)gl__GetProc("glVertexAttribDivisor");
    gl->MinSampleShading = (PFNGLMINSAMPLESHADINGPROC)gl__GetProc("glMinSampleShading");
    gl->BlendEquationi = (PFNGLBLENDEQUATIONIPROC)gl__GetProc("glBlendEquationi");
    gl->BlendEquationSeparatei =
        (PFNGLBLENDEQUATIONSEPARATEIPROC)gl__GetProc("glBlendEquationSeparatei");
    gl->BlendFunci = (PFNGLBLENDFUNCIPROC)gl__GetProc("glBlendFunci");
    gl->BlendFuncSeparatei = (PFNGLBLENDFUNCSEPARATEIPROC)gl__GetProc("glBlendFuncSeparatei");
    gl->IsRenderbuffer = (PFNGLISRENDERBUFFERPROC)gl__GetProc("glIsRenderbuffer");
    gl->BindRenderbuffer = (PFNGLBINDRENDERBUFFERPROC)gl__GetProc("glBindRenderbuffer");
    gl->DeleteRenderbuffers = (PFNGLDELETERENDERBUFFERSPROC)gl__GetProc("glDeleteRenderbuffers");
    gl->GenRenderbuffers = (PFNGLGENRENDERBUFFERSPROC)gl__GetProc("glGenRenderbuffers");
    gl->RenderbufferStorage = (PFNGLRENDERBUFFERSTORAGEPROC)gl__GetProc("glRenderbufferStorage");
    gl->GetRenderbufferParameteriv =
        (PFNGLGETRENDERBUFFERPARAMETERIVPROC)gl__GetProc("glGetRenderbufferParameteriv");
    gl->IsFramebuffer = (PFNGLISFRAMEBUFFERPROC)gl__GetProc("glIsFramebuffer");
    gl->BindFramebuffer = (PFNGLBINDFRAMEBUFFERPROC)gl__GetProc("glBindFramebuffer");
    gl->DeleteFramebuffers = (PFNGLDELETEFRAMEBUFFERSPROC)gl__GetProc("glDeleteFramebuffers");
    gl->GenFramebuffers = (PFNGLGENFRAMEBUFFERSPROC)gl__GetProc("glGenFramebuffers");
    gl->CheckFramebufferStatus =
        (PFNGLCHECKFRAMEBUFFERSTATUSPROC)gl__GetProc("glCheckFramebufferStatus");
    gl->FramebufferTexture1D = (PFNGLFRAMEBUFFERTEXTURE1DPROC)gl__GetProc("glFramebufferTexture1D");
    gl->FramebufferTexture2D = (PFNGLFRAMEBUFFERTEXTURE2DPROC)gl__GetProc("glFramebufferTexture2D");
    gl->FramebufferTexture3D = (PFNGLFRAMEBUFFERTEXTURE3DPROC)gl__GetProc("glFramebufferTexture3D");
    gl->FramebufferRenderbuffer =
        (PFNGLFRAMEBUFFERRENDERBUFFERPROC)gl__GetProc("glFramebufferRenderbuffer");
    gl->GetFramebufferAttachmentParameteriv =
        (PFNGLGETFRAMEBUFFERATTACHMENTPARAMETERIVPROC)gl__GetProc(
            "glGetFramebufferAttachmentParameteriv");
    gl->GenerateMipmap = (PFNGLGENERATEMIPMAPPROC)gl__GetProc("glGenerateMipmap");
    gl->BlitFramebuffer = (PFNGLBLITFRAMEBUFFERPROC)gl__GetProc("glBlitFramebuffer");
    gl->RenderbufferStorageMultisample =
        (PFNGLRENDERBUFFERSTORAGEMULTISAMPLEPROC)gl__GetProc("glRenderbufferStorageMultisample");
    gl->FramebufferTextureLayer =
        (PFNGLFRAMEBUFFERTEXTURELAYERPROC)gl__GetProc("glFramebufferTextureLayer");
    gl->MapBufferRange = (PFNGLMAPBUFFERRANGEPROC)gl__GetProc("glMapBufferRange");
    gl->FlushMappedBufferRange =
        (PFNGLFLUSHMAPPEDBUFFERRANGEPROC)gl__GetProc("glFlushMappedBufferRange");
    gl->BindVertexArray = (PFNGLBINDVERTEXARRAYPROC)gl__GetProc("glBindVertexArray");
    gl->DeleteVertexArrays = (PFNGLDELETEVERTEXARRAYSPROC)gl__GetProc("glDeleteVertexArrays");
    gl->GenVertexArrays = (PFNGLGENVERTEXARRAYSPROC)gl__GetProc("glGenVertexArrays");
    gl->IsVertexArray = (PFNGLISVERTEXARRAYPROC)gl__GetProc("glIsVertexArray");
    gl->GetUniformIndices = (PFNGLGETUNIFORMINDICESPROC)gl__GetProc("glGetUniformIndices");
    gl->GetActiveUniformsiv = (PFNGLGETACTIVEUNIFORMSIVPROC)gl__GetProc("glGetActiveUniformsiv");
    gl->GetActiveUniformName = (PFNGLGETACTIVEUNIFORMNAMEPROC)gl__GetProc("glGetActiveUniformName");
    gl->GetUniformBlockIndex = (PFNGLGETUNIFORMBLOCKINDEXPROC)gl__GetProc("glGetUniformBlockIndex");
    gl->GetActiveUniformBlockiv =
        (PFNGLGETACTIVEUNIFORMBLOCKIVPROC)gl__GetProc("glGetActiveUniformBlockiv");
    gl->GetActiveUniformBlockName =
        (PFNGLGETACTIVEUNIFORMBLOCKNAMEPROC)gl__GetProc("glGetActiveUniformBlockName");
    gl->UniformBlockBinding = (PFNGLUNIFORMBLOCKBINDINGPROC)gl__GetProc("glUniformBlockBinding");
    gl->CopyBufferSubData = (PFNGLCOPYBUFFERSUBDATAPROC)gl__GetProc("glCopyBufferSubData");
    gl->DrawElementsBaseVertex =
        (PFNGLDRAWELEMENTSBASEVERTEXPROC)gl__GetProc("glDrawElementsBaseVertex");
    gl->DrawRangeElementsBaseVertex =
        (PFNGLDRAWRANGEELEMENTSBASEVERTEXPROC)gl__GetProc("glDrawRangeElementsBaseVertex");
    gl->DrawElementsInstancedBaseVertex =
        (PFNGLDRAWELEMENTSINSTANCEDBASEVERTEXPROC)gl__GetProc("glDrawElementsInstancedBaseVertex");
    gl->MultiDrawElementsBaseVertex =
        (PFNGLMULTIDRAWELEMENTSBASEVERTEXPROC)gl__GetProc("glMultiDrawElementsBaseVertex");
    gl->ProvokingVertex = (PFNGLPROVOKINGVERTEXPROC)gl__GetProc("glProvokingVertex");
    gl->FenceSync = (PFNGLFENCESYNCPROC)gl__GetProc("glFenceSync");
    gl->IsSync = (PFNGLISSYNCPROC)gl__GetProc("glIsSync");
    gl->DeleteSync = (PFNGLDELETESYNCPROC)gl__GetProc("glDeleteSync");
    gl->ClientWaitSync = (PFNGLCLIENTWAITSYNCPROC)gl__GetProc("glClientWaitSync");
    gl->WaitSync = (PFNGLWAITSYNCPROC)gl__GetProc("glWaitSync");
    gl->GetInteger64v = (PFNGLGETINTEGER64VPROC)gl__GetProc("glGetInteger64v");
    gl->GetSynciv = (PFNGLGETSYNCIVPROC)gl__GetProc("glGetSynciv");
    gl->TexImage2DMultisample =
        (PFNGLTEXIMAGE2DMULTISAMPLEPROC)gl__GetProc("glTexImage2DMultisample");
    gl->TexImage3DMultisample =
        (PFNGLTEXIMAGE3DMULTISAMPLEPROC)gl__GetProc("glTexImage3DMultisample");
    gl->GetMultisamplefv = (PFNGLGETMULTISAMPLEFVPROC)gl__GetProc("glGetMultisamplefv");
    gl->SampleMaski = (PFNGLSAMPLEMASKIPROC)gl__GetProc("glSampleMaski");
    gl->BlendEquationiARB = (PFNGLBLENDEQUATIONIARBPROC)gl__GetProc("glBlendEquationiARB");
    gl->BlendEquationSeparateiARB =
        (PFNGLBLENDEQUATIONSEPARATEIARBPROC)gl__GetProc("glBlendEquationSeparateiARB");
    gl->BlendFunciARB = (PFNGLBLENDFUNCIARBPROC)gl__GetProc("glBlendFunciARB");
    gl->BlendFuncSeparateiARB =
        (PFNGLBLENDFUNCSEPARATEIARBPROC)gl__GetProc("glBlendFuncSeparateiARB");
    gl->MinSampleShadingARB = (PFNGLMINSAMPLESHADINGARBPROC)gl__GetProc("glMinSampleShadingARB");
    gl->NamedStringARB = (PFNGLNAMEDSTRINGARBPROC)gl__GetProc("glNamedStringARB");
    gl->DeleteNamedStringARB = (PFNGLDELETENAMEDSTRINGARBPROC)gl__GetProc("glDeleteNamedStringARB");
    gl->CompileShaderIncludeARB =
        (PFNGLCOMPILESHADERINCLUDEARBPROC)gl__GetProc("glCompileShaderIncludeARB");
    gl->IsNamedStringARB = (PFNGLISNAMEDSTRINGARBPROC)gl__GetProc("glIsNamedStringARB");
    gl->GetNamedStringARB = (PFNGLGETNAMEDSTRINGARBPROC)gl__GetProc("glGetNamedStringARB");
    gl->GetNamedStringivARB = (PFNGLGETNAMEDSTRINGIVARBPROC)gl__GetProc("glGetNamedStringivARB");
    gl->BindFragDataLocationIndexed =
        (PFNGLBINDFRAGDATALOCATIONINDEXEDPROC)gl__GetProc("glBindFragDataLocationIndexed");
    gl->GetFragDataIndex = (PFNGLGETFRAGDATAINDEXPROC)gl__GetProc("glGetFragDataIndex");
    gl->GenSamplers = (PFNGLGENSAMPLERSPROC)gl__GetProc("glGenSamplers");
    gl->DeleteSamplers = (PFNGLDELETESAMPLERSPROC)gl__GetProc("glDeleteSamplers");
    gl->IsSampler = (PFNGLISSAMPLERPROC)gl__GetProc("glIsSampler");
    gl->BindSampler = (PFNGLBINDSAMPLERPROC)gl__GetProc("glBindSampler");
    gl->SamplerParameteri = (PFNGLSAMPLERPARAMETERIPROC)gl__GetProc("glSamplerParameteri");
    gl->SamplerParameteriv = (PFNGLSAMPLERPARAMETERIVPROC)gl__GetProc("glSamplerParameteriv");
    gl->SamplerParameterf = (PFNGLSAMPLERPARAMETERFPROC)gl__GetProc("glSamplerParameterf");
    gl->SamplerParameterfv = (PFNGLSAMPLERPARAMETERFVPROC)gl__GetProc("glSamplerParameterfv");
    gl->SamplerParameterIiv = (PFNGLSAMPLERPARAMETERIIVPROC)gl__GetProc("glSamplerParameterIiv");
    gl->SamplerParameterIuiv = (PFNGLSAMPLERPARAMETERIUIVPROC)gl__GetProc("glSamplerParameterIuiv");
    gl->GetSamplerParameteriv =
        (PFNGLGETSAMPLERPARAMETERIVPROC)gl__GetProc("glGetSamplerParameteriv");
    gl->GetSamplerParameterIiv =
        (PFNGLGETSAMPLERPARAMETERIIVPROC)gl__GetProc("glGetSamplerParameterIiv");
    gl->GetSamplerParameterfv =
        (PFNGLGETSAMPLERPARAMETERFVPROC)gl__GetProc("glGetSamplerParameterfv");
    gl->GetSamplerParameterIuiv =
        (PFNGLGETSAMPLERPARAMETERIUIVPROC)gl__GetProc("glGetSamplerParameterIuiv");
    gl->QueryCounter = (PFNGLQUERYCOUNTERPROC)gl__GetProc("glQueryCounter");
    gl->GetQueryObjecti64v = (PFNGLGETQUERYOBJECTI64VPROC)gl__GetProc("glGetQueryObjecti64v");
    gl->GetQueryObjectui64v = (PFNGLGETQUERYOBJECTUI64VPROC)gl__GetProc("glGetQueryObjectui64v");
    gl->VertexP2ui = (PFNGLVERTEXP2UIPROC)gl__GetProc("glVertexP2ui");
    gl->VertexP2uiv = (PFNGLVERTEXP2UIVPROC)gl__GetProc("glVertexP2uiv");
    gl->VertexP3ui = (PFNGLVERTEXP3UIPROC)gl__GetProc("glVertexP3ui");
    gl->VertexP3uiv = (PFNGLVERTEXP3UIVPROC)gl__GetProc("glVertexP3uiv");
    gl->VertexP4ui = (PFNGLVERTEXP4UIPROC)gl__GetProc("glVertexP4ui");
    gl->VertexP4uiv = (PFNGLVERTEXP4UIVPROC)gl__GetProc("glVertexP4uiv");
    gl->TexCoordP1ui = (PFNGLTEXCOORDP1UIPROC)gl__GetProc("glTexCoordP1ui");
    gl->TexCoordP1uiv = (PFNGLTEXCOORDP1UIVPROC)gl__GetProc("glTexCoordP1uiv");
    gl->TexCoordP2ui = (PFNGLTEXCOORDP2UIPROC)gl__GetProc("glTexCoordP2ui");
    gl->TexCoordP2uiv = (PFNGLTEXCOORDP2UIVPROC)gl__GetProc("glTexCoordP2uiv");
    gl->TexCoordP3ui = (PFNGLTEXCOORDP3UIPROC)gl__GetProc("glTexCoordP3ui");
    gl->TexCoordP3uiv = (PFNGLTEXCOORDP3UIVPROC)gl__GetProc("glTexCoordP3uiv");
    gl->TexCoordP4ui = (PFNGLTEXCOORDP4UIPROC)gl__GetProc("glTexCoordP4ui");
    gl->TexCoordP4uiv = (PFNGLTEXCOORDP4UIVPROC)gl__GetProc("glTexCoordP4uiv");
    gl->MultiTexCoordP1ui = (PFNGLMULTITEXCOORDP1UIPROC)gl__GetProc("glMultiTexCoordP1ui");
    gl->MultiTexCoordP1uiv = (PFNGLMULTITEXCOORDP1UIVPROC)gl__GetProc("glMultiTexCoordP1uiv");
    gl->MultiTexCoordP2ui = (PFNGLMULTITEXCOORDP2UIPROC)gl__GetProc("glMultiTexCoordP2ui");
    gl->MultiTexCoordP2uiv = (PFNGLMULTITEXCOORDP2UIVPROC)gl__GetProc("glMultiTexCoordP2uiv");
    gl->MultiTexCoordP3ui = (PFNGLMULTITEXCOORDP3UIPROC)gl__GetProc("glMultiTexCoordP3ui");
    gl->MultiTexCoordP3uiv = (PFNGLMULTITEXCOORDP3UIVPROC)gl__GetProc("glMultiTexCoordP3uiv");
    gl->MultiTexCoordP4ui = (PFNGLMULTITEXCOORDP4UIPROC)gl__GetProc("glMultiTexCoordP4ui");
    gl->MultiTexCoordP4uiv = (PFNGLMULTITEXCOORDP4UIVPROC)gl__GetProc("glMultiTexCoordP4uiv");
    gl->NormalP3ui = (PFNGLNORMALP3UIPROC)gl__GetProc("glNormalP3ui");
    gl->NormalP3uiv = (PFNGLNORMALP3UIVPROC)gl__GetProc("glNormalP3uiv");
    gl->ColorP3ui = (PFNGLCOLORP3UIPROC)gl__GetProc("glColorP3ui");
    gl->ColorP3uiv = (PFNGLCOLORP3UIVPROC)gl__GetProc("glColorP3uiv");
    gl->ColorP4ui = (PFNGLCOLORP4UIPROC)gl__GetProc("glColorP4ui");
    gl->ColorP4uiv = (PFNGLCOLORP4UIVPROC)gl__GetProc("glColorP4uiv");
    gl->SecondaryColorP3ui = (PFNGLSECONDARYCOLORP3UIPROC)gl__GetProc("glSecondaryColorP3ui");
    gl->SecondaryColorP3uiv = (PFNGLSECONDARYCOLORP3UIVPROC)gl__GetProc("glSecondaryColorP3uiv");
    gl->VertexAttribP1ui = (PFNGLVERTEXATTRIBP1UIPROC)gl__GetProc("glVertexAttribP1ui");
    gl->VertexAttribP1uiv = (PFNGLVERTEXATTRIBP1UIVPROC)gl__GetProc("glVertexAttribP1uiv");
    gl->VertexAttribP2ui = (PFNGLVERTEXATTRIBP2UIPROC)gl__GetProc("glVertexAttribP2ui");
    gl->VertexAttribP2uiv = (PFNGLVERTEXATTRIBP2UIVPROC)gl__GetProc("glVertexAttribP2uiv");
    gl->VertexAttribP3ui = (PFNGLVERTEXATTRIBP3UIPROC)gl__GetProc("glVertexAttribP3ui");
    gl->VertexAttribP3uiv = (PFNGLVERTEXATTRIBP3UIVPROC)gl__GetProc("glVertexAttribP3uiv");
    gl->VertexAttribP4ui = (PFNGLVERTEXATTRIBP4UIPROC)gl__GetProc("glVertexAttribP4ui");
    gl->VertexAttribP4uiv = (PFNGLVERTEXATTRIBP4UIVPROC)gl__GetProc("glVertexAttribP4uiv");
    gl->DrawArraysIndirect = (PFNGLDRAWARRAYSINDIRECTPROC)gl__GetProc("glDrawArraysIndirect");
    gl->DrawElementsIndirect = (PFNGLDRAWELEMENTSINDIRECTPROC)gl__GetProc("glDrawElementsIndirect");
    gl->Uniform1d = (PFNGLUNIFORM1DPROC)gl__GetProc("glUniform1d");
    gl->Uniform2d = (PFNGLUNIFORM2DPROC)gl__GetProc("glUniform2d");
    gl->Uniform3d = (PFNGLUNIFORM3DPROC)gl__GetProc("glUniform3d");
    gl->Uniform4d = (PFNGLUNIFORM4DPROC)gl__GetProc("glUniform4d");
    gl->Uniform1dv = (PFNGLUNIFORM1DVPROC)gl__GetProc("glUniform1dv");
    gl->Uniform2dv = (PFNGLUNIFORM2DVPROC)gl__GetProc("glUniform2dv");
    gl->Uniform3dv = (PFNGLUNIFORM3DVPROC)gl__GetProc("glUniform3dv");
    gl->Uniform4dv = (PFNGLUNIFORM4DVPROC)gl__GetProc("glUniform4dv");
    gl->UniformMatrix2dv = (PFNGLUNIFORMMATRIX2DVPROC)gl__GetProc("glUniformMatrix2dv");
    gl->UniformMatrix3dv = (PFNGLUNIFORMMATRIX3DVPROC)gl__GetProc("glUniformMatrix3dv");
    gl->UniformMatrix4dv = (PFNGLUNIFORMMATRIX4DVPROC)gl__GetProc("glUniformMatrix4dv");
    gl->UniformMatrix2x3dv = (PFNGLUNIFORMMATRIX2X3DVPROC)gl__GetProc("glUniformMatrix2x3dv");
    gl->UniformMatrix2x4dv = (PFNGLUNIFORMMATRIX2X4DVPROC)gl__GetProc("glUniformMatrix2x4dv");
    gl->UniformMatrix3x2dv = (PFNGLUNIFORMMATRIX3X2DVPROC)gl__GetProc("glUniformMatrix3x2dv");
    gl->UniformMatrix3x4dv = (PFNGLUNIFORMMATRIX3X4DVPROC)gl__GetProc("glUniformMatrix3x4dv");
    gl->UniformMatrix4x2dv = (PFNGLUNIFORMMATRIX4X2DVPROC)gl__GetProc("glUniformMatrix4x2dv");
    gl->UniformMatrix4x3dv = (PFNGLUNIFORMMATRIX4X3DVPROC)gl__GetProc("glUniformMatrix4x3dv");
    gl->GetUniformdv = (PFNGLGETUNIFORMDVPROC)gl__GetProc("glGetUniformdv");
    gl->GetSubroutineUniformLocation =
        (PFNGLGETSUBROUTINEUNIFORMLOCATIONPROC)gl__GetProc("glGetSubroutineUniformLocation");
    gl->GetSubroutineIndex = (PFNGLGETSUBROUTINEINDEXPROC)gl__GetProc("glGetSubroutineIndex");
    gl->GetActiveSubroutineUniformiv =
        (PFNGLGETACTIVESUBROUTINEUNIFORMIVPROC)gl__GetProc("glGetActiveSubroutineUniformiv");
    gl->GetActiveSubroutineUniformName =
        (PFNGLGETACTIVESUBROUTINEUNIFORMNAMEPROC)gl__GetProc("glGetActiveSubroutineUniformName");
    gl->GetActiveSubroutineName =
        (PFNGLGETACTIVESUBROUTINENAMEPROC)gl__GetProc("glGetActiveSubroutineName");
    gl->UniformSubroutinesuiv =
        (PFNGLUNIFORMSUBROUTINESUIVPROC)gl__GetProc("glUniformSubroutinesuiv");
    gl->GetUniformSubroutineuiv =
        (PFNGLGETUNIFORMSUBROUTINEUIVPROC)gl__GetProc("glGetUniformSubroutineuiv");
    gl->GetProgramStageiv = (PFNGLGETPROGRAMSTAGEIVPROC)gl__GetProc("glGetProgramStageiv");
    gl->PatchParameteri = (PFNGLPATCHPARAMETERIPROC)gl__GetProc("glPatchParameteri");
    gl->PatchParameterfv = (PFNGLPATCHPARAMETERFVPROC)gl__GetProc("glPatchParameterfv");
    gl->BindTransformFeedback =
        (PFNGLBINDTRANSFORMFEEDBACKPROC)gl__GetProc("glBindTransformFeedback");
    gl->DeleteTransformFeedbacks =
        (PFNGLDELETETRANSFORMFEEDBACKSPROC)gl__GetProc("glDeleteTransformFeedbacks");
    gl->GenTransformFeedbacks =
        (PFNGLGENTRANSFORMFEEDBACKSPROC)gl__GetProc("glGenTransformFeedbacks");
    gl->IsTransformFeedback = (PFNGLISTRANSFORMFEEDBACKPROC)gl__GetProc("glIsTransformFeedback");
    gl->PauseTransformFeedback =
        (PFNGLPAUSETRANSFORMFEEDBACKPROC)gl__GetProc("glPauseTransformFeedback");
    gl->ResumeTransformFeedback =
        (PFNGLRESUMETRANSFORMFEEDBACKPROC)gl__GetProc("glResumeTransformFeedback");
    gl->DrawTransformFeedback =
        (PFNGLDRAWTRANSFORMFEEDBACKPROC)gl__GetProc("glDrawTransformFeedback");
    gl->DrawTransformFeedbackStream =
        (PFNGLDRAWTRANSFORMFEEDBACKSTREAMPROC)gl__GetProc("glDrawTransformFeedbackStream");
    gl->BeginQueryIndexed = (PFNGLBEGINQUERYINDEXEDPROC)gl__GetProc("glBeginQueryIndexed");
    gl->EndQueryIndexed = (PFNGLENDQUERYINDEXEDPROC)gl__GetProc("glEndQueryIndexed");
    gl->GetQueryIndexediv = (PFNGLGETQUERYINDEXEDIVPROC)gl__GetProc("glGetQueryIndexediv");
    gl->ReleaseShaderCompiler =
        (PFNGLRELEASESHADERCOMPILERPROC)gl__GetProc("glReleaseShaderCompiler");
    gl->ShaderBinary = (PFNGLSHADERBINARYPROC)gl__GetProc("glShaderBinary");
    gl->GetShaderPrecisionFormat =
        (PFNGLGETSHADERPRECISIONFORMATPROC)gl__GetProc("glGetShaderPrecisionFormat");
    gl->DepthRangef = (PFNGLDEPTHRANGEFPROC)gl__GetProc("glDepthRangef");
    gl->ClearDepthf = (PFNGLCLEARDEPTHFPROC)gl__GetProc("glClearDepthf");
    gl->GetProgramBinary = (PFNGLGETPROGRAMBINARYPROC)gl__GetProc("glGetProgramBinary");
    gl->ProgramBinary = (PFNGLPROGRAMBINARYPROC)gl__GetProc("glProgramBinary");
    gl->ProgramParameteri = (PFNGLPROGRAMPARAMETERIPROC)gl__GetProc("glProgramParameteri");
    gl->UseProgramStages = (PFNGLUSEPROGRAMSTAGESPROC)gl__GetProc("glUseProgramStages");
    gl->ActiveShaderProgram = (PFNGLACTIVESHADERPROGRAMPROC)gl__GetProc("glActiveShaderProgram");
    gl->CreateShaderProgramv = (PFNGLCREATESHADERPROGRAMVPROC)gl__GetProc("glCreateShaderProgramv");
    gl->BindProgramPipeline = (PFNGLBINDPROGRAMPIPELINEPROC)gl__GetProc("glBindProgramPipeline");
    gl->DeleteProgramPipelines =
        (PFNGLDELETEPROGRAMPIPELINESPROC)gl__GetProc("glDeleteProgramPipelines");
    gl->GenProgramPipelines = (PFNGLGENPROGRAMPIPELINESPROC)gl__GetProc("glGenProgramPipelines");
    gl->IsProgramPipeline = (PFNGLISPROGRAMPIPELINEPROC)gl__GetProc("glIsProgramPipeline");
    gl->GetProgramPipelineiv = (PFNGLGETPROGRAMPIPELINEIVPROC)gl__GetProc("glGetProgramPipelineiv");
    gl->ProgramUniform1i = (PFNGLPROGRAMUNIFORM1IPROC)gl__GetProc("glProgramUniform1i");
    gl->ProgramUniform1iv = (PFNGLPROGRAMUNIFORM1IVPROC)gl__GetProc("glProgramUniform1iv");
    gl->ProgramUniform1f = (PFNGLPROGRAMUNIFORM1FPROC)gl__GetProc("glProgramUniform1f");
    gl->ProgramUniform1fv = (PFNGLPROGRAMUNIFORM1FVPROC)gl__GetProc("glProgramUniform1fv");
    gl->ProgramUniform1d = (PFNGLPROGRAMUNIFORM1DPROC)gl__GetProc("glProgramUniform1d");
    gl->ProgramUniform1dv = (PFNGLPROGRAMUNIFORM1DVPROC)gl__GetProc("glProgramUniform1dv");
    gl->ProgramUniform1ui = (PFNGLPROGRAMUNIFORM1UIPROC)gl__GetProc("glProgramUniform1ui");
    gl->ProgramUniform1uiv = (PFNGLPROGRAMUNIFORM1UIVPROC)gl__GetProc("glProgramUniform1uiv");
    gl->ProgramUniform2i = (PFNGLPROGRAMUNIFORM2IPROC)gl__GetProc("glProgramUniform2i");
    gl->ProgramUniform2iv = (PFNGLPROGRAMUNIFORM2IVPROC)gl__GetProc("glProgramUniform2iv");
    gl->ProgramUniform2f = (PFNGLPROGRAMUNIFORM2FPROC)gl__GetProc("glProgramUniform2f");
    gl->ProgramUniform2fv = (PFNGLPROGRAMUNIFORM2FVPROC)gl__GetProc("glProgramUniform2fv");
    gl->ProgramUniform2d = (PFNGLPROGRAMUNIFORM2DPROC)gl__GetProc("glProgramUniform2d");
    gl->ProgramUniform2dv = (PFNGLPROGRAMUNIFORM2DVPROC)gl__GetProc("glProgramUniform2dv");
    gl->ProgramUniform2ui = (PFNGLPROGRAMUNIFORM2UIPROC)gl__GetProc("glProgramUniform2ui");
    gl->ProgramUniform2uiv = (PFNGLPROGRAMUNIFORM2UIVPROC)gl__GetProc("glProgramUniform2uiv");
    gl->ProgramUniform3i = (PFNGLPROGRAMUNIFORM3IPROC)gl__GetProc("glProgramUniform3i");
    gl->ProgramUniform3iv = (PFNGLPROGRAMUNIFORM3IVPROC)gl__GetProc("glProgramUniform3iv");
    gl->ProgramUniform3f = (PFNGLPROGRAMUNIFORM3FPROC)gl__GetProc("glProgramUniform3f");
    gl->ProgramUniform3fv = (PFNGLPROGRAMUNIFORM3FVPROC)gl__GetProc("glProgramUniform3fv");
    gl->ProgramUniform3d = (PFNGLPROGRAMUNIFORM3DPROC)gl__GetProc("glProgramUniform3d");
    gl->ProgramUniform3dv = (PFNGLPROGRAMUNIFORM3DVPROC)gl__GetProc("glProgramUniform3dv");
    gl->ProgramUniform3ui = (PFNGLPROGRAMUNIFORM3UIPROC)gl__GetProc("glProgramUniform3ui");
    gl->ProgramUniform3uiv = (PFNGLPROGRAMUNIFORM3UIVPROC)gl__GetProc("glProgramUniform3uiv");
    gl->ProgramUniform4i = (PFNGLPROGRAMUNIFORM4IPROC)gl__GetProc("glProgramUniform4i");
    gl->ProgramUniform4iv = (PFNGLPROGRAMUNIFORM4IVPROC)gl__GetProc("glProgramUniform4iv");
    gl->ProgramUniform4f = (PFNGLPROGRAMUNIFORM4FPROC)gl__GetProc("glProgramUniform4f");
    gl->ProgramUniform4fv = (PFNGLPROGRAMUNIFORM4FVPROC)gl__GetProc("glProgramUniform4fv");
    gl->ProgramUniform4d = (PFNGLPROGRAMUNIFORM4DPROC)gl__GetProc("glProgramUniform4d");
    gl->ProgramUniform4dv = (PFNGLPROGRAMUNIFORM4DVPROC)gl__GetProc("glProgramUniform4dv");
    gl->ProgramUniform4ui = (PFNGLPROGRAMUNIFORM4UIPROC)gl__GetProc("glProgramUniform4ui");
    gl->ProgramUniform4uiv = (PFNGLPROGRAMUNIFORM4UIVPROC)gl__GetProc("glProgramUniform4uiv");
    gl->ProgramUniformMatrix2fv =
        (PFNGLPROGRAMUNIFORMMATRIX2FVPROC)gl__GetProc("glProgramUniformMatrix2fv");
    gl->ProgramUniformMatrix3fv =
        (PFNGLPROGRAMUNIFORMMATRIX3FVPROC)gl__GetProc("glProgramUniformMatrix3fv");
    gl->ProgramUniformMatrix4fv =
        (PFNGLPROGRAMUNIFORMMATRIX4FVPROC)gl__GetProc("glProgramUniformMatrix4fv");
    gl->ProgramUniformMatrix2dv =
        (PFNGLPROGRAMUNIFORMMATRIX2DVPROC)gl__GetProc("glProgramUniformMatrix2dv");
    gl->ProgramUniformMatrix3dv =
        (PFNGLPROGRAMUNIFORMMATRIX3DVPROC)gl__GetProc("glProgramUniformMatrix3dv");
    gl->ProgramUniformMatrix4dv =
        (PFNGLPROGRAMUNIFORMMATRIX4DVPROC)gl__GetProc("glProgramUniformMatrix4dv");
    gl->ProgramUniformMatrix2x3fv =
        (PFNGLPROGRAMUNIFORMMATRIX2X3FVPROC)gl__GetProc("glProgramUniformMatrix2x3fv");
    gl->ProgramUniformMatrix3x2fv =
        (PFNGLPROGRAMUNIFORMMATRIX3X2FVPROC)gl__GetProc("glProgramUniformMatrix3x2fv");
    gl->ProgramUniformMatrix2x4fv =
        (PFNGLPROGRAMUNIFORMMATRIX2X4FVPROC)gl__GetProc("glProgramUniformMatrix2x4fv");
    gl->ProgramUniformMatrix4x2fv =
        (PFNGLPROGRAMUNIFORMMATRIX4X2FVPROC)gl__GetProc("glProgramUniformMatrix4x2fv");
    gl->ProgramUniformMatrix3x4fv =
        (PFNGLPROGRAMUNIFORMMATRIX3X4FVPROC)gl__GetProc("glProgramUniformMatrix3x4fv");
    gl->ProgramUniformMatrix4x3fv =
        (PFNGLPROGRAMUNIFORMMATRIX4X3FVPROC)gl__GetProc("glProgramUniformMatrix4x3fv");
    gl->ProgramUniformMatrix2x3dv =
        (PFNGLPROGRAMUNIFORMMATRIX2X3DVPROC)gl__GetProc("glProgramUniformMatrix2x3dv");
    gl->ProgramUniformMatrix3x2dv =
        (PFNGLPROGRAMUNIFORMMATRIX3X2DVPROC)gl__GetProc("glProgramUniformMatrix3x2dv");
    gl->ProgramUniformMatrix2x4dv =
        (PFNGLPROGRAMUNIFORMMATRIX2X4DVPROC)gl__GetProc("glProgramUniformMatrix2x4dv");
    gl->ProgramUniformMatrix4x2dv =
        (PFNGLPROGRAMUNIFORMMATRIX4X2DVPROC)gl__GetProc("glProgramUniformMatrix4x2dv");
    gl->ProgramUniformMatrix3x4dv =
        (PFNGLPROGRAMUNIFORMMATRIX3X4DVPROC)gl__GetProc("glProgramUniformMatrix3x4dv");
    gl->ProgramUniformMatrix4x3dv =
        (PFNGLPROGRAMUNIFORMMATRIX4X3DVPROC)gl__GetProc("glProgramUniformMatrix4x3dv");
    gl->ValidateProgramPipeline =
        (PFNGLVALIDATEPROGRAMPIPELINEPROC)gl__GetProc("glValidateProgramPipeline");
    gl->GetProgramPipelineInfoLog =
        (PFNGLGETPROGRAMPIPELINEINFOLOGPROC)gl__GetProc("glGetProgramPipelineInfoLog");
    gl->VertexAttribL1d = (PFNGLVERTEXATTRIBL1DPROC)gl__GetProc("glVertexAttribL1d");
    gl->VertexAttribL2d = (PFNGLVERTEXATTRIBL2DPROC)gl__GetProc("glVertexAttribL2d");
    gl->VertexAttribL3d = (PFNGLVERTEXATTRIBL3DPROC)gl__GetProc("glVertexAttribL3d");
    gl->VertexAttribL4d = (PFNGLVERTEXATTRIBL4DPROC)gl__GetProc("glVertexAttribL4d");
    gl->VertexAttribL1dv = (PFNGLVERTEXATTRIBL1DVPROC)gl__GetProc("glVertexAttribL1dv");
    gl->VertexAttribL2dv = (PFNGLVERTEXATTRIBL2DVPROC)gl__GetProc("glVertexAttribL2dv");
    gl->VertexAttribL3dv = (PFNGLVERTEXATTRIBL3DVPROC)gl__GetProc("glVertexAttribL3dv");
    gl->VertexAttribL4dv = (PFNGLVERTEXATTRIBL4DVPROC)gl__GetProc("glVertexAttribL4dv");
    gl->VertexAttribLPointer = (PFNGLVERTEXATTRIBLPOINTERPROC)gl__GetProc("glVertexAttribLPointer");
    gl->GetVertexAttribLdv = (PFNGLGETVERTEXATTRIBLDVPROC)gl__GetProc("glGetVertexAttribLdv");
    gl->ViewportArrayv = (PFNGLVIEWPORTARRAYVPROC)gl__GetProc("glViewportArrayv");
    gl->ViewportIndexedf = (PFNGLVIEWPORTINDEXEDFPROC)gl__GetProc("glViewportIndexedf");
    gl->ViewportIndexedfv = (PFNGLVIEWPORTINDEXEDFVPROC)gl__GetProc("glViewportIndexedfv");
    gl->ScissorArrayv = (PFNGLSCISSORARRAYVPROC)gl__GetProc("glScissorArrayv");
    gl->ScissorIndexed = (PFNGLSCISSORINDEXEDPROC)gl__GetProc("glScissorIndexed");
    gl->ScissorIndexedv = (PFNGLSCISSORINDEXEDVPROC)gl__GetProc("glScissorIndexedv");
    gl->DepthRangeArrayv = (PFNGLDEPTHRANGEARRAYVPROC)gl__GetProc("glDepthRangeArrayv");
    gl->DepthRangeIndexed = (PFNGLDEPTHRANGEINDEXEDPROC)gl__GetProc("glDepthRangeIndexed");
    gl->GetFloati_v = (PFNGLGETFLOATI_VPROC)gl__GetProc("glGetFloati_v");
    gl->GetDoublei_v = (PFNGLGETDOUBLEI_VPROC)gl__GetProc("glGetDoublei_v");
    gl->CreateSyncFromCLeventARB =
        (PFNGLCREATESYNCFROMCLEVENTARBPROC)gl__GetProc("glCreateSyncFromCLeventARB");
    gl->DebugMessageControlARB =
        (PFNGLDEBUGMESSAGECONTROLARBPROC)gl__GetProc("glDebugMessageControlARB");
    gl->DebugMessageInsertARB =
        (PFNGLDEBUGMESSAGEINSERTARBPROC)gl__GetProc("glDebugMessageInsertARB");
    gl->DebugMessageCallbackARB =
        (PFNGLDEBUGMESSAGECALLBACKARBPROC)gl__GetProc("glDebugMessageCallbackARB");
    gl->GetDebugMessageLogARB =
        (PFNGLGETDEBUGMESSAGELOGARBPROC)gl__GetProc("glGetDebugMessageLogARB");
    gl->GetGraphicsResetStatusARB =
        (PFNGLGETGRAPHICSRESETSTATUSARBPROC)gl__GetProc("glGetGraphicsResetStatusARB");
    gl->GetnTexImageARB = (PFNGLGETNTEXIMAGEARBPROC)gl__GetProc("glGetnTexImageARB");
    gl->ReadnPixelsARB = (PFNGLREADNPIXELSARBPROC)gl__GetProc("glReadnPixelsARB");
    gl->GetnCompressedTexImageARB =
        (PFNGLGETNCOMPRESSEDTEXIMAGEARBPROC)gl__GetProc("glGetnCompressedTexImageARB");
    gl->GetnUniformfvARB = (PFNGLGETNUNIFORMFVARBPROC)gl__GetProc("glGetnUniformfvARB");
    gl->GetnUniformivARB = (PFNGLGETNUNIFORMIVARBPROC)gl__GetProc("glGetnUniformivARB");
    gl->GetnUniformuivARB = (PFNGLGETNUNIFORMUIVARBPROC)gl__GetProc("glGetnUniformuivARB");
    gl->GetnUniformdvARB = (PFNGLGETNUNIFORMDVARBPROC)gl__GetProc("glGetnUniformdvARB");
    gl->DrawArraysInstancedBaseInstance =
        (PFNGLDRAWARRAYSINSTANCEDBASEINSTANCEPROC)gl__GetProc("glDrawArraysInstancedBaseInstance");
    gl->DrawElementsInstancedBaseInstance = (PFNGLDRAWELEMENTSINSTANCEDBASEINSTANCEPROC)gl__GetProc(
        "glDrawElementsInstancedBaseInstance");
    gl->DrawElementsInstancedBaseVertexBaseInstance =
        (PFNGLDRAWELEMENTSINSTANCEDBASEVERTEXBASEINSTANCEPROC)gl__GetProc(
            "glDrawElementsInstancedBaseVertexBaseInstance");
    gl->DrawTransformFeedbackInstanced =
        (PFNGLDRAWTRANSFORMFEEDBACKINSTANCEDPROC)gl__GetProc("glDrawTransformFeedbackInstanced");
    gl->DrawTransformFeedbackStreamInstanced =
        (PFNGLDRAWTRANSFORMFEEDBACKSTREAMINSTANCEDPROC)gl__GetProc(
            "glDrawTransformFeedbackStreamInstanced");
    gl->GetInternalformativ = (PFNGLGETINTERNALFORMATIVPROC)gl__GetProc("glGetInternalformativ");
    gl->GetActiveAtomicCounterBufferiv =
        (PFNGLGETACTIVEATOMICCOUNTERBUFFERIVPROC)gl__GetProc("glGetActiveAtomicCounterBufferiv");
    gl->BindImageTexture = (PFNGLBINDIMAGETEXTUREPROC)gl__GetProc("glBindImageTexture");
    gl->MemoryBarrier = (PFNGLMEMORYBARRIERPROC)gl__GetProc("glMemoryBarrier");
    gl->TexStorage1D = (PFNGLTEXSTORAGE1DPROC)gl__GetProc("glTexStorage1D");
    gl->TexStorage2D = (PFNGLTEXSTORAGE2DPROC)gl__GetProc("glTexStorage2D");
    gl->TexStorage3D = (PFNGLTEXSTORAGE3DPROC)gl__GetProc("glTexStorage3D");
    gl->TextureStorage1DEXT = (PFNGLTEXTURESTORAGE1DEXTPROC)gl__GetProc("glTextureStorage1DEXT");
    gl->TextureStorage2DEXT = (PFNGLTEXTURESTORAGE2DEXTPROC)gl__GetProc("glTextureStorage2DEXT");
    gl->TextureStorage3DEXT = (PFNGLTEXTURESTORAGE3DEXTPROC)gl__GetProc("glTextureStorage3DEXT");
    gl->DebugMessageControl = (PFNGLDEBUGMESSAGECONTROLPROC)gl__GetProc("glDebugMessageControl");
    gl->DebugMessageInsert = (PFNGLDEBUGMESSAGEINSERTPROC)gl__GetProc("glDebugMessageInsert");
    gl->DebugMessageCallback = (PFNGLDEBUGMESSAGECALLBACKPROC)gl__GetProc("glDebugMessageCallback");
    gl->GetDebugMessageLog = (PFNGLGETDEBUGMESSAGELOGPROC)gl__GetProc("glGetDebugMessageLog");
    gl->PushDebugGroup = (PFNGLPUSHDEBUGGROUPPROC)gl__GetProc("glPushDebugGroup");
    gl->PopDebugGroup = (PFNGLPOPDEBUGGROUPPROC)gl__GetProc("glPopDebugGroup");
    gl->ObjectLabel = (PFNGLOBJECTLABELPROC)gl__GetProc("glObjectLabel");
    gl->GetObjectLabel = (PFNGLGETOBJECTLABELPROC)gl__GetProc("glGetObjectLabel");
    gl->ObjectPtrLabel = (PFNGLOBJECTPTRLABELPROC)gl__GetProc("glObjectPtrLabel");
    gl->GetObjectPtrLabel = (PFNGLGETOBJECTPTRLABELPROC)gl__GetProc("glGetObjectPtrLabel");
    gl->ClearBufferData = (PFNGLCLEARBUFFERDATAPROC)gl__GetProc("glClearBufferData");
    gl->ClearBufferSubData = (PFNGLCLEARBUFFERSUBDATAPROC)gl__GetProc("glClearBufferSubData");
    gl->ClearNamedBufferDataEXT =
        (PFNGLCLEARNAMEDBUFFERDATAEXTPROC)gl__GetProc("glClearNamedBufferDataEXT");
    gl->ClearNamedBufferSubDataEXT =
        (PFNGLCLEARNAMEDBUFFERSUBDATAEXTPROC)gl__GetProc("glClearNamedBufferSubDataEXT");
    gl->DispatchCompute = (PFNGLDISPATCHCOMPUTEPROC)gl__GetProc("glDispatchCompute");
    gl->DispatchComputeIndirect =
        (PFNGLDISPATCHCOMPUTEINDIRECTPROC)gl__GetProc("glDispatchComputeIndirect");
    gl->CopyImageSubData = (PFNGLCOPYIMAGESUBDATAPROC)gl__GetProc("glCopyImageSubData");
    gl->TextureView = (PFNGLTEXTUREVIEWPROC)gl__GetProc("glTextureView");
    gl->BindVertexBuffer = (PFNGLBINDVERTEXBUFFERPROC)gl__GetProc("glBindVertexBuffer");
    gl->VertexAttribFormat = (PFNGLVERTEXATTRIBFORMATPROC)gl__GetProc("glVertexAttribFormat");
    gl->VertexAttribIFormat = (PFNGLVERTEXATTRIBIFORMATPROC)gl__GetProc("glVertexAttribIFormat");
    gl->VertexAttribLFormat = (PFNGLVERTEXATTRIBLFORMATPROC)gl__GetProc("glVertexAttribLFormat");
    gl->VertexAttribBinding = (PFNGLVERTEXATTRIBBINDINGPROC)gl__GetProc("glVertexAttribBinding");
    gl->VertexBindingDivisor = (PFNGLVERTEXBINDINGDIVISORPROC)gl__GetProc("glVertexBindingDivisor");
    gl->VertexArrayBindVertexBufferEXT =
        (PFNGLVERTEXARRAYBINDVERTEXBUFFEREXTPROC)gl__GetProc("glVertexArrayBindVertexBufferEXT");
    gl->VertexArrayVertexAttribFormatEXT = (PFNGLVERTEXARRAYVERTEXATTRIBFORMATEXTPROC)gl__GetProc(
        "glVertexArrayVertexAttribFormatEXT");
    gl->VertexArrayVertexAttribIFormatEXT = (PFNGLVERTEXARRAYVERTEXATTRIBIFORMATEXTPROC)gl__GetProc(
        "glVertexArrayVertexAttribIFormatEXT");
    gl->VertexArrayVertexAttribLFormatEXT = (PFNGLVERTEXARRAYVERTEXATTRIBLFORMATEXTPROC)gl__GetProc(
        "glVertexArrayVertexAttribLFormatEXT");
    gl->VertexArrayVertexAttribBindingEXT = (PFNGLVERTEXARRAYVERTEXATTRIBBINDINGEXTPROC)gl__GetProc(
        "glVertexArrayVertexAttribBindingEXT");
    gl->VertexArrayVertexBindingDivisorEXT =
        (PFNGLVERTEXARRAYVERTEXBINDINGDIVISOREXTPROC)gl__GetProc(
            "glVertexArrayVertexBindingDivisorEXT");
    gl->FramebufferParameteri =
        (PFNGLFRAMEBUFFERPARAMETERIPROC)gl__GetProc("glFramebufferParameteri");
    gl->GetFramebufferParameteriv =
        (PFNGLGETFRAMEBUFFERPARAMETERIVPROC)gl__GetProc("glGetFramebufferParameteriv");
    gl->NamedFramebufferParameteriEXT =
        (PFNGLNAMEDFRAMEBUFFERPARAMETERIEXTPROC)gl__GetProc("glNamedFramebufferParameteriEXT");
    gl->GetNamedFramebufferParameterivEXT = (PFNGLGETNAMEDFRAMEBUFFERPARAMETERIVEXTPROC)gl__GetProc(
        "glGetNamedFramebufferParameterivEXT");
    gl->GetInternalformati64v =
        (PFNGLGETINTERNALFORMATI64VPROC)gl__GetProc("glGetInternalformati64v");
    gl->InvalidateTexSubImage =
        (PFNGLINVALIDATETEXSUBIMAGEPROC)gl__GetProc("glInvalidateTexSubImage");
    gl->InvalidateTexImage = (PFNGLINVALIDATETEXIMAGEPROC)gl__GetProc("glInvalidateTexImage");
    gl->InvalidateBufferSubData =
        (PFNGLINVALIDATEBUFFERSUBDATAPROC)gl__GetProc("glInvalidateBufferSubData");
    gl->InvalidateBufferData = (PFNGLINVALIDATEBUFFERDATAPROC)gl__GetProc("glInvalidateBufferData");
    gl->InvalidateFramebuffer =
        (PFNGLINVALIDATEFRAMEBUFFERPROC)gl__GetProc("glInvalidateFramebuffer");
    gl->InvalidateSubFramebuffer =
        (PFNGLINVALIDATESUBFRAMEBUFFERPROC)gl__GetProc("glInvalidateSubFramebuffer");
    gl->MultiDrawArraysIndirect =
        (PFNGLMULTIDRAWARRAYSINDIRECTPROC)gl__GetProc("glMultiDrawArraysIndirect");
    gl->MultiDrawElementsIndirect =
        (PFNGLMULTIDRAWELEMENTSINDIRECTPROC)gl__GetProc("glMultiDrawElementsIndirect");
    gl->GetProgramInterfaceiv =
        (PFNGLGETPROGRAMINTERFACEIVPROC)gl__GetProc("glGetProgramInterfaceiv");
    gl->GetProgramResourceIndex =
        (PFNGLGETPROGRAMRESOURCEINDEXPROC)gl__GetProc("glGetProgramResourceIndex");
    gl->GetProgramResourceName =
        (PFNGLGETPROGRAMRESOURCENAMEPROC)gl__GetProc("glGetProgramResourceName");
    gl->GetProgramResourceiv = (PFNGLGETPROGRAMRESOURCEIVPROC)gl__GetProc("glGetProgramResourceiv");
    gl->GetProgramResourceLocation =
        (PFNGLGETPROGRAMRESOURCELOCATIONPROC)gl__GetProc("glGetProgramResourceLocation");
    gl->GetProgramResourceLocationIndex =
        (PFNGLGETPROGRAMRESOURCELOCATIONINDEXPROC)gl__GetProc("glGetProgramResourceLocationIndex");
    gl->ShaderStorageBlockBinding =
        (PFNGLSHADERSTORAGEBLOCKBINDINGPROC)gl__GetProc("glShaderStorageBlockBinding");
    gl->TexBufferRange = (PFNGLTEXBUFFERRANGEPROC)gl__GetProc("glTexBufferRange");
    gl->TextureBufferRangeEXT =
        (PFNGLTEXTUREBUFFERRANGEEXTPROC)gl__GetProc("glTextureBufferRangeEXT");
    gl->TexStorage2DMultisample =
        (PFNGLTEXSTORAGE2DMULTISAMPLEPROC)gl__GetProc("glTexStorage2DMultisample");
    gl->TexStorage3DMultisample =
        (PFNGLTEXSTORAGE3DMULTISAMPLEPROC)gl__GetProc("glTexStorage3DMultisample");
    gl->TextureStorage2DMultisampleEXT =
        (PFNGLTEXTURESTORAGE2DMULTISAMPLEEXTPROC)gl__GetProc("glTextureStorage2DMultisampleEXT");
    gl->TextureStorage3DMultisampleEXT =
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
    void *res = wglGetProcAddress(proc);
    if (!res) res = GetProcAddress(libgl, proc);
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

#if defined(_MSC_VER) && !defined(__clang__)
#pragma warning(pop)
#endif
