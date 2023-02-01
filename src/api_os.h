#ifndef API_OS_H
#define API_OS_H

#include "config.h"

//~ Common
#include "common_defs.h"
#include "common_types.h"

API void* OS_VirtualReserve(uintsize size);
API bool OS_VirtualCommit(void* ptr, uintsize size);
API void OS_VirtualFree(void* ptr, uintsize size);
#define Arena_OsReserve_(size) OS_VirtualReserve(size)
#define Arena_OsCommit_(ptr, size) OS_VirtualCommit(ptr, size)
#define Arena_OsFree_(ptr, size) OS_VirtualFree(ptr, size)

API void OS_ExitWithErrorMessage(const char* fmt, ...);
#define SafeAssert_OnFailure(expr, file, line, func) \
OS_ExitWithErrorMessage("Safety Assertion Failure!\nExpr: %s\nFile: %s\nLine: %i\nFunction: %s", expr, file, line, func)
#define Assert_OnFailure(expr, file, line, func) \
OS_ExitWithErrorMessage("Assertion Failure!\nExpr: %s\nFile: %s\nLine: %i\nFunction: %s", expr, file, line, func)

#include "common.h"

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

static_assert(sizeof(OS_ButtonState) == 1);

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
	uint8 buffered[128];
}
typedef OS_KeyboardState;

enum OS_MouseButton
{
	OS_MouseButton_Left,
	OS_MouseButton_Middle,
	OS_MouseButton_Right,
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
	// [0] = left < 0 < right
	// [1] = up < 0 < down
	float32 left[2];
	float32 right[2];
	
	// 0..1
	float32 lt, rt;
	
	OS_ButtonState buttons[OS_GamepadButton_Count];
}
typedef OS_GamepadState;

#define OS_MAX_GAMEPAD_COUNT 16

struct OS_InputState
{
	OS_KeyboardState keyboard;
	OS_MouseState mouse;
	OS_GamepadState gamepads[OS_MAX_GAMEPAD_COUNT];
	
	uint16 connected_gamepads;
}
typedef OS_InputState;

// NOTE(ljre): Helper macros.
//             Returns a bool.
//
//             state: OS_GamepadState, OS_MouseState, or OS_KeyboardState.
//             btn: A button enum.
#define OS_IsPressed(state, btn) ((state).buttons[btn].is_down && (state).buttons[btn].changes & 1)
#define OS_IsDown(state, btn) ((state).buttons[btn].is_down)
#define OS_IsReleased(state, btn) (!(state).buttons[btn].is_down && (state).buttons[btn].changes & 1)
#define OS_IsUp(state, btn) (!(state).buttons[btn].is_down)

static inline bool
OS_IsGamepadConnected(const OS_InputState* input, uint32 index)
{ return 0 != (input->connected_gamepads & (1 << index)); }

static inline int32
OS_ConnectedGamepadCount(const OS_InputState* input)
{ return Mem_PopCnt64(input->connected_gamepads); }

static inline int32
OS_ConnectedGamepadsIndices(const OS_InputState* input, int32 out_indices[OS_MAX_GAMEPAD_COUNT])
{
	uint64 bits = input->connected_gamepads;
	int32 count = 0;
	
	while (bits)
	{
		out_indices[count++] = Mem_BitCtz64(bits);
		bits &= bits-1;
	}
	
	return count;
}

//~ Window State
struct OS_WindowState
{
	bool should_close;
	bool resized_by_user;
	
	bool show_cursor;
	bool lock_cursor;
	bool center_window;
	bool fullscreen;
	
	int32 x, y, width, height;
	uint8 title[64];
}
typedef OS_WindowState;

//~ Init
enum OS_InitFlags
{
	OS_InitFlags_Null = 0,
	
	OS_InitFlags_WindowAndGraphics = 1,
	OS_InitFlags_SimpleAudio = 2,
}
typedef OS_InitFlags;

struct OS_InitDesc
{
	OS_InitFlags flags;
	
	// OS_InitFlags_WindowAndGraphics
	OS_WindowState window_initial_state;
	OS_WindowGraphicsApi window_desired_api;
	
	// OS_InitFlags_SimpleAudio
	int32 simpleaudio_desired_sample_rate; // 44100 or 48000
}
typedef OS_InitDesc;

struct OS_InitOutput
{
	OS_WindowState window_state;
	const OS_WindowGraphicsContext* graphics_context;
	OS_InputState input_state;
}
typedef OS_InitOutput;

//~ Functions
API int32 OS_UserMain(int32 argc, char* argv[]);

API bool OS_Init(const OS_InitDesc* desc, OS_InitOutput* out_output);
API void OS_DefaultWindowState(OS_WindowState* out_state);
API void OS_PollEvents(OS_WindowState* inout_state, OS_InputState* out_input);
API bool OS_WaitForVsync(void);

API void OS_ExitWithErrorMessage(const char* fmt, ...);
API void OS_MessageBox(String title, String message);

API uint64 OS_CurrentPosixTime(void);
API uint64 OS_CurrentTick(uint64* out_ticks_per_second);
API float64 OS_GetTimeInSeconds(void);

API void* OS_HeapAlloc(uintsize size);
API void* OS_HeapRealloc(void* ptr, uintsize size);
API void OS_HeapFree(void* ptr);

API void* OS_VirtualReserve(uintsize size);
API bool OS_VirtualCommit(void* ptr, uintsize size);
API void OS_VirtualDecommit(void* ptr, uintsize size);
API void OS_VirtualRelease(void* ptr, uintsize size);

API bool OS_ReadEntireFile(String path, Arena* output_arena, void** out_data, uintsize* out_size);
API bool OS_WriteEntireFile(String path, const void* data, uintsize size);

API void* OS_LoadDiscordLibrary(void);
#ifdef CONFIG_ENABLE_HOT
API void* OS_LoadGameLibrary(void);
#endif

#ifdef CONFIG_DEBUG
API void OS_DebugMessageBox(const char* fmt, ...);
API void OS_DebugLog(const char* fmt, ...);
#else
#   define OS_DebugMessageBox(...) ((void)0)
#   define OS_DebugLog(...) ((void)0)
#endif

//- RWLock
struct OS_RWLock typedef OS_RWLock;

#if defined(_WIN32)
struct OS_RWLock
{
	void* ptr_;
};
#elif defined(__linux__)
#   include <pthread.h>
struct OS_RWLock
{
	pthread_rwlock_t impl;
};
#endif //defined(__linux__)

API void OS_InitRWLock(OS_RWLock* lock);
API void OS_LockShared(OS_RWLock* lock);
API void OS_LockExclusive(OS_RWLock* lock);
API bool OS_TryLockShared(OS_RWLock* lock);
API bool OS_TryLockExclusive(OS_RWLock* lock);
API void OS_UnlockShared(OS_RWLock* lock);
API void OS_UnlockExclusive(OS_RWLock* lock);
API void OS_DeinitRWLock(OS_RWLock* lock);

//- Simple Audio
API int16* OS_RequestSoundBuffer(int32* out_sample_count, int32* out_channels, int32* out_sample_rate, int32* out_elapsed_frames);
API void OS_CloseSoundBuffer(int16* sound_buffer);

#endif //API_OS_H
