
struct
{
	OS_State state;
	OS_WindowGraphicsContext graphics_context;
	OS_OpenGlApi opengl;
	
	SDL_Window* window;
	SDL_GLContext glcontext;
	uint64 started_at;
}
static g_sdl2;

static Arena*
GetThreadScratchArena(void)
{
	static thread_local Arena* this_arena;
	
	if (!this_arena)
		this_arena = Arena_Create(64 << 10, 64 << 10);
	
	return this_arena;
}

static bool
PresentAndVsync(int32 vsync_count)
{
	(void)vsync_count;
	
	SDL_GL_SwapWindow(g_sdl2.window);
	return true;
}

static void SDLMAIN_DECLSPEC
android_main22222(struct android_app* state)
{
	app_dummy();
	
	SDL_SetMainReady();
	
	int32 argc = 1;
	char** argv = (char*[]) { "app", NULL };
	
	uint32 flags =
		SDL_INIT_TIMER |
		SDL_INIT_AUDIO |
		SDL_INIT_VIDEO |
		SDL_INIT_EVENTS |
		SDL_INIT_GAMECONTROLLER;
	
	if (SDL_Init(flags))
	{
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Could not initialize SDL.", SDL_GetError(), 0);
		return;
	}
	
	g_sdl2.started_at = SDL_GetPerformanceCounter();
	
	OS_UserMainArgs args = {
		.argc = argc,
		.argv = (const char* const*)argv,
		
		.default_window_state = {
#if defined(CONFIG_OS_ANDROID)
			.fullscreen = true,
#else
			.center_window = true,
#endif
			
			.x = 0,
			.y = 0,
			.width = 800,
			.height = 600,
		},
		
		.cpu_core_count = 1//SDL_GetCPUCount(),
	};
	
	int32 result = OS_UserMain(&args);
	
	//- Deinit
	if (g_sdl2.window)
		SDL_DestroyWindow(g_sdl2.window);
	
	SDL_Quit();
}

static bool LoadOpenGLFunctions(OS_OpenGlApi* opengl, void* SDLCALL get_proc_address(const char* proc));

API bool
OS_Init(const OS_InitDesc* desc, OS_State** out_state)
{
	Trace();
	
	OS_WindowState config = desc->window_initial_state;
	
#if defined(CONFIG_OS_ANDROID)
	config.fullscreen = true;
#endif
	
	int x = config.x;
	int y = config.y;
	int width = config.width;
	int height = config.height;
	uint32 flags = SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL;
	
	if (config.fullscreen)
	{
		x = 0;
		y = 0;
		width = 0;
		height = 0;
		
		flags |= SDL_WINDOW_FULLSCREEN;
	}
	else if(config.center_window)
	{
		x = SDL_WINDOWPOS_CENTERED;
		y = SDL_WINDOWPOS_CENTERED;
	}
	
	SDL_SetHint(SDL_HINT_OPENGL_ES_DRIVER, "1");
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_EGL, 1);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
	
	SDL_Window* window = SDL_CreateWindow((char*)config.title, x, y, width, height, flags);
	if (!window)
		return false;
	
	SDL_GetWindowPosition(window, &config.x, &config.y);
	SDL_GetWindowSize(window, &config.width, &config.height);
	
	SDL_GLContext context = SDL_GL_CreateContext(window);
	if (!context)
	{
		SDL_DestroyWindow(window);
		return false;
	}
	
	if (SDL_GL_MakeCurrent(window, context) == 0)
	{
		SDL_GL_DeleteContext(context);
		SDL_DestroyWindow(window);
		return false;
	}
	
	if (SDL_GL_SetSwapInterval(-1) != 0 && SDL_GL_SetSwapInterval(1) != 0)
		SDL_GL_SetSwapInterval(0);
	
	if (!LoadOpenGLFunctions(&g_sdl2.opengl, SDL_GL_GetProcAddress))
	{
		SDL_GL_MakeCurrent(window, NULL);
		SDL_GL_DeleteContext(context);
		SDL_DestroyWindow(window);
		return false;
	}
	
	g_sdl2.window = window;
	g_sdl2.glcontext = context;
	g_sdl2.state.graphics_context = &g_sdl2.graphics_context;
	g_sdl2.state.window = config;
	g_sdl2.graphics_context.api = OS_WindowGraphicsApi_OpenGL;
	g_sdl2.graphics_context.opengl = &g_sdl2.opengl;
	
	return true;
}

API void
OS_PollEvents(void)
{
	Trace();
	
	SDL_Event event;
	while (SDL_PollEvent(&event))
	{
		switch (event.type)
		{
			case SDL_QUIT:
			{
				g_sdl2.state.window.should_close = true;
			} break;
			
			case SDL_KEYDOWN:
			{
				
			} break;
			
			default: break;
		}
	}
}

API bool
OS_WaitForVsync(void)
{
	Trace();
	
	// TODO(ljre)
	return false;
}

API void
OS_ExitWithErrorMessage(const char* fmt, ...)
{
	Trace();
	
	Arena* scratch_arena = GetThreadScratchArena();
	
	va_list args;
	va_start(args, fmt);
	String str = Arena_VPrintf(scratch_arena, fmt, args);
	Arena_PushData(scratch_arena, &""); // NOTE(ljre): Null terminator
	va_end(args);
	
	SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Fatal Error!", (const char*)str.data, NULL);
	exit(1);
}

API void
OS_MessageBox(String title, String message)
{
	Trace();
	
	Arena* scratch_arena = GetThreadScratchArena();
	
	for Arena_TempScope(scratch_arena)
	{
		const char* cstr_title = Arena_PushCString(scratch_arena, title);
		const char* cstr_message = Arena_PushCString(scratch_arena, message);
		
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, cstr_title, cstr_message, NULL);
	}
}

API uint64
OS_CurrentPosixTime(void)
{
	// TODO(ljre)
	return 0;
}

API uint64
OS_CurrentTick(uint64* out_ticks_per_second)
{
	if (out_ticks_per_second)
		*out_ticks_per_second = SDL_GetPerformanceFrequency();
	
	return SDL_GetPerformanceCounter();
}

API float64
OS_GetTimeInSeconds(void)
{
	return (float64)(SDL_GetPerformanceCounter() - g_sdl2.started_at) / SDL_GetPerformanceFrequency();
}

API void*
OS_HeapAlloc(uintsize size)
{
	Trace();
	
	return SDL_malloc(size);
}

API void*
OS_HeapRealloc(void* ptr, uintsize size)
{
	Trace();
	
	return SDL_realloc(ptr, size);
}

API void
OS_HeapFree(void* ptr)
{
	Trace();
	
	return SDL_free(ptr);
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
	
	return mprotect(ptr, size, PROT_READ|PROT_WRITE);
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
	
	// TODO(ljre)
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
	
	lock->ptr = SDL_CreateMutex();
}

API void
OS_LockShared(OS_RWLock* lock)
{
	Trace();
	
	SDL_LockMutex(lock->ptr);
}

API void
OS_LockExclusive(OS_RWLock* lock)
{
	Trace();
	
	SDL_LockMutex(lock->ptr);
}

API bool
OS_TryLockShared(OS_RWLock* lock)
{
	Trace();
	
	return SDL_TryLockMutex(lock->ptr) == 0;
}

API bool
OS_TryLockExclusive(OS_RWLock* lock)
{
	Trace();
	
	return SDL_TryLockMutex(lock->ptr) == 0;
}

API void
OS_UnlockShared(OS_RWLock* lock)
{
	Trace();
	
	SDL_UnlockMutex(lock->ptr);
}

API void
OS_UnlockExclusive(OS_RWLock* lock)
{
	Trace();
	
	SDL_UnlockMutex(lock->ptr);
}

API void
OS_DeinitRWLock(OS_RWLock* lock)
{
	Trace();
	
	SDL_DestroyMutex(lock->ptr);
	lock->ptr = NULL;
}

API void
OS_InitSemaphore(OS_Semaphore* sem, int32 max_count)
{
	Trace();
	SafeAssert(max_count > 0);
	
	sem->ptr = SDL_CreateSemaphore((uint32)max_count);
}

API bool
OS_WaitForSemaphore(OS_Semaphore* sem)
{
	Trace();
	bool result = false;
	
	if (sem->ptr)
	{
		result = SDL_SemWait(sem->ptr) == 0;
	}
	
	return result;
}

API void
OS_SignalSemaphore(OS_Semaphore* sem, int32 count)
{
	Trace();
	
	if (sem->ptr)
	{
		while (count --> 0)
			SDL_SemPost(sem->ptr);
	}
}

API void
OS_DeinitSemaphore(OS_Semaphore* sem)
{
	Trace();
	
	if (sem->ptr)
	{
		SDL_DestroySemaphore(sem->ptr);
		sem->ptr = NULL;
	}
}

API void
OS_InitEventSignal(OS_EventSignal* sig)
{
	Trace();
	
	// TODO
}

API bool
OS_WaitEventSignal(OS_EventSignal* sig)
{
	Trace();
	
	// TODO
	return false;
}

API void
OS_SetEventSignal(OS_EventSignal* sig)
{
	Trace();
	
	// TODO(ljre)
}

API void
OS_ResetEventSignal(OS_EventSignal* sig)
{
	Trace();
	
	// TODO(ljre)
}

API void
OS_DeinitEventSignal(OS_EventSignal* sig)
{
	Trace();
	
	// TODO(ljre)
}

API int32
OS_InterlockedCompareExchange32(volatile int32* ptr, int32 new_value, int32 expected)
{
	Trace();
	
	if (SDL_AtomicCAS((SDL_atomic_t*)ptr, expected, new_value))
		return new_value;
	
	return expected;
}

API int64
OS_InterlockedCompareExchange64(volatile int64* ptr, int64 new_value, int64 expected)
{
	Trace();
	
	// TODO(ljre)
}

API void*
OS_InterlockedCompareExchangePtr(void* volatile* ptr, void* new_value, void* expected)
{
	Trace();
	
	// TODO(ljre)
}

API int32
OS_InterlockedIncrement32(volatile int32* ptr)
{
	Trace();
	
	// TODO(ljre)
}

API int32
OS_InterlockedDecrement32(volatile int32* ptr)
{
	Trace();
	
	// TODO(ljre)
}

#ifdef CONFIG_ENABLE_HOT
#   error "CONFIG_ENABLE_HOT not implemented for SDL2."
#endif

#ifdef CONFIG_DEBUG
API void
OS_DebugMessageBox(const char* fmt, ...)
{
	// TODO(ljre)
}

API void
OS_DebugLog(const char* fmt, ...)
{
	// TODO(ljre)
}

API int
OS_DebugLogPrintfFormat(const char* fmt, ...)
{
	// TODO(ljre)
}
#endif //CONFIG_DEBUG

//~ Loading OpenGL Functions
static bool
LoadOpenGLFunctions(OS_OpenGlApi* opengl, void* SDLCALL loader(const char* proc))
{
	Trace();
	
	opengl->glGetString = loader("glGetString");
	if (!opengl->glGetString)
		return false;
	
	const char* str = (const char*)opengl->glGetString(GL_VERSION);
	if (!str)
		return false;
	
	OS_DebugLog("OpenGL Version: %s\n", str);
	
#ifdef CONFIG_DEBUG
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
