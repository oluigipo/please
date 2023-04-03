#include <SDL2/SDL.h>

static Arena*
GetThreadScratchArena(void)
{
	static thread_local Arena* this_arena;
	
	if (!this_arena)
		this_arena = Arena_Create(64 << 10, 64 << 10);
	
	return this_arena;
}

int
main(int32 argc, char* argv[])
{
	uint32 flags =
		SDL_INIT_AUDIO |
		SDL_INIT_VIDEO |
		SDL_INIT_GAMECONTROLLER;
	
	if (SDL_Init(flags))
	{
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Could not initialize SDL.", SDL_GetError(), 0);
		return 1;
	}
	
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
	};
	
	int32 result = OS_UserMain(&args);
	
	return result;
}

API bool
OS_Init(const OS_InitDesc* desc, OS_State** out_state)
{
	Trace();
	
	// TODO(ljre)
}

API void
OS_PollEvents(void)
{
	Trace();
	
	// TODO(ljre)
}

API bool
OS_WaitForVsync(void)
{
	Trace();
	
	// TODO(ljre)
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
}

API uint64
OS_CurrentTick(uint64* out_ticks_per_second)
{
	// TODO(ljre)
}

API float64
OS_GetTimeInSeconds(void)
{
	// TODO(ljre)
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
	
	// TODO(ljre)
}

API bool
OS_VirtualCommit(void* ptr, uintsize size)
{
	Trace();
	
	// TODO(ljre)
}

API void
OS_VirtualDecommit(void* ptr, uintsize size)
{
	Trace();
	
	// TODO(ljre)
}

API void
OS_VirtualRelease(void* ptr, uintsize size)
{
	Trace();
	
	// TODO(ljre)
}

API bool
OS_ReadEntireFile(String path, Arena* output_arena, void** out_data, uintsize* out_size)
{
	Trace();
	
	// TODO(ljre)
}

API bool
OS_WriteEntireFile(String path, const void* data, uintsize size)
{
	Trace();
	
	// TODO(ljre)
}

API void*
OS_LoadDiscordLibrary(void)
{
	return NULL;
}

API void OS_InitRWLock(OS_RWLock* lock)
{
	Trace();
	
	// TODO(ljre)
}

API void OS_LockShared(OS_RWLock* lock)
{
	Trace();
	
	// TODO(ljre)
}

API void OS_LockExclusive(OS_RWLock* lock)
{
	Trace();
	
	// TODO(ljre)
}

API bool OS_TryLockShared(OS_RWLock* lock)
{
	Trace();
	
	// TODO(ljre)
}

API bool OS_TryLockExclusive(OS_RWLock* lock)
{
	Trace();
	
	// TODO(ljre)
}

API void OS_UnlockShared(OS_RWLock* lock)
{
	Trace();
	
	// TODO(ljre)
}

API void OS_UnlockExclusive(OS_RWLock* lock)
{
	Trace();
	
	// TODO(ljre)
}

API void OS_DeinitRWLock(OS_RWLock* lock)
{
	Trace();
	
	// TODO(ljre)
}

API void OS_InitSemaphore(OS_Semaphore* sem, int32 max_count)
{
	Trace();
	
	// TODO(ljre)
}

API bool OS_WaitForSemaphore(OS_Semaphore* sem)
{
	Trace();
	
	// TODO(ljre)
}

API void OS_SignalSemaphore(OS_Semaphore* sem, int32 count)
{
	Trace();
	
	// TODO(ljre)
}

API void OS_DeinitSemaphore(OS_Semaphore* sem)
{
	Trace();
	
	// TODO(ljre)
}


API void OS_InitEventSignal(OS_EventSignal* sig)
{
	Trace();
	
	// TODO(ljre)
}

API bool OS_WaitEventSignal(OS_EventSignal* sig)
{
	Trace();
	
	// TODO(ljre)
}

API void OS_SetEventSignal(OS_EventSignal* sig)
{
	Trace();
	
	// TODO(ljre)
}

API void OS_ResetEventSignal(OS_EventSignal* sig)
{
	Trace();
	
	// TODO(ljre)
}

API void OS_DeinitEventSignal(OS_EventSignal* sig)
{
	Trace();
	
	// TODO(ljre)
}

API int32 OS_InterlockedCompareExchange32(volatile int32* ptr, int32 new_value, int32 expected)
{
	Trace();
	
	// TODO(ljre)
}

API int64 OS_InterlockedCompareExchange64(volatile int64* ptr, int64 new_value, int64 expected)
{
	Trace();
	
	// TODO(ljre)
}

API void* OS_InterlockedCompareExchangePtr(void* volatile* ptr, void* new_value, void* expected)
{
	Trace();
	
	// TODO(ljre)
}

API int32 OS_InterlockedIncrement32(volatile int32* ptr)
{
	Trace();
	
	// TODO(ljre)
}

API int32 OS_InterlockedDecrement32(volatile int32* ptr)
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

