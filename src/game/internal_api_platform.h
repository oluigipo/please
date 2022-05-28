#ifndef INTERNAL_API_PLATFORM_H
#define INTERNAL_API_PLATFORM_H

enum Platform_GraphicsApi
{
	Platform_GraphicsApi_None = 0,
	
	Platform_GraphicsApi_OpenGL = 1,
	Platform_GraphicsApi_Direct3D = 2, // NOTE(ljre): not implemented yet :(
	
	Platform_GraphicsApi_Any = Platform_GraphicsApi_OpenGL | Platform_GraphicsApi_Direct3D,
}
typedef Platform_GraphicsApi;

struct Platform_OpenGLGraphicsContext typedef Platform_OpenGLGraphicsContext;
struct Platform_Direct3DGraphicsContext typedef Platform_Direct3DGraphicsContext;

struct Platform_GraphicsContext
{
	Platform_GraphicsApi api;
	union
	{
		const Platform_OpenGLGraphicsContext* opengl;
		const Platform_Direct3DGraphicsContext* d3d;
	};
}
typedef Platform_GraphicsContext;

struct Platform_Config
{
	// NOTE(ljre): For bools, -1 is "false" and 1 is "true".
	//             Zero Is Initialization!
	
	bool8 show_cursor; // if false, hide cursor when in window
	bool8 lock_cursor; // lock cursor in the middle of the window
	bool8 center_window; // if true, we ignore 'window_x' and 'window_y'
	bool8 fullscreen; // if true, we ignore 'center_window' and 'window_x/y/width/height'
	
	int32 window_x, window_y, window_width, window_height;
	String window_title;
	
	Platform_GraphicsApi graphics_api;
}
typedef Platform_Config;

// NOTE(ljre): This function also does general initialization
API bool32 Platform_CreateWindow(const Platform_Config* config, const Platform_GraphicsContext** out_graphics, Engine_InputData* input_data);

API void Platform_ExitWithErrorMessage(String message);
API void Platform_MessageBox(String title, String message);
API int32 Platform_WindowWidth(void);
API int32 Platform_WindowHeight(void);
API bool32 Platform_WindowShouldClose(void);
API float64 Platform_GetTime(void);
API uint64 Platform_CurrentPosixTime(void);
API void Platform_PollEvents(Engine_InputData* out_input_data);
API void Platform_FinishFrame(void);

// NOTE(ljre): Configuration will not be applied until 'Platform_PollEvents' is called.
API void Platform_UpdateConfig(const Platform_Config* config);

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
API bool32 Platform_WriteEntireFile(String path, const void* data, uintsize size);
API void Platform_FreeFileMemory(void* ptr, uintsize size);

#ifdef DEBUG
API void Platform_DebugMessageBox(const char* restrict format, ...);
API void Platform_DebugLog(const char* restrict format, ...);
#else // DEBUG
#   define Platform_DebugMessageBox(...) ((void)0)
#   define Platform_DebugLog(...) ((void)0)
#endif // DEBUG

//- Platform Audio
API int16* Platform_RequestSoundBuffer(int32* out_sample_count, int32* out_channels, int32* out_sample_rate, int32* out_elapsed_frames);
API void Platform_CloseSoundBuffer(int16* sound_buffer);
API bool32 Platform_IsAudioAvailable(void);

#endif //INTERNAL_API_PLATFORM_H
