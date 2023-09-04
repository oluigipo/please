#ifndef API_OS_H
#define API_OS_H

#include "config.h"

//~ Common
#include "common_defs.h"
#include "common_types.h"

API void* OS_VirtualReserve(void* address, uintsize size);
API bool OS_VirtualCommit(void* ptr, uintsize size);
API void OS_VirtualRelease(void* ptr, uintsize size);
#define ArenaOsReserve_(size) OS_VirtualReserve(0, size)
#define ArenaOsCommit_(ptr, size) OS_VirtualCommit(ptr, size)
#define ArenaOsFree_(ptr, size) OS_VirtualRelease(ptr, size)

API void OS_ExitWithErrorMessage(const char* fmt, ...);
#define SafeAssert_OnFailure(expr, file, line, func) \
OS_ExitWithErrorMessage("Safety Assertion Failure!\nExpr: %s\nFile: %s\nLine: %i\nFunction: %s", expr, file, line, func)
#define Assert_OnFailure(expr, file, line, func) \
OS_ExitWithErrorMessage("Assertion Failure!\nExpr: %s\nFile: %s\nLine: %i\nFunction: %s", expr, file, line, func)

#include "common.h"

//~
enum
{
	OS_Limits_MaxGamepadCount = 16,
	OS_Limits_MaxGestureCount = 5,
	OS_Limits_MaxWindowTitleLength = 64,
	OS_Limits_MaxBufferedInput = 256,
	OS_Limits_MaxCodepointsPerFrame = 64,
	OS_Limits_MaxWorkerThreadCount = 4,
};

//~ Graphics Context
enum OS_WindowGraphicsApi
{
	OS_WindowGraphicsApi_Null = 0,
	
	OS_WindowGraphicsApi_OpenGL,
	OS_WindowGraphicsApi_Direct3D11,
}
typedef OS_WindowGraphicsApi;

struct OS_OpenGlApi typedef OS_OpenGlApi;
struct OS_D3d11Api typedef OS_D3d11Api;

struct OS_WindowGraphicsContext
{
	OS_WindowGraphicsApi api;
	
	union
	{
		const OS_OpenGlApi* opengl;
		const OS_D3d11Api* d3d11;
	};
	
	bool (*present_and_vsync)(int32 vsync_count);
}
typedef OS_WindowGraphicsContext;

//~ Input
struct OS_ButtonState
{
	uint8 changes: 7;
	bool is_down: 1;
}
typedef OS_ButtonState;

static_assert(sizeof(OS_ButtonState) == 1, "bitfield didn't work?");

// TODO(ljre): Properly support buffered sequence of inputs.
enum OS_KeyboardKey
{
	OS_KeyboardKey_Any = 0,
	
	OS_KeyboardKey_Escape,
	OS_KeyboardKey_Up, OS_KeyboardKey_Down, OS_KeyboardKey_Left, OS_KeyboardKey_Right,
	OS_KeyboardKey_LeftControl, OS_KeyboardKey_RightControl, OS_KeyboardKey_Control,
	OS_KeyboardKey_LeftShift, OS_KeyboardKey_RightShift, OS_KeyboardKey_Shift,
	OS_KeyboardKey_LeftAlt, OS_KeyboardKey_RightAlt, OS_KeyboardKey_Alt,
	OS_KeyboardKey_Tab, OS_KeyboardKey_Enter, OS_KeyboardKey_Backspace,
	OS_KeyboardKey_PageUp, OS_KeyboardKey_PageDown, OS_KeyboardKey_End, OS_KeyboardKey_Home,
	// NOTE(ljre): Try to not mess up. OS_KeyboardKey_Space = 32, and those above may reach it if you blindly add stuff.
	
	OS_KeyboardKey_Space = ' ',
	
	OS_KeyboardKey_0 = '0',
	OS_KeyboardKey_1, OS_KeyboardKey_2, OS_KeyboardKey_3, OS_KeyboardKey_4,
	OS_KeyboardKey_5, OS_KeyboardKey_6, OS_KeyboardKey_7, OS_KeyboardKey_8,
	OS_KeyboardKey_9,
	
	OS_KeyboardKey_A = 'A',
	OS_KeyboardKey_B, OS_KeyboardKey_C, OS_KeyboardKey_D, OS_KeyboardKey_E,
	OS_KeyboardKey_F, OS_KeyboardKey_G, OS_KeyboardKey_H, OS_KeyboardKey_I,
	OS_KeyboardKey_J, OS_KeyboardKey_K, OS_KeyboardKey_L, OS_KeyboardKey_M,
	OS_KeyboardKey_N, OS_KeyboardKey_O, OS_KeyboardKey_P, OS_KeyboardKey_Q,
	OS_KeyboardKey_R, OS_KeyboardKey_S, OS_KeyboardKey_T, OS_KeyboardKey_U,
	OS_KeyboardKey_V, OS_KeyboardKey_W, OS_KeyboardKey_X, OS_KeyboardKey_Y,
	OS_KeyboardKey_Z,
	
	OS_KeyboardKey_Numpad0,
	OS_KeyboardKey_Numpad1, OS_KeyboardKey_Numpad2, OS_KeyboardKey_Numpad3, OS_KeyboardKey_Numpad4,
	OS_KeyboardKey_Numpad5, OS_KeyboardKey_Numpad6, OS_KeyboardKey_Numpad7, OS_KeyboardKey_Numpad8,
	OS_KeyboardKey_Numpad9,
	
	OS_KeyboardKey_F1,
	OS_KeyboardKey_F2,  OS_KeyboardKey_F3,  OS_KeyboardKey_F4,  OS_KeyboardKey_F5,
	OS_KeyboardKey_F6,  OS_KeyboardKey_F7,  OS_KeyboardKey_F8,  OS_KeyboardKey_F9,
	OS_KeyboardKey_F10, OS_KeyboardKey_F11, OS_KeyboardKey_F12, OS_KeyboardKey_F13,
	OS_KeyboardKey_F14,
	
	OS_KeyboardKey_Count,
}
typedef OS_KeyboardKey;

struct OS_KeyboardState
{
	int32 buffered_count;
	
	OS_ButtonState buttons[OS_KeyboardKey_Count];
	uint8 buffered[OS_Limits_MaxBufferedInput];
}
typedef OS_KeyboardState;

enum OS_MouseButton
{
	OS_MouseButton_Left,
	OS_MouseButton_Right,
	OS_MouseButton_Middle,
	OS_MouseButton_Other0,
	OS_MouseButton_Other1,
	OS_MouseButton_Other2,
	
	OS_MouseButton_Count,
}
typedef OS_MouseButton;

struct OS_MouseState
{
	// NOTE(ljre): Position relative to window.
	float32 pos[2];
	float32 old_pos[2];
	int32 scroll;
	
	OS_ButtonState buttons[OS_MouseButton_Count];
}
typedef OS_MouseState;

enum OS_GamepadButton
{
	OS_GamepadButton_A,
	OS_GamepadButton_B,
	OS_GamepadButton_X,
	OS_GamepadButton_Y,
	OS_GamepadButton_Left,
	OS_GamepadButton_Right,
	OS_GamepadButton_Up,
	OS_GamepadButton_Down,
	OS_GamepadButton_LB,
	OS_GamepadButton_RB,
	OS_GamepadButton_RS,
	OS_GamepadButton_LS,
	OS_GamepadButton_Start,
	OS_GamepadButton_Back,
	
	OS_GamepadButton_Count,
}
typedef OS_GamepadButton;

struct OS_GamepadState
{
	// [0] = -1 <= left < 0 < right <= 1
	// [1] = -1 <= up < 0 < down <= 1
	float32 left[2];
	float32 right[2];
	
	// 0..1
	float32 lt, rt;
	
	OS_ButtonState buttons[OS_GamepadButton_Count];
}
typedef OS_GamepadState;

// NOTE(ljre): Helper macros.
//             Returns a bool.
//
//             state: OS_GamepadState, OS_MouseState, or OS_KeyboardState.
//             btn: A button enum.
#define OS_IsPressed(state, btn) ((state).buttons[btn].is_down && (state).buttons[btn].changes & 1)
#define OS_IsDown(state, btn) ((state).buttons[btn].is_down)
#define OS_IsReleased(state, btn) (!(state).buttons[btn].is_down && (state).buttons[btn].changes & 1)
#define OS_IsUp(state, btn) (!(state).buttons[btn].is_down)

struct OS_GestureState
{
	bool active;
	bool released;
	
	float32 start_pos[2];
	float32 current_pos[2];
	float32 delta[2];
}
typedef OS_GestureState;

//~ Window State
struct OS_WindowState
{
	bool should_close : 1;
	bool resized_by_user : 1;
	
	bool show_cursor : 1;
	bool lock_cursor : 1;
	bool fullscreen : 1;
	
	int32 x, y, width, height;
}
typedef OS_WindowState;

struct OS_AudioDeviceInfo
{
	String interface_name;
	String description;
	String name;
	
	uint32 id;
}
typedef OS_AudioDeviceInfo;

struct OS_AudioState
{
	// Mutable
	uint32 current_device_id;
	
	// Specified by platform layer
	int32 mix_sample_rate;
	int32 mix_channels;
	int32 mix_frame_pull_rate;
	
	int32 device_count;
	const OS_AudioDeviceInfo* devices;
}
typedef OS_AudioState;

struct OS_State
{
	bool has_audio : 1;
	bool has_keyboard : 1;
	bool has_gamepad_support : 1;
	bool has_mouse : 1;
	bool has_gestures : 1;
	bool is_terminating : 1;
	
	//- Window
	const OS_WindowGraphicsContext* graphics_context;
	OS_WindowState window;
	
	//- Input
	OS_KeyboardState keyboard;
	OS_MouseState mouse;
	
	OS_GamepadState gamepads[OS_Limits_MaxGamepadCount];
	uint16 connected_gamepads; // NOTE(ljre): Bitset
	
	OS_GestureState gestures[OS_Limits_MaxGestureCount];
	int32 gesture_count;
	
	int16 text_codepoints_count;
	uint32 text_codepoints[OS_Limits_MaxCodepointsPerFrame];
	
	//- Audio
	OS_AudioState audio;
}
typedef OS_State;

static inline bool
OS_IsGamepadConnected(const OS_State* os_state, int32 index)
{ return 0 != (os_state->connected_gamepads & (1 << index)); }

static inline int32
OS_ConnectedGamepadCount(const OS_State* os_state)
{ return PopCnt64(os_state->connected_gamepads); }

static inline int32
OS_ConnectedGamepadsIndices(const OS_State* os_state, int32 out_indices[OS_Limits_MaxGamepadCount])
{
	uint64 bits = os_state->connected_gamepads;
	int32 count = 0;
	
	while (bits && count < OS_Limits_MaxGamepadCount)
	{
		out_indices[count++] = BitCtz64(bits);
		bits &= bits-1;
	}
	
	return count;
}

//~ Init
void typedef OS_AudioThreadProc(void* user_data, int16* restrict out_buffer, int32 channels, int32 sample_rate, int32 sample_count);

struct OS_InitDesc
{
	void* workerthread_user_data;
	void* audiothread_user_data;
}
typedef OS_InitDesc;

struct OS_SetupArgs
{
	int32 worker_thread_count;
	int32 argc;
	const char* const* argv;
	
	void* memory;
	uintsize memory_size;
}
typedef OS_SetupArgs;

struct OS_AppApi
{
	void (*setup)(const OS_SetupArgs* args, OS_InitDesc* out_init);
	void (*update)(OS_State* os);
	void (*shutdown)(OS_State* os);
	void (*pull_audio_samples)(void* user_data, int16* restrict out_samples, int32 channels, int32 sample_rate, int32 sample_count);
	void (*worker_thread_proc)(void* user_data, int32 index);
}
typedef OS_AppApi;

API bool OS_Init(const OS_InitDesc* desc, OS_State** out_state);

//~ Functions
API const OS_AppApi* GetAppApi(void);

API void OS_PollEvents(void);
API void OS_ExitWithErrorMessage(const char* fmt, ...);
API void OS_MessageBox(String title, String message);
#ifdef CONFIG_ENABLE_HOT
API void* OS_LoadGameLibrary(void);
#endif
API uint64 OS_CurrentPosixTime(void);
API uint64 OS_CurrentTick(uint64* out_ticks_per_second);
API float64 OS_GetTimeInSeconds(void);

/* NOTE(ljre): sketching out some stuff
API bool OS_PleaseResizeWindow(int32 desired_width, int32 desired_height);
API bool OS_PleaseSetFullscreen(bool enabled);
API bool OS_PleaseHideCursor(bool enabled);
API bool OS_PleaseLockMouse(bool lock);
API bool OS_PleaseTerminate(void);
*/

#ifdef CONFIG_DEBUG
API void OS_DebugMessageBox(const char* fmt, ...);
API void OS_DebugLog(const char* fmt, ...);
API int OS_DebugLogPrintfFormat(const char* fmt, ...);
#else
#	define OS_DebugMessageBox(...) ((void)0)
#	define OS_DebugLog(...) ((void)0)
#	define OS_DebugLogPrintfFormat(...) ((void)0)
#endif

//- Memory & file stuff
API void* OS_HeapAlloc(uintsize size);
API void* OS_HeapRealloc(void* ptr, uintsize size);
API void OS_HeapFree(void* ptr);

API void* OS_VirtualReserve(void* address, uintsize size);
API bool OS_VirtualCommit(void* ptr, uintsize size);
API void OS_VirtualDecommit(void* ptr, uintsize size);
API void OS_VirtualRelease(void* ptr, uintsize size);

API bool OS_ReadEntireFile(String path, Arena* output_arena, void** out_data, uintsize* out_size);
API bool OS_WriteEntireFile(String path, const void* data, uintsize size);
API bool OS_IsFileOlderThan(String path, uint64 posix_timestamp);

struct OS_MappedFile
{ void* ptr; }
typedef OS_MappedFile;
API bool OS_MapFile(String path, OS_MappedFile* out_mapped_file, Buffer* out_buffer);
API bool OS_GetMappedFileBuffer(OS_MappedFile mapped_file, Buffer* out_buffer);
API void OS_UnmapFile(OS_MappedFile mapped_file);

struct OS_LibraryHandle
{ void* ptr; }
typedef OS_LibraryHandle;
API OS_LibraryHandle OS_LoadLibrary(String name);
API void* OS_LoadSymbol(OS_LibraryHandle library, const char* symbol_name);
API void OS_UnloadLibrary(OS_LibraryHandle* library);

//- Threading stuff
struct OS_RWLock
{ void* ptr; }
typedef OS_RWLock;
API void OS_InitRWLock(OS_RWLock* lock);
API void OS_LockShared(OS_RWLock* lock);
API void OS_LockExclusive(OS_RWLock* lock);
API bool OS_TryLockShared(OS_RWLock* lock);
API bool OS_TryLockExclusive(OS_RWLock* lock);
API void OS_UnlockShared(OS_RWLock* lock);
API void OS_UnlockExclusive(OS_RWLock* lock);
API void OS_DeinitRWLock(OS_RWLock* lock);

struct OS_Semaphore
{ void* ptr; }
typedef OS_Semaphore;
API void OS_InitSemaphore(OS_Semaphore* sem, int32 max_count);
API bool OS_WaitForSemaphore(OS_Semaphore* sem);
API void OS_SignalSemaphore(OS_Semaphore* sem, int32 count);
API void OS_DeinitSemaphore(OS_Semaphore* sem);

struct OS_EventSignal
{ void* ptr; }
typedef OS_EventSignal;
API void OS_InitEventSignal(OS_EventSignal* sig);
API bool OS_WaitEventSignal(OS_EventSignal* sig);
API void OS_SetEventSignal(OS_EventSignal* sig);
API void OS_ResetEventSignal(OS_EventSignal* sig);
API void OS_DeinitEventSignal(OS_EventSignal* sig);

API int32 OS_InterlockedCompareExchange32(volatile int32* ptr, int32 new_value, int32 expected);
API int64 OS_InterlockedCompareExchange64(volatile int64* ptr, int64 new_value, int64 expected);
API void* OS_InterlockedCompareExchangePtr(void* volatile* ptr, void* new_value, void* expected);
API int32 OS_InterlockedIncrement32(volatile int32* ptr);
API int32 OS_InterlockedDecrement32(volatile int32* ptr);

#endif //API_OS_H
