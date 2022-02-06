#ifndef INTERNAL_API_PLATFORM_H
#define INTERNAL_API_PLATFORM_H

enum GraphicsAPI
{
	GraphicsAPI_None = 0,
	
	GraphicsAPI_OpenGL = 1,
	GraphicsAPI_Direct3D = 2, // NOTE(ljre): not implemented yet :(
	
	GraphicsAPI_Any = GraphicsAPI_OpenGL | GraphicsAPI_Direct3D,
}
typedef GraphicsAPI;

struct GraphicsContext_OpenGL typedef GraphicsContext_OpenGL;
struct GraphicsContext_Direct3D typedef GraphicsContext_Direct3D;

struct GraphicsContext
{
	GraphicsAPI api;
	union
	{
		GraphicsContext_OpenGL* opengl;
		GraphicsContext_Direct3D* d3d;
	};
}
typedef GraphicsContext;

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
	
	GraphicsAPI graphics_api;
}
typedef Platform_Config;

// NOTE(ljre): This function also does general initialization
API bool32 Platform_CreateWindow(const Platform_Config* config, const GraphicsContext** out_graphics);

API void Platform_ExitWithErrorMessage(String message);
API void Platform_MessageBox(String title, String message);
API int32 Platform_WindowWidth(void);
API int32 Platform_WindowHeight(void);
API bool32 Platform_WindowShouldClose(void);
API float64 Platform_GetTime(void);
API uint64 Platform_CurrentPosixTime(void);
API void Platform_PollEvents(void);
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
API void Platform_DebugDumpBitmap(const char* restrict path, const void* data, int32 width, int32 height, int32 channels);
#else // DEBUG
#   define Platform_DebugMessageBox(...) ((void)0)
#   define Platform_DebugLog(...) ((void)0)
#   define Platform_DebugDumpBitmap(...) ((void)0)
#endif // DEBUG

//- Platform Audio
API int16* Platform_RequestSoundBuffer(int32* out_sample_count, int32* out_channels, int32* out_sample_rate, int32* out_elapsed_frames);
API void Platform_CloseSoundBuffer(int16* sound_buffer);
API bool32 Platform_IsAudioAvailable(void);

//- Platform Input
enum Input_KeyboardKey
{
	Input_KeyboardKey_Any = 0,
	
	Input_KeyboardKey_Escape,
	Input_KeyboardKey_Up, Input_KeyboardKey_Down, Input_KeyboardKey_Left, Input_KeyboardKey_Right,
	Input_KeyboardKey_LeftControl, Input_KeyboardKey_RightControl, Input_KeyboardKey_Control,
	Input_KeyboardKey_LeftShift, Input_KeyboardKey_RightShift, Input_KeyboardKey_Shift,
	Input_KeyboardKey_LeftAlt, Input_KeyboardKey_RightAlt, Input_KeyboardKey_Alt,
	Input_KeyboardKey_Tab, Input_KeyboardKey_Enter, Input_KeyboardKey_Backspace,
	Input_KeyboardKey_PageUp, Input_KeyboardKey_PageDown, Input_KeyboardKey_End, Input_KeyboardKey_Home,
	// NOTE(ljre): Try to not mess up. Input_KeyboardKey_Space = 32, and those above can be (but shouldn't)
	
	Input_KeyboardKey_Space = ' ',
	
	Input_KeyboardKey_0 = '0',
	Input_KeyboardKey_1, Input_KeyboardKey_2, Input_KeyboardKey_3, Input_KeyboardKey_4,
	Input_KeyboardKey_5, Input_KeyboardKey_6, Input_KeyboardKey_7, Input_KeyboardKey_8,
	Input_KeyboardKey_9,
	
	Input_KeyboardKey_A = 'A',
	Input_KeyboardKey_B, Input_KeyboardKey_C, Input_KeyboardKey_D, Input_KeyboardKey_E,
	Input_KeyboardKey_F, Input_KeyboardKey_G, Input_KeyboardKey_H, Input_KeyboardKey_I,
	Input_KeyboardKey_J, Input_KeyboardKey_K, Input_KeyboardKey_L, Input_KeyboardKey_M,
	Input_KeyboardKey_N, Input_KeyboardKey_O, Input_KeyboardKey_P, Input_KeyboardKey_Q,
	Input_KeyboardKey_R, Input_KeyboardKey_S, Input_KeyboardKey_T, Input_KeyboardKey_U,
	Input_KeyboardKey_V, Input_KeyboardKey_W, Input_KeyboardKey_X, Input_KeyboardKey_Y,
	Input_KeyboardKey_Z,
	
	Input_KeyboardKey_Numpad0,
	Input_KeyboardKey_Numpad1, Input_KeyboardKey_Numpad2, Input_KeyboardKey_Numpad3, Input_KeyboardKey_Numpad4,
	Input_KeyboardKey_Numpad5, Input_KeyboardKey_Numpad6, Input_KeyboardKey_Numpad7, Input_KeyboardKey_Numpad8,
	Input_KeyboardKey_Numpad9,
	
	Input_KeyboardKey_F1,
	Input_KeyboardKey_F2,  Input_KeyboardKey_F3,  Input_KeyboardKey_F4,  Input_KeyboardKey_F5,
	Input_KeyboardKey_F6,  Input_KeyboardKey_F7,  Input_KeyboardKey_F8,  Input_KeyboardKey_F9,
	Input_KeyboardKey_F10, Input_KeyboardKey_F11, Input_KeyboardKey_F12, Input_KeyboardKey_F13,
	Input_KeyboardKey_F14,
	
	Input_KeyboardKey_Count,
}
typedef Input_KeyboardKey;

API bool32 Input_KeyboardIsPressed(int32 key);
API bool32 Input_KeyboardIsDown(int32 key);
API bool32 Input_KeyboardIsReleased(int32 key);
API bool32 Input_KeyboardIsUp(int32 key);

enum Input_MouseButton
{
	Input_MouseButton_Left,
	Input_MouseButton_Middle,
	Input_MouseButton_Right,
	Input_MouseButton_Other0,
	Input_MouseButton_Other1,
	Input_MouseButton_Other2,
	
	Input_MouseButton_Count,
}
typedef Input_MouseButton;

struct Input_Mouse
{
	// NOTE(ljre): Position relative to window.
	float32 pos[2];
	float32 old_pos[2];
	int32 scroll;
	
	bool8 buttons[Input_MouseButton_Count];
}
typedef Input_Mouse;

API void Input_GetMouse(Input_Mouse* out_mouse);

enum Input_GamepadButton
{
	Input_GamepadButton_A,
	Input_GamepadButton_B,
	Input_GamepadButton_X,
	Input_GamepadButton_Y,
	Input_GamepadButton_Left,
	Input_GamepadButton_Right,
	Input_GamepadButton_Up,
	Input_GamepadButton_Down,
	Input_GamepadButton_LB,
	Input_GamepadButton_RB,
	Input_GamepadButton_RS,
	Input_GamepadButton_LS,
	Input_GamepadButton_Start,
	Input_GamepadButton_Back,
	
	Input_GamepadButton_Count,
}
typedef Input_GamepadButton;

struct Input_Gamepad
{
	// [0] = left < 0 < right
	// [1] = up < 0 < down
	float32 left[2];
	float32 right[2];
	
	// 0..1
	float32 lt, rt;
	
	bool8 buttons[Input_GamepadButton_Count];
}
typedef Input_Gamepad;

// NOTE(ljre): Get the state of a gamepad. Returns true if gamepad is connected, false otherwise.
API bool32 Input_GetGamepad(int32 index, Input_Gamepad* out_gamepad);

// NOTE(ljre): Set the state of a gamepad. This may have no effect if gamepad or platform doesn't
//             support it.
API void Input_SetGamepad(int32 index, float32 vibration); // vibration = 0..1

// NOTE(ljre): Fetch the currently connected gamepads, write them to 'out_gamepads' array and
//             return how many were written, but never write more than 'max_count' gamepads.
//
//             If 'max_count' is less than 1, then just return the number of gamepads
//             currently connected.
API int32 Input_ConnectedGamepads(Input_Gamepad* out_gamepads, int32 max_count);

// Same thing as above, but write their indices instead of their states.
API int32 Input_ConnectedGamepadsIndices(int32* out_indices, int32 max_count);

// NOTE(ljre): Helper macros - takes a Input_Mouse or Input_Gamepad, and a button enum.
//             Returns true or false.
#define Input_IsPressed(state, btn) (!((state).buttons[btn] & 2) && ((state).buttons[btn] & 1))
#define Input_IsDown(state, btn) (((state).buttons[btn] & 1) != 0)
#define Input_IsReleased(state, btn) (((state).buttons[btn] & 2) && !((state).buttons[btn] & 1))
#define Input_IsUp(state, btn) (!((state).buttons[btn] & 1))

#endif //INTERNAL_API_PLATFORM_H
