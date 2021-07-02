#ifndef INTERNAL_H
#define INTERNAL_H

#ifdef __cplusplus
#   error This code should not be compiled as C++ code.
#endif

//~ Macros
#ifdef __GNUC__
#   define alignas(_) __attribute__((aligned(_)))
#else
#   define alignas(_) __declspec(align(_))
#endif

#if 0
;
#define restrict
#define NULL
#endif

#define internal static
#define global extern
#define true 1
#define false 0
#define API

#define AlignUp(x, mask) (((x) + (mask)) & ~(mask))
#define Kilobytes(n) ((uintsize)(n) * 1024)
#define Megabytes(n) (1024*Kilobytes(n))
#define Gigabytes(n) (1024*Megabytes(n))
#define ArrayLength(x) (sizeof(x) / sizeof(*(x)))
#define PI64 3.141592653589793238462643383279
#define PI32 ((float32)PI64)

//~ Standard headers & Libraries
#include <stdint.h>
#include <string.h>
#include <math.h>
#include <cglm/cglm.h>

//~ Types
typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;
typedef uintptr_t uintptr;
typedef size_t uintsize;

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;
typedef intptr_t intptr;
typedef ptrdiff_t intsize;

typedef int8_t bool8;
typedef int16_t bool16;
typedef int32_t bool32;
typedef int64_t bool64;

typedef float float32;
typedef double float64;

#if 0
#define UINT8_MIN 0
#define UINT8_MAX 255
#define UINT16_MIN 0
#define UINT16_MAX 65535
#define UINT32_MIN 0
#define UINT32_MAX ((uint32)4294967295ULL)
#define UINT64_MIN 0
#define UINT64_MAX ((uint64)18446744073709551615ULL)

#define INT8_MIN (-128)
#define INT8_MAX 127
#define INT16_MIN (-32768)
#define INT16_MAX 32767
#define INT32_MIN ((int32)-2147483648LL)
#define INT32_MAX ((int32)2147483647LL)
#define INT64_MIN ((int64)-9223372036854775808LL)
#define INT64_MAX ((int64)9223372036854775807LL)
#endif

//~ Debug
#ifndef DEBUG
#   define Assert(x) ((void)0)
#   define Trace(x) ((void)0)
#else
#   undef NDEBUG
#   include <assert.h>
#   define Assert assert
//#   define Trace(x) Platform_DebugLog("[TRACE] " x "\n")
#endif

#ifndef Trace
#   define Trace(x) ((void)0)
#endif

//~ Others
#include "internal_string.h"
#include "internal_opengl.h"

//~ Forward Declarations
API int32 Engine_Main(int32 argc, char** argv);
API uint64 Random_U64(void);
API uint32 Random_U32(void);
API float64 Random_F64(void);

//- Game
API int32 Game_MainScene(void);

//- Platform
enum GraphicsAPI
{
	GraphicsAPI_None = 0,
	
	GraphicsAPI_OpenGL = 1,
} typedef GraphicsAPI;

API void Platform_ExitWithErrorMessage(String message);
API void Platform_MessageBox(String title, String message);
API bool32 Platform_CreateWindow(int32 width, int32 height, String name, uint32 flags);
API bool32 Platform_WindowShouldClose(void);
API float64 Platform_CurrentTime(void);
API uint64 Platform_CurrentPosixTime(void);
API void Platform_PollEvents(void);
API void Platform_FinishFrame(void);
API const OpenGL_VTable* Platform_GetOpenGLVTable(void);

API void* Platform_HeapAlloc(uintsize size);
API void* Platform_HeapRealloc(void* ptr, uintsize size);
API void Platform_HeapFree(void* ptr);
API void* Platform_VirtualAlloc(uintsize size);
API void Platform_VirtualCommit(void* ptr, uintsize size);
API void Platform_VirtualFree(void* ptr);

#ifdef DEBUG
API void Platform_DebugMessageBox(const char* restrict format, ...);
API void Platform_DebugLog(const char* restrict format, ...);
#else // DEBUG
#   define Platform_DebugMessageBox(...) ((void)0)
#   define Platform_DebugLog(...) ((void)0)
#endif // DEBUG

//- Platform Audio
API int16* Platform_RequestSoundBuffer(uint32* out_sample_count, uint32* out_channels, uint32* out_sample_rate);

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
	// NOTE(ljre): Try to not messup. Input_KeyboardKey_Space = 32, and any of those above can be (but shouldn't)
	
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
} typedef Input_KeyboardKey;

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
} typedef Input_MouseButton;

struct Input_Mouse
{
	float32 pos[2];
	float32 old_pos[2];
	int32 scroll;
	
	bool8 buttons[Input_MouseButton_Count];
} typedef Input_Mouse;

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
} typedef Input_GamepadButton;

struct Input_Gamepad
{
	// [0] = left < 0 < right
	// [1] = up < 0 < down
	float32 left[2];
	float32 right[2];
	
	// 0..1
	float32 lt, rt;
	
	bool8 buttons[Input_GamepadButton_Count];
} typedef Input_Gamepad;

// NOTE(ljre): Get the state of a gamepad. Returns true if gamepad is connected, false otherwise.
API bool32 Input_GetGamepad(int32 index, Input_Gamepad* out_gamepad);

// NOTE(ljre): Set the state of a gamepad. This may have no effect if gamepad or platform doesn't
//             support it.
API void Input_SetGamepad(int32 index, float32 vibration); // vibration = 0..1

// NOTE(ljre): Fetch the currently connected gamepads and write them to 'out_gamepads' array and
//             return how many were written, but never write more than 'max_count' gamepads.
//
//             If 'max_count' is not greater than 0, then just return the number of gamepads
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

#endif //INTERNAL_H
