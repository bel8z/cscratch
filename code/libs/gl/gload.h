#pragma once

#if defined(__cplusplus)
extern "C"
{
#endif

#if defined(_MSC_VER)
#    if defined(__clang__)
#        pragma clang diagnostic push
#        pragma clang diagnostic ignored "-Wlanguage-extension-token"
#    else
#        pragma warning(push)
#        pragma warning(disable : 5105)
#    endif
#endif

#include "glcorearb.h"

#if defined(_MSC_VER)
#    if defined(__clang__)
#        pragma clang diagnostic pop
#    else
#        pragma warning(pop)
#    endif
#endif

#ifndef __gl_h_
#    define __gl_h_
#endif

    //-----------
    // GLoad API
    //-----------

    typedef struct GlApi GlApi;

    extern GlApi *gl;

    int gloadInit(GlApi *api);
    int gloadIsSupported(int major, int minor);
    void *gloadGetProc(const char *proc);

    //------------
    // OpenGL API
    //------------

#define glCullFace gl->CullFace
#define glFrontFace gl->FrontFace
#define glHint gl->Hint
#define glLineWidth gl->LineWidth
#define glPointSize gl->PointSize
#define glPolygonMode gl->PolygonMode
#define glScissor gl->Scissor
#define glTexParameterf gl->TexParameterf
#define glTexParameterfv gl->TexParameterfv
#define glTexParameteri gl->TexParameteri
#define glTexParameteriv gl->TexParameteriv
#define glTexImage1D gl->TexImage1D
#define glTexImage2D gl->TexImage2D
#define glDrawBuffer gl->DrawBuffer
#define glClear gl->Clear
#define glClearColor gl->ClearColor
#define glClearStencil gl->ClearStencil
#define glClearDepth gl->ClearDepth
#define glStencilMask gl->StencilMask
#define glColorMask gl->ColorMask
#define glDepthMask gl->DepthMask
#define glDisable gl->Disable
#define glEnable gl->Enable
#define glFinish gl->Finish
#define glFlush gl->Flush
#define glBlendFunc gl->BlendFunc
#define glLogicOp gl->LogicOp
#define glStencilFunc gl->StencilFunc
#define glStencilOp gl->StencilOp
#define glDepthFunc gl->DepthFunc
#define glPixelStoref gl->PixelStoref
#define glPixelStorei gl->PixelStorei
#define glReadBuffer gl->ReadBuffer
#define glReadPixels gl->ReadPixels
#define glGetBooleanv gl->GetBooleanv
#define glGetDoublev gl->GetDoublev
#define glGetError gl->GetError
#define glGetFloatv gl->GetFloatv
#define glGetIntegerv gl->GetIntegerv
#define glGetString gl->GetString
#define glGetTexImage gl->GetTexImage
#define glGetTexParameterfv gl->GetTexParameterfv
#define glGetTexParameteriv gl->GetTexParameteriv
#define glGetTexLevelParameterfv gl->GetTexLevelParameterfv
#define glGetTexLevelParameteriv gl->GetTexLevelParameteriv
#define glIsEnabled gl->IsEnabled
#define glDepthRange gl->DepthRange
#define glViewport gl->Viewport
#define glDrawArrays gl->DrawArrays
#define glDrawElements gl->DrawElements
#define glGetPointerv gl->GetPointerv
#define glPolygonOffset gl->PolygonOffset
#define glCopyTexImage1D gl->CopyTexImage1D
#define glCopyTexImage2D gl->CopyTexImage2D
#define glCopyTexSubImage1D gl->CopyTexSubImage1D
#define glCopyTexSubImage2D gl->CopyTexSubImage2D
#define glTexSubImage1D gl->TexSubImage1D
#define glTexSubImage2D gl->TexSubImage2D
#define glBindTexture gl->BindTexture
#define glDeleteTextures gl->DeleteTextures
#define glGenTextures gl->GenTextures
#define glIsTexture gl->IsTexture
#define glBlendColor gl->BlendColor
#define glBlendEquation gl->BlendEquation
#define glDrawRangeElements gl->DrawRangeElements
#define glTexImage3D gl->TexImage3D
#define glTexSubImage3D gl->TexSubImage3D
#define glCopyTexSubImage3D gl->CopyTexSubImage3D
#define glActiveTexture gl->ActiveTexture
#define glSampleCoverage gl->SampleCoverage
#define glCompressedTexImage3D gl->CompressedTexImage3D
#define glCompressedTexImage2D gl->CompressedTexImage2D
#define glCompressedTexImage1D gl->CompressedTexImage1D
#define glCompressedTexSubImage3D gl->CompressedTexSubImage3D
#define glCompressedTexSubImage2D gl->CompressedTexSubImage2D
#define glCompressedTexSubImage1D gl->CompressedTexSubImage1D
#define glGetCompressedTexImage gl->GetCompressedTexImage
#define glBlendFuncSeparate gl->BlendFuncSeparate
#define glMultiDrawArrays gl->MultiDrawArrays
#define glMultiDrawElements gl->MultiDrawElements
#define glPointParameterf gl->PointParameterf
#define glPointParameterfv gl->PointParameterfv
#define glPointParameteri gl->PointParameteri
#define glPointParameteriv gl->PointParameteriv
#define glGenQueries gl->GenQueries
#define glDeleteQueries gl->DeleteQueries
#define glIsQuery gl->IsQuery
#define glBeginQuery gl->BeginQuery
#define glEndQuery gl->EndQuery
#define glGetQueryiv gl->GetQueryiv
#define glGetQueryObjectiv gl->GetQueryObjectiv
#define glGetQueryObjectuiv gl->GetQueryObjectuiv
#define glBindBuffer gl->BindBuffer
#define glDeleteBuffers gl->DeleteBuffers
#define glGenBuffers gl->GenBuffers
#define glIsBuffer gl->IsBuffer
#define glBufferData gl->BufferData
#define glBufferSubData gl->BufferSubData
#define glGetBufferSubData gl->GetBufferSubData
#define glMapBuffer gl->MapBuffer
#define glUnmapBuffer gl->UnmapBuffer
#define glGetBufferParameteriv gl->GetBufferParameteriv
#define glGetBufferPointerv gl->GetBufferPointerv
#define glBlendEquationSeparate gl->BlendEquationSeparate
#define glDrawBuffers gl->DrawBuffers
#define glStencilOpSeparate gl->StencilOpSeparate
#define glStencilFuncSeparate gl->StencilFuncSeparate
#define glStencilMaskSeparate gl->StencilMaskSeparate
#define glAttachShader gl->AttachShader
#define glBindAttribLocation gl->BindAttribLocation
#define glCompileShader gl->CompileShader
#define glCreateProgram gl->CreateProgram
#define glCreateShader gl->CreateShader
#define glDeleteProgram gl->DeleteProgram
#define glDeleteShader gl->DeleteShader
#define glDetachShader gl->DetachShader
#define glDisableVertexAttribArray gl->DisableVertexAttribArray
#define glEnableVertexAttribArray gl->EnableVertexAttribArray
#define glGetActiveAttrib gl->GetActiveAttrib
#define glGetActiveUniform gl->GetActiveUniform
#define glGetAttachedShaders gl->GetAttachedShaders
#define glGetAttribLocation gl->GetAttribLocation
#define glGetProgramiv gl->GetProgramiv
#define glGetProgramInfoLog gl->GetProgramInfoLog
#define glGetShaderiv gl->GetShaderiv
#define glGetShaderInfoLog gl->GetShaderInfoLog
#define glGetShaderSource gl->GetShaderSource
#define glGetUniformLocation gl->GetUniformLocation
#define glGetUniformfv gl->GetUniformfv
#define glGetUniformiv gl->GetUniformiv
#define glGetVertexAttribdv gl->GetVertexAttribdv
#define glGetVertexAttribfv gl->GetVertexAttribfv
#define glGetVertexAttribiv gl->GetVertexAttribiv
#define glGetVertexAttribPointerv gl->GetVertexAttribPointerv
#define glIsProgram gl->IsProgram
#define glIsShader gl->IsShader
#define glLinkProgram gl->LinkProgram
#define glShaderSource gl->ShaderSource
#define glUseProgram gl->UseProgram
#define glUniform1f gl->Uniform1f
#define glUniform2f gl->Uniform2f
#define glUniform3f gl->Uniform3f
#define glUniform4f gl->Uniform4f
#define glUniform1i gl->Uniform1i
#define glUniform2i gl->Uniform2i
#define glUniform3i gl->Uniform3i
#define glUniform4i gl->Uniform4i
#define glUniform1fv gl->Uniform1fv
#define glUniform2fv gl->Uniform2fv
#define glUniform3fv gl->Uniform3fv
#define glUniform4fv gl->Uniform4fv
#define glUniform1iv gl->Uniform1iv
#define glUniform2iv gl->Uniform2iv
#define glUniform3iv gl->Uniform3iv
#define glUniform4iv gl->Uniform4iv
#define glUniformMatrix2fv gl->UniformMatrix2fv
#define glUniformMatrix3fv gl->UniformMatrix3fv
#define glUniformMatrix4fv gl->UniformMatrix4fv
#define glValidateProgram gl->ValidateProgram
#define glVertexAttrib1d gl->VertexAttrib1d
#define glVertexAttrib1dv gl->VertexAttrib1dv
#define glVertexAttrib1f gl->VertexAttrib1f
#define glVertexAttrib1fv gl->VertexAttrib1fv
#define glVertexAttrib1s gl->VertexAttrib1s
#define glVertexAttrib1sv gl->VertexAttrib1sv
#define glVertexAttrib2d gl->VertexAttrib2d
#define glVertexAttrib2dv gl->VertexAttrib2dv
#define glVertexAttrib2f gl->VertexAttrib2f
#define glVertexAttrib2fv gl->VertexAttrib2fv
#define glVertexAttrib2s gl->VertexAttrib2s
#define glVertexAttrib2sv gl->VertexAttrib2sv
#define glVertexAttrib3d gl->VertexAttrib3d
#define glVertexAttrib3dv gl->VertexAttrib3dv
#define glVertexAttrib3f gl->VertexAttrib3f
#define glVertexAttrib3fv gl->VertexAttrib3fv
#define glVertexAttrib3s gl->VertexAttrib3s
#define glVertexAttrib3sv gl->VertexAttrib3sv
#define glVertexAttrib4Nbv gl->VertexAttrib4Nbv
#define glVertexAttrib4Niv gl->VertexAttrib4Niv
#define glVertexAttrib4Nsv gl->VertexAttrib4Nsv
#define glVertexAttrib4Nub gl->VertexAttrib4Nub
#define glVertexAttrib4Nubv gl->VertexAttrib4Nubv
#define glVertexAttrib4Nuiv gl->VertexAttrib4Nuiv
#define glVertexAttrib4Nusv gl->VertexAttrib4Nusv
#define glVertexAttrib4bv gl->VertexAttrib4bv
#define glVertexAttrib4d gl->VertexAttrib4d
#define glVertexAttrib4dv gl->VertexAttrib4dv
#define glVertexAttrib4f gl->VertexAttrib4f
#define glVertexAttrib4fv gl->VertexAttrib4fv
#define glVertexAttrib4iv gl->VertexAttrib4iv
#define glVertexAttrib4s gl->VertexAttrib4s
#define glVertexAttrib4sv gl->VertexAttrib4sv
#define glVertexAttrib4ubv gl->VertexAttrib4ubv
#define glVertexAttrib4uiv gl->VertexAttrib4uiv
#define glVertexAttrib4usv gl->VertexAttrib4usv
#define glVertexAttribPointer gl->VertexAttribPointer
#define glUniformMatrix2x3fv gl->UniformMatrix2x3fv
#define glUniformMatrix3x2fv gl->UniformMatrix3x2fv
#define glUniformMatrix2x4fv gl->UniformMatrix2x4fv
#define glUniformMatrix4x2fv gl->UniformMatrix4x2fv
#define glUniformMatrix3x4fv gl->UniformMatrix3x4fv
#define glUniformMatrix4x3fv gl->UniformMatrix4x3fv
#define glColorMaski gl->ColorMaski
#define glGetBooleani_v gl->GetBooleani_v
#define glGetIntegeri_v gl->GetIntegeri_v
#define glEnablei gl->Enablei
#define glDisablei gl->Disablei
#define glIsEnabledi gl->IsEnabledi
#define glBeginTransformFeedback gl->BeginTransformFeedback
#define glEndTransformFeedback gl->EndTransformFeedback
#define glBindBufferRange gl->BindBufferRange
#define glBindBufferBase gl->BindBufferBase
#define glTransformFeedbackVaryings gl->TransformFeedbackVaryings
#define glGetTransformFeedbackVarying gl->GetTransformFeedbackVarying
#define glClampColor gl->ClampColor
#define glBeginConditionalRender gl->BeginConditionalRender
#define glEndConditionalRender gl->EndConditionalRender
#define glVertexAttribIPointer gl->VertexAttribIPointer
#define glGetVertexAttribIiv gl->GetVertexAttribIiv
#define glGetVertexAttribIuiv gl->GetVertexAttribIuiv
#define glVertexAttribI1i gl->VertexAttribI1i
#define glVertexAttribI2i gl->VertexAttribI2i
#define glVertexAttribI3i gl->VertexAttribI3i
#define glVertexAttribI4i gl->VertexAttribI4i
#define glVertexAttribI1ui gl->VertexAttribI1ui
#define glVertexAttribI2ui gl->VertexAttribI2ui
#define glVertexAttribI3ui gl->VertexAttribI3ui
#define glVertexAttribI4ui gl->VertexAttribI4ui
#define glVertexAttribI1iv gl->VertexAttribI1iv
#define glVertexAttribI2iv gl->VertexAttribI2iv
#define glVertexAttribI3iv gl->VertexAttribI3iv
#define glVertexAttribI4iv gl->VertexAttribI4iv
#define glVertexAttribI1uiv gl->VertexAttribI1uiv
#define glVertexAttribI2uiv gl->VertexAttribI2uiv
#define glVertexAttribI3uiv gl->VertexAttribI3uiv
#define glVertexAttribI4uiv gl->VertexAttribI4uiv
#define glVertexAttribI4bv gl->VertexAttribI4bv
#define glVertexAttribI4sv gl->VertexAttribI4sv
#define glVertexAttribI4ubv gl->VertexAttribI4ubv
#define glVertexAttribI4usv gl->VertexAttribI4usv
#define glGetUniformuiv gl->GetUniformuiv
#define glBindFragDataLocation gl->BindFragDataLocation
#define glGetFragDataLocation gl->GetFragDataLocation
#define glUniform1ui gl->Uniform1ui
#define glUniform2ui gl->Uniform2ui
#define glUniform3ui gl->Uniform3ui
#define glUniform4ui gl->Uniform4ui
#define glUniform1uiv gl->Uniform1uiv
#define glUniform2uiv gl->Uniform2uiv
#define glUniform3uiv gl->Uniform3uiv
#define glUniform4uiv gl->Uniform4uiv
#define glTexParameterIiv gl->TexParameterIiv
#define glTexParameterIuiv gl->TexParameterIuiv
#define glGetTexParameterIiv gl->GetTexParameterIiv
#define glGetTexParameterIuiv gl->GetTexParameterIuiv
#define glClearBufferiv gl->ClearBufferiv
#define glClearBufferuiv gl->ClearBufferuiv
#define glClearBufferfv gl->ClearBufferfv
#define glClearBufferfi gl->ClearBufferfi
#define glGetStringi gl->GetStringi
#define glDrawArraysInstanced gl->DrawArraysInstanced
#define glDrawElementsInstanced gl->DrawElementsInstanced
#define glTexBuffer gl->TexBuffer
#define glPrimitiveRestartIndex gl->PrimitiveRestartIndex
#define glGetInteger64i_v gl->GetInteger64i_v
#define glGetBufferParameteri64v gl->GetBufferParameteri64v
#define glFramebufferTexture gl->FramebufferTexture
#define glVertexAttribDivisor gl->VertexAttribDivisor
#define glMinSampleShading gl->MinSampleShading
#define glBlendEquationi gl->BlendEquationi
#define glBlendEquationSeparatei gl->BlendEquationSeparatei
#define glBlendFunci gl->BlendFunci
#define glBlendFuncSeparatei gl->BlendFuncSeparatei
#define glIsRenderbuffer gl->IsRenderbuffer
#define glBindRenderbuffer gl->BindRenderbuffer
#define glDeleteRenderbuffers gl->DeleteRenderbuffers
#define glGenRenderbuffers gl->GenRenderbuffers
#define glRenderbufferStorage gl->RenderbufferStorage
#define glGetRenderbufferParameteriv gl->GetRenderbufferParameteriv
#define glIsFramebuffer gl->IsFramebuffer
#define glBindFramebuffer gl->BindFramebuffer
#define glDeleteFramebuffers gl->DeleteFramebuffers
#define glGenFramebuffers gl->GenFramebuffers
#define glCheckFramebufferStatus gl->CheckFramebufferStatus
#define glFramebufferTexture1D gl->FramebufferTexture1D
#define glFramebufferTexture2D gl->FramebufferTexture2D
#define glFramebufferTexture3D gl->FramebufferTexture3D
#define glFramebufferRenderbuffer gl->FramebufferRenderbuffer
#define glGetFramebufferAttachmentParameteriv gl->GetFramebufferAttachmentParameteriv
#define glGenerateMipmap gl->GenerateMipmap
#define glBlitFramebuffer gl->BlitFramebuffer
#define glRenderbufferStorageMultisample gl->RenderbufferStorageMultisample
#define glFramebufferTextureLayer gl->FramebufferTextureLayer
#define glMapBufferRange gl->MapBufferRange
#define glFlushMappedBufferRange gl->FlushMappedBufferRange
#define glBindVertexArray gl->BindVertexArray
#define glDeleteVertexArrays gl->DeleteVertexArrays
#define glGenVertexArrays gl->GenVertexArrays
#define glIsVertexArray gl->IsVertexArray
#define glGetUniformIndices gl->GetUniformIndices
#define glGetActiveUniformsiv gl->GetActiveUniformsiv
#define glGetActiveUniformName gl->GetActiveUniformName
#define glGetUniformBlockIndex gl->GetUniformBlockIndex
#define glGetActiveUniformBlockiv gl->GetActiveUniformBlockiv
#define glGetActiveUniformBlockName gl->GetActiveUniformBlockName
#define glUniformBlockBinding gl->UniformBlockBinding
#define glCopyBufferSubData gl->CopyBufferSubData
#define glDrawElementsBaseVertex gl->DrawElementsBaseVertex
#define glDrawRangeElementsBaseVertex gl->DrawRangeElementsBaseVertex
#define glDrawElementsInstancedBaseVertex gl->DrawElementsInstancedBaseVertex
#define glMultiDrawElementsBaseVertex gl->MultiDrawElementsBaseVertex
#define glProvokingVertex gl->ProvokingVertex
#define glFenceSync gl->FenceSync
#define glIsSync gl->IsSync
#define glDeleteSync gl->DeleteSync
#define glClientWaitSync gl->ClientWaitSync
#define glWaitSync gl->WaitSync
#define glGetInteger64v gl->GetInteger64v
#define glGetSynciv gl->GetSynciv
#define glTexImage2DMultisample gl->TexImage2DMultisample
#define glTexImage3DMultisample gl->TexImage3DMultisample
#define glGetMultisamplefv gl->GetMultisamplefv
#define glSampleMaski gl->SampleMaski
#define glBlendEquationiARB gl->BlendEquationiARB
#define glBlendEquationSeparateiARB gl->BlendEquationSeparateiARB
#define glBlendFunciARB gl->BlendFunciARB
#define glBlendFuncSeparateiARB gl->BlendFuncSeparateiARB
#define glMinSampleShadingARB gl->MinSampleShadingARB
#define glNamedStringARB gl->NamedStringARB
#define glDeleteNamedStringARB gl->DeleteNamedStringARB
#define glCompileShaderIncludeARB gl->CompileShaderIncludeARB
#define glIsNamedStringARB gl->IsNamedStringARB
#define glGetNamedStringARB gl->GetNamedStringARB
#define glGetNamedStringivARB gl->GetNamedStringivARB
#define glBindFragDataLocationIndexed gl->BindFragDataLocationIndexed
#define glGetFragDataIndex gl->GetFragDataIndex
#define glGenSamplers gl->GenSamplers
#define glDeleteSamplers gl->DeleteSamplers
#define glIsSampler gl->IsSampler
#define glBindSampler gl->BindSampler
#define glSamplerParameteri gl->SamplerParameteri
#define glSamplerParameteriv gl->SamplerParameteriv
#define glSamplerParameterf gl->SamplerParameterf
#define glSamplerParameterfv gl->SamplerParameterfv
#define glSamplerParameterIiv gl->SamplerParameterIiv
#define glSamplerParameterIuiv gl->SamplerParameterIuiv
#define glGetSamplerParameteriv gl->GetSamplerParameteriv
#define glGetSamplerParameterIiv gl->GetSamplerParameterIiv
#define glGetSamplerParameterfv gl->GetSamplerParameterfv
#define glGetSamplerParameterIuiv gl->GetSamplerParameterIuiv
#define glQueryCounter gl->QueryCounter
#define glGetQueryObjecti64v gl->GetQueryObjecti64v
#define glGetQueryObjectui64v gl->GetQueryObjectui64v
#define glVertexP2ui gl->VertexP2ui
#define glVertexP2uiv gl->VertexP2uiv
#define glVertexP3ui gl->VertexP3ui
#define glVertexP3uiv gl->VertexP3uiv
#define glVertexP4ui gl->VertexP4ui
#define glVertexP4uiv gl->VertexP4uiv
#define glTexCoordP1ui gl->TexCoordP1ui
#define glTexCoordP1uiv gl->TexCoordP1uiv
#define glTexCoordP2ui gl->TexCoordP2ui
#define glTexCoordP2uiv gl->TexCoordP2uiv
#define glTexCoordP3ui gl->TexCoordP3ui
#define glTexCoordP3uiv gl->TexCoordP3uiv
#define glTexCoordP4ui gl->TexCoordP4ui
#define glTexCoordP4uiv gl->TexCoordP4uiv
#define glMultiTexCoordP1ui gl->MultiTexCoordP1ui
#define glMultiTexCoordP1uiv gl->MultiTexCoordP1uiv
#define glMultiTexCoordP2ui gl->MultiTexCoordP2ui
#define glMultiTexCoordP2uiv gl->MultiTexCoordP2uiv
#define glMultiTexCoordP3ui gl->MultiTexCoordP3ui
#define glMultiTexCoordP3uiv gl->MultiTexCoordP3uiv
#define glMultiTexCoordP4ui gl->MultiTexCoordP4ui
#define glMultiTexCoordP4uiv gl->MultiTexCoordP4uiv
#define glNormalP3ui gl->NormalP3ui
#define glNormalP3uiv gl->NormalP3uiv
#define glColorP3ui gl->ColorP3ui
#define glColorP3uiv gl->ColorP3uiv
#define glColorP4ui gl->ColorP4ui
#define glColorP4uiv gl->ColorP4uiv
#define glSecondaryColorP3ui gl->SecondaryColorP3ui
#define glSecondaryColorP3uiv gl->SecondaryColorP3uiv
#define glVertexAttribP1ui gl->VertexAttribP1ui
#define glVertexAttribP1uiv gl->VertexAttribP1uiv
#define glVertexAttribP2ui gl->VertexAttribP2ui
#define glVertexAttribP2uiv gl->VertexAttribP2uiv
#define glVertexAttribP3ui gl->VertexAttribP3ui
#define glVertexAttribP3uiv gl->VertexAttribP3uiv
#define glVertexAttribP4ui gl->VertexAttribP4ui
#define glVertexAttribP4uiv gl->VertexAttribP4uiv
#define glDrawArraysIndirect gl->DrawArraysIndirect
#define glDrawElementsIndirect gl->DrawElementsIndirect
#define glUniform1d gl->Uniform1d
#define glUniform2d gl->Uniform2d
#define glUniform3d gl->Uniform3d
#define glUniform4d gl->Uniform4d
#define glUniform1dv gl->Uniform1dv
#define glUniform2dv gl->Uniform2dv
#define glUniform3dv gl->Uniform3dv
#define glUniform4dv gl->Uniform4dv
#define glUniformMatrix2dv gl->UniformMatrix2dv
#define glUniformMatrix3dv gl->UniformMatrix3dv
#define glUniformMatrix4dv gl->UniformMatrix4dv
#define glUniformMatrix2x3dv gl->UniformMatrix2x3dv
#define glUniformMatrix2x4dv gl->UniformMatrix2x4dv
#define glUniformMatrix3x2dv gl->UniformMatrix3x2dv
#define glUniformMatrix3x4dv gl->UniformMatrix3x4dv
#define glUniformMatrix4x2dv gl->UniformMatrix4x2dv
#define glUniformMatrix4x3dv gl->UniformMatrix4x3dv
#define glGetUniformdv gl->GetUniformdv
#define glGetSubroutineUniformLocation gl->GetSubroutineUniformLocation
#define glGetSubroutineIndex gl->GetSubroutineIndex
#define glGetActiveSubroutineUniformiv gl->GetActiveSubroutineUniformiv
#define glGetActiveSubroutineUniformName gl->GetActiveSubroutineUniformName
#define glGetActiveSubroutineName gl->GetActiveSubroutineName
#define glUniformSubroutinesuiv gl->UniformSubroutinesuiv
#define glGetUniformSubroutineuiv gl->GetUniformSubroutineuiv
#define glGetProgramStageiv gl->GetProgramStageiv
#define glPatchParameteri gl->PatchParameteri
#define glPatchParameterfv gl->PatchParameterfv
#define glBindTransformFeedback gl->BindTransformFeedback
#define glDeleteTransformFeedbacks gl->DeleteTransformFeedbacks
#define glGenTransformFeedbacks gl->GenTransformFeedbacks
#define glIsTransformFeedback gl->IsTransformFeedback
#define glPauseTransformFeedback gl->PauseTransformFeedback
#define glResumeTransformFeedback gl->ResumeTransformFeedback
#define glDrawTransformFeedback gl->DrawTransformFeedback
#define glDrawTransformFeedbackStream gl->DrawTransformFeedbackStream
#define glBeginQueryIndexed gl->BeginQueryIndexed
#define glEndQueryIndexed gl->EndQueryIndexed
#define glGetQueryIndexediv gl->GetQueryIndexediv
#define glReleaseShaderCompiler gl->ReleaseShaderCompiler
#define glShaderBinary gl->ShaderBinary
#define glGetShaderPrecisionFormat gl->GetShaderPrecisionFormat
#define glDepthRangef gl->DepthRangef
#define glClearDepthf gl->ClearDepthf
#define glGetProgramBinary gl->GetProgramBinary
#define glProgramBinary gl->ProgramBinary
#define glProgramParameteri gl->ProgramParameteri
#define glUseProgramStages gl->UseProgramStages
#define glActiveShaderProgram gl->ActiveShaderProgram
#define glCreateShaderProgramv gl->CreateShaderProgramv
#define glBindProgramPipeline gl->BindProgramPipeline
#define glDeleteProgramPipelines gl->DeleteProgramPipelines
#define glGenProgramPipelines gl->GenProgramPipelines
#define glIsProgramPipeline gl->IsProgramPipeline
#define glGetProgramPipelineiv gl->GetProgramPipelineiv
#define glProgramUniform1i gl->ProgramUniform1i
#define glProgramUniform1iv gl->ProgramUniform1iv
#define glProgramUniform1f gl->ProgramUniform1f
#define glProgramUniform1fv gl->ProgramUniform1fv
#define glProgramUniform1d gl->ProgramUniform1d
#define glProgramUniform1dv gl->ProgramUniform1dv
#define glProgramUniform1ui gl->ProgramUniform1ui
#define glProgramUniform1uiv gl->ProgramUniform1uiv
#define glProgramUniform2i gl->ProgramUniform2i
#define glProgramUniform2iv gl->ProgramUniform2iv
#define glProgramUniform2f gl->ProgramUniform2f
#define glProgramUniform2fv gl->ProgramUniform2fv
#define glProgramUniform2d gl->ProgramUniform2d
#define glProgramUniform2dv gl->ProgramUniform2dv
#define glProgramUniform2ui gl->ProgramUniform2ui
#define glProgramUniform2uiv gl->ProgramUniform2uiv
#define glProgramUniform3i gl->ProgramUniform3i
#define glProgramUniform3iv gl->ProgramUniform3iv
#define glProgramUniform3f gl->ProgramUniform3f
#define glProgramUniform3fv gl->ProgramUniform3fv
#define glProgramUniform3d gl->ProgramUniform3d
#define glProgramUniform3dv gl->ProgramUniform3dv
#define glProgramUniform3ui gl->ProgramUniform3ui
#define glProgramUniform3uiv gl->ProgramUniform3uiv
#define glProgramUniform4i gl->ProgramUniform4i
#define glProgramUniform4iv gl->ProgramUniform4iv
#define glProgramUniform4f gl->ProgramUniform4f
#define glProgramUniform4fv gl->ProgramUniform4fv
#define glProgramUniform4d gl->ProgramUniform4d
#define glProgramUniform4dv gl->ProgramUniform4dv
#define glProgramUniform4ui gl->ProgramUniform4ui
#define glProgramUniform4uiv gl->ProgramUniform4uiv
#define glProgramUniformMatrix2fv gl->ProgramUniformMatrix2fv
#define glProgramUniformMatrix3fv gl->ProgramUniformMatrix3fv
#define glProgramUniformMatrix4fv gl->ProgramUniformMatrix4fv
#define glProgramUniformMatrix2dv gl->ProgramUniformMatrix2dv
#define glProgramUniformMatrix3dv gl->ProgramUniformMatrix3dv
#define glProgramUniformMatrix4dv gl->ProgramUniformMatrix4dv
#define glProgramUniformMatrix2x3fv gl->ProgramUniformMatrix2x3fv
#define glProgramUniformMatrix3x2fv gl->ProgramUniformMatrix3x2fv
#define glProgramUniformMatrix2x4fv gl->ProgramUniformMatrix2x4fv
#define glProgramUniformMatrix4x2fv gl->ProgramUniformMatrix4x2fv
#define glProgramUniformMatrix3x4fv gl->ProgramUniformMatrix3x4fv
#define glProgramUniformMatrix4x3fv gl->ProgramUniformMatrix4x3fv
#define glProgramUniformMatrix2x3dv gl->ProgramUniformMatrix2x3dv
#define glProgramUniformMatrix3x2dv gl->ProgramUniformMatrix3x2dv
#define glProgramUniformMatrix2x4dv gl->ProgramUniformMatrix2x4dv
#define glProgramUniformMatrix4x2dv gl->ProgramUniformMatrix4x2dv
#define glProgramUniformMatrix3x4dv gl->ProgramUniformMatrix3x4dv
#define glProgramUniformMatrix4x3dv gl->ProgramUniformMatrix4x3dv
#define glValidateProgramPipeline gl->ValidateProgramPipeline
#define glGetProgramPipelineInfoLog gl->GetProgramPipelineInfoLog
#define glVertexAttribL1d gl->VertexAttribL1d
#define glVertexAttribL2d gl->VertexAttribL2d
#define glVertexAttribL3d gl->VertexAttribL3d
#define glVertexAttribL4d gl->VertexAttribL4d
#define glVertexAttribL1dv gl->VertexAttribL1dv
#define glVertexAttribL2dv gl->VertexAttribL2dv
#define glVertexAttribL3dv gl->VertexAttribL3dv
#define glVertexAttribL4dv gl->VertexAttribL4dv
#define glVertexAttribLPointer gl->VertexAttribLPointer
#define glGetVertexAttribLdv gl->GetVertexAttribLdv
#define glViewportArrayv gl->ViewportArrayv
#define glViewportIndexedf gl->ViewportIndexedf
#define glViewportIndexedfv gl->ViewportIndexedfv
#define glScissorArrayv gl->ScissorArrayv
#define glScissorIndexed gl->ScissorIndexed
#define glScissorIndexedv gl->ScissorIndexedv
#define glDepthRangeArrayv gl->DepthRangeArrayv
#define glDepthRangeIndexed gl->DepthRangeIndexed
#define glGetFloati_v gl->GetFloati_v
#define glGetDoublei_v gl->GetDoublei_v
#define glCreateSyncFromCLeventARB gl->CreateSyncFromCLeventARB
#define glDebugMessageControlARB gl->DebugMessageControlARB
#define glDebugMessageInsertARB gl->DebugMessageInsertARB
#define glDebugMessageCallbackARB gl->DebugMessageCallbackARB
#define glGetDebugMessageLogARB gl->GetDebugMessageLogARB
#define glGetGraphicsResetStatusARB gl->GetGraphicsResetStatusARB
#define glGetnTexImageARB gl->GetnTexImageARB
#define glReadnPixelsARB gl->ReadnPixelsARB
#define glGetnCompressedTexImageARB gl->GetnCompressedTexImageARB
#define glGetnUniformfvARB gl->GetnUniformfvARB
#define glGetnUniformivARB gl->GetnUniformivARB
#define glGetnUniformuivARB gl->GetnUniformuivARB
#define glGetnUniformdvARB gl->GetnUniformdvARB
#define glDrawArraysInstancedBaseInstance gl->DrawArraysInstancedBaseInstance
#define glDrawElementsInstancedBaseInstance gl->DrawElementsInstancedBaseInstance
#define glDrawElementsInstancedBaseVertexBaseInstance \
    gl->DrawElementsInstancedBaseVertexBaseInstance
#define glDrawTransformFeedbackInstanced gl->DrawTransformFeedbackInstanced
#define glDrawTransformFeedbackStreamInstanced gl->DrawTransformFeedbackStreamInstanced
#define glGetInternalformativ gl->GetInternalformativ
#define glGetActiveAtomicCounterBufferiv gl->GetActiveAtomicCounterBufferiv
#define glBindImageTexture gl->BindImageTexture
#define glMemoryBarrier gl->MemoryBarrier
#define glTexStorage1D gl->TexStorage1D
#define glTexStorage2D gl->TexStorage2D
#define glTexStorage3D gl->TexStorage3D
#define glTextureStorage1DEXT gl->TextureStorage1DEXT
#define glTextureStorage2DEXT gl->TextureStorage2DEXT
#define glTextureStorage3DEXT gl->TextureStorage3DEXT
#define glDebugMessageControl gl->DebugMessageControl
#define glDebugMessageInsert gl->DebugMessageInsert
#define glDebugMessageCallback gl->DebugMessageCallback
#define glGetDebugMessageLog gl->GetDebugMessageLog
#define glPushDebugGroup gl->PushDebugGroup
#define glPopDebugGroup gl->PopDebugGroup
#define glObjectLabel gl->ObjectLabel
#define glGetObjectLabel gl->GetObjectLabel
#define glObjectPtrLabel gl->ObjectPtrLabel
#define glGetObjectPtrLabel gl->GetObjectPtrLabel
#define glClearBufferData gl->ClearBufferData
#define glClearBufferSubData gl->ClearBufferSubData
#define glClearNamedBufferDataEXT gl->ClearNamedBufferDataEXT
#define glClearNamedBufferSubDataEXT gl->ClearNamedBufferSubDataEXT
#define glDispatchCompute gl->DispatchCompute
#define glDispatchComputeIndirect gl->DispatchComputeIndirect
#define glCopyImageSubData gl->CopyImageSubData
#define glTextureView gl->TextureView
#define glBindVertexBuffer gl->BindVertexBuffer
#define glVertexAttribFormat gl->VertexAttribFormat
#define glVertexAttribIFormat gl->VertexAttribIFormat
#define glVertexAttribLFormat gl->VertexAttribLFormat
#define glVertexAttribBinding gl->VertexAttribBinding
#define glVertexBindingDivisor gl->VertexBindingDivisor
#define glVertexArrayBindVertexBufferEXT gl->VertexArrayBindVertexBufferEXT
#define glVertexArrayVertexAttribFormatEXT gl->VertexArrayVertexAttribFormatEXT
#define glVertexArrayVertexAttribIFormatEXT gl->VertexArrayVertexAttribIFormatEXT
#define glVertexArrayVertexAttribLFormatEXT gl->VertexArrayVertexAttribLFormatEXT
#define glVertexArrayVertexAttribBindingEXT gl->VertexArrayVertexAttribBindingEXT
#define glVertexArrayVertexBindingDivisorEXT gl->VertexArrayVertexBindingDivisorEXT
#define glFramebufferParameteri gl->FramebufferParameteri
#define glGetFramebufferParameteriv gl->GetFramebufferParameteriv
#define glNamedFramebufferParameteriEXT gl->NamedFramebufferParameteriEXT
#define glGetNamedFramebufferParameterivEXT gl->GetNamedFramebufferParameterivEXT
#define glGetInternalformati64v gl->GetInternalformati64v
#define glInvalidateTexSubImage gl->InvalidateTexSubImage
#define glInvalidateTexImage gl->InvalidateTexImage
#define glInvalidateBufferSubData gl->InvalidateBufferSubData
#define glInvalidateBufferData gl->InvalidateBufferData
#define glInvalidateFramebuffer gl->InvalidateFramebuffer
#define glInvalidateSubFramebuffer gl->InvalidateSubFramebuffer
#define glMultiDrawArraysIndirect gl->MultiDrawArraysIndirect
#define glMultiDrawElementsIndirect gl->MultiDrawElementsIndirect
#define glGetProgramInterfaceiv gl->GetProgramInterfaceiv
#define glGetProgramResourceIndex gl->GetProgramResourceIndex
#define glGetProgramResourceName gl->GetProgramResourceName
#define glGetProgramResourceiv gl->GetProgramResourceiv
#define glGetProgramResourceLocation gl->GetProgramResourceLocation
#define glGetProgramResourceLocationIndex gl->GetProgramResourceLocationIndex
#define glShaderStorageBlockBinding gl->ShaderStorageBlockBinding
#define glTexBufferRange gl->TexBufferRange
#define glTextureBufferRangeEXT gl->TextureBufferRangeEXT
#define glTexStorage2DMultisample gl->TexStorage2DMultisample
#define glTexStorage3DMultisample gl->TexStorage3DMultisample
#define glTextureStorage2DMultisampleEXT gl->TextureStorage2DMultisampleEXT
#define glTextureStorage3DMultisampleEXT gl->TextureStorage3DMultisampleEXT

    //-------------------------
    // OpenGL functions struct
    //-------------------------

    typedef struct GlApi
    {
        PFNGLCULLFACEPROC CullFace;
        PFNGLFRONTFACEPROC FrontFace;
        PFNGLHINTPROC Hint;
        PFNGLLINEWIDTHPROC LineWidth;
        PFNGLPOINTSIZEPROC PointSize;
        PFNGLPOLYGONMODEPROC PolygonMode;
        PFNGLSCISSORPROC Scissor;
        PFNGLTEXPARAMETERFPROC TexParameterf;
        PFNGLTEXPARAMETERFVPROC TexParameterfv;
        PFNGLTEXPARAMETERIPROC TexParameteri;
        PFNGLTEXPARAMETERIVPROC TexParameteriv;
        PFNGLTEXIMAGE1DPROC TexImage1D;
        PFNGLTEXIMAGE2DPROC TexImage2D;
        PFNGLDRAWBUFFERPROC DrawBuffer;
        PFNGLCLEARPROC Clear;
        PFNGLCLEARCOLORPROC ClearColor;
        PFNGLCLEARSTENCILPROC ClearStencil;
        PFNGLCLEARDEPTHPROC ClearDepth;
        PFNGLSTENCILMASKPROC StencilMask;
        PFNGLCOLORMASKPROC ColorMask;
        PFNGLDEPTHMASKPROC DepthMask;
        PFNGLDISABLEPROC Disable;
        PFNGLENABLEPROC Enable;
        PFNGLFINISHPROC Finish;
        PFNGLFLUSHPROC Flush;
        PFNGLBLENDFUNCPROC BlendFunc;
        PFNGLLOGICOPPROC LogicOp;
        PFNGLSTENCILFUNCPROC StencilFunc;
        PFNGLSTENCILOPPROC StencilOp;
        PFNGLDEPTHFUNCPROC DepthFunc;
        PFNGLPIXELSTOREFPROC PixelStoref;
        PFNGLPIXELSTOREIPROC PixelStorei;
        PFNGLREADBUFFERPROC ReadBuffer;
        PFNGLREADPIXELSPROC ReadPixels;
        PFNGLGETBOOLEANVPROC GetBooleanv;
        PFNGLGETDOUBLEVPROC GetDoublev;
        PFNGLGETERRORPROC GetError;
        PFNGLGETFLOATVPROC GetFloatv;
        PFNGLGETINTEGERVPROC GetIntegerv;
        PFNGLGETSTRINGPROC GetString;
        PFNGLGETTEXIMAGEPROC GetTexImage;
        PFNGLGETTEXPARAMETERFVPROC GetTexParameterfv;
        PFNGLGETTEXPARAMETERIVPROC GetTexParameteriv;
        PFNGLGETTEXLEVELPARAMETERFVPROC GetTexLevelParameterfv;
        PFNGLGETTEXLEVELPARAMETERIVPROC GetTexLevelParameteriv;
        PFNGLISENABLEDPROC IsEnabled;
        PFNGLDEPTHRANGEPROC DepthRange;
        PFNGLVIEWPORTPROC Viewport;
        PFNGLDRAWARRAYSPROC DrawArrays;
        PFNGLDRAWELEMENTSPROC DrawElements;
        PFNGLGETPOINTERVPROC GetPointerv;
        PFNGLPOLYGONOFFSETPROC PolygonOffset;
        PFNGLCOPYTEXIMAGE1DPROC CopyTexImage1D;
        PFNGLCOPYTEXIMAGE2DPROC CopyTexImage2D;
        PFNGLCOPYTEXSUBIMAGE1DPROC CopyTexSubImage1D;
        PFNGLCOPYTEXSUBIMAGE2DPROC CopyTexSubImage2D;
        PFNGLTEXSUBIMAGE1DPROC TexSubImage1D;
        PFNGLTEXSUBIMAGE2DPROC TexSubImage2D;
        PFNGLBINDTEXTUREPROC BindTexture;
        PFNGLDELETETEXTURESPROC DeleteTextures;
        PFNGLGENTEXTURESPROC GenTextures;
        PFNGLISTEXTUREPROC IsTexture;
        PFNGLBLENDCOLORPROC BlendColor;
        PFNGLBLENDEQUATIONPROC BlendEquation;
        PFNGLDRAWRANGEELEMENTSPROC DrawRangeElements;
        PFNGLTEXIMAGE3DPROC TexImage3D;
        PFNGLTEXSUBIMAGE3DPROC TexSubImage3D;
        PFNGLCOPYTEXSUBIMAGE3DPROC CopyTexSubImage3D;
        PFNGLACTIVETEXTUREPROC ActiveTexture;
        PFNGLSAMPLECOVERAGEPROC SampleCoverage;
        PFNGLCOMPRESSEDTEXIMAGE3DPROC CompressedTexImage3D;
        PFNGLCOMPRESSEDTEXIMAGE2DPROC CompressedTexImage2D;
        PFNGLCOMPRESSEDTEXIMAGE1DPROC CompressedTexImage1D;
        PFNGLCOMPRESSEDTEXSUBIMAGE3DPROC CompressedTexSubImage3D;
        PFNGLCOMPRESSEDTEXSUBIMAGE2DPROC CompressedTexSubImage2D;
        PFNGLCOMPRESSEDTEXSUBIMAGE1DPROC CompressedTexSubImage1D;
        PFNGLGETCOMPRESSEDTEXIMAGEPROC GetCompressedTexImage;
        PFNGLBLENDFUNCSEPARATEPROC BlendFuncSeparate;
        PFNGLMULTIDRAWARRAYSPROC MultiDrawArrays;
        PFNGLMULTIDRAWELEMENTSPROC MultiDrawElements;
        PFNGLPOINTPARAMETERFPROC PointParameterf;
        PFNGLPOINTPARAMETERFVPROC PointParameterfv;
        PFNGLPOINTPARAMETERIPROC PointParameteri;
        PFNGLPOINTPARAMETERIVPROC PointParameteriv;
        PFNGLGENQUERIESPROC GenQueries;
        PFNGLDELETEQUERIESPROC DeleteQueries;
        PFNGLISQUERYPROC IsQuery;
        PFNGLBEGINQUERYPROC BeginQuery;
        PFNGLENDQUERYPROC EndQuery;
        PFNGLGETQUERYIVPROC GetQueryiv;
        PFNGLGETQUERYOBJECTIVPROC GetQueryObjectiv;
        PFNGLGETQUERYOBJECTUIVPROC GetQueryObjectuiv;
        PFNGLBINDBUFFERPROC BindBuffer;
        PFNGLDELETEBUFFERSPROC DeleteBuffers;
        PFNGLGENBUFFERSPROC GenBuffers;
        PFNGLISBUFFERPROC IsBuffer;
        PFNGLBUFFERDATAPROC BufferData;
        PFNGLBUFFERSUBDATAPROC BufferSubData;
        PFNGLGETBUFFERSUBDATAPROC GetBufferSubData;
        PFNGLMAPBUFFERPROC MapBuffer;
        PFNGLUNMAPBUFFERPROC UnmapBuffer;
        PFNGLGETBUFFERPARAMETERIVPROC GetBufferParameteriv;
        PFNGLGETBUFFERPOINTERVPROC GetBufferPointerv;
        PFNGLBLENDEQUATIONSEPARATEPROC BlendEquationSeparate;
        PFNGLDRAWBUFFERSPROC DrawBuffers;
        PFNGLSTENCILOPSEPARATEPROC StencilOpSeparate;
        PFNGLSTENCILFUNCSEPARATEPROC StencilFuncSeparate;
        PFNGLSTENCILMASKSEPARATEPROC StencilMaskSeparate;
        PFNGLATTACHSHADERPROC AttachShader;
        PFNGLBINDATTRIBLOCATIONPROC BindAttribLocation;
        PFNGLCOMPILESHADERPROC CompileShader;
        PFNGLCREATEPROGRAMPROC CreateProgram;
        PFNGLCREATESHADERPROC CreateShader;
        PFNGLDELETEPROGRAMPROC DeleteProgram;
        PFNGLDELETESHADERPROC DeleteShader;
        PFNGLDETACHSHADERPROC DetachShader;
        PFNGLDISABLEVERTEXATTRIBARRAYPROC DisableVertexAttribArray;
        PFNGLENABLEVERTEXATTRIBARRAYPROC EnableVertexAttribArray;
        PFNGLGETACTIVEATTRIBPROC GetActiveAttrib;
        PFNGLGETACTIVEUNIFORMPROC GetActiveUniform;
        PFNGLGETATTACHEDSHADERSPROC GetAttachedShaders;
        PFNGLGETATTRIBLOCATIONPROC GetAttribLocation;
        PFNGLGETPROGRAMIVPROC GetProgramiv;
        PFNGLGETPROGRAMINFOLOGPROC GetProgramInfoLog;
        PFNGLGETSHADERIVPROC GetShaderiv;
        PFNGLGETSHADERINFOLOGPROC GetShaderInfoLog;
        PFNGLGETSHADERSOURCEPROC GetShaderSource;
        PFNGLGETUNIFORMLOCATIONPROC GetUniformLocation;
        PFNGLGETUNIFORMFVPROC GetUniformfv;
        PFNGLGETUNIFORMIVPROC GetUniformiv;
        PFNGLGETVERTEXATTRIBDVPROC GetVertexAttribdv;
        PFNGLGETVERTEXATTRIBFVPROC GetVertexAttribfv;
        PFNGLGETVERTEXATTRIBIVPROC GetVertexAttribiv;
        PFNGLGETVERTEXATTRIBPOINTERVPROC GetVertexAttribPointerv;
        PFNGLISPROGRAMPROC IsProgram;
        PFNGLISSHADERPROC IsShader;
        PFNGLLINKPROGRAMPROC LinkProgram;
        PFNGLSHADERSOURCEPROC ShaderSource;
        PFNGLUSEPROGRAMPROC UseProgram;
        PFNGLUNIFORM1FPROC Uniform1f;
        PFNGLUNIFORM2FPROC Uniform2f;
        PFNGLUNIFORM3FPROC Uniform3f;
        PFNGLUNIFORM4FPROC Uniform4f;
        PFNGLUNIFORM1IPROC Uniform1i;
        PFNGLUNIFORM2IPROC Uniform2i;
        PFNGLUNIFORM3IPROC Uniform3i;
        PFNGLUNIFORM4IPROC Uniform4i;
        PFNGLUNIFORM1FVPROC Uniform1fv;
        PFNGLUNIFORM2FVPROC Uniform2fv;
        PFNGLUNIFORM3FVPROC Uniform3fv;
        PFNGLUNIFORM4FVPROC Uniform4fv;
        PFNGLUNIFORM1IVPROC Uniform1iv;
        PFNGLUNIFORM2IVPROC Uniform2iv;
        PFNGLUNIFORM3IVPROC Uniform3iv;
        PFNGLUNIFORM4IVPROC Uniform4iv;
        PFNGLUNIFORMMATRIX2FVPROC UniformMatrix2fv;
        PFNGLUNIFORMMATRIX3FVPROC UniformMatrix3fv;
        PFNGLUNIFORMMATRIX4FVPROC UniformMatrix4fv;
        PFNGLVALIDATEPROGRAMPROC ValidateProgram;
        PFNGLVERTEXATTRIB1DPROC VertexAttrib1d;
        PFNGLVERTEXATTRIB1DVPROC VertexAttrib1dv;
        PFNGLVERTEXATTRIB1FPROC VertexAttrib1f;
        PFNGLVERTEXATTRIB1FVPROC VertexAttrib1fv;
        PFNGLVERTEXATTRIB1SPROC VertexAttrib1s;
        PFNGLVERTEXATTRIB1SVPROC VertexAttrib1sv;
        PFNGLVERTEXATTRIB2DPROC VertexAttrib2d;
        PFNGLVERTEXATTRIB2DVPROC VertexAttrib2dv;
        PFNGLVERTEXATTRIB2FPROC VertexAttrib2f;
        PFNGLVERTEXATTRIB2FVPROC VertexAttrib2fv;
        PFNGLVERTEXATTRIB2SPROC VertexAttrib2s;
        PFNGLVERTEXATTRIB2SVPROC VertexAttrib2sv;
        PFNGLVERTEXATTRIB3DPROC VertexAttrib3d;
        PFNGLVERTEXATTRIB3DVPROC VertexAttrib3dv;
        PFNGLVERTEXATTRIB3FPROC VertexAttrib3f;
        PFNGLVERTEXATTRIB3FVPROC VertexAttrib3fv;
        PFNGLVERTEXATTRIB3SPROC VertexAttrib3s;
        PFNGLVERTEXATTRIB3SVPROC VertexAttrib3sv;
        PFNGLVERTEXATTRIB4NBVPROC VertexAttrib4Nbv;
        PFNGLVERTEXATTRIB4NIVPROC VertexAttrib4Niv;
        PFNGLVERTEXATTRIB4NSVPROC VertexAttrib4Nsv;
        PFNGLVERTEXATTRIB4NUBPROC VertexAttrib4Nub;
        PFNGLVERTEXATTRIB4NUBVPROC VertexAttrib4Nubv;
        PFNGLVERTEXATTRIB4NUIVPROC VertexAttrib4Nuiv;
        PFNGLVERTEXATTRIB4NUSVPROC VertexAttrib4Nusv;
        PFNGLVERTEXATTRIB4BVPROC VertexAttrib4bv;
        PFNGLVERTEXATTRIB4DPROC VertexAttrib4d;
        PFNGLVERTEXATTRIB4DVPROC VertexAttrib4dv;
        PFNGLVERTEXATTRIB4FPROC VertexAttrib4f;
        PFNGLVERTEXATTRIB4FVPROC VertexAttrib4fv;
        PFNGLVERTEXATTRIB4IVPROC VertexAttrib4iv;
        PFNGLVERTEXATTRIB4SPROC VertexAttrib4s;
        PFNGLVERTEXATTRIB4SVPROC VertexAttrib4sv;
        PFNGLVERTEXATTRIB4UBVPROC VertexAttrib4ubv;
        PFNGLVERTEXATTRIB4UIVPROC VertexAttrib4uiv;
        PFNGLVERTEXATTRIB4USVPROC VertexAttrib4usv;
        PFNGLVERTEXATTRIBPOINTERPROC VertexAttribPointer;
        PFNGLUNIFORMMATRIX2X3FVPROC UniformMatrix2x3fv;
        PFNGLUNIFORMMATRIX3X2FVPROC UniformMatrix3x2fv;
        PFNGLUNIFORMMATRIX2X4FVPROC UniformMatrix2x4fv;
        PFNGLUNIFORMMATRIX4X2FVPROC UniformMatrix4x2fv;
        PFNGLUNIFORMMATRIX3X4FVPROC UniformMatrix3x4fv;
        PFNGLUNIFORMMATRIX4X3FVPROC UniformMatrix4x3fv;
        PFNGLCOLORMASKIPROC ColorMaski;
        PFNGLGETBOOLEANI_VPROC GetBooleani_v;
        PFNGLGETINTEGERI_VPROC GetIntegeri_v;
        PFNGLENABLEIPROC Enablei;
        PFNGLDISABLEIPROC Disablei;
        PFNGLISENABLEDIPROC IsEnabledi;
        PFNGLBEGINTRANSFORMFEEDBACKPROC BeginTransformFeedback;
        PFNGLENDTRANSFORMFEEDBACKPROC EndTransformFeedback;
        PFNGLBINDBUFFERRANGEPROC BindBufferRange;
        PFNGLBINDBUFFERBASEPROC BindBufferBase;
        PFNGLTRANSFORMFEEDBACKVARYINGSPROC TransformFeedbackVaryings;
        PFNGLGETTRANSFORMFEEDBACKVARYINGPROC GetTransformFeedbackVarying;
        PFNGLCLAMPCOLORPROC ClampColor;
        PFNGLBEGINCONDITIONALRENDERPROC BeginConditionalRender;
        PFNGLENDCONDITIONALRENDERPROC EndConditionalRender;
        PFNGLVERTEXATTRIBIPOINTERPROC VertexAttribIPointer;
        PFNGLGETVERTEXATTRIBIIVPROC GetVertexAttribIiv;
        PFNGLGETVERTEXATTRIBIUIVPROC GetVertexAttribIuiv;
        PFNGLVERTEXATTRIBI1IPROC VertexAttribI1i;
        PFNGLVERTEXATTRIBI2IPROC VertexAttribI2i;
        PFNGLVERTEXATTRIBI3IPROC VertexAttribI3i;
        PFNGLVERTEXATTRIBI4IPROC VertexAttribI4i;
        PFNGLVERTEXATTRIBI1UIPROC VertexAttribI1ui;
        PFNGLVERTEXATTRIBI2UIPROC VertexAttribI2ui;
        PFNGLVERTEXATTRIBI3UIPROC VertexAttribI3ui;
        PFNGLVERTEXATTRIBI4UIPROC VertexAttribI4ui;
        PFNGLVERTEXATTRIBI1IVPROC VertexAttribI1iv;
        PFNGLVERTEXATTRIBI2IVPROC VertexAttribI2iv;
        PFNGLVERTEXATTRIBI3IVPROC VertexAttribI3iv;
        PFNGLVERTEXATTRIBI4IVPROC VertexAttribI4iv;
        PFNGLVERTEXATTRIBI1UIVPROC VertexAttribI1uiv;
        PFNGLVERTEXATTRIBI2UIVPROC VertexAttribI2uiv;
        PFNGLVERTEXATTRIBI3UIVPROC VertexAttribI3uiv;
        PFNGLVERTEXATTRIBI4UIVPROC VertexAttribI4uiv;
        PFNGLVERTEXATTRIBI4BVPROC VertexAttribI4bv;
        PFNGLVERTEXATTRIBI4SVPROC VertexAttribI4sv;
        PFNGLVERTEXATTRIBI4UBVPROC VertexAttribI4ubv;
        PFNGLVERTEXATTRIBI4USVPROC VertexAttribI4usv;
        PFNGLGETUNIFORMUIVPROC GetUniformuiv;
        PFNGLBINDFRAGDATALOCATIONPROC BindFragDataLocation;
        PFNGLGETFRAGDATALOCATIONPROC GetFragDataLocation;
        PFNGLUNIFORM1UIPROC Uniform1ui;
        PFNGLUNIFORM2UIPROC Uniform2ui;
        PFNGLUNIFORM3UIPROC Uniform3ui;
        PFNGLUNIFORM4UIPROC Uniform4ui;
        PFNGLUNIFORM1UIVPROC Uniform1uiv;
        PFNGLUNIFORM2UIVPROC Uniform2uiv;
        PFNGLUNIFORM3UIVPROC Uniform3uiv;
        PFNGLUNIFORM4UIVPROC Uniform4uiv;
        PFNGLTEXPARAMETERIIVPROC TexParameterIiv;
        PFNGLTEXPARAMETERIUIVPROC TexParameterIuiv;
        PFNGLGETTEXPARAMETERIIVPROC GetTexParameterIiv;
        PFNGLGETTEXPARAMETERIUIVPROC GetTexParameterIuiv;
        PFNGLCLEARBUFFERIVPROC ClearBufferiv;
        PFNGLCLEARBUFFERUIVPROC ClearBufferuiv;
        PFNGLCLEARBUFFERFVPROC ClearBufferfv;
        PFNGLCLEARBUFFERFIPROC ClearBufferfi;
        PFNGLGETSTRINGIPROC GetStringi;
        PFNGLDRAWARRAYSINSTANCEDPROC DrawArraysInstanced;
        PFNGLDRAWELEMENTSINSTANCEDPROC DrawElementsInstanced;
        PFNGLTEXBUFFERPROC TexBuffer;
        PFNGLPRIMITIVERESTARTINDEXPROC PrimitiveRestartIndex;
        PFNGLGETINTEGER64I_VPROC GetInteger64i_v;
        PFNGLGETBUFFERPARAMETERI64VPROC GetBufferParameteri64v;
        PFNGLFRAMEBUFFERTEXTUREPROC FramebufferTexture;
        PFNGLVERTEXATTRIBDIVISORPROC VertexAttribDivisor;
        PFNGLMINSAMPLESHADINGPROC MinSampleShading;
        PFNGLBLENDEQUATIONIPROC BlendEquationi;
        PFNGLBLENDEQUATIONSEPARATEIPROC BlendEquationSeparatei;
        PFNGLBLENDFUNCIPROC BlendFunci;
        PFNGLBLENDFUNCSEPARATEIPROC BlendFuncSeparatei;
        PFNGLISRENDERBUFFERPROC IsRenderbuffer;
        PFNGLBINDRENDERBUFFERPROC BindRenderbuffer;
        PFNGLDELETERENDERBUFFERSPROC DeleteRenderbuffers;
        PFNGLGENRENDERBUFFERSPROC GenRenderbuffers;
        PFNGLRENDERBUFFERSTORAGEPROC RenderbufferStorage;
        PFNGLGETRENDERBUFFERPARAMETERIVPROC GetRenderbufferParameteriv;
        PFNGLISFRAMEBUFFERPROC IsFramebuffer;
        PFNGLBINDFRAMEBUFFERPROC BindFramebuffer;
        PFNGLDELETEFRAMEBUFFERSPROC DeleteFramebuffers;
        PFNGLGENFRAMEBUFFERSPROC GenFramebuffers;
        PFNGLCHECKFRAMEBUFFERSTATUSPROC CheckFramebufferStatus;
        PFNGLFRAMEBUFFERTEXTURE1DPROC FramebufferTexture1D;
        PFNGLFRAMEBUFFERTEXTURE2DPROC FramebufferTexture2D;
        PFNGLFRAMEBUFFERTEXTURE3DPROC FramebufferTexture3D;
        PFNGLFRAMEBUFFERRENDERBUFFERPROC FramebufferRenderbuffer;
        PFNGLGETFRAMEBUFFERATTACHMENTPARAMETERIVPROC GetFramebufferAttachmentParameteriv;
        PFNGLGENERATEMIPMAPPROC GenerateMipmap;
        PFNGLBLITFRAMEBUFFERPROC BlitFramebuffer;
        PFNGLRENDERBUFFERSTORAGEMULTISAMPLEPROC RenderbufferStorageMultisample;
        PFNGLFRAMEBUFFERTEXTURELAYERPROC FramebufferTextureLayer;
        PFNGLMAPBUFFERRANGEPROC MapBufferRange;
        PFNGLFLUSHMAPPEDBUFFERRANGEPROC FlushMappedBufferRange;
        PFNGLBINDVERTEXARRAYPROC BindVertexArray;
        PFNGLDELETEVERTEXARRAYSPROC DeleteVertexArrays;
        PFNGLGENVERTEXARRAYSPROC GenVertexArrays;
        PFNGLISVERTEXARRAYPROC IsVertexArray;
        PFNGLGETUNIFORMINDICESPROC GetUniformIndices;
        PFNGLGETACTIVEUNIFORMSIVPROC GetActiveUniformsiv;
        PFNGLGETACTIVEUNIFORMNAMEPROC GetActiveUniformName;
        PFNGLGETUNIFORMBLOCKINDEXPROC GetUniformBlockIndex;
        PFNGLGETACTIVEUNIFORMBLOCKIVPROC GetActiveUniformBlockiv;
        PFNGLGETACTIVEUNIFORMBLOCKNAMEPROC GetActiveUniformBlockName;
        PFNGLUNIFORMBLOCKBINDINGPROC UniformBlockBinding;
        PFNGLCOPYBUFFERSUBDATAPROC CopyBufferSubData;
        PFNGLDRAWELEMENTSBASEVERTEXPROC DrawElementsBaseVertex;
        PFNGLDRAWRANGEELEMENTSBASEVERTEXPROC DrawRangeElementsBaseVertex;
        PFNGLDRAWELEMENTSINSTANCEDBASEVERTEXPROC DrawElementsInstancedBaseVertex;
        PFNGLMULTIDRAWELEMENTSBASEVERTEXPROC MultiDrawElementsBaseVertex;
        PFNGLPROVOKINGVERTEXPROC ProvokingVertex;
        PFNGLFENCESYNCPROC FenceSync;
        PFNGLISSYNCPROC IsSync;
        PFNGLDELETESYNCPROC DeleteSync;
        PFNGLCLIENTWAITSYNCPROC ClientWaitSync;
        PFNGLWAITSYNCPROC WaitSync;
        PFNGLGETINTEGER64VPROC GetInteger64v;
        PFNGLGETSYNCIVPROC GetSynciv;
        PFNGLTEXIMAGE2DMULTISAMPLEPROC TexImage2DMultisample;
        PFNGLTEXIMAGE3DMULTISAMPLEPROC TexImage3DMultisample;
        PFNGLGETMULTISAMPLEFVPROC GetMultisamplefv;
        PFNGLSAMPLEMASKIPROC SampleMaski;
        PFNGLBLENDEQUATIONIARBPROC BlendEquationiARB;
        PFNGLBLENDEQUATIONSEPARATEIARBPROC BlendEquationSeparateiARB;
        PFNGLBLENDFUNCIARBPROC BlendFunciARB;
        PFNGLBLENDFUNCSEPARATEIARBPROC BlendFuncSeparateiARB;
        PFNGLMINSAMPLESHADINGARBPROC MinSampleShadingARB;
        PFNGLNAMEDSTRINGARBPROC NamedStringARB;
        PFNGLDELETENAMEDSTRINGARBPROC DeleteNamedStringARB;
        PFNGLCOMPILESHADERINCLUDEARBPROC CompileShaderIncludeARB;
        PFNGLISNAMEDSTRINGARBPROC IsNamedStringARB;
        PFNGLGETNAMEDSTRINGARBPROC GetNamedStringARB;
        PFNGLGETNAMEDSTRINGIVARBPROC GetNamedStringivARB;
        PFNGLBINDFRAGDATALOCATIONINDEXEDPROC BindFragDataLocationIndexed;
        PFNGLGETFRAGDATAINDEXPROC GetFragDataIndex;
        PFNGLGENSAMPLERSPROC GenSamplers;
        PFNGLDELETESAMPLERSPROC DeleteSamplers;
        PFNGLISSAMPLERPROC IsSampler;
        PFNGLBINDSAMPLERPROC BindSampler;
        PFNGLSAMPLERPARAMETERIPROC SamplerParameteri;
        PFNGLSAMPLERPARAMETERIVPROC SamplerParameteriv;
        PFNGLSAMPLERPARAMETERFPROC SamplerParameterf;
        PFNGLSAMPLERPARAMETERFVPROC SamplerParameterfv;
        PFNGLSAMPLERPARAMETERIIVPROC SamplerParameterIiv;
        PFNGLSAMPLERPARAMETERIUIVPROC SamplerParameterIuiv;
        PFNGLGETSAMPLERPARAMETERIVPROC GetSamplerParameteriv;
        PFNGLGETSAMPLERPARAMETERIIVPROC GetSamplerParameterIiv;
        PFNGLGETSAMPLERPARAMETERFVPROC GetSamplerParameterfv;
        PFNGLGETSAMPLERPARAMETERIUIVPROC GetSamplerParameterIuiv;
        PFNGLQUERYCOUNTERPROC QueryCounter;
        PFNGLGETQUERYOBJECTI64VPROC GetQueryObjecti64v;
        PFNGLGETQUERYOBJECTUI64VPROC GetQueryObjectui64v;
        PFNGLVERTEXP2UIPROC VertexP2ui;
        PFNGLVERTEXP2UIVPROC VertexP2uiv;
        PFNGLVERTEXP3UIPROC VertexP3ui;
        PFNGLVERTEXP3UIVPROC VertexP3uiv;
        PFNGLVERTEXP4UIPROC VertexP4ui;
        PFNGLVERTEXP4UIVPROC VertexP4uiv;
        PFNGLTEXCOORDP1UIPROC TexCoordP1ui;
        PFNGLTEXCOORDP1UIVPROC TexCoordP1uiv;
        PFNGLTEXCOORDP2UIPROC TexCoordP2ui;
        PFNGLTEXCOORDP2UIVPROC TexCoordP2uiv;
        PFNGLTEXCOORDP3UIPROC TexCoordP3ui;
        PFNGLTEXCOORDP3UIVPROC TexCoordP3uiv;
        PFNGLTEXCOORDP4UIPROC TexCoordP4ui;
        PFNGLTEXCOORDP4UIVPROC TexCoordP4uiv;
        PFNGLMULTITEXCOORDP1UIPROC MultiTexCoordP1ui;
        PFNGLMULTITEXCOORDP1UIVPROC MultiTexCoordP1uiv;
        PFNGLMULTITEXCOORDP2UIPROC MultiTexCoordP2ui;
        PFNGLMULTITEXCOORDP2UIVPROC MultiTexCoordP2uiv;
        PFNGLMULTITEXCOORDP3UIPROC MultiTexCoordP3ui;
        PFNGLMULTITEXCOORDP3UIVPROC MultiTexCoordP3uiv;
        PFNGLMULTITEXCOORDP4UIPROC MultiTexCoordP4ui;
        PFNGLMULTITEXCOORDP4UIVPROC MultiTexCoordP4uiv;
        PFNGLNORMALP3UIPROC NormalP3ui;
        PFNGLNORMALP3UIVPROC NormalP3uiv;
        PFNGLCOLORP3UIPROC ColorP3ui;
        PFNGLCOLORP3UIVPROC ColorP3uiv;
        PFNGLCOLORP4UIPROC ColorP4ui;
        PFNGLCOLORP4UIVPROC ColorP4uiv;
        PFNGLSECONDARYCOLORP3UIPROC SecondaryColorP3ui;
        PFNGLSECONDARYCOLORP3UIVPROC SecondaryColorP3uiv;
        PFNGLVERTEXATTRIBP1UIPROC VertexAttribP1ui;
        PFNGLVERTEXATTRIBP1UIVPROC VertexAttribP1uiv;
        PFNGLVERTEXATTRIBP2UIPROC VertexAttribP2ui;
        PFNGLVERTEXATTRIBP2UIVPROC VertexAttribP2uiv;
        PFNGLVERTEXATTRIBP3UIPROC VertexAttribP3ui;
        PFNGLVERTEXATTRIBP3UIVPROC VertexAttribP3uiv;
        PFNGLVERTEXATTRIBP4UIPROC VertexAttribP4ui;
        PFNGLVERTEXATTRIBP4UIVPROC VertexAttribP4uiv;
        PFNGLDRAWARRAYSINDIRECTPROC DrawArraysIndirect;
        PFNGLDRAWELEMENTSINDIRECTPROC DrawElementsIndirect;
        PFNGLUNIFORM1DPROC Uniform1d;
        PFNGLUNIFORM2DPROC Uniform2d;
        PFNGLUNIFORM3DPROC Uniform3d;
        PFNGLUNIFORM4DPROC Uniform4d;
        PFNGLUNIFORM1DVPROC Uniform1dv;
        PFNGLUNIFORM2DVPROC Uniform2dv;
        PFNGLUNIFORM3DVPROC Uniform3dv;
        PFNGLUNIFORM4DVPROC Uniform4dv;
        PFNGLUNIFORMMATRIX2DVPROC UniformMatrix2dv;
        PFNGLUNIFORMMATRIX3DVPROC UniformMatrix3dv;
        PFNGLUNIFORMMATRIX4DVPROC UniformMatrix4dv;
        PFNGLUNIFORMMATRIX2X3DVPROC UniformMatrix2x3dv;
        PFNGLUNIFORMMATRIX2X4DVPROC UniformMatrix2x4dv;
        PFNGLUNIFORMMATRIX3X2DVPROC UniformMatrix3x2dv;
        PFNGLUNIFORMMATRIX3X4DVPROC UniformMatrix3x4dv;
        PFNGLUNIFORMMATRIX4X2DVPROC UniformMatrix4x2dv;
        PFNGLUNIFORMMATRIX4X3DVPROC UniformMatrix4x3dv;
        PFNGLGETUNIFORMDVPROC GetUniformdv;
        PFNGLGETSUBROUTINEUNIFORMLOCATIONPROC GetSubroutineUniformLocation;
        PFNGLGETSUBROUTINEINDEXPROC GetSubroutineIndex;
        PFNGLGETACTIVESUBROUTINEUNIFORMIVPROC GetActiveSubroutineUniformiv;
        PFNGLGETACTIVESUBROUTINEUNIFORMNAMEPROC GetActiveSubroutineUniformName;
        PFNGLGETACTIVESUBROUTINENAMEPROC GetActiveSubroutineName;
        PFNGLUNIFORMSUBROUTINESUIVPROC UniformSubroutinesuiv;
        PFNGLGETUNIFORMSUBROUTINEUIVPROC GetUniformSubroutineuiv;
        PFNGLGETPROGRAMSTAGEIVPROC GetProgramStageiv;
        PFNGLPATCHPARAMETERIPROC PatchParameteri;
        PFNGLPATCHPARAMETERFVPROC PatchParameterfv;
        PFNGLBINDTRANSFORMFEEDBACKPROC BindTransformFeedback;
        PFNGLDELETETRANSFORMFEEDBACKSPROC DeleteTransformFeedbacks;
        PFNGLGENTRANSFORMFEEDBACKSPROC GenTransformFeedbacks;
        PFNGLISTRANSFORMFEEDBACKPROC IsTransformFeedback;
        PFNGLPAUSETRANSFORMFEEDBACKPROC PauseTransformFeedback;
        PFNGLRESUMETRANSFORMFEEDBACKPROC ResumeTransformFeedback;
        PFNGLDRAWTRANSFORMFEEDBACKPROC DrawTransformFeedback;
        PFNGLDRAWTRANSFORMFEEDBACKSTREAMPROC DrawTransformFeedbackStream;
        PFNGLBEGINQUERYINDEXEDPROC BeginQueryIndexed;
        PFNGLENDQUERYINDEXEDPROC EndQueryIndexed;
        PFNGLGETQUERYINDEXEDIVPROC GetQueryIndexediv;
        PFNGLRELEASESHADERCOMPILERPROC ReleaseShaderCompiler;
        PFNGLSHADERBINARYPROC ShaderBinary;
        PFNGLGETSHADERPRECISIONFORMATPROC GetShaderPrecisionFormat;
        PFNGLDEPTHRANGEFPROC DepthRangef;
        PFNGLCLEARDEPTHFPROC ClearDepthf;
        PFNGLGETPROGRAMBINARYPROC GetProgramBinary;
        PFNGLPROGRAMBINARYPROC ProgramBinary;
        PFNGLPROGRAMPARAMETERIPROC ProgramParameteri;
        PFNGLUSEPROGRAMSTAGESPROC UseProgramStages;
        PFNGLACTIVESHADERPROGRAMPROC ActiveShaderProgram;
        PFNGLCREATESHADERPROGRAMVPROC CreateShaderProgramv;
        PFNGLBINDPROGRAMPIPELINEPROC BindProgramPipeline;
        PFNGLDELETEPROGRAMPIPELINESPROC DeleteProgramPipelines;
        PFNGLGENPROGRAMPIPELINESPROC GenProgramPipelines;
        PFNGLISPROGRAMPIPELINEPROC IsProgramPipeline;
        PFNGLGETPROGRAMPIPELINEIVPROC GetProgramPipelineiv;
        PFNGLPROGRAMUNIFORM1IPROC ProgramUniform1i;
        PFNGLPROGRAMUNIFORM1IVPROC ProgramUniform1iv;
        PFNGLPROGRAMUNIFORM1FPROC ProgramUniform1f;
        PFNGLPROGRAMUNIFORM1FVPROC ProgramUniform1fv;
        PFNGLPROGRAMUNIFORM1DPROC ProgramUniform1d;
        PFNGLPROGRAMUNIFORM1DVPROC ProgramUniform1dv;
        PFNGLPROGRAMUNIFORM1UIPROC ProgramUniform1ui;
        PFNGLPROGRAMUNIFORM1UIVPROC ProgramUniform1uiv;
        PFNGLPROGRAMUNIFORM2IPROC ProgramUniform2i;
        PFNGLPROGRAMUNIFORM2IVPROC ProgramUniform2iv;
        PFNGLPROGRAMUNIFORM2FPROC ProgramUniform2f;
        PFNGLPROGRAMUNIFORM2FVPROC ProgramUniform2fv;
        PFNGLPROGRAMUNIFORM2DPROC ProgramUniform2d;
        PFNGLPROGRAMUNIFORM2DVPROC ProgramUniform2dv;
        PFNGLPROGRAMUNIFORM2UIPROC ProgramUniform2ui;
        PFNGLPROGRAMUNIFORM2UIVPROC ProgramUniform2uiv;
        PFNGLPROGRAMUNIFORM3IPROC ProgramUniform3i;
        PFNGLPROGRAMUNIFORM3IVPROC ProgramUniform3iv;
        PFNGLPROGRAMUNIFORM3FPROC ProgramUniform3f;
        PFNGLPROGRAMUNIFORM3FVPROC ProgramUniform3fv;
        PFNGLPROGRAMUNIFORM3DPROC ProgramUniform3d;
        PFNGLPROGRAMUNIFORM3DVPROC ProgramUniform3dv;
        PFNGLPROGRAMUNIFORM3UIPROC ProgramUniform3ui;
        PFNGLPROGRAMUNIFORM3UIVPROC ProgramUniform3uiv;
        PFNGLPROGRAMUNIFORM4IPROC ProgramUniform4i;
        PFNGLPROGRAMUNIFORM4IVPROC ProgramUniform4iv;
        PFNGLPROGRAMUNIFORM4FPROC ProgramUniform4f;
        PFNGLPROGRAMUNIFORM4FVPROC ProgramUniform4fv;
        PFNGLPROGRAMUNIFORM4DPROC ProgramUniform4d;
        PFNGLPROGRAMUNIFORM4DVPROC ProgramUniform4dv;
        PFNGLPROGRAMUNIFORM4UIPROC ProgramUniform4ui;
        PFNGLPROGRAMUNIFORM4UIVPROC ProgramUniform4uiv;
        PFNGLPROGRAMUNIFORMMATRIX2FVPROC ProgramUniformMatrix2fv;
        PFNGLPROGRAMUNIFORMMATRIX3FVPROC ProgramUniformMatrix3fv;
        PFNGLPROGRAMUNIFORMMATRIX4FVPROC ProgramUniformMatrix4fv;
        PFNGLPROGRAMUNIFORMMATRIX2DVPROC ProgramUniformMatrix2dv;
        PFNGLPROGRAMUNIFORMMATRIX3DVPROC ProgramUniformMatrix3dv;
        PFNGLPROGRAMUNIFORMMATRIX4DVPROC ProgramUniformMatrix4dv;
        PFNGLPROGRAMUNIFORMMATRIX2X3FVPROC ProgramUniformMatrix2x3fv;
        PFNGLPROGRAMUNIFORMMATRIX3X2FVPROC ProgramUniformMatrix3x2fv;
        PFNGLPROGRAMUNIFORMMATRIX2X4FVPROC ProgramUniformMatrix2x4fv;
        PFNGLPROGRAMUNIFORMMATRIX4X2FVPROC ProgramUniformMatrix4x2fv;
        PFNGLPROGRAMUNIFORMMATRIX3X4FVPROC ProgramUniformMatrix3x4fv;
        PFNGLPROGRAMUNIFORMMATRIX4X3FVPROC ProgramUniformMatrix4x3fv;
        PFNGLPROGRAMUNIFORMMATRIX2X3DVPROC ProgramUniformMatrix2x3dv;
        PFNGLPROGRAMUNIFORMMATRIX3X2DVPROC ProgramUniformMatrix3x2dv;
        PFNGLPROGRAMUNIFORMMATRIX2X4DVPROC ProgramUniformMatrix2x4dv;
        PFNGLPROGRAMUNIFORMMATRIX4X2DVPROC ProgramUniformMatrix4x2dv;
        PFNGLPROGRAMUNIFORMMATRIX3X4DVPROC ProgramUniformMatrix3x4dv;
        PFNGLPROGRAMUNIFORMMATRIX4X3DVPROC ProgramUniformMatrix4x3dv;
        PFNGLVALIDATEPROGRAMPIPELINEPROC ValidateProgramPipeline;
        PFNGLGETPROGRAMPIPELINEINFOLOGPROC GetProgramPipelineInfoLog;
        PFNGLVERTEXATTRIBL1DPROC VertexAttribL1d;
        PFNGLVERTEXATTRIBL2DPROC VertexAttribL2d;
        PFNGLVERTEXATTRIBL3DPROC VertexAttribL3d;
        PFNGLVERTEXATTRIBL4DPROC VertexAttribL4d;
        PFNGLVERTEXATTRIBL1DVPROC VertexAttribL1dv;
        PFNGLVERTEXATTRIBL2DVPROC VertexAttribL2dv;
        PFNGLVERTEXATTRIBL3DVPROC VertexAttribL3dv;
        PFNGLVERTEXATTRIBL4DVPROC VertexAttribL4dv;
        PFNGLVERTEXATTRIBLPOINTERPROC VertexAttribLPointer;
        PFNGLGETVERTEXATTRIBLDVPROC GetVertexAttribLdv;
        PFNGLVIEWPORTARRAYVPROC ViewportArrayv;
        PFNGLVIEWPORTINDEXEDFPROC ViewportIndexedf;
        PFNGLVIEWPORTINDEXEDFVPROC ViewportIndexedfv;
        PFNGLSCISSORARRAYVPROC ScissorArrayv;
        PFNGLSCISSORINDEXEDPROC ScissorIndexed;
        PFNGLSCISSORINDEXEDVPROC ScissorIndexedv;
        PFNGLDEPTHRANGEARRAYVPROC DepthRangeArrayv;
        PFNGLDEPTHRANGEINDEXEDPROC DepthRangeIndexed;
        PFNGLGETFLOATI_VPROC GetFloati_v;
        PFNGLGETDOUBLEI_VPROC GetDoublei_v;
        PFNGLCREATESYNCFROMCLEVENTARBPROC CreateSyncFromCLeventARB;
        PFNGLDEBUGMESSAGECONTROLARBPROC DebugMessageControlARB;
        PFNGLDEBUGMESSAGEINSERTARBPROC DebugMessageInsertARB;
        PFNGLDEBUGMESSAGECALLBACKARBPROC DebugMessageCallbackARB;
        PFNGLGETDEBUGMESSAGELOGARBPROC GetDebugMessageLogARB;
        PFNGLGETGRAPHICSRESETSTATUSARBPROC GetGraphicsResetStatusARB;
        PFNGLGETNTEXIMAGEARBPROC GetnTexImageARB;
        PFNGLREADNPIXELSARBPROC ReadnPixelsARB;
        PFNGLGETNCOMPRESSEDTEXIMAGEARBPROC GetnCompressedTexImageARB;
        PFNGLGETNUNIFORMFVARBPROC GetnUniformfvARB;
        PFNGLGETNUNIFORMIVARBPROC GetnUniformivARB;
        PFNGLGETNUNIFORMUIVARBPROC GetnUniformuivARB;
        PFNGLGETNUNIFORMDVARBPROC GetnUniformdvARB;
        PFNGLDRAWARRAYSINSTANCEDBASEINSTANCEPROC DrawArraysInstancedBaseInstance;
        PFNGLDRAWELEMENTSINSTANCEDBASEINSTANCEPROC DrawElementsInstancedBaseInstance;
        PFNGLDRAWELEMENTSINSTANCEDBASEVERTEXBASEINSTANCEPROC
        DrawElementsInstancedBaseVertexBaseInstance;
        PFNGLDRAWTRANSFORMFEEDBACKINSTANCEDPROC DrawTransformFeedbackInstanced;
        PFNGLDRAWTRANSFORMFEEDBACKSTREAMINSTANCEDPROC DrawTransformFeedbackStreamInstanced;
        PFNGLGETINTERNALFORMATIVPROC GetInternalformativ;
        PFNGLGETACTIVEATOMICCOUNTERBUFFERIVPROC GetActiveAtomicCounterBufferiv;
        PFNGLBINDIMAGETEXTUREPROC BindImageTexture;
        PFNGLMEMORYBARRIERPROC MemoryBarrier;
        PFNGLTEXSTORAGE1DPROC TexStorage1D;
        PFNGLTEXSTORAGE2DPROC TexStorage2D;
        PFNGLTEXSTORAGE3DPROC TexStorage3D;
        PFNGLTEXTURESTORAGE1DEXTPROC TextureStorage1DEXT;
        PFNGLTEXTURESTORAGE2DEXTPROC TextureStorage2DEXT;
        PFNGLTEXTURESTORAGE3DEXTPROC TextureStorage3DEXT;
        PFNGLDEBUGMESSAGECONTROLPROC DebugMessageControl;
        PFNGLDEBUGMESSAGEINSERTPROC DebugMessageInsert;
        PFNGLDEBUGMESSAGECALLBACKPROC DebugMessageCallback;
        PFNGLGETDEBUGMESSAGELOGPROC GetDebugMessageLog;
        PFNGLPUSHDEBUGGROUPPROC PushDebugGroup;
        PFNGLPOPDEBUGGROUPPROC PopDebugGroup;
        PFNGLOBJECTLABELPROC ObjectLabel;
        PFNGLGETOBJECTLABELPROC GetObjectLabel;
        PFNGLOBJECTPTRLABELPROC ObjectPtrLabel;
        PFNGLGETOBJECTPTRLABELPROC GetObjectPtrLabel;
        PFNGLCLEARBUFFERDATAPROC ClearBufferData;
        PFNGLCLEARBUFFERSUBDATAPROC ClearBufferSubData;
        PFNGLCLEARNAMEDBUFFERDATAEXTPROC ClearNamedBufferDataEXT;
        PFNGLCLEARNAMEDBUFFERSUBDATAEXTPROC ClearNamedBufferSubDataEXT;
        PFNGLDISPATCHCOMPUTEPROC DispatchCompute;
        PFNGLDISPATCHCOMPUTEINDIRECTPROC DispatchComputeIndirect;
        PFNGLCOPYIMAGESUBDATAPROC CopyImageSubData;
        PFNGLTEXTUREVIEWPROC TextureView;
        PFNGLBINDVERTEXBUFFERPROC BindVertexBuffer;
        PFNGLVERTEXATTRIBFORMATPROC VertexAttribFormat;
        PFNGLVERTEXATTRIBIFORMATPROC VertexAttribIFormat;
        PFNGLVERTEXATTRIBLFORMATPROC VertexAttribLFormat;
        PFNGLVERTEXATTRIBBINDINGPROC VertexAttribBinding;
        PFNGLVERTEXBINDINGDIVISORPROC VertexBindingDivisor;
        PFNGLVERTEXARRAYBINDVERTEXBUFFEREXTPROC VertexArrayBindVertexBufferEXT;
        PFNGLVERTEXARRAYVERTEXATTRIBFORMATEXTPROC VertexArrayVertexAttribFormatEXT;
        PFNGLVERTEXARRAYVERTEXATTRIBIFORMATEXTPROC VertexArrayVertexAttribIFormatEXT;
        PFNGLVERTEXARRAYVERTEXATTRIBLFORMATEXTPROC VertexArrayVertexAttribLFormatEXT;
        PFNGLVERTEXARRAYVERTEXATTRIBBINDINGEXTPROC VertexArrayVertexAttribBindingEXT;
        PFNGLVERTEXARRAYVERTEXBINDINGDIVISOREXTPROC VertexArrayVertexBindingDivisorEXT;
        PFNGLFRAMEBUFFERPARAMETERIPROC FramebufferParameteri;
        PFNGLGETFRAMEBUFFERPARAMETERIVPROC GetFramebufferParameteriv;
        PFNGLNAMEDFRAMEBUFFERPARAMETERIEXTPROC NamedFramebufferParameteriEXT;
        PFNGLGETNAMEDFRAMEBUFFERPARAMETERIVEXTPROC GetNamedFramebufferParameterivEXT;
        PFNGLGETINTERNALFORMATI64VPROC GetInternalformati64v;
        PFNGLINVALIDATETEXSUBIMAGEPROC InvalidateTexSubImage;
        PFNGLINVALIDATETEXIMAGEPROC InvalidateTexImage;
        PFNGLINVALIDATEBUFFERSUBDATAPROC InvalidateBufferSubData;
        PFNGLINVALIDATEBUFFERDATAPROC InvalidateBufferData;
        PFNGLINVALIDATEFRAMEBUFFERPROC InvalidateFramebuffer;
        PFNGLINVALIDATESUBFRAMEBUFFERPROC InvalidateSubFramebuffer;
        PFNGLMULTIDRAWARRAYSINDIRECTPROC MultiDrawArraysIndirect;
        PFNGLMULTIDRAWELEMENTSINDIRECTPROC MultiDrawElementsIndirect;
        PFNGLGETPROGRAMINTERFACEIVPROC GetProgramInterfaceiv;
        PFNGLGETPROGRAMRESOURCEINDEXPROC GetProgramResourceIndex;
        PFNGLGETPROGRAMRESOURCENAMEPROC GetProgramResourceName;
        PFNGLGETPROGRAMRESOURCEIVPROC GetProgramResourceiv;
        PFNGLGETPROGRAMRESOURCELOCATIONPROC GetProgramResourceLocation;
        PFNGLGETPROGRAMRESOURCELOCATIONINDEXPROC GetProgramResourceLocationIndex;
        PFNGLSHADERSTORAGEBLOCKBINDINGPROC ShaderStorageBlockBinding;
        PFNGLTEXBUFFERRANGEPROC TexBufferRange;
        PFNGLTEXTUREBUFFERRANGEEXTPROC TextureBufferRangeEXT;
        PFNGLTEXSTORAGE2DMULTISAMPLEPROC TexStorage2DMultisample;
        PFNGLTEXSTORAGE3DMULTISAMPLEPROC TexStorage3DMultisample;
        PFNGLTEXTURESTORAGE2DMULTISAMPLEEXTPROC TextureStorage2DMultisampleEXT;
        PFNGLTEXTURESTORAGE3DMULTISAMPLEEXTPROC TextureStorage3DMultisampleEXT;
    } GlApi;

#if defined(__cplusplus)
}
#endif
