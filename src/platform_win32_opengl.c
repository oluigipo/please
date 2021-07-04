//~ Types and Macros
#define WGL_DRAW_TO_WINDOW_ARB                  0x2001
#define WGL_SUPPORT_OPENGL_ARB                  0x2010
#define WGL_DOUBLE_BUFFER_ARB                   0x2011
#define WGL_PIXEL_TYPE_ARB                      0x2013
#define WGL_TYPE_RGBA_ARB                       0x202B
#define WGL_COLOR_BITS_ARB                      0x2014
#define WGL_DEPTH_BITS_ARB                      0x2022
#define WGL_STENCIL_BITS_ARB                    0x2023

#define WGL_CONTEXT_MAJOR_VERSION_ARB           0x2091
#define WGL_CONTEXT_MINOR_VERSION_ARB           0x2092
#define WGL_CONTEXT_PROFILE_MASK_ARB            0x9126
#define WGL_CONTEXT_CORE_PROFILE_BIT_ARB        0x0001

#define DwmIsCompositionEnabled Win32_ProcDwmIsCompositionEnabled
#define DwmFlush Win32_ProcDwnFlush

typedef HRESULT(WINAPI* PFNDWMFLUSHPROC)(void);
typedef HRESULT(WINAPI* PFNDWMISCOMPOSITIONENABLEDPROC)(BOOL*);

typedef PROC(WINAPI* PFNWGLGETPROCADDRESSPROC)(LPCSTR);
typedef HGLRC(WINAPI* PFNWGLCREATECONTEXTPROC)(HDC);
typedef BOOL(WINAPI* PFNWGLMAKECURRENTPROC)(HDC, HGLRC);
typedef BOOL(WINAPI* PFNWGLDELETECONTEXTPROC)(HGLRC);
typedef BOOL(WINAPI* PFNWGLSWAPLAYERBUFFERSPROC)(HDC, UINT);

typedef BOOL(WINAPI* PFNWGLCHOOSEPIXELFORMATARBPROC)(HDC, const int*, const FLOAT*, UINT, int*, UINT*);
typedef HGLRC(WINAPI* PFNWGLCREATECONTEXTATTRIBSARBPROC)(HDC, HGLRC, const int*);
typedef BOOL(WINAPI* PFNWGLMAKECONTEXTCURRENTARBPROC)(HDC, HDC, HGLRC);
typedef BOOL(WINAPI* PFNWGLSWAPINTERVALEXTPROC)(int);

struct Win32_OpenGL
{
    HMODULE library;
    HGLRC context;
    
    PFNWGLGETPROCADDRESSPROC wglGetProcAddress;
    PFNWGLCREATECONTEXTPROC wglCreateContext;
    PFNWGLMAKECURRENTPROC wglMakeCurrent;
    PFNWGLDELETECONTEXTPROC wglDeleteContext;
    PFNWGLSWAPLAYERBUFFERSPROC wglSwapLayerBuffers;
    
    PFNWGLCHOOSEPIXELFORMATARBPROC wglChoosePixelFormatARB;
    PFNWGLCREATECONTEXTATTRIBSARBPROC wglCreateContextAttribsARB;
    PFNWGLMAKECONTEXTCURRENTARBPROC wglMakeContextCurrentARB;
    PFNWGLSWAPINTERVALEXTPROC wglSwapIntervalEXT;
    
    GraphicsContext_OpenGL vtable;
} typedef Win32_OpenGL;

//~ Globals
internal Win32_OpenGL global_opengl;
internal bool32 global_should_flush_dwm;
internal int32 global_swap_interval;
internal PFNDWMISCOMPOSITIONENABLEDPROC DwmIsCompositionEnabled;
internal PFNDWMFLUSHPROC DwmFlush;

//~ Functions
internal void*
OpenGLGetProc(const char* name)
{
    void* result = NULL;
    
    if (global_opengl.wglGetProcAddress)
        result = global_opengl.wglGetProcAddress(name);
    
    if (!result || result == (void *)0x1 ||
        result == (void *)0x2 || result == (void *)0x3 ||
        result == (void *)-1)
    {
        result = GetProcAddress(global_opengl.library, name);
    }
    
    return result;
}

internal bool32
LoadOpenGLFunctions(void)
{
    GraphicsContext_OpenGL* opengl = &global_opengl.vtable;
    void* (*loader)(const char*) = OpenGLGetProc;
    
    opengl->glGetString = loader("glGetString");
    if (!opengl->glGetString)
        return false;
    
    const char* str = (const char*)opengl->glGetString(GL_VERSION);
    if (!str)
        return false;
    
    Platform_DebugLog("OpenGL Version: %s\n", str);
    
#ifdef DEBUG
    opengl->glDebugMessageCallback = (PFNGLDEBUGMESSAGECALLBACKPROC)loader("glDebugMessageCallback");
#endif
    
    opengl->glCullFace = (PFNGLCULLFACEPROC)loader("glCullFace");
    opengl->glFrontFace = (PFNGLFRONTFACEPROC)loader("glFrontFace");
    opengl->glHint = (PFNGLHINTPROC)loader("glHint");
    opengl->glLineWidth = (PFNGLLINEWIDTHPROC)loader("glLineWidth");
    opengl->glPointSize = (PFNGLPOINTSIZEPROC)loader("glPointSize");
    opengl->glPolygonMode = (PFNGLPOLYGONMODEPROC)loader("glPolygonMode");
    opengl->glScissor = (PFNGLSCISSORPROC)loader("glScissor");
    opengl->glTexParameterf = (PFNGLTEXPARAMETERFPROC)loader("glTexParameterf");
    opengl->glTexParameterfv = (PFNGLTEXPARAMETERFVPROC)loader("glTexParameterfv");
    opengl->glTexParameteri = (PFNGLTEXPARAMETERIPROC)loader("glTexParameteri");
    opengl->glTexParameteriv = (PFNGLTEXPARAMETERIVPROC)loader("glTexParameteriv");
    opengl->glTexImage1D = (PFNGLTEXIMAGE1DPROC)loader("glTexImage1D");
    opengl->glTexImage2D = (PFNGLTEXIMAGE2DPROC)loader("glTexImage2D");
    opengl->glDrawBuffer = (PFNGLDRAWBUFFERPROC)loader("glDrawBuffer");
    opengl->glClear = (PFNGLCLEARPROC)loader("glClear");
    opengl->glClearColor = (PFNGLCLEARCOLORPROC)loader("glClearColor");
    opengl->glClearStencil = (PFNGLCLEARSTENCILPROC)loader("glClearStencil");
    opengl->glClearDepth = (PFNGLCLEARDEPTHPROC)loader("glClearDepth");
    opengl->glStencilMask = (PFNGLSTENCILMASKPROC)loader("glStencilMask");
    opengl->glColorMask = (PFNGLCOLORMASKPROC)loader("glColorMask");
    opengl->glDepthMask = (PFNGLDEPTHMASKPROC)loader("glDepthMask");
    opengl->glDisable = (PFNGLDISABLEPROC)loader("glDisable");
    opengl->glEnable = (PFNGLENABLEPROC)loader("glEnable");
    opengl->glFinish = (PFNGLFINISHPROC)loader("glFinish");
    opengl->glFlush = (PFNGLFLUSHPROC)loader("glFlush");
    opengl->glBlendFunc = (PFNGLBLENDFUNCPROC)loader("glBlendFunc");
    opengl->glLogicOp = (PFNGLLOGICOPPROC)loader("glLogicOp");
    opengl->glStencilFunc = (PFNGLSTENCILFUNCPROC)loader("glStencilFunc");
    opengl->glStencilOp = (PFNGLSTENCILOPPROC)loader("glStencilOp");
    opengl->glDepthFunc = (PFNGLDEPTHFUNCPROC)loader("glDepthFunc");
    opengl->glPixelStoref = (PFNGLPIXELSTOREFPROC)loader("glPixelStoref");
    opengl->glPixelStorei = (PFNGLPIXELSTOREIPROC)loader("glPixelStorei");
    opengl->glReadBuffer = (PFNGLREADBUFFERPROC)loader("glReadBuffer");
    opengl->glReadPixels = (PFNGLREADPIXELSPROC)loader("glReadPixels");
    opengl->glGetBooleanv = (PFNGLGETBOOLEANVPROC)loader("glGetBooleanv");
    opengl->glGetDoublev = (PFNGLGETDOUBLEVPROC)loader("glGetDoublev");
    opengl->glGetError = (PFNGLGETERRORPROC)loader("glGetError");
    opengl->glGetFloatv = (PFNGLGETFLOATVPROC)loader("glGetFloatv");
    opengl->glGetIntegerv = (PFNGLGETINTEGERVPROC)loader("glGetIntegerv");
    opengl->glGetString = (PFNGLGETSTRINGPROC)loader("glGetString");
    opengl->glGetTexImage = (PFNGLGETTEXIMAGEPROC)loader("glGetTexImage");
    opengl->glGetTexParameterfv = (PFNGLGETTEXPARAMETERFVPROC)loader("glGetTexParameterfv");
    opengl->glGetTexParameteriv = (PFNGLGETTEXPARAMETERIVPROC)loader("glGetTexParameteriv");
    opengl->glGetTexLevelParameterfv = (PFNGLGETTEXLEVELPARAMETERFVPROC)loader("glGetTexLevelParameterfv");
    opengl->glGetTexLevelParameteriv = (PFNGLGETTEXLEVELPARAMETERIVPROC)loader("glGetTexLevelParameteriv");
    opengl->glIsEnabled = (PFNGLISENABLEDPROC)loader("glIsEnabled");
    opengl->glDepthRange = (PFNGLDEPTHRANGEPROC)loader("glDepthRange");
    opengl->glViewport = (PFNGLVIEWPORTPROC)loader("glViewport");
    opengl->glDrawArrays = (PFNGLDRAWARRAYSPROC)loader("glDrawArrays");
    opengl->glDrawElements = (PFNGLDRAWELEMENTSPROC)loader("glDrawElements");
    opengl->glPolygonOffset = (PFNGLPOLYGONOFFSETPROC)loader("glPolygonOffset");
    opengl->glCopyTexImage1D = (PFNGLCOPYTEXIMAGE1DPROC)loader("glCopyTexImage1D");
    opengl->glCopyTexImage2D = (PFNGLCOPYTEXIMAGE2DPROC)loader("glCopyTexImage2D");
    opengl->glCopyTexSubImage1D = (PFNGLCOPYTEXSUBIMAGE1DPROC)loader("glCopyTexSubImage1D");
    opengl->glCopyTexSubImage2D = (PFNGLCOPYTEXSUBIMAGE2DPROC)loader("glCopyTexSubImage2D");
    opengl->glTexSubImage1D = (PFNGLTEXSUBIMAGE1DPROC)loader("glTexSubImage1D");
    opengl->glTexSubImage2D = (PFNGLTEXSUBIMAGE2DPROC)loader("glTexSubImage2D");
    opengl->glBindTexture = (PFNGLBINDTEXTUREPROC)loader("glBindTexture");
    opengl->glDeleteTextures = (PFNGLDELETETEXTURESPROC)loader("glDeleteTextures");
    opengl->glGenTextures = (PFNGLGENTEXTURESPROC)loader("glGenTextures");
    opengl->glIsTexture = (PFNGLISTEXTUREPROC)loader("glIsTexture");
    opengl->glDrawRangeElements = (PFNGLDRAWRANGEELEMENTSPROC)loader("glDrawRangeElements");
    opengl->glTexImage3D = (PFNGLTEXIMAGE3DPROC)loader("glTexImage3D");
    opengl->glTexSubImage3D = (PFNGLTEXSUBIMAGE3DPROC)loader("glTexSubImage3D");
    opengl->glCopyTexSubImage3D = (PFNGLCOPYTEXSUBIMAGE3DPROC)loader("glCopyTexSubImage3D");
    opengl->glActiveTexture = (PFNGLACTIVETEXTUREPROC)loader("glActiveTexture");
    opengl->glSampleCoverage = (PFNGLSAMPLECOVERAGEPROC)loader("glSampleCoverage");
    opengl->glCompressedTexImage3D = (PFNGLCOMPRESSEDTEXIMAGE3DPROC)loader("glCompressedTexImage3D");
    opengl->glCompressedTexImage2D = (PFNGLCOMPRESSEDTEXIMAGE2DPROC)loader("glCompressedTexImage2D");
    opengl->glCompressedTexImage1D = (PFNGLCOMPRESSEDTEXIMAGE1DPROC)loader("glCompressedTexImage1D");
    opengl->glCompressedTexSubImage3D = (PFNGLCOMPRESSEDTEXSUBIMAGE3DPROC)loader("glCompressedTexSubImage3D");
    opengl->glCompressedTexSubImage2D = (PFNGLCOMPRESSEDTEXSUBIMAGE2DPROC)loader("glCompressedTexSubImage2D");
    opengl->glCompressedTexSubImage1D = (PFNGLCOMPRESSEDTEXSUBIMAGE1DPROC)loader("glCompressedTexSubImage1D");
    opengl->glGetCompressedTexImage = (PFNGLGETCOMPRESSEDTEXIMAGEPROC)loader("glGetCompressedTexImage");
    opengl->glBlendFuncSeparate = (PFNGLBLENDFUNCSEPARATEPROC)loader("glBlendFuncSeparate");
    opengl->glMultiDrawArrays = (PFNGLMULTIDRAWARRAYSPROC)loader("glMultiDrawArrays");
    opengl->glMultiDrawElements = (PFNGLMULTIDRAWELEMENTSPROC)loader("glMultiDrawElements");
    opengl->glPointParameterf = (PFNGLPOINTPARAMETERFPROC)loader("glPointParameterf");
    opengl->glPointParameterfv = (PFNGLPOINTPARAMETERFVPROC)loader("glPointParameterfv");
    opengl->glPointParameteri = (PFNGLPOINTPARAMETERIPROC)loader("glPointParameteri");
    opengl->glPointParameteriv = (PFNGLPOINTPARAMETERIVPROC)loader("glPointParameteriv");
    opengl->glBlendColor = (PFNGLBLENDCOLORPROC)loader("glBlendColor");
    opengl->glBlendEquation = (PFNGLBLENDEQUATIONPROC)loader("glBlendEquation");
    opengl->glGenQueries = (PFNGLGENQUERIESPROC)loader("glGenQueries");
    opengl->glDeleteQueries = (PFNGLDELETEQUERIESPROC)loader("glDeleteQueries");
    opengl->glIsQuery = (PFNGLISQUERYPROC)loader("glIsQuery");
    opengl->glBeginQuery = (PFNGLBEGINQUERYPROC)loader("glBeginQuery");
    opengl->glEndQuery = (PFNGLENDQUERYPROC)loader("glEndQuery");
    opengl->glGetQueryiv = (PFNGLGETQUERYIVPROC)loader("glGetQueryiv");
    opengl->glGetQueryObjectiv = (PFNGLGETQUERYOBJECTIVPROC)loader("glGetQueryObjectiv");
    opengl->glGetQueryObjectuiv = (PFNGLGETQUERYOBJECTUIVPROC)loader("glGetQueryObjectuiv");
    opengl->glBindBuffer = (PFNGLBINDBUFFERPROC)loader("glBindBuffer");
    opengl->glDeleteBuffers = (PFNGLDELETEBUFFERSPROC)loader("glDeleteBuffers");
    opengl->glGenBuffers = (PFNGLGENBUFFERSPROC)loader("glGenBuffers");
    opengl->glIsBuffer = (PFNGLISBUFFERPROC)loader("glIsBuffer");
    opengl->glBufferData = (PFNGLBUFFERDATAPROC)loader("glBufferData");
    opengl->glBufferSubData = (PFNGLBUFFERSUBDATAPROC)loader("glBufferSubData");
    opengl->glGetBufferSubData = (PFNGLGETBUFFERSUBDATAPROC)loader("glGetBufferSubData");
    opengl->glMapBuffer = (PFNGLMAPBUFFERPROC)loader("glMapBuffer");
    opengl->glUnmapBuffer = (PFNGLUNMAPBUFFERPROC)loader("glUnmapBuffer");
    opengl->glGetBufferParameteriv = (PFNGLGETBUFFERPARAMETERIVPROC)loader("glGetBufferParameteriv");
    opengl->glGetBufferPointerv = (PFNGLGETBUFFERPOINTERVPROC)loader("glGetBufferPointerv");
    opengl->glBlendEquationSeparate = (PFNGLBLENDEQUATIONSEPARATEPROC)loader("glBlendEquationSeparate");
    opengl->glDrawBuffers = (PFNGLDRAWBUFFERSPROC)loader("glDrawBuffers");
    opengl->glStencilOpSeparate = (PFNGLSTENCILOPSEPARATEPROC)loader("glStencilOpSeparate");
    opengl->glStencilFuncSeparate = (PFNGLSTENCILFUNCSEPARATEPROC)loader("glStencilFuncSeparate");
    opengl->glStencilMaskSeparate = (PFNGLSTENCILMASKSEPARATEPROC)loader("glStencilMaskSeparate");
    opengl->glAttachShader = (PFNGLATTACHSHADERPROC)loader("glAttachShader");
    opengl->glBindAttribLocation = (PFNGLBINDATTRIBLOCATIONPROC)loader("glBindAttribLocation");
    opengl->glCompileShader = (PFNGLCOMPILESHADERPROC)loader("glCompileShader");
    opengl->glCreateProgram = (PFNGLCREATEPROGRAMPROC)loader("glCreateProgram");
    opengl->glCreateShader = (PFNGLCREATESHADERPROC)loader("glCreateShader");
    opengl->glDeleteProgram = (PFNGLDELETEPROGRAMPROC)loader("glDeleteProgram");
    opengl->glDeleteShader = (PFNGLDELETESHADERPROC)loader("glDeleteShader");
    opengl->glDetachShader = (PFNGLDETACHSHADERPROC)loader("glDetachShader");
    opengl->glDisableVertexAttribArray = (PFNGLDISABLEVERTEXATTRIBARRAYPROC)loader("glDisableVertexAttribArray");
    opengl->glEnableVertexAttribArray = (PFNGLENABLEVERTEXATTRIBARRAYPROC)loader("glEnableVertexAttribArray");
    opengl->glGetActiveAttrib = (PFNGLGETACTIVEATTRIBPROC)loader("glGetActiveAttrib");
    opengl->glGetActiveUniform = (PFNGLGETACTIVEUNIFORMPROC)loader("glGetActiveUniform");
    opengl->glGetAttachedShaders = (PFNGLGETATTACHEDSHADERSPROC)loader("glGetAttachedShaders");
    opengl->glGetAttribLocation = (PFNGLGETATTRIBLOCATIONPROC)loader("glGetAttribLocation");
    opengl->glGetProgramiv = (PFNGLGETPROGRAMIVPROC)loader("glGetProgramiv");
    opengl->glGetProgramInfoLog = (PFNGLGETPROGRAMINFOLOGPROC)loader("glGetProgramInfoLog");
    opengl->glGetShaderiv = (PFNGLGETSHADERIVPROC)loader("glGetShaderiv");
    opengl->glGetShaderInfoLog = (PFNGLGETSHADERINFOLOGPROC)loader("glGetShaderInfoLog");
    opengl->glGetShaderSource = (PFNGLGETSHADERSOURCEPROC)loader("glGetShaderSource");
    opengl->glGetUniformLocation = (PFNGLGETUNIFORMLOCATIONPROC)loader("glGetUniformLocation");
    opengl->glGetUniformfv = (PFNGLGETUNIFORMFVPROC)loader("glGetUniformfv");
    opengl->glGetUniformiv = (PFNGLGETUNIFORMIVPROC)loader("glGetUniformiv");
    opengl->glGetVertexAttribdv = (PFNGLGETVERTEXATTRIBDVPROC)loader("glGetVertexAttribdv");
    opengl->glGetVertexAttribfv = (PFNGLGETVERTEXATTRIBFVPROC)loader("glGetVertexAttribfv");
    opengl->glGetVertexAttribiv = (PFNGLGETVERTEXATTRIBIVPROC)loader("glGetVertexAttribiv");
    opengl->glGetVertexAttribPointerv = (PFNGLGETVERTEXATTRIBPOINTERVPROC)loader("glGetVertexAttribPointerv");
    opengl->glIsProgram = (PFNGLISPROGRAMPROC)loader("glIsProgram");
    opengl->glIsShader = (PFNGLISSHADERPROC)loader("glIsShader");
    opengl->glLinkProgram = (PFNGLLINKPROGRAMPROC)loader("glLinkProgram");
    opengl->glShaderSource = (PFNGLSHADERSOURCEPROC)loader("glShaderSource");
    opengl->glUseProgram = (PFNGLUSEPROGRAMPROC)loader("glUseProgram");
    opengl->glUniform1f = (PFNGLUNIFORM1FPROC)loader("glUniform1f");
    opengl->glUniform2f = (PFNGLUNIFORM2FPROC)loader("glUniform2f");
    opengl->glUniform3f = (PFNGLUNIFORM3FPROC)loader("glUniform3f");
    opengl->glUniform4f = (PFNGLUNIFORM4FPROC)loader("glUniform4f");
    opengl->glUniform1i = (PFNGLUNIFORM1IPROC)loader("glUniform1i");
    opengl->glUniform2i = (PFNGLUNIFORM2IPROC)loader("glUniform2i");
    opengl->glUniform3i = (PFNGLUNIFORM3IPROC)loader("glUniform3i");
    opengl->glUniform4i = (PFNGLUNIFORM4IPROC)loader("glUniform4i");
    opengl->glUniform1fv = (PFNGLUNIFORM1FVPROC)loader("glUniform1fv");
    opengl->glUniform2fv = (PFNGLUNIFORM2FVPROC)loader("glUniform2fv");
    opengl->glUniform3fv = (PFNGLUNIFORM3FVPROC)loader("glUniform3fv");
    opengl->glUniform4fv = (PFNGLUNIFORM4FVPROC)loader("glUniform4fv");
    opengl->glUniform1iv = (PFNGLUNIFORM1IVPROC)loader("glUniform1iv");
    opengl->glUniform2iv = (PFNGLUNIFORM2IVPROC)loader("glUniform2iv");
    opengl->glUniform3iv = (PFNGLUNIFORM3IVPROC)loader("glUniform3iv");
    opengl->glUniform4iv = (PFNGLUNIFORM4IVPROC)loader("glUniform4iv");
    opengl->glUniformMatrix2fv = (PFNGLUNIFORMMATRIX2FVPROC)loader("glUniformMatrix2fv");
    opengl->glUniformMatrix3fv = (PFNGLUNIFORMMATRIX3FVPROC)loader("glUniformMatrix3fv");
    opengl->glUniformMatrix4fv = (PFNGLUNIFORMMATRIX4FVPROC)loader("glUniformMatrix4fv");
    opengl->glValidateProgram = (PFNGLVALIDATEPROGRAMPROC)loader("glValidateProgram");
    opengl->glVertexAttrib1d = (PFNGLVERTEXATTRIB1DPROC)loader("glVertexAttrib1d");
    opengl->glVertexAttrib1dv = (PFNGLVERTEXATTRIB1DVPROC)loader("glVertexAttrib1dv");
    opengl->glVertexAttrib1f = (PFNGLVERTEXATTRIB1FPROC)loader("glVertexAttrib1f");
    opengl->glVertexAttrib1fv = (PFNGLVERTEXATTRIB1FVPROC)loader("glVertexAttrib1fv");
    opengl->glVertexAttrib1s = (PFNGLVERTEXATTRIB1SPROC)loader("glVertexAttrib1s");
    opengl->glVertexAttrib1sv = (PFNGLVERTEXATTRIB1SVPROC)loader("glVertexAttrib1sv");
    opengl->glVertexAttrib2d = (PFNGLVERTEXATTRIB2DPROC)loader("glVertexAttrib2d");
    opengl->glVertexAttrib2dv = (PFNGLVERTEXATTRIB2DVPROC)loader("glVertexAttrib2dv");
    opengl->glVertexAttrib2f = (PFNGLVERTEXATTRIB2FPROC)loader("glVertexAttrib2f");
    opengl->glVertexAttrib2fv = (PFNGLVERTEXATTRIB2FVPROC)loader("glVertexAttrib2fv");
    opengl->glVertexAttrib2s = (PFNGLVERTEXATTRIB2SPROC)loader("glVertexAttrib2s");
    opengl->glVertexAttrib2sv = (PFNGLVERTEXATTRIB2SVPROC)loader("glVertexAttrib2sv");
    opengl->glVertexAttrib3d = (PFNGLVERTEXATTRIB3DPROC)loader("glVertexAttrib3d");
    opengl->glVertexAttrib3dv = (PFNGLVERTEXATTRIB3DVPROC)loader("glVertexAttrib3dv");
    opengl->glVertexAttrib3f = (PFNGLVERTEXATTRIB3FPROC)loader("glVertexAttrib3f");
    opengl->glVertexAttrib3fv = (PFNGLVERTEXATTRIB3FVPROC)loader("glVertexAttrib3fv");
    opengl->glVertexAttrib3s = (PFNGLVERTEXATTRIB3SPROC)loader("glVertexAttrib3s");
    opengl->glVertexAttrib3sv = (PFNGLVERTEXATTRIB3SVPROC)loader("glVertexAttrib3sv");
    opengl->glVertexAttrib4Nbv = (PFNGLVERTEXATTRIB4NBVPROC)loader("glVertexAttrib4Nbv");
    opengl->glVertexAttrib4Niv = (PFNGLVERTEXATTRIB4NIVPROC)loader("glVertexAttrib4Niv");
    opengl->glVertexAttrib4Nsv = (PFNGLVERTEXATTRIB4NSVPROC)loader("glVertexAttrib4Nsv");
    opengl->glVertexAttrib4Nub = (PFNGLVERTEXATTRIB4NUBPROC)loader("glVertexAttrib4Nub");
    opengl->glVertexAttrib4Nubv = (PFNGLVERTEXATTRIB4NUBVPROC)loader("glVertexAttrib4Nubv");
    opengl->glVertexAttrib4Nuiv = (PFNGLVERTEXATTRIB4NUIVPROC)loader("glVertexAttrib4Nuiv");
    opengl->glVertexAttrib4Nusv = (PFNGLVERTEXATTRIB4NUSVPROC)loader("glVertexAttrib4Nusv");
    opengl->glVertexAttrib4bv = (PFNGLVERTEXATTRIB4BVPROC)loader("glVertexAttrib4bv");
    opengl->glVertexAttrib4d = (PFNGLVERTEXATTRIB4DPROC)loader("glVertexAttrib4d");
    opengl->glVertexAttrib4dv = (PFNGLVERTEXATTRIB4DVPROC)loader("glVertexAttrib4dv");
    opengl->glVertexAttrib4f = (PFNGLVERTEXATTRIB4FPROC)loader("glVertexAttrib4f");
    opengl->glVertexAttrib4fv = (PFNGLVERTEXATTRIB4FVPROC)loader("glVertexAttrib4fv");
    opengl->glVertexAttrib4iv = (PFNGLVERTEXATTRIB4IVPROC)loader("glVertexAttrib4iv");
    opengl->glVertexAttrib4s = (PFNGLVERTEXATTRIB4SPROC)loader("glVertexAttrib4s");
    opengl->glVertexAttrib4sv = (PFNGLVERTEXATTRIB4SVPROC)loader("glVertexAttrib4sv");
    opengl->glVertexAttrib4ubv = (PFNGLVERTEXATTRIB4UBVPROC)loader("glVertexAttrib4ubv");
    opengl->glVertexAttrib4uiv = (PFNGLVERTEXATTRIB4UIVPROC)loader("glVertexAttrib4uiv");
    opengl->glVertexAttrib4usv = (PFNGLVERTEXATTRIB4USVPROC)loader("glVertexAttrib4usv");
    opengl->glVertexAttribPointer = (PFNGLVERTEXATTRIBPOINTERPROC)loader("glVertexAttribPointer");
    opengl->glUniformMatrix2x3fv = (PFNGLUNIFORMMATRIX2X3FVPROC)loader("glUniformMatrix2x3fv");
    opengl->glUniformMatrix3x2fv = (PFNGLUNIFORMMATRIX3X2FVPROC)loader("glUniformMatrix3x2fv");
    opengl->glUniformMatrix2x4fv = (PFNGLUNIFORMMATRIX2X4FVPROC)loader("glUniformMatrix2x4fv");
    opengl->glUniformMatrix4x2fv = (PFNGLUNIFORMMATRIX4X2FVPROC)loader("glUniformMatrix4x2fv");
    opengl->glUniformMatrix3x4fv = (PFNGLUNIFORMMATRIX3X4FVPROC)loader("glUniformMatrix3x4fv");
    opengl->glUniformMatrix4x3fv = (PFNGLUNIFORMMATRIX4X3FVPROC)loader("glUniformMatrix4x3fv");
    opengl->glColorMaski = (PFNGLCOLORMASKIPROC)loader("glColorMaski");
    opengl->glGetBooleani_v = (PFNGLGETBOOLEANI_VPROC)loader("glGetBooleani_v");
    opengl->glGetIntegeri_v = (PFNGLGETINTEGERI_VPROC)loader("glGetIntegeri_v");
    opengl->glEnablei = (PFNGLENABLEIPROC)loader("glEnablei");
    opengl->glDisablei = (PFNGLDISABLEIPROC)loader("glDisablei");
    opengl->glIsEnabledi = (PFNGLISENABLEDIPROC)loader("glIsEnabledi");
    opengl->glBeginTransformFeedback = (PFNGLBEGINTRANSFORMFEEDBACKPROC)loader("glBeginTransformFeedback");
    opengl->glEndTransformFeedback = (PFNGLENDTRANSFORMFEEDBACKPROC)loader("glEndTransformFeedback");
    opengl->glBindBufferRange = (PFNGLBINDBUFFERRANGEPROC)loader("glBindBufferRange");
    opengl->glBindBufferBase = (PFNGLBINDBUFFERBASEPROC)loader("glBindBufferBase");
    opengl->glTransformFeedbackVaryings = (PFNGLTRANSFORMFEEDBACKVARYINGSPROC)loader("glTransformFeedbackVaryings");
    opengl->glGetTransformFeedbackVarying = (PFNGLGETTRANSFORMFEEDBACKVARYINGPROC)loader("glGetTransformFeedbackVarying");
    opengl->glClampColor = (PFNGLCLAMPCOLORPROC)loader("glClampColor");
    opengl->glBeginConditionalRender = (PFNGLBEGINCONDITIONALRENDERPROC)loader("glBeginConditionalRender");
    opengl->glEndConditionalRender = (PFNGLENDCONDITIONALRENDERPROC)loader("glEndConditionalRender");
    opengl->glVertexAttribIPointer = (PFNGLVERTEXATTRIBIPOINTERPROC)loader("glVertexAttribIPointer");
    opengl->glGetVertexAttribIiv = (PFNGLGETVERTEXATTRIBIIVPROC)loader("glGetVertexAttribIiv");
    opengl->glGetVertexAttribIuiv = (PFNGLGETVERTEXATTRIBIUIVPROC)loader("glGetVertexAttribIuiv");
    opengl->glVertexAttribI1i = (PFNGLVERTEXATTRIBI1IPROC)loader("glVertexAttribI1i");
    opengl->glVertexAttribI2i = (PFNGLVERTEXATTRIBI2IPROC)loader("glVertexAttribI2i");
    opengl->glVertexAttribI3i = (PFNGLVERTEXATTRIBI3IPROC)loader("glVertexAttribI3i");
    opengl->glVertexAttribI4i = (PFNGLVERTEXATTRIBI4IPROC)loader("glVertexAttribI4i");
    opengl->glVertexAttribI1ui = (PFNGLVERTEXATTRIBI1UIPROC)loader("glVertexAttribI1ui");
    opengl->glVertexAttribI2ui = (PFNGLVERTEXATTRIBI2UIPROC)loader("glVertexAttribI2ui");
    opengl->glVertexAttribI3ui = (PFNGLVERTEXATTRIBI3UIPROC)loader("glVertexAttribI3ui");
    opengl->glVertexAttribI4ui = (PFNGLVERTEXATTRIBI4UIPROC)loader("glVertexAttribI4ui");
    opengl->glVertexAttribI1iv = (PFNGLVERTEXATTRIBI1IVPROC)loader("glVertexAttribI1iv");
    opengl->glVertexAttribI2iv = (PFNGLVERTEXATTRIBI2IVPROC)loader("glVertexAttribI2iv");
    opengl->glVertexAttribI3iv = (PFNGLVERTEXATTRIBI3IVPROC)loader("glVertexAttribI3iv");
    opengl->glVertexAttribI4iv = (PFNGLVERTEXATTRIBI4IVPROC)loader("glVertexAttribI4iv");
    opengl->glVertexAttribI1uiv = (PFNGLVERTEXATTRIBI1UIVPROC)loader("glVertexAttribI1uiv");
    opengl->glVertexAttribI2uiv = (PFNGLVERTEXATTRIBI2UIVPROC)loader("glVertexAttribI2uiv");
    opengl->glVertexAttribI3uiv = (PFNGLVERTEXATTRIBI3UIVPROC)loader("glVertexAttribI3uiv");
    opengl->glVertexAttribI4uiv = (PFNGLVERTEXATTRIBI4UIVPROC)loader("glVertexAttribI4uiv");
    opengl->glVertexAttribI4bv = (PFNGLVERTEXATTRIBI4BVPROC)loader("glVertexAttribI4bv");
    opengl->glVertexAttribI4sv = (PFNGLVERTEXATTRIBI4SVPROC)loader("glVertexAttribI4sv");
    opengl->glVertexAttribI4ubv = (PFNGLVERTEXATTRIBI4UBVPROC)loader("glVertexAttribI4ubv");
    opengl->glVertexAttribI4usv = (PFNGLVERTEXATTRIBI4USVPROC)loader("glVertexAttribI4usv");
    opengl->glGetUniformuiv = (PFNGLGETUNIFORMUIVPROC)loader("glGetUniformuiv");
    opengl->glBindFragDataLocation = (PFNGLBINDFRAGDATALOCATIONPROC)loader("glBindFragDataLocation");
    opengl->glGetFragDataLocation = (PFNGLGETFRAGDATALOCATIONPROC)loader("glGetFragDataLocation");
    opengl->glUniform1ui = (PFNGLUNIFORM1UIPROC)loader("glUniform1ui");
    opengl->glUniform2ui = (PFNGLUNIFORM2UIPROC)loader("glUniform2ui");
    opengl->glUniform3ui = (PFNGLUNIFORM3UIPROC)loader("glUniform3ui");
    opengl->glUniform4ui = (PFNGLUNIFORM4UIPROC)loader("glUniform4ui");
    opengl->glUniform1uiv = (PFNGLUNIFORM1UIVPROC)loader("glUniform1uiv");
    opengl->glUniform2uiv = (PFNGLUNIFORM2UIVPROC)loader("glUniform2uiv");
    opengl->glUniform3uiv = (PFNGLUNIFORM3UIVPROC)loader("glUniform3uiv");
    opengl->glUniform4uiv = (PFNGLUNIFORM4UIVPROC)loader("glUniform4uiv");
    opengl->glTexParameterIiv = (PFNGLTEXPARAMETERIIVPROC)loader("glTexParameterIiv");
    opengl->glTexParameterIuiv = (PFNGLTEXPARAMETERIUIVPROC)loader("glTexParameterIuiv");
    opengl->glGetTexParameterIiv = (PFNGLGETTEXPARAMETERIIVPROC)loader("glGetTexParameterIiv");
    opengl->glGetTexParameterIuiv = (PFNGLGETTEXPARAMETERIUIVPROC)loader("glGetTexParameterIuiv");
    opengl->glClearBufferiv = (PFNGLCLEARBUFFERIVPROC)loader("glClearBufferiv");
    opengl->glClearBufferuiv = (PFNGLCLEARBUFFERUIVPROC)loader("glClearBufferuiv");
    opengl->glClearBufferfv = (PFNGLCLEARBUFFERFVPROC)loader("glClearBufferfv");
    opengl->glClearBufferfi = (PFNGLCLEARBUFFERFIPROC)loader("glClearBufferfi");
    opengl->glGetStringi = (PFNGLGETSTRINGIPROC)loader("glGetStringi");
    opengl->glIsRenderbuffer = (PFNGLISRENDERBUFFERPROC)loader("glIsRenderbuffer");
    opengl->glBindRenderbuffer = (PFNGLBINDRENDERBUFFERPROC)loader("glBindRenderbuffer");
    opengl->glDeleteRenderbuffers = (PFNGLDELETERENDERBUFFERSPROC)loader("glDeleteRenderbuffers");
    opengl->glGenRenderbuffers = (PFNGLGENRENDERBUFFERSPROC)loader("glGenRenderbuffers");
    opengl->glRenderbufferStorage = (PFNGLRENDERBUFFERSTORAGEPROC)loader("glRenderbufferStorage");
    opengl->glGetRenderbufferParameteriv = (PFNGLGETRENDERBUFFERPARAMETERIVPROC)loader("glGetRenderbufferParameteriv");
    opengl->glIsFramebuffer = (PFNGLISFRAMEBUFFERPROC)loader("glIsFramebuffer");
    opengl->glBindFramebuffer = (PFNGLBINDFRAMEBUFFERPROC)loader("glBindFramebuffer");
    opengl->glDeleteFramebuffers = (PFNGLDELETEFRAMEBUFFERSPROC)loader("glDeleteFramebuffers");
    opengl->glGenFramebuffers = (PFNGLGENFRAMEBUFFERSPROC)loader("glGenFramebuffers");
    opengl->glCheckFramebufferStatus = (PFNGLCHECKFRAMEBUFFERSTATUSPROC)loader("glCheckFramebufferStatus");
    opengl->glFramebufferTexture1D = (PFNGLFRAMEBUFFERTEXTURE1DPROC)loader("glFramebufferTexture1D");
    opengl->glFramebufferTexture2D = (PFNGLFRAMEBUFFERTEXTURE2DPROC)loader("glFramebufferTexture2D");
    opengl->glFramebufferTexture3D = (PFNGLFRAMEBUFFERTEXTURE3DPROC)loader("glFramebufferTexture3D");
    opengl->glFramebufferRenderbuffer = (PFNGLFRAMEBUFFERRENDERBUFFERPROC)loader("glFramebufferRenderbuffer");
    opengl->glGetFramebufferAttachmentParameteriv = (PFNGLGETFRAMEBUFFERATTACHMENTPARAMETERIVPROC)loader("glGetFramebufferAttachmentParameteriv");
    opengl->glGenerateMipmap = (PFNGLGENERATEMIPMAPPROC)loader("glGenerateMipmap");
    opengl->glBlitFramebuffer = (PFNGLBLITFRAMEBUFFERPROC)loader("glBlitFramebuffer");
    opengl->glRenderbufferStorageMultisample = (PFNGLRENDERBUFFERSTORAGEMULTISAMPLEPROC)loader("glRenderbufferStorageMultisample");
    opengl->glFramebufferTextureLayer = (PFNGLFRAMEBUFFERTEXTURELAYERPROC)loader("glFramebufferTextureLayer");
    opengl->glMapBufferRange = (PFNGLMAPBUFFERRANGEPROC)loader("glMapBufferRange");
    opengl->glFlushMappedBufferRange = (PFNGLFLUSHMAPPEDBUFFERRANGEPROC)loader("glFlushMappedBufferRange");
    opengl->glBindVertexArray = (PFNGLBINDVERTEXARRAYPROC)loader("glBindVertexArray");
    opengl->glDeleteVertexArrays = (PFNGLDELETEVERTEXARRAYSPROC)loader("glDeleteVertexArrays");
    opengl->glGenVertexArrays = (PFNGLGENVERTEXARRAYSPROC)loader("glGenVertexArrays");
    opengl->glIsVertexArray = (PFNGLISVERTEXARRAYPROC)loader("glIsVertexArray");
    opengl->glDrawArraysInstanced = (PFNGLDRAWARRAYSINSTANCEDPROC)loader("glDrawArraysInstanced");
    opengl->glDrawElementsInstanced = (PFNGLDRAWELEMENTSINSTANCEDPROC)loader("glDrawElementsInstanced");
    opengl->glTexBuffer = (PFNGLTEXBUFFERPROC)loader("glTexBuffer");
    opengl->glPrimitiveRestartIndex = (PFNGLPRIMITIVERESTARTINDEXPROC)loader("glPrimitiveRestartIndex");
    opengl->glCopyBufferSubData = (PFNGLCOPYBUFFERSUBDATAPROC)loader("glCopyBufferSubData");
    opengl->glGetUniformIndices = (PFNGLGETUNIFORMINDICESPROC)loader("glGetUniformIndices");
    opengl->glGetActiveUniformsiv = (PFNGLGETACTIVEUNIFORMSIVPROC)loader("glGetActiveUniformsiv");
    opengl->glGetActiveUniformName = (PFNGLGETACTIVEUNIFORMNAMEPROC)loader("glGetActiveUniformName");
    opengl->glGetUniformBlockIndex = (PFNGLGETUNIFORMBLOCKINDEXPROC)loader("glGetUniformBlockIndex");
    opengl->glGetActiveUniformBlockiv = (PFNGLGETACTIVEUNIFORMBLOCKIVPROC)loader("glGetActiveUniformBlockiv");
    opengl->glGetActiveUniformBlockName = (PFNGLGETACTIVEUNIFORMBLOCKNAMEPROC)loader("glGetActiveUniformBlockName");
    opengl->glUniformBlockBinding = (PFNGLUNIFORMBLOCKBINDINGPROC)loader("glUniformBlockBinding");
    opengl->glBindBufferRange = (PFNGLBINDBUFFERRANGEPROC)loader("glBindBufferRange");
    opengl->glBindBufferBase = (PFNGLBINDBUFFERBASEPROC)loader("glBindBufferBase");
    opengl->glGetIntegeri_v = (PFNGLGETINTEGERI_VPROC)loader("glGetIntegeri_v");
    opengl->glDrawElementsBaseVertex = (PFNGLDRAWELEMENTSBASEVERTEXPROC)loader("glDrawElementsBaseVertex");
    opengl->glDrawRangeElementsBaseVertex = (PFNGLDRAWRANGEELEMENTSBASEVERTEXPROC)loader("glDrawRangeElementsBaseVertex");
    opengl->glDrawElementsInstancedBaseVertex = (PFNGLDRAWELEMENTSINSTANCEDBASEVERTEXPROC)loader("glDrawElementsInstancedBaseVertex");
    opengl->glMultiDrawElementsBaseVertex = (PFNGLMULTIDRAWELEMENTSBASEVERTEXPROC)loader("glMultiDrawElementsBaseVertex");
    opengl->glProvokingVertex = (PFNGLPROVOKINGVERTEXPROC)loader("glProvokingVertex");
    opengl->glFenceSync = (PFNGLFENCESYNCPROC)loader("glFenceSync");
    opengl->glIsSync = (PFNGLISSYNCPROC)loader("glIsSync");
    opengl->glDeleteSync = (PFNGLDELETESYNCPROC)loader("glDeleteSync");
    opengl->glClientWaitSync = (PFNGLCLIENTWAITSYNCPROC)loader("glClientWaitSync");
    opengl->glWaitSync = (PFNGLWAITSYNCPROC)loader("glWaitSync");
    opengl->glGetInteger64v = (PFNGLGETINTEGER64VPROC)loader("glGetInteger64v");
    opengl->glGetSynciv = (PFNGLGETSYNCIVPROC)loader("glGetSynciv");
    opengl->glGetInteger64i_v = (PFNGLGETINTEGER64I_VPROC)loader("glGetInteger64i_v");
    opengl->glGetBufferParameteri64v = (PFNGLGETBUFFERPARAMETERI64VPROC)loader("glGetBufferParameteri64v");
    opengl->glFramebufferTexture = (PFNGLFRAMEBUFFERTEXTUREPROC)loader("glFramebufferTexture");
    opengl->glTexImage2DMultisample = (PFNGLTEXIMAGE2DMULTISAMPLEPROC)loader("glTexImage2DMultisample");
    opengl->glTexImage3DMultisample = (PFNGLTEXIMAGE3DMULTISAMPLEPROC)loader("glTexImage3DMultisample");
    opengl->glGetMultisamplefv = (PFNGLGETMULTISAMPLEFVPROC)loader("glGetMultisamplefv");
    opengl->glSampleMaski = (PFNGLSAMPLEMASKIPROC)loader("glSampleMaski");
    opengl->glBindFragDataLocationIndexed = (PFNGLBINDFRAGDATALOCATIONINDEXEDPROC)loader("glBindFragDataLocationIndexed");
    opengl->glGetFragDataIndex = (PFNGLGETFRAGDATAINDEXPROC)loader("glGetFragDataIndex");
    opengl->glGenSamplers = (PFNGLGENSAMPLERSPROC)loader("glGenSamplers");
    opengl->glDeleteSamplers = (PFNGLDELETESAMPLERSPROC)loader("glDeleteSamplers");
    opengl->glIsSampler = (PFNGLISSAMPLERPROC)loader("glIsSampler");
    opengl->glBindSampler = (PFNGLBINDSAMPLERPROC)loader("glBindSampler");
    opengl->glSamplerParameteri = (PFNGLSAMPLERPARAMETERIPROC)loader("glSamplerParameteri");
    opengl->glSamplerParameteriv = (PFNGLSAMPLERPARAMETERIVPROC)loader("glSamplerParameteriv");
    opengl->glSamplerParameterf = (PFNGLSAMPLERPARAMETERFPROC)loader("glSamplerParameterf");
    opengl->glSamplerParameterfv = (PFNGLSAMPLERPARAMETERFVPROC)loader("glSamplerParameterfv");
    opengl->glSamplerParameterIiv = (PFNGLSAMPLERPARAMETERIIVPROC)loader("glSamplerParameterIiv");
    opengl->glSamplerParameterIuiv = (PFNGLSAMPLERPARAMETERIUIVPROC)loader("glSamplerParameterIuiv");
    opengl->glGetSamplerParameteriv = (PFNGLGETSAMPLERPARAMETERIVPROC)loader("glGetSamplerParameteriv");
    opengl->glGetSamplerParameterIiv = (PFNGLGETSAMPLERPARAMETERIIVPROC)loader("glGetSamplerParameterIiv");
    opengl->glGetSamplerParameterfv = (PFNGLGETSAMPLERPARAMETERFVPROC)loader("glGetSamplerParameterfv");
    opengl->glGetSamplerParameterIuiv = (PFNGLGETSAMPLERPARAMETERIUIVPROC)loader("glGetSamplerParameterIuiv");
    opengl->glQueryCounter = (PFNGLQUERYCOUNTERPROC)loader("glQueryCounter");
    opengl->glGetQueryObjecti64v = (PFNGLGETQUERYOBJECTI64VPROC)loader("glGetQueryObjecti64v");
    opengl->glGetQueryObjectui64v = (PFNGLGETQUERYOBJECTUI64VPROC)loader("glGetQueryObjectui64v");
    opengl->glVertexAttribDivisor = (PFNGLVERTEXATTRIBDIVISORPROC)loader("glVertexAttribDivisor");
    opengl->glVertexAttribP1ui = (PFNGLVERTEXATTRIBP1UIPROC)loader("glVertexAttribP1ui");
    opengl->glVertexAttribP1uiv = (PFNGLVERTEXATTRIBP1UIVPROC)loader("glVertexAttribP1uiv");
    opengl->glVertexAttribP2ui = (PFNGLVERTEXATTRIBP2UIPROC)loader("glVertexAttribP2ui");
    opengl->glVertexAttribP2uiv = (PFNGLVERTEXATTRIBP2UIVPROC)loader("glVertexAttribP2uiv");
    opengl->glVertexAttribP3ui = (PFNGLVERTEXATTRIBP3UIPROC)loader("glVertexAttribP3ui");
    opengl->glVertexAttribP3uiv = (PFNGLVERTEXATTRIBP3UIVPROC)loader("glVertexAttribP3uiv");
    opengl->glVertexAttribP4ui = (PFNGLVERTEXATTRIBP4UIPROC)loader("glVertexAttribP4ui");
    opengl->glVertexAttribP4uiv = (PFNGLVERTEXATTRIBP4UIVPROC)loader("glVertexAttribP4uiv");
    opengl->glVertexP2ui = (PFNGLVERTEXP2UIPROC)loader("glVertexP2ui");
    opengl->glVertexP2uiv = (PFNGLVERTEXP2UIVPROC)loader("glVertexP2uiv");
    opengl->glVertexP3ui = (PFNGLVERTEXP3UIPROC)loader("glVertexP3ui");
    opengl->glVertexP3uiv = (PFNGLVERTEXP3UIVPROC)loader("glVertexP3uiv");
    opengl->glVertexP4ui = (PFNGLVERTEXP4UIPROC)loader("glVertexP4ui");
    opengl->glVertexP4uiv = (PFNGLVERTEXP4UIVPROC)loader("glVertexP4uiv");
    opengl->glTexCoordP1ui = (PFNGLTEXCOORDP1UIPROC)loader("glTexCoordP1ui");
    opengl->glTexCoordP1uiv = (PFNGLTEXCOORDP1UIVPROC)loader("glTexCoordP1uiv");
    opengl->glTexCoordP2ui = (PFNGLTEXCOORDP2UIPROC)loader("glTexCoordP2ui");
    opengl->glTexCoordP2uiv = (PFNGLTEXCOORDP2UIVPROC)loader("glTexCoordP2uiv");
    opengl->glTexCoordP3ui = (PFNGLTEXCOORDP3UIPROC)loader("glTexCoordP3ui");
    opengl->glTexCoordP3uiv = (PFNGLTEXCOORDP3UIVPROC)loader("glTexCoordP3uiv");
    opengl->glTexCoordP4ui = (PFNGLTEXCOORDP4UIPROC)loader("glTexCoordP4ui");
    opengl->glTexCoordP4uiv = (PFNGLTEXCOORDP4UIVPROC)loader("glTexCoordP4uiv");
    opengl->glMultiTexCoordP1ui = (PFNGLMULTITEXCOORDP1UIPROC)loader("glMultiTexCoordP1ui");
    opengl->glMultiTexCoordP1uiv = (PFNGLMULTITEXCOORDP1UIVPROC)loader("glMultiTexCoordP1uiv");
    opengl->glMultiTexCoordP2ui = (PFNGLMULTITEXCOORDP2UIPROC)loader("glMultiTexCoordP2ui");
    opengl->glMultiTexCoordP2uiv = (PFNGLMULTITEXCOORDP2UIVPROC)loader("glMultiTexCoordP2uiv");
    opengl->glMultiTexCoordP3ui = (PFNGLMULTITEXCOORDP3UIPROC)loader("glMultiTexCoordP3ui");
    opengl->glMultiTexCoordP3uiv = (PFNGLMULTITEXCOORDP3UIVPROC)loader("glMultiTexCoordP3uiv");
    opengl->glMultiTexCoordP4ui = (PFNGLMULTITEXCOORDP4UIPROC)loader("glMultiTexCoordP4ui");
    opengl->glMultiTexCoordP4uiv = (PFNGLMULTITEXCOORDP4UIVPROC)loader("glMultiTexCoordP4uiv");
    opengl->glNormalP3ui = (PFNGLNORMALP3UIPROC)loader("glNormalP3ui");
    opengl->glNormalP3uiv = (PFNGLNORMALP3UIVPROC)loader("glNormalP3uiv");
    opengl->glColorP3ui = (PFNGLCOLORP3UIPROC)loader("glColorP3ui");
    opengl->glColorP3uiv = (PFNGLCOLORP3UIVPROC)loader("glColorP3uiv");
    opengl->glColorP4ui = (PFNGLCOLORP4UIPROC)loader("glColorP4ui");
    opengl->glColorP4uiv = (PFNGLCOLORP4UIVPROC)loader("glColorP4uiv");
    opengl->glSecondaryColorP3ui = (PFNGLSECONDARYCOLORP3UIPROC)loader("glSecondaryColorP3ui");
    opengl->glSecondaryColorP3uiv = (PFNGLSECONDARYCOLORP3UIVPROC)loader("glSecondaryColorP3uiv");
    
    return true;
}

//~ Internal API
internal void
Win32_DestroyOpenGLWindow(void)
{
    global_opengl.wglMakeCurrent(global_hdc, NULL);
    global_opengl.wglDeleteContext(global_opengl.context);
    DestroyWindow(global_window);
}

internal void
Win32_OpenGLSwapBuffers(void)
{
    global_opengl.vtable.glFlush();
    global_opengl.wglSwapLayerBuffers(global_hdc, WGL_SWAP_MAIN_PLANE);
    //SwapBuffers(global_hdc);
    
    if (global_should_flush_dwm)
    {
        int32 count = global_swap_interval;
        
        while (count --> 0)
            DwmFlush();
    }
}

internal bool32
Win32_CreateOpenGLWindow(int32 width, int32 height, const wchar_t* title)
{
    Trace("Win32_CreateOpenGLWindow");
    
    DWORD style = (WS_OVERLAPPEDWINDOW) & ~(WS_THICKFRAME | WS_MAXIMIZEBOX);
    HWND window = CreateWindowExW(0, global_class_name, title, style,
                                  CW_USEDEFAULT, CW_USEDEFAULT,
                                  width, height,
                                  NULL, NULL, global_instance, NULL);
    
    if (!window)
        return false;
    
    HDC hdc = GetDC(window);
    
    HMODULE library = LoadLibrary("opengl32.dll");
    if (!library)
    {
        DestroyWindow(window);
        return false;
    }
    
    Platform_DebugLog("Loaded Library: opengl32.dll\n");
    
    global_opengl.library = library;
    global_opengl.wglGetProcAddress = (void*)GetProcAddress(library, "wglGetProcAddress");
    global_opengl.wglCreateContext = (void*)GetProcAddress(library, "wglCreateContext");
    global_opengl.wglMakeCurrent = (void*)GetProcAddress(library, "wglMakeCurrent");
    global_opengl.wglDeleteContext = (void*)GetProcAddress(library, "wglDeleteContext");
    global_opengl.wglSwapLayerBuffers = (void*)GetProcAddress(library, "wglSwapLayerBuffers");
    
    if (!global_opengl.wglGetProcAddress ||
        !global_opengl.wglCreateContext ||
        !global_opengl.wglMakeCurrent ||
        !global_opengl.wglDeleteContext ||
        !global_opengl.wglSwapLayerBuffers)
    {
        DestroyWindow(window);
        return false;
    }
    
    // Pixel Format
    PIXELFORMATDESCRIPTOR pfd = {
        sizeof(PIXELFORMATDESCRIPTOR),
        1,
        PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
        PFD_TYPE_RGBA,
        32,
        0, 0, 0, 0, 0, 0,
        0,
        0,
        0,
        0, 0, 0, 0,
        24,
        8,
        0,
        PFD_MAIN_PLANE,
        0,
        0, 0, 0
    };
    
    int32 pixel_format = ChoosePixelFormat(hdc, &pfd);
    if (!pixel_format)
    {
        DestroyWindow(window);
        return false;
    }
    
    // Dummy Context
    SetPixelFormat(hdc, pixel_format, &pfd);
    HGLRC dummy_context = global_opengl.wglCreateContext(hdc);
    global_opengl.wglMakeCurrent(hdc, dummy_context);
    
    // Load Functions
    global_opengl.wglChoosePixelFormatARB = OpenGLGetProc("wglChoosePixelFormatARB");
    global_opengl.wglCreateContextAttribsARB = OpenGLGetProc("wglCreateContextAttribsARB");
    global_opengl.wglMakeContextCurrentARB = OpenGLGetProc("wglMakeContextCurrentARB");
    global_opengl.wglSwapIntervalEXT = OpenGLGetProc("wglSwapIntervalEXT");
    
    if (!global_opengl.wglChoosePixelFormatARB ||
        !global_opengl.wglCreateContextAttribsARB ||
        !global_opengl.wglMakeContextCurrentARB ||
        !global_opengl.wglSwapIntervalEXT)
    {
        global_opengl.wglMakeCurrent(hdc, NULL);
        global_opengl.wglDeleteContext(dummy_context);
        DestroyWindow(window);
        return false;
    }
    
    // Real Pixel Format
    int32 attribs[] =
    {
        WGL_DRAW_TO_WINDOW_ARB, GL_TRUE,
        WGL_SUPPORT_OPENGL_ARB, GL_TRUE,
        WGL_DOUBLE_BUFFER_ARB, GL_TRUE,
        WGL_PIXEL_TYPE_ARB, WGL_TYPE_RGBA_ARB,
        WGL_COLOR_BITS_ARB, 32,
        WGL_DEPTH_BITS_ARB, 24,
        WGL_STENCIL_BITS_ARB, 8,
        0
    };
    
    UINT num_formats = 0;
    global_opengl.wglChoosePixelFormatARB(hdc, attribs, 0, 1, &pixel_format, &num_formats);
    
    if (!pixel_format)
    {
        global_opengl.wglMakeCurrent(hdc, NULL);
        global_opengl.wglDeleteContext(dummy_context);
        DestroyWindow(window);
        return false;
    }
    
    // Create Context
    const int32 context_attribs[] = {
        WGL_CONTEXT_MAJOR_VERSION_ARB, 3,
        WGL_CONTEXT_MINOR_VERSION_ARB, 3,
        WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
        0
    };
    
    global_opengl.context = global_opengl.wglCreateContextAttribsARB(hdc, dummy_context, context_attribs);
    
    global_opengl.wglMakeCurrent(hdc, NULL);
    global_opengl.wglDeleteContext(dummy_context);
    
    if (!global_opengl.context)
    {
        DestroyWindow(window);
        return false;
    }
    
    // Make Current
    global_opengl.wglMakeCurrent(hdc, global_opengl.context);
    global_opengl.wglDeleteContext(dummy_context);
    
    // Load All Function
    if (!LoadOpenGLFunctions())
    {
        Win32_DestroyOpenGLWindow();
        return false;
    }
    
    int32 interval = 1;
    global_swap_interval = interval;
    
    if (IsWindowsVistaOrGreater())
    {
        HMODULE library = LoadLibrary("dwmapi.dll");
        
        if (library)
        {
            DwmIsCompositionEnabled = (void*)GetProcAddress(library, "DwmIsCompositionEnabled");
            DwmFlush = (void*)GetProcAddress(library, "DwmFlush");
            
            BOOL enabled = IsWindows8OrGreater();
            
            if (enabled || (SUCCEEDED(DwmIsCompositionEnabled(&enabled)) && enabled))
            {
                global_should_flush_dwm = true;
                interval = 0;
            }
        }
    }
    
    global_opengl.wglSwapIntervalEXT(interval);
    
    // Globals
    global_window = window;
    global_hdc = hdc;
    global_graphics_context.api = GraphicsAPI_OpenGL;
    global_graphics_context.opengl = &global_opengl.vtable;
    
    ShowWindow(global_window, SW_SHOWDEFAULT);
    
    return true;
}
