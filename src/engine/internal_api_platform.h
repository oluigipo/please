#ifndef INTERNAL_API_PLATFORM_H
#define INTERNAL_API_PLATFORM_H

void typedef Platform_AudioThreadProc(void* user_data, int16* out_buffer, uint32 sample_count, int32 channels, uint32 sample_rate, uint32 elapsed_frames);

struct Engine_PlatformData
{
	// NOTE(ljre): Platform state data. Just change it and we'll update once 'Platform_PollEvents' is called.
	bool show_cursor; // if false, hide cursor when in window
	bool lock_cursor; // lock cursor in the middle of the window
	bool center_window; // if true, we ignore 'window_x/y'
	bool fullscreen; // if true, we ignore 'center_window' and 'window_x/y'
	
	int32 window_x, window_y, window_width, window_height;
	String window_title;
	
	Engine_GraphicsApi graphics_api;
	
	// NOTE(ljre): Immutable. Changing them does nothing.
	bool window_should_close;
	bool user_resized_window; // if true, the user just resized the window. thus window_width/height will be ignored.
}
typedef Engine_PlatformData;

struct Platform_InitDesc
{
	Engine_Data* engine;
	Platform_AudioThreadProc* audio_thread_proc;
	void* audio_thread_data;
	
	Engine_PlatformData* inout_state;
	const Engine_GraphicsContext** out_graphics;
}
typedef Platform_InitDesc;

API bool Platform_Init(const Platform_InitDesc* desc);
API void Platform_DefaultState(Engine_PlatformData* out_state);
API void Platform_PollEvents(Engine_PlatformData* inout_state, Engine_InputData* out_input_data);
API void Platform_FinishFrame(void);

API void Platform_ExitWithErrorMessage(String message);
API void Platform_MessageBox(String title, String message);
API float64 Platform_GetTime(void);
API uint64 Platform_CurrentPosixTime(void);

// Optional Libraries
API void* Platform_LoadDiscordLibrary(void);
#ifdef INTERNAL_ENABLE_HOT
API void* Platform_LoadGameLibrary(void);
#endif

// Memory
API void* Platform_HeapAlloc(uintsize size);
API void* Platform_HeapRealloc(void* ptr, uintsize size);
API void Platform_HeapFree(void* ptr);

API void* Platform_VirtualReserve(uintsize size);
API void Platform_VirtualCommit(void* ptr, uintsize size);
API void Platform_VirtualFree(void* ptr, uintsize size);

API void* Platform_ReadEntireFile(String path, uintsize* out_size, Arena* opt_arena); // 'opt_arena' can be NULL
API bool Platform_WriteEntireFile(String path, const void* data, uintsize size);
API void Platform_FreeFileMemory(void* ptr, uintsize size);

#ifdef DEBUG
API void Platform_DebugMessageBox(const char* restrict format, ...);
API void Platform_DebugLog(const char* restrict format, ...);
API void Platform_DebugDumpBitmap(const char* restrict path, const void* data, int32 width, int32 height, int32 channels);
#else // DEBUG
#   define Platform_DebugMessageBox(...) ((void)0)
#   define Platform_DebugLog(...) ((void)0)
#   define Platform_DebugDumpBitmap(...) ((void)0)
#endif // DEBUG

//- Platform Audio
API int16* Platform_RequestSoundBuffer(int32* out_sample_count, int32* out_channels, int32* out_sample_rate, int32* out_elapsed_frames);
API void Platform_CloseSoundBuffer(int16* sound_buffer);
API bool Platform_IsAudioAvailable(void);

#endif //INTERNAL_API_PLATFORM_H
