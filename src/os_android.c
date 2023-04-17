#include "api_os_opengl.h"

#include <EGL/egl.h>
#include <GLES3/gl3.h>
#include <jni.h>
#include <android_native_app_glue.h>
#include <android/log.h>
#include <android/input.h>
#include <android/asset_manager.h>
#include <aaudio/AAudio.h>

#include <sys/eventfd.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <time.h>

#ifndef CONFIG_ENABLE_OPENGL
#   error "CONFIG_ENABLE_OPENGL is required for Android build."
#endif

struct
{
	bool has_focus;
	bool can_initialize_window;
	struct android_app* app_state;
	struct timespec started_time;
	
	OS_State state;
	OS_WindowGraphicsContext graphics_context;
	OS_OpenGlApi opengl_graphics_context;
	
	EGLDisplay gl_display;
	EGLContext gl_context;
	EGLSurface gl_surface;
	
	OS_AudioThreadProc* audio_user_thread_proc;
	void* audio_user_thread_data;
	AAudioStream* audio_stream;
	int32 audio_sample_rate;
	int32 audio_channel_count;
	int32 audio_samples_per_frame;
}
static g_android;

static Arena*
GetThreadScratchArena_(void)
{
	static thread_local Arena* this_arena;
	
	if (!this_arena)
		this_arena = Arena_Create(64 << 10, 64 << 10);
	
	return this_arena;
}

static bool
PresentAndVsync_(int32 vsync_count)
{
	Trace();
	
	eglSwapBuffers(g_android.gl_display, g_android.gl_surface);
	return false;
}

static bool
LoadOpenGLFunctions_(void)
{
	Trace();
	
	const GLubyte* version = glGetString(GL_VERSION);
	if (!version)
		return false;
	
	OS_DebugLog("OpenGL version: %s\n", (const char*)version);
	
	OS_OpenGlApi* opengl = &g_android.opengl_graphics_context;
	
	opengl->is_es = true;
#ifdef CONFIG_DEBUG
	opengl->glDebugMessageCallback = (PFNGLDEBUGMESSAGECALLBACKPROC)eglGetProcAddress("glDebugMessageCallback");
#endif
	opengl->glGetString = glGetString;
	opengl->glCullFace = glCullFace;
	opengl->glFrontFace = glFrontFace;
	opengl->glHint = glHint;
	opengl->glLineWidth = glLineWidth;
	opengl->glScissor = glScissor;
	opengl->glTexParameterf = glTexParameterf;
	opengl->glTexParameterfv = glTexParameterfv;
	opengl->glTexParameteri = glTexParameteri;
	opengl->glTexParameteriv = glTexParameteriv;
	opengl->glTexImage2D = glTexImage2D;
	opengl->glClear = glClear;
	opengl->glClearColor = glClearColor;
	opengl->glClearStencil = glClearStencil;
	opengl->glStencilMask = glStencilMask;
	opengl->glColorMask = glColorMask;
	opengl->glDepthMask = glDepthMask;
	opengl->glDisable = glDisable;
	opengl->glEnable = glEnable;
	opengl->glFinish = glFinish;
	opengl->glFlush = glFlush;
	opengl->glBlendFunc = glBlendFunc;
	opengl->glStencilFunc = glStencilFunc;
	opengl->glStencilOp = glStencilOp;
	opengl->glDepthFunc = glDepthFunc;
	opengl->glPixelStorei = glPixelStorei;
	opengl->glReadBuffer = glReadBuffer;
	opengl->glReadPixels = glReadPixels;
	opengl->glGetBooleanv = glGetBooleanv;
	opengl->glGetError = glGetError;
	opengl->glGetFloatv = glGetFloatv;
	opengl->glGetIntegerv = glGetIntegerv;
	opengl->glGetString = glGetString;
	opengl->glGetTexParameterfv = glGetTexParameterfv;
	opengl->glGetTexParameteriv = glGetTexParameteriv;
	opengl->glIsEnabled = glIsEnabled;
	opengl->glViewport = glViewport;
	opengl->glDrawArrays = glDrawArrays;
	opengl->glDrawElements = glDrawElements;
	opengl->glPolygonOffset = glPolygonOffset;
	opengl->glCopyTexImage2D = glCopyTexImage2D;
	opengl->glCopyTexSubImage2D = glCopyTexSubImage2D;
	opengl->glTexSubImage2D = glTexSubImage2D;
	opengl->glBindTexture = glBindTexture;
	opengl->glDeleteTextures = glDeleteTextures;
	opengl->glGenTextures = glGenTextures;
	opengl->glIsTexture = glIsTexture;
	opengl->glDrawRangeElements = glDrawRangeElements;
	opengl->glTexImage3D = glTexImage3D;
	opengl->glTexSubImage3D = glTexSubImage3D;
	opengl->glCopyTexSubImage3D = glCopyTexSubImage3D;
	opengl->glActiveTexture = glActiveTexture;
	opengl->glSampleCoverage = glSampleCoverage;
	opengl->glCompressedTexImage3D = glCompressedTexImage3D;
	opengl->glCompressedTexImage2D = glCompressedTexImage2D;
	opengl->glCompressedTexSubImage3D = glCompressedTexSubImage3D;
	opengl->glCompressedTexSubImage2D = glCompressedTexSubImage2D;
	opengl->glBlendFuncSeparate = glBlendFuncSeparate;
	opengl->glBlendColor = glBlendColor;
	opengl->glBlendEquation = glBlendEquation;
	opengl->glGenQueries = glGenQueries;
	opengl->glDeleteQueries = glDeleteQueries;
	opengl->glIsQuery = glIsQuery;
	opengl->glBeginQuery = glBeginQuery;
	opengl->glEndQuery = glEndQuery;
	opengl->glGetQueryiv = glGetQueryiv;
	opengl->glGetQueryObjectuiv = glGetQueryObjectuiv;
	opengl->glBindBuffer = glBindBuffer;
	opengl->glDeleteBuffers = glDeleteBuffers;
	opengl->glGenBuffers = glGenBuffers;
	opengl->glIsBuffer = glIsBuffer;
	opengl->glBufferData = glBufferData;
	opengl->glBufferSubData = glBufferSubData;
	opengl->glGetBufferParameteriv = glGetBufferParameteriv;
	opengl->glGetBufferPointerv = glGetBufferPointerv;
	opengl->glBlendEquationSeparate = glBlendEquationSeparate;
	opengl->glDrawBuffers = glDrawBuffers;
	opengl->glStencilOpSeparate = glStencilOpSeparate;
	opengl->glStencilFuncSeparate = glStencilFuncSeparate;
	opengl->glStencilMaskSeparate = glStencilMaskSeparate;
	opengl->glAttachShader = glAttachShader;
	opengl->glBindAttribLocation = glBindAttribLocation;
	opengl->glCompileShader = glCompileShader;
	opengl->glCreateProgram = glCreateProgram;
	opengl->glCreateShader = glCreateShader;
	opengl->glDeleteProgram = glDeleteProgram;
	opengl->glDeleteShader = glDeleteShader;
	opengl->glDetachShader = glDetachShader;
	opengl->glDisableVertexAttribArray = glDisableVertexAttribArray;
	opengl->glEnableVertexAttribArray = glEnableVertexAttribArray;
	opengl->glGetActiveAttrib = glGetActiveAttrib;
	opengl->glGetActiveUniform = glGetActiveUniform;
	opengl->glGetAttachedShaders = glGetAttachedShaders;
	opengl->glGetAttribLocation = glGetAttribLocation;
	opengl->glGetProgramiv = glGetProgramiv;
	opengl->glGetProgramInfoLog = glGetProgramInfoLog;
	opengl->glGetShaderiv = glGetShaderiv;
	opengl->glGetShaderInfoLog = glGetShaderInfoLog;
	opengl->glGetShaderSource = glGetShaderSource;
	opengl->glGetUniformLocation = glGetUniformLocation;
	opengl->glGetUniformfv = glGetUniformfv;
	opengl->glGetUniformiv = glGetUniformiv;
	opengl->glGetVertexAttribPointerv = glGetVertexAttribPointerv;
	opengl->glIsProgram = glIsProgram;
	opengl->glIsShader = glIsShader;
	opengl->glLinkProgram = glLinkProgram;
	opengl->glShaderSource = glShaderSource;
	opengl->glUseProgram = glUseProgram;
	opengl->glUniform1f = glUniform1f;
	opengl->glUniform2f = glUniform2f;
	opengl->glUniform3f = glUniform3f;
	opengl->glUniform4f = glUniform4f;
	opengl->glUniform1i = glUniform1i;
	opengl->glUniform2i = glUniform2i;
	opengl->glUniform3i = glUniform3i;
	opengl->glUniform4i = glUniform4i;
	opengl->glUniform1fv = glUniform1fv;
	opengl->glUniform2fv = glUniform2fv;
	opengl->glUniform3fv = glUniform3fv;
	opengl->glUniform4fv = glUniform4fv;
	opengl->glUniform1iv = glUniform1iv;
	opengl->glUniform2iv = glUniform2iv;
	opengl->glUniform3iv = glUniform3iv;
	opengl->glUniform4iv = glUniform4iv;
	opengl->glUniformMatrix2fv = glUniformMatrix2fv;
	opengl->glUniformMatrix3fv = glUniformMatrix3fv;
	opengl->glUniformMatrix4fv = glUniformMatrix4fv;
	opengl->glValidateProgram = glValidateProgram;
	opengl->glVertexAttribPointer = glVertexAttribPointer;
	opengl->glUniformMatrix2x3fv = glUniformMatrix2x3fv;
	opengl->glUniformMatrix3x2fv = glUniformMatrix3x2fv;
	opengl->glUniformMatrix2x4fv = glUniformMatrix2x4fv;
	opengl->glUniformMatrix4x2fv = glUniformMatrix4x2fv;
	opengl->glUniformMatrix3x4fv = glUniformMatrix3x4fv;
	opengl->glUniformMatrix4x3fv = glUniformMatrix4x3fv;
	opengl->glGetBooleanv = glGetBooleanv;
	opengl->glGetIntegerv = glGetIntegerv;
	opengl->glBeginTransformFeedback = glBeginTransformFeedback;
	opengl->glEndTransformFeedback = glEndTransformFeedback;
	opengl->glBindBufferRange = glBindBufferRange;
	opengl->glBindBufferBase = glBindBufferBase;
	opengl->glTransformFeedbackVaryings = glTransformFeedbackVaryings;
	opengl->glGetTransformFeedbackVarying = glGetTransformFeedbackVarying;
	opengl->glVertexAttribIPointer = glVertexAttribIPointer;
	opengl->glGetUniformuiv = glGetUniformuiv;
	opengl->glGetFragDataLocation = glGetFragDataLocation;
	opengl->glUniform1ui = glUniform1ui;
	opengl->glUniform2ui = glUniform2ui;
	opengl->glUniform3ui = glUniform3ui;
	opengl->glUniform4ui = glUniform4ui;
	opengl->glUniform1uiv = glUniform1uiv;
	opengl->glUniform2uiv = glUniform2uiv;
	opengl->glUniform3uiv = glUniform3uiv;
	opengl->glUniform4uiv = glUniform4uiv;
	opengl->glClearBufferiv = glClearBufferiv;
	opengl->glClearBufferuiv = glClearBufferuiv;
	opengl->glClearBufferfv = glClearBufferfv;
	opengl->glClearBufferfi = glClearBufferfi;
	opengl->glGetStringi = glGetStringi;
	opengl->glIsRenderbuffer = glIsRenderbuffer;
	opengl->glBindRenderbuffer = glBindRenderbuffer;
	opengl->glDeleteRenderbuffers = glDeleteRenderbuffers;
	opengl->glGenRenderbuffers = glGenRenderbuffers;
	opengl->glRenderbufferStorage = glRenderbufferStorage;
	opengl->glGetRenderbufferParameteriv = glGetRenderbufferParameteriv;
	opengl->glIsFramebuffer = glIsFramebuffer;
	opengl->glBindFramebuffer = glBindFramebuffer;
	opengl->glDeleteFramebuffers = glDeleteFramebuffers;
	opengl->glGenFramebuffers = glGenFramebuffers;
	opengl->glCheckFramebufferStatus = glCheckFramebufferStatus;
	opengl->glFramebufferTexture2D = glFramebufferTexture2D;
	opengl->glFramebufferRenderbuffer = glFramebufferRenderbuffer;
	opengl->glGetFramebufferAttachmentParameteriv = glGetFramebufferAttachmentParameteriv;
	opengl->glGenerateMipmap = glGenerateMipmap;
	opengl->glBlitFramebuffer = glBlitFramebuffer;
	opengl->glRenderbufferStorageMultisample = glRenderbufferStorageMultisample;
	opengl->glFramebufferTextureLayer = glFramebufferTextureLayer;
	opengl->glMapBufferRange = glMapBufferRange;
	opengl->glFlushMappedBufferRange = glFlushMappedBufferRange;
	opengl->glBindVertexArray = glBindVertexArray;
	opengl->glDeleteVertexArrays = glDeleteVertexArrays;
	opengl->glGenVertexArrays = glGenVertexArrays;
	opengl->glIsVertexArray = glIsVertexArray;
	opengl->glDrawArraysInstanced = glDrawArraysInstanced;
	opengl->glDrawElementsInstanced = glDrawElementsInstanced;
	opengl->glCopyBufferSubData = glCopyBufferSubData;
	opengl->glGetUniformIndices = glGetUniformIndices;
	opengl->glGetActiveUniformsiv = glGetActiveUniformsiv;
	opengl->glGetUniformBlockIndex = glGetUniformBlockIndex;
	opengl->glGetActiveUniformBlockiv = glGetActiveUniformBlockiv;
	opengl->glGetActiveUniformBlockName = glGetActiveUniformBlockName;
	opengl->glUniformBlockBinding = glUniformBlockBinding;
	opengl->glBindBufferRange = glBindBufferRange;
	opengl->glBindBufferBase = glBindBufferBase;
	opengl->glFenceSync = glFenceSync;
	opengl->glIsSync = glIsSync;
	opengl->glDeleteSync = glDeleteSync;
	opengl->glClientWaitSync = glClientWaitSync;
	opengl->glWaitSync = glWaitSync;
	opengl->glGetInteger64v = glGetInteger64v;
	opengl->glGetSynciv = glGetSynciv;
	opengl->glGetInteger64i_v = glGetInteger64i_v;
	opengl->glGetBufferParameteri64v = glGetBufferParameteri64v;
	opengl->glGenSamplers = glGenSamplers;
	opengl->glDeleteSamplers = glDeleteSamplers;
	opengl->glIsSampler = glIsSampler;
	opengl->glBindSampler = glBindSampler;
	opengl->glSamplerParameteri = glSamplerParameteri;
	opengl->glSamplerParameteriv = glSamplerParameteriv;
	opengl->glSamplerParameterf = glSamplerParameterf;
	opengl->glSamplerParameterfv = glSamplerParameterfv;
	opengl->glGetSamplerParameteriv = glGetSamplerParameteriv;
	opengl->glGetSamplerParameterfv = glGetSamplerParameterfv;
	opengl->glVertexAttribDivisor = glVertexAttribDivisor;
	opengl->glUnmapBuffer = glUnmapBuffer;
	
	// NOTE(ljre): Not available in OpenGL ES 3.0, but it's ok
	//opengl->glDrawBuffer = glDrawBuffer;
	//opengl->glPointSize = glPointSize;
	//opengl->glPolygonMode = glPolygonMode;
	//opengl->glTexImage1D = glTexImage1D;
	//opengl->glClearDepth = glClearDepth;
	//opengl->glLogicOp = glLogicOp;
	//opengl->glPixelStoref = glPixelStoref;
	//opengl->glGetDoublev = glGetDoublev;
	//opengl->glGetTexImage = glGetTexImage;
	//opengl->glGetTexLevelParameterfv = glGetTexLevelParameterfv;
	//opengl->glGetTexLevelParameteriv = glGetTexLevelParameteriv;
	//opengl->glDepthRange = glDepthRange;
	//opengl->glCopyTexImage1D = glCopyTexImage1D;
	//opengl->glCopyTexSubImage1D = glCopyTexSubImage1D;
	//opengl->glTexSubImage1D = glTexSubImage1D;
	//opengl->glCompressedTexImage1D = glCompressedTexImage1D;
	//opengl->glCompressedTexSubImage1D = glCompressedTexSubImage1D;
	//opengl->glGetCompressedTexImage = glGetCompressedTexImage;
	//opengl->glMultiDrawArrays = glMultiDrawArrays;
	//opengl->glMultiDrawElements = glMultiDrawElements;
	//opengl->glPointParameterf = glPointParameterf;
	//opengl->glPointParameterfv = glPointParameterfv;
	//opengl->glPointParameteri = glPointParameteri;
	//opengl->glPointParameteriv = glPointParameteriv;
	//opengl->glGetQueryObjectiv = glGetQueryObjectiv;
	//opengl->glGetBufferSubData = glGetBufferSubData;
	//opengl->glMapBuffer = glMapBuffer;
	//opengl->glGetVertexAttribdv = glGetVertexAttribdv;
	//opengl->glGetVertexAttribfv = glGetVertexAttribfv;
	//opengl->glGetVertexAttribiv = glGetVertexAttribiv;
	//opengl->glVertexAttrib1d = glVertexAttrib1d;
	//opengl->glVertexAttrib1dv = glVertexAttrib1dv;
	//opengl->glVertexAttrib1f = glVertexAttrib1f;
	//opengl->glVertexAttrib1fv = glVertexAttrib1fv;
	//opengl->glVertexAttrib1s = glVertexAttrib1s;
	//opengl->glVertexAttrib1sv = glVertexAttrib1sv;
	//opengl->glVertexAttrib2d = glVertexAttrib2d;
	//opengl->glVertexAttrib2dv = glVertexAttrib2dv;
	//opengl->glVertexAttrib2f = glVertexAttrib2f;
	//opengl->glVertexAttrib2fv = glVertexAttrib2fv;
	//opengl->glVertexAttrib2s = glVertexAttrib2s;
	//opengl->glVertexAttrib2sv = glVertexAttrib2sv;
	//opengl->glVertexAttrib3d = glVertexAttrib3d;
	//opengl->glVertexAttrib3dv = glVertexAttrib3dv;
	//opengl->glVertexAttrib3f = glVertexAttrib3f;
	//opengl->glVertexAttrib3fv = glVertexAttrib3fv;
	//opengl->glVertexAttrib3s = glVertexAttrib3s;
	//opengl->glVertexAttrib3sv = glVertexAttrib3sv;
	//opengl->glVertexAttrib4Nbv = glVertexAttrib4Nbv;
	//opengl->glVertexAttrib4Niv = glVertexAttrib4Niv;
	//opengl->glVertexAttrib4Nsv = glVertexAttrib4Nsv;
	//opengl->glVertexAttrib4Nub = glVertexAttrib4Nub;
	//opengl->glVertexAttrib4Nubv = glVertexAttrib4Nubv;
	//opengl->glVertexAttrib4Nuiv = glVertexAttrib4Nuiv;
	//opengl->glVertexAttrib4Nusv = glVertexAttrib4Nusv;
	//opengl->glVertexAttrib4bv = glVertexAttrib4bv;
	//opengl->glVertexAttrib4d = glVertexAttrib4d;
	//opengl->glVertexAttrib4dv = glVertexAttrib4dv;
	//opengl->glVertexAttrib4f = glVertexAttrib4f;
	//opengl->glVertexAttrib4fv = glVertexAttrib4fv;
	//opengl->glVertexAttrib4iv = glVertexAttrib4iv;
	//opengl->glVertexAttrib4s = glVertexAttrib4s;
	//opengl->glVertexAttrib4sv = glVertexAttrib4sv;
	//opengl->glVertexAttrib4ubv = glVertexAttrib4ubv;
	//opengl->glVertexAttrib4uiv = glVertexAttrib4uiv;
	//opengl->glVertexAttrib4usv = glVertexAttrib4usv;
	//opengl->glColorMaski = glColorMaski;
	//opengl->glEnablei = glEnablei;
	//opengl->glDisablei = glDisablei;
	//opengl->glIsEnabledi = glIsEnabledi;
	//opengl->glClampColor = glClampColor;
	//opengl->glBeginConditionalRender = glBeginConditionalRender;
	//opengl->glEndConditionalRender = glEndConditionalRender;
	//opengl->glGetVertexAttribIiv = glGetVertexAttribIiv;
	//opengl->glGetVertexAttribIuiv = glGetVertexAttribIuiv;
	//opengl->glVertexAttribI1i = glVertexAttribI1i;
	//opengl->glVertexAttribI2i = glVertexAttribI2i;
	//opengl->glVertexAttribI3i = glVertexAttribI3i;
	//opengl->glVertexAttribI4i = glVertexAttribI4i;
	//opengl->glVertexAttribI1ui = glVertexAttribI1ui;
	//opengl->glVertexAttribI2ui = glVertexAttribI2ui;
	//opengl->glVertexAttribI3ui = glVertexAttribI3ui;
	//opengl->glVertexAttribI4ui = glVertexAttribI4ui;
	//opengl->glVertexAttribI1iv = glVertexAttribI1iv;
	//opengl->glVertexAttribI2iv = glVertexAttribI2iv;
	//opengl->glVertexAttribI3iv = glVertexAttribI3iv;
	//opengl->glVertexAttribI4iv = glVertexAttribI4iv;
	//opengl->glVertexAttribI1uiv = glVertexAttribI1uiv;
	//opengl->glVertexAttribI2uiv = glVertexAttribI2uiv;
	//opengl->glVertexAttribI3uiv = glVertexAttribI3uiv;
	//opengl->glVertexAttribI4uiv = glVertexAttribI4uiv;
	//opengl->glVertexAttribI4bv = glVertexAttribI4bv;
	//opengl->glVertexAttribI4sv = glVertexAttribI4sv;
	//opengl->glVertexAttribI4ubv = glVertexAttribI4ubv;
	//opengl->glVertexAttribI4usv = glVertexAttribI4usv;
	//opengl->glBindFragDataLocation = glBindFragDataLocation;
	//opengl->glTexParameterIiv = glTexParameterIiv;
	//opengl->glTexParameterIuiv = glTexParameterIuiv;
	//opengl->glGetTexParameterIiv = glGetTexParameterIiv;
	//opengl->glGetTexParameterIuiv = glGetTexParameterIuiv;
	//opengl->glFramebufferTexture1D = glFramebufferTexture1D;
	//opengl->glFramebufferTexture3D = glFramebufferTexture3D;
	//opengl->glTexBuffer = glTexBuffer;
	//opengl->glPrimitiveRestartIndex = glPrimitiveRestartIndex;
	//opengl->glGetActiveUniformName = glGetActiveUniformName;
	//opengl->glGetIntegeri_v = glGetIntegeri_v;
	//opengl->glDrawElementsBaseVertex = glDrawElementsBaseVertex;
	//opengl->glDrawRangeElementsBaseVertex = glDrawRangeElementsBaseVertex;
	//opengl->glDrawElementsInstancedBaseVertex = glDrawElementsInstancedBaseVertex;
	//opengl->glMultiDrawElementsBaseVertex = glMultiDrawElementsBaseVertex;
	//opengl->glProvokingVertex = glProvokingVertex;
	//opengl->glFramebufferTexture = glFramebufferTexture;
	//opengl->glTexImage2DMultisample = glTexImage2DMultisample;
	//opengl->glTexImage3DMultisample = glTexImage3DMultisample;
	//opengl->glGetMultisamplefv = glGetMultisamplefv;
	//opengl->glSampleMaski = glSampleMaski;
	//opengl->glBindFragDataLocationIndexed = glBindFragDataLocationIndexed;
	//opengl->glGetFragDataIndex = glGetFragDataIndex;
	//opengl->glSamplerParameterIiv = glSamplerParameterIiv;
	//opengl->glSamplerParameterIuiv = glSamplerParameterIuiv;
	//opengl->glGetSamplerParameterIiv = glGetSamplerParameterIiv;
	//opengl->glGetSamplerParameterIuiv = glGetSamplerParameterIuiv;
	//opengl->glQueryCounter = glQueryCounter;
	//opengl->glGetQueryObjecti64v = glGetQueryObjecti64v;
	//opengl->glGetQueryObjectui64v = glGetQueryObjectui64v;
	//opengl->glVertexAttribP1ui = glVertexAttribP1ui;
	//opengl->glVertexAttribP1uiv = glVertexAttribP1uiv;
	//opengl->glVertexAttribP2ui = glVertexAttribP2ui;
	//opengl->glVertexAttribP2uiv = glVertexAttribP2uiv;
	//opengl->glVertexAttribP3ui = glVertexAttribP3ui;
	//opengl->glVertexAttribP3uiv = glVertexAttribP3uiv;
	//opengl->glVertexAttribP4ui = glVertexAttribP4ui;
	//opengl->glVertexAttribP4uiv = glVertexAttribP4uiv;
	//opengl->glVertexP2ui = glVertexP2ui;
	//opengl->glVertexP2uiv = glVertexP2uiv;
	//opengl->glVertexP3ui = glVertexP3ui;
	//opengl->glVertexP3uiv = glVertexP3uiv;
	//opengl->glVertexP4ui = glVertexP4ui;
	//opengl->glVertexP4uiv = glVertexP4uiv;
	//opengl->glTexCoordP1ui = glTexCoordP1ui;
	//opengl->glTexCoordP1uiv = glTexCoordP1uiv;
	//opengl->glTexCoordP2ui = glTexCoordP2ui;
	//opengl->glTexCoordP2uiv = glTexCoordP2uiv;
	//opengl->glTexCoordP3ui = glTexCoordP3ui;
	//opengl->glTexCoordP3uiv = glTexCoordP3uiv;
	//opengl->glTexCoordP4ui = glTexCoordP4ui;
	//opengl->glTexCoordP4uiv = glTexCoordP4uiv;
	//opengl->glMultiTexCoordP1ui = glMultiTexCoordP1ui;
	//opengl->glMultiTexCoordP1uiv = glMultiTexCoordP1uiv;
	//opengl->glMultiTexCoordP2ui = glMultiTexCoordP2ui;
	//opengl->glMultiTexCoordP2uiv = glMultiTexCoordP2uiv;
	//opengl->glMultiTexCoordP3ui = glMultiTexCoordP3ui;
	//opengl->glMultiTexCoordP3uiv = glMultiTexCoordP3uiv;
	//opengl->glMultiTexCoordP4ui = glMultiTexCoordP4ui;
	//opengl->glMultiTexCoordP4uiv = glMultiTexCoordP4uiv;
	//opengl->glNormalP3ui = glNormalP3ui;
	//opengl->glNormalP3uiv = glNormalP3uiv;
	//opengl->glColorP3ui = glColorP3ui;
	//opengl->glColorP3uiv = glColorP3uiv;
	//opengl->glColorP4ui = glColorP4ui;
	//opengl->glColorP4uiv = glColorP4uiv;
	//opengl->glSecondaryColorP3ui = glSecondaryColorP3ui;
	//opengl->glSecondaryColorP3uiv = glSecondaryColorP3uiv;
	
	return true;
}

static aaudio_data_callback_result_t
AudioCallback_(AAudioStream* stream, void* user_data, void* audio_data, int32 num_frames)
{
	const int32 sample_rate       = g_android.audio_sample_rate;
	const int32 channel_count     = g_android.audio_channel_count;
	const int32 samples_per_frame = g_android.audio_samples_per_frame;
	const int32 sample_count      = num_frames * samples_per_frame;
	
	OS_AudioThreadProc* const user_proc = g_android.audio_user_thread_proc;
	void* const user_proc_data          = g_android.audio_user_thread_data;
	
	Mem_Zero(audio_data, sizeof(int16) * sample_count);
	user_proc(user_proc_data, audio_data, channel_count, sample_rate, sample_count);
	
	return AAUDIO_CALLBACK_RESULT_CONTINUE;
}

static bool
InitAudio_(const OS_InitDesc* init_desc)
{
	aaudio_result_t r;
	
	AAudioStreamBuilder* stream_builder;
	r = AAudio_createStreamBuilder(&stream_builder);
	if (r != AAUDIO_OK)
		return false;
	
	AAudioStreamBuilder_setDirection(stream_builder, AAUDIO_DIRECTION_OUTPUT);
	AAudioStreamBuilder_setSharingMode(stream_builder, AAUDIO_SHARING_MODE_SHARED);
	AAudioStreamBuilder_setSampleRate(stream_builder, 44100);
	AAudioStreamBuilder_setChannelCount(stream_builder, 2);
	AAudioStreamBuilder_setFormat(stream_builder, AAUDIO_FORMAT_PCM_I16);
	AAudioStreamBuilder_setDataCallback(stream_builder, AudioCallback_, NULL);
	
	AAudioStream* audio_stream;
	r = AAudioStreamBuilder_openStream(stream_builder, &audio_stream);
	
	AAudioStreamBuilder_delete(stream_builder);
	if (r != AAUDIO_OK)
		return false;
	
	g_android.audio_stream = audio_stream;
	g_android.audio_sample_rate = AAudioStream_getSampleRate(audio_stream);
	g_android.audio_channel_count = AAudioStream_getChannelCount(audio_stream);
	g_android.audio_samples_per_frame = AAudioStream_getSamplesPerFrame(audio_stream);
	g_android.audio_user_thread_proc = init_desc->audiothread_proc;
	g_android.audio_user_thread_data = init_desc->audiothread_user_data;
	
	r = AAudioStream_requestStart(audio_stream);
	if (r != AAUDIO_OK)
	{
		AAudioStream_close(g_android.audio_stream);
		g_android.audio_stream = NULL;
		return false;
	}
	
	return true;
}

static int32
OnInputEvent_(struct android_app* app, AInputEvent* event)
{
	int32 type = AInputEvent_getType(event);
	
	switch (type)
	{
		case AINPUT_EVENT_TYPE_MOTION:
		{
			int32 flags = AMotionEvent_getFlags(event);
			int32 action = AMotionEvent_getAction(event);
			uintsize pointer_index = 0;
			
			float32 x_axis = AMotionEvent_getAxisValue(event, AMOTION_EVENT_AXIS_X, pointer_index);
			float32 y_axis = AMotionEvent_getAxisValue(event, AMOTION_EVENT_AXIS_Y, pointer_index);
			
			g_android.state.mouse.pos[0] = x_axis;
			g_android.state.mouse.pos[1] = y_axis;
			
			if (action == 0 || action == 2)
			{
				g_android.state.mouse.buttons[OS_MouseButton_Left].is_down = (action == 0);
				g_android.state.mouse.buttons[OS_MouseButton_Left].changes += 1;
			}
		} break;
		
		//case AINPUT_EVENT_TYPE_KEY: OS_DebugLog("[OnInputEvent] AINPUT_EVENT_TYPE_KEY"); break;
		//case AINPUT_EVENT_TYPE_FOCUS: OS_DebugLog("[OnInputEvent] AINPUT_EVENT_TYPE_FOCUS"); break;
		//case AINPUT_EVENT_TYPE_CAPTURE: OS_DebugLog("[OnInputEvent] AINPUT_EVENT_TYPE_CAPTURE"); break;
		//case AINPUT_EVENT_TYPE_DRAG: OS_DebugLog("[OnInputEvent] AINPUT_EVENT_TYPE_DRAG"); break;
	}
	
	return 0;
}

static void
OnAppCommand_(struct android_app* app, int32_t cmd)
{
	switch (cmd)
	{
		case APP_CMD_INIT_WINDOW:
		{
			g_android.can_initialize_window = true;
		} break;
		
		case APP_CMD_TERM_WINDOW:
		{
			g_android.state.is_terminating = true;
		} break;
		
		case APP_CMD_GAINED_FOCUS:
		{
			g_android.has_focus = true;
		} break;
		
		case APP_CMD_LOST_FOCUS:
		{
			g_android.has_focus = false;
		} break;
	}
}

static void
ExitApp_(bool should_call_exit)
{
	if (g_android.audio_stream)
	{
		AAudioStream_requestStop(g_android.audio_stream);
		AAudioStream_close(g_android.audio_stream);
		
		g_android.audio_stream = NULL;
	}
	
	if (g_android.gl_context)
	{
		eglMakeCurrent(g_android.gl_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
		eglDestroyContext(g_android.gl_display, g_android.gl_context);
		eglDestroySurface(g_android.gl_display, g_android.gl_surface);
		eglTerminate(g_android.gl_display);
		
		g_android.gl_display = NULL;
		g_android.gl_context = NULL;
		g_android.gl_surface = NULL;
	}
	
	ANativeActivity_finish(g_android.app_state->activity);
	
	while (!g_android.state.is_terminating)
		OS_PollEvents();
	
	if (should_call_exit)
		exit(0);
}

static void
MessageBox_(const char* title, const char* message)
{
	// TODO(ljre): Debug this. It only reaches "Step 1" before crashing.
	return;
	
	// NOTE(ljre): Here are some useful links for the future if I need to modify this:
	// 
	// https://docs.oracle.com/javase/8/docs/technotes/guides/jni/spec/types.html#field_and_method_ids
	// https://docs.oracle.com/javase/8/docs/technotes/guides/jni/spec/functions.html#interface_function_table
	
	JNIEnv* env = g_android.app_state->activity->env;
	jobject activity_object = g_android.app_state->activity->clazz;
	jobject builder_object;
	jobject dialog_object;
	jstring title_string;
	jstring message_string;
	jclass builder_class;
	jclass dialog_class;
	jmethodID builder_class_ctor;
	jmethodID builder_set_title;
	jmethodID builder_set_text;
	jmethodID builder_set_positive_button;
	jmethodID builder_create;
	jmethodID dialog_show;
	jmethodID dialog_is_showing;
	
	OS_DebugLog("Step 1\n");
	
	// NOTE(ljre): Get classes and method
	builder_class = (*env)->FindClass(env, "com/google/android/material/dialog/MaterialAlertDialogBuilder");
	dialog_class = (*env)->FindClass(env, "androidx/appcompat/app/AlertDialog");
	if (!builder_class)
		return;
	
	OS_DebugLog("Step 2\n");
	builder_class_ctor = (*env)->GetMethodID(env, builder_class, "<init>", "(Landroid/content/Context;)");
	builder_set_title  = (*env)->GetMethodID(env, builder_class, "setTitle", "(Ljava/lang/CharSequence;)Lcom/google/android/material/dialog/MaterialAlertDialogBuilder;");
	builder_set_text   = (*env)->GetMethodID(env, builder_class, "setText", "(Ljava/lang/CharSequence;)Lcom/google/android/material/dialog/MaterialAlertDialogBuilder;");
	builder_set_positive_button = (*env)->GetMethodID(env, builder_class, "setPositiveButton", "(Ljava/lang/CharSequence;Landroid/content/DialogInterface/OnClickListener;)Lcom/google/android/material/dialog/MaterialAlertDialogBuilder;");
	builder_create     = (*env)->GetMethodID(env, builder_class, "create", "()Landroidx/appcompat/app/AlertDialog;");
	dialog_show        = (*env)->GetMethodID(env, dialog_class, "show", "()");
	dialog_is_showing  = (*env)->GetMethodID(env, dialog_class, "isShowing", "()Z");
	title_string       = (*env)->NewStringUTF(env, title); // NOTE(ljre): This one may leak if the next fails.
	message_string     = (*env)->NewStringUTF(env, message);
	if (!builder_class_ctor || !builder_set_title || !builder_set_text ||
		!builder_create || !title_string || !message_string)
		return;
	
	OS_DebugLog("Step 3\n");
	
	//- NOTE(ljre): Actually calling the API
	builder_object = (*env)->NewObject(env, builder_class, builder_class_ctor, activity_object);
	(*env)->CallObjectMethod(env, builder_object, builder_set_title, title_string);
	(*env)->CallObjectMethod(env, builder_object, builder_set_text, message_string);
	dialog_object = (*env)->CallObjectMethod(env, builder_object, builder_create);
	(*env)->CallVoidMethod(env, dialog_object, dialog_show);
	
	OS_DebugLog("Step 4\n");
	
	// NOTE(ljre): Block until dialog is closed.
	int32 ident;
	int32 events;
	struct android_poll_source* source;
	struct android_app* state = g_android.app_state;
	
	while ((ident = ALooper_pollAll(-1, NULL, &events, (void**)&source)) >= 0)
	{
		if (source != NULL)
			source->process(state, source);
		
		if (state->destroyRequested != 0)
		{
			g_android.state.window.should_close = true;
			break;
		}
		
		jboolean is_showing = (*env)->CallBooleanMethod(env, dialog_object, dialog_is_showing);
		if (!is_showing)
			break;
	}
	
	OS_DebugLog("Step 5\n");
	// NOTE(ljre): Clean-up
	(*env)->ReleaseStringUTFChars(env, title_string, title);
	(*env)->ReleaseStringUTFChars(env, message_string, message);
}

void
android_main(struct android_app* state)
{
	Trace();
	
	DisableWarnings();
	app_dummy();
	ReenableWarnings();
	
	clock_gettime(CLOCK_MONOTONIC, &g_android.started_time);
	
	g_android.app_state = state;
	state->onAppCmd = OnAppCommand_;
	state->onInputEvent = OnInputEvent_;
	
	while (!g_android.can_initialize_window)
		OS_PollEvents();
	
	OS_UserMainArgs user_args = {
		.thread_id = 0,
		.thread_count = 1,
		
		.argc = 1,
		.argv = (const char*[]) { "app" },
	};
	
	int32 result = OS_UserMain(&user_args);
	(void)result;
	
	ExitApp_(false);
}

API bool
OS_Init(const OS_InitDesc* desc, OS_State** out_state)
{
	Trace();
	
	const EGLint attribs[] =
	{
		EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
		EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT,
		EGL_BLUE_SIZE, 8,
		EGL_GREEN_SIZE, 8,
		EGL_RED_SIZE, 8,
		EGL_DEPTH_SIZE, 24,
		EGL_STENCIL_SIZE, 8,
		EGL_NONE,
	};
	
	EGLDisplay display;
	if ((display = eglGetDisplay(EGL_DEFAULT_DISPLAY)) == EGL_NO_DISPLAY)
		return false;
	
	if (!eglInitialize(display, 0, 0))
		return false;
	
	EGLConfig config;
	EGLint numConfigs;
	if (!eglChooseConfig(display, attribs, &config, 1, &numConfigs))
		return false;
	
	EGLint format;
	if (!eglGetConfigAttrib(display, config, EGL_NATIVE_VISUAL_ID, &format))
		return false;
	
	ANativeWindow_setBuffersGeometry(g_android.app_state->window, 0, 0, format);
	
	EGLSurface surface;
	if (!(surface = eglCreateWindowSurface(display, config, g_android.app_state->window, NULL)))
		return false;
	
	const EGLint ctx_attrib[] = { EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE };
	EGLContext context;
	if (!(context = eglCreateContext(display, config, NULL, ctx_attrib)))
		return false;
	
	if (eglMakeCurrent(display, surface, surface, context) == EGL_FALSE)
		return false;
	
	if (!LoadOpenGLFunctions_())
		return false;
	
	g_android.gl_display = display;
	g_android.gl_context = context;
	g_android.gl_surface = surface;
	g_android.graphics_context.api = OS_WindowGraphicsApi_OpenGL;
	g_android.graphics_context.opengl = &g_android.opengl_graphics_context;
	g_android.graphics_context.present_and_vsync = PresentAndVsync_;
	
	int32 width = 0, height = 0;
	eglQuerySurface(display, surface, EGL_WIDTH, &width);
	eglQuerySurface(display, surface, EGL_HEIGHT, &height);
	
	g_android.state.has_audio = InitAudio_(desc);
	g_android.state.has_keyboard = false;
	g_android.state.has_mouse = false;
	g_android.state.has_gestures = true;
	g_android.state.is_terminating = false;
	
	g_android.state.graphics_context = &g_android.graphics_context;
	g_android.state.window = (OS_WindowState) {
		.should_close = false,
		.fullscreen = true,
		.width = width,
		.height = height,
	};
	
	*out_state = &g_android.state;
	
	return true;
}

API void
OS_PollEvents(void)
{
	Trace();
	
	g_android.state.mouse.old_pos[0] = g_android.state.mouse.pos[0];
	g_android.state.mouse.old_pos[1] = g_android.state.mouse.pos[1];
	for (intsize i = 0; i < ArrayLength(g_android.state.mouse.buttons); ++i)
		g_android.state.mouse.buttons[i].changes = 0;
	
	int32 ident;
	int32 events;
	struct android_poll_source* source;
	struct android_app* state = g_android.app_state;
	
	// NOTE(ljre): If app is not in focus, timeout is -1 (block until next event)
	while ((ident = ALooper_pollAll(!g_android.has_focus ? -1 : 0, NULL, &events, (void**)&source)) >= 0)
	{
		if (source != NULL)
			source->process(state, source);
		
		if (state->destroyRequested != 0)
		{
			g_android.state.window.should_close = true;
			return;
		}
	}
}

API void
OS_ExitWithErrorMessage(const char* fmt, ...)
{
	Trace();
	Arena* scratch_arena = GetThreadScratchArena_();
	
	for Arena_TempScope(scratch_arena)
	{
		va_list args;
		va_start(args, fmt);
		String str = Arena_VPrintf(scratch_arena, fmt, args);
		Arena_PushData(scratch_arena, &"");
		va_end(args);
		
		//MessageBox_("Fatal Error!", (const char*)str.data);
		__android_log_print(ANDROID_LOG_INFO, "NativeExample", "%s", (const char*)str.data);
	}
	
	ExitApp_(true);
}

API void
OS_MessageBox(String title, String message)
{
	Trace();
	Arena* scratch_arena = GetThreadScratchArena_();
	
	for Arena_TempScope(scratch_arena)
	{
		const char* title_cstr = Arena_PushCString(scratch_arena, title);
		const char* message_cstr = Arena_PushCString(scratch_arena, message);
		
		MessageBox_(title_cstr, message_cstr);
	}
}

API uint64
OS_CurrentPosixTime(void)
{
	time_t result = time(NULL);
	if (result == -1)
		result = 0;
	
	return (uint64)result;
}

API uint64
OS_CurrentTick(uint64* out_ticks_per_second)
{
	uint64 result = 0;
	uint64 ticks_per_second = 1000 * 1000 * 10; // 100ns granularity
	
	struct timespec t;
	if (clock_gettime(CLOCK_MONOTONIC, &t) == 0)
	{
		result += t.tv_sec * ticks_per_second;
		result += t.tv_nsec / 100;
	}
	
	if (out_ticks_per_second)
		*out_ticks_per_second = ticks_per_second;
	
	return result;
}

API float64
OS_GetTimeInSeconds(void)
{
	struct timespec t;
	if (0 != clock_gettime(CLOCK_MONOTONIC, &t))
		return 0.0;
	
	t.tv_sec -= g_android.started_time.tv_sec;
	t.tv_nsec -= g_android.started_time.tv_nsec;
	
	if (t.tv_nsec < 0)
	{
		t.tv_nsec += 1000000000L;
		t.tv_sec -= 1;
	}
	
	return (float64)t.tv_nsec / 1000000000.0 + (float64)t.tv_sec;
}

API void*
OS_HeapAlloc(uintsize size)
{
	Trace();
	
	return malloc(size);
}

API void*
OS_HeapRealloc(void* ptr, uintsize size)
{
	Trace();
	
	return realloc(ptr, size);
}

API void
OS_HeapFree(void* ptr)
{
	Trace();
	
	return free(ptr);
}

API void*
OS_VirtualReserve(void* address, uintsize size)
{
	Trace();
	
	return mmap(address, size, PROT_NONE, MAP_ANONYMOUS|MAP_PRIVATE, -1, 0);
}

API bool
OS_VirtualCommit(void* ptr, uintsize size)
{
	Trace();
	
	return mprotect(ptr, size, PROT_READ|PROT_WRITE) == 0;
}

API void
OS_VirtualDecommit(void* ptr, uintsize size)
{
	Trace();
	
	mprotect(ptr, size, PROT_NONE);
}

API void
OS_VirtualRelease(void* ptr, uintsize size)
{
	Trace();
	
	munmap(ptr, size);
}

API bool
OS_ReadEntireFile(String path, Arena* output_arena, void** out_data, uintsize* out_size)
{
	Trace();
	String asset_prefix = StrInit("assets/");
	Arena* scratch_arena = GetThreadScratchArena_();
	
	if (String_StartsWith(path, asset_prefix))
	{
		String substr = String_Substr(path, asset_prefix.size, -1);
		AAsset* asset = NULL;
		
		for Arena_TempScope(scratch_arena)
		{
			const char* cstr = Arena_PushCString(scratch_arena, substr);
			asset = AAssetManager_open(g_android.app_state->activity->assetManager, cstr, AASSET_MODE_BUFFER);
		}
		
		if (asset)
		{
			const void* data = AAsset_getBuffer(asset);
			uintsize size = AAsset_getLength(asset);
			
			*out_data = Arena_PushMemory(output_arena, data, size);
			*out_size = size;
			
			AAsset_close(asset);
			return true;
		}
		
		return false;
	}
	
	return false;
}

API bool
OS_WriteEntireFile(String path, const void* data, uintsize size)
{
	Trace();
	
	// TODO(ljre)
	return false;
}

API void*
OS_LoadDiscordLibrary(void)
{
	return NULL;
}

API void
OS_InitRWLock(OS_RWLock* lock)
{
	Trace();
	
	pthread_rwlock_t* mem = OS_HeapAlloc(sizeof(pthread_rwlock_t));
	SafeAssert(pthread_rwlock_init(mem, NULL) == 0);
	
	lock->ptr = mem;
}

API void
OS_LockShared(OS_RWLock* lock)
{
	Trace();
	
	SafeAssert(pthread_rwlock_rdlock(lock->ptr) == 0);
}

API void
OS_LockExclusive(OS_RWLock* lock)
{
	Trace();
	
	SafeAssert(pthread_rwlock_wrlock(lock->ptr) == 0);
}

API bool
OS_TryLockShared(OS_RWLock* lock)
{
	Trace();
	
	return pthread_rwlock_tryrdlock(lock->ptr) == 0;
}

API bool
OS_TryLockExclusive(OS_RWLock* lock)
{
	Trace();
	
	return pthread_rwlock_trywrlock(lock->ptr) == 0;
}

API void
OS_UnlockShared(OS_RWLock* lock)
{
	Trace();
	
	SafeAssert(pthread_rwlock_unlock(lock->ptr) == 0);
}

API void
OS_UnlockExclusive(OS_RWLock* lock)
{
	Trace();
	
	SafeAssert(pthread_rwlock_unlock(lock->ptr) == 0);
}

API void
OS_DeinitRWLock(OS_RWLock* lock)
{
	Trace();
	
	if (lock->ptr)
	{
		SafeAssert(pthread_rwlock_destroy(lock->ptr) == 0);
		OS_HeapFree(lock->ptr);
		lock->ptr = NULL;
	}
}

API void
OS_InitSemaphore(OS_Semaphore* sem, int32 max_count)
{
	Trace();
	
	sem->ptr = NULL;
	
	if (max_count > 0)
	{
		sem_t* mem = OS_HeapAlloc(sizeof(sem_t));
		SafeAssert(sem_init(mem, 0, max_count) == 0);
		sem->ptr = mem;
	}
}

API bool
OS_WaitForSemaphore(OS_Semaphore* sem)
{
	Trace();
	
	if (sem->ptr)
		return sem_wait(sem->ptr) == 0;
	
	return false;
}

API void
OS_SignalSemaphore(OS_Semaphore* sem, int32 count)
{
	Trace();
	
	if (sem->ptr)
	{
		while (count-- && sem_post(sem->ptr) == 0)
		{}
	}
}

API void
OS_DeinitSemaphore(OS_Semaphore* sem)
{
	Trace();
	
	if (sem->ptr)
	{
		sem_destroy(sem->ptr);
		OS_HeapFree(sem->ptr);
		sem->ptr = NULL;
	}
}

API void
OS_InitEventSignal(OS_EventSignal* sig)
{
	Trace();
	
	int fd = eventfd(0, 0);
	SafeAssert(fd != -1);
	sig->ptr = (void*)(uintptr)fd;
}

API bool
OS_WaitEventSignal(OS_EventSignal* sig)
{
	Trace();
	SafeAssert(sig && sig->ptr);
	
	uint64 count;
	int fd = (int)(uintptr)sig->ptr;
	
	return read(fd, &count, sizeof(count)) >= 0;
}

API void
OS_SetEventSignal(OS_EventSignal* sig)
{
	Trace();
	SafeAssert(sig && sig->ptr);
	
	uint64 count = 1;
	int fd = (int)(uintptr)sig->ptr;
	
	write(fd, &count, sizeof(count));
}

API void
OS_ResetEventSignal(OS_EventSignal* sig)
{
	Trace();
	SafeAssert(sig && sig->ptr);
	
	// TODO(ljre): Is this even remotely right?
	// https://man7.org/linux/man-pages/man2/eventfd.2.html
	
	uint64 count;
	int fd = (int)(uintptr)sig->ptr;
	struct pollfd pfd = {
		.fd = fd,
		.events = POLLIN,
	};
	
	if (poll(&pfd, 1, 0) > 0 && (pfd.revents & POLLIN) != 0)
	{
		do
			read(fd, &count, sizeof(count));
		while (count > 0);
	}
}

API void
OS_DeinitEventSignal(OS_EventSignal* sig)
{
	Trace();
	SafeAssert(sig && sig->ptr);
	
	int fd = (int)(uintptr)sig->ptr;
	close(fd);
	
	sig->ptr = NULL;
}

API int32
OS_InterlockedCompareExchange32(volatile int32* ptr, int32 new_value, int32 expected)
{
	Trace();
	
	int32 result = __sync_val_compare_and_swap(ptr, expected, new_value);
	__sync_synchronize();
	return result;
}

API int64
OS_InterlockedCompareExchange64(volatile int64* ptr, int64 new_value, int64 expected)
{
	Trace();
	
	int64 result = __sync_val_compare_and_swap(ptr, expected, new_value);
	__sync_synchronize();
	return result;
}

API void*
OS_InterlockedCompareExchangePtr(void* volatile* ptr, void* new_value, void* expected)
{
	Trace();
	
	void* result = __sync_val_compare_and_swap(ptr, expected, new_value);
	__sync_synchronize();
	return result;
}

API int32
OS_InterlockedIncrement32(volatile int32* ptr)
{
	Trace();
	
	int32 result = __sync_add_and_fetch(ptr, 1);
	__sync_synchronize();
	return result;
}

API int32
OS_InterlockedDecrement32(volatile int32* ptr)
{
	Trace();
	
	int32 result = __sync_sub_and_fetch(ptr, 1);
	__sync_synchronize();
	return result;
}

#ifdef CONFIG_DEBUG
API void
OS_DebugMessageBox(const char* fmt, ...)
{
	// TODO(ljre)
}

API void
OS_DebugLog(const char* fmt, ...)
{
	Arena* scratch_arena = GetThreadScratchArena_();
	
	for Arena_TempScope(scratch_arena)
	{
		va_list args;
		va_start(args, fmt);
		String str = Arena_VPrintf(scratch_arena, fmt, args);
		Arena_PushData(scratch_arena, &"");
		va_end(args);
		
		__android_log_print(ANDROID_LOG_INFO, "NativeExample", "%s", (char*)str.data);
	}
}

API int
OS_DebugLogPrintfFormat(const char* fmt, ...)
{
	Arena* scratch_arena = GetThreadScratchArena_();
	int result = 0;
	
	for Arena_TempScope(scratch_arena)
	{
		va_list args, args2;
		va_start(args, fmt);
		va_copy(args2, args);
		
		int count = vsnprintf(NULL, 0, fmt, args2)+1;
		char* buffer = Arena_PushDirtyAligned(scratch_arena, count, 1);
		vsnprintf(buffer, count, fmt, args);
		
		va_end(args);
		va_end(args2);
		
		__android_log_print(ANDROID_LOG_INFO, "NativeExample", "%s", buffer);
		result = count-1;
	}
	
	return result;
}
#endif //CONFIG_DEBUG
