#include "internal.h"

//- Includes
DisableWarnings();

#define WIN32_LEAN_AND_MEAN
#define COBJMACROS
#define DIRECTINPUT_VERSION 0x0800

#define near
#define far

// General
#include <windows.h>
#include <dwmapi.h>
#include <versionhelpers.h>
#include <dbt.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <synchapi.h>

// Input
#include <xinput.h>
#include <dinput.h>

// Audio
#include <audioclient.h>
#include <audiopolicy.h>
#include <mmdeviceapi.h>

// NOTE(ljre): MinGW decls
void WINAPI AcquireSRWLockExclusive(PSRWLOCK SRWLock);
void WINAPI ReleaseSRWLockExclusive(PSRWLOCK SRWLock);

#undef near
#undef far

#include "platform_win32_guid.c"

ReenableWarnings();

#define ENABLE_SEH_IN_MAIN

#if defined(_MSC_VER)
#   pragma comment(lib, "kernel32.lib")
#   pragma comment(lib, "user32.lib")
#   pragma comment(lib, "gdi32.lib")
#   pragma comment(lib, "hid.lib")
#   ifdef ENABLE_SEH_IN_MAIN
#       pragma comment(lib, "dbghelp.lib")
#       include <minidumpapiset.h>
#   endif
#endif

//~ Globals
static HANDLE global_heap;
static const wchar_t* global_class_name;
static HINSTANCE global_instance;
static int64 global_time_frequency;
static int64 global_process_started_time;
static HWND global_window;
static HDC global_hdc;
static bool global_lock_cursor;
static Platform_GraphicsContext global_graphics_context;
static Platform_Data global_platform_data = { 0 };
static RECT global_monitor;

//~ Internal API
static int64
Win32_GetTimer(void)
{
	LARGE_INTEGER value;
	QueryPerformanceCounter(&value);
	return value.QuadPart;
}

static void
Win32_CheckForErrors(void)
{
	DWORD error = GetLastError();
	if (error != ERROR_SUCCESS)
	{
		char buffer[512];
		String_PrintfBuffer(buffer, sizeof(buffer), "%U", (uint64)error);
		MessageBoxA(NULL, buffer, "Error Code", MB_OK);
	}
}

static void
Win32_ExitWithErrorMessage(const wchar_t* message)
{
	MessageBoxW(NULL, message, L"Error", MB_OK);
	exit(1);
}

static wchar_t*
Win32_ConvertStringToWSTR(String str, wchar_t* buffer, uintsize size)
{
	if (!buffer)
	{
		size = 1 + (uintsize)MultiByteToWideChar(CP_UTF8, 0, (char*)str.data, (int32)str.size, NULL, 0);
		buffer = HeapAlloc(global_heap, 0, size * sizeof(wchar_t));
	}
	
	size = MultiByteToWideChar(CP_UTF8, 0, (char*)str.data, (int32)str.size, buffer, (int32)size - 1);
	buffer[size] = 0;
	
	return buffer;
}

static inline HMODULE
Win32_LoadLibrary(const char* name)
{
	HMODULE result = LoadLibraryA(name);
	
	if (result)
		Platform_DebugLog("Loaded Library: %s\n", name);
	
	return result;
}

static void
Win32_UpdatePlatformConfigIfNeeded(Platform_Data* inout_data)
{
	// TODO(ljre): Update Graphics API at runtime
	Assert(inout_data->graphics_api == global_platform_data.graphics_api);
	
	//- NOTE(ljre): Simple-idk-how-to-call-it data.
	{
		inout_data->window_should_close = global_platform_data.window_should_close;
		inout_data->user_resized_window = global_platform_data.user_resized_window;
		
		if (global_platform_data.user_resized_window)
		{
			inout_data->window_width = global_platform_data.window_width;
			inout_data->window_height = global_platform_data.window_height;
		}
	}
	
	//- NOTE(ljre): Window Position & Size
	{
		UINT flags = SWP_NOSIZE | SWP_NOMOVE;
		
		int32 x = inout_data->window_x;
		int32 y = inout_data->window_y;
		int32 width = inout_data->window_width;
		int32 height = inout_data->window_height;
		
		if (width != global_platform_data.window_width || height != global_platform_data.window_height)
			flags &= ~(UINT)SWP_NOSIZE;
		
		if (inout_data->center_window)
		{
			inout_data->window_x = x = (global_monitor.right - global_monitor.left) / 2 - width / 2;
			inout_data->window_y = y = (global_monitor.bottom - global_monitor.top) / 2 - height / 2;
			inout_data->center_window = false;
			
			flags &= ~(UINT)SWP_NOMOVE;
		}
		else if (x != global_platform_data.window_x || y != global_platform_data.window_y)
			flags &= ~(UINT)SWP_NOMOVE;
		
		if (flags)
			SetWindowPos(global_window, NULL, x, y, width, height, flags | SWP_NOZORDER);
	}
	
	//- NOTE(ljre): Window Title
	{
		if (!String_Equals(inout_data->window_title, global_platform_data.window_title))
		{
			wchar_t* name = Win32_ConvertStringToWSTR(inout_data->window_title, NULL, 0);
			SetWindowTextW(global_window, name);
			HeapFree(global_heap, 0, name);
		}
	}
	
	//- NOTE(ljre): Cursor
	{
		if (inout_data->show_cursor != global_platform_data.show_cursor)
			ShowCursor(inout_data->show_cursor);
		
		global_lock_cursor = inout_data->lock_cursor;
	}
	
	//- NOTE(ljre): Sync
	global_platform_data = *inout_data;
	global_platform_data.user_resized_window = false;
	//global_platform_data.window_should_close = false;
}

//~ Entry Point
#include "platform_win32_input.c"
#include "platform_win32_audio.c"
#include "platform_win32_opengl.c"
#include "platform_win32_direct3d.c"

//~ Functions
static LRESULT CALLBACK
WindowProc(HWND window, UINT message, WPARAM wparam, LPARAM lparam)
{
	LRESULT result = 0;
	Engine_InputData* input = (void*)GetWindowLongPtrW(window, GWLP_USERDATA);
	
	switch (message)
	{
		case WM_CLOSE:
		case WM_DESTROY:
		case WM_QUIT:
		{
			if (global_window == window)
				global_platform_data.window_should_close = true;
		} break;
		
		case WM_SIZE:
		{
			global_platform_data.window_width = LOWORD(lparam);
			global_platform_data.window_height = HIWORD(lparam);
			
			global_platform_data.user_resized_window = true;
		} break;
		
		case WM_SYSKEYDOWN:
		case WM_SYSKEYUP:
		case WM_KEYUP:
		case WM_KEYDOWN:
		{
			uint32 vkcode = (uint32)wparam;
			bool was_down = ((lparam & (1 << 30)) != 0);
			bool is_down = ((lparam & (1 << 31)) == 0);
			
			(void)was_down;
			
			if (vkcode >= ArrayLength(global_keyboard_key_table))
				break;
			
			// NOTE(ljre): Checks for which side specifically.
			// TODO(ljre): This might fuck up a bit the buffering...
			if (vkcode == VK_SHIFT)
			{
				Win32_UpdateKeyboardKey(input, VK_LSHIFT, !!(GetKeyState(VK_LSHIFT) & 0x8000));
				Win32_UpdateKeyboardKey(input, VK_RSHIFT, !!(GetKeyState(VK_RSHIFT) & 0x8000));
			}
			else if (vkcode == VK_CONTROL)
			{
				Win32_UpdateKeyboardKey(input, VK_LCONTROL, !!(GetKeyState(VK_LCONTROL) & 0x8000));
				Win32_UpdateKeyboardKey(input, VK_RCONTROL, !!(GetKeyState(VK_RCONTROL) & 0x8000));
			}
			else
				Win32_UpdateKeyboardKey(input, vkcode, is_down);
			
			// NOTE(ljre): Always close on Alt+F4
			if (vkcode == VK_F4 && GetKeyState(VK_MENU) & 0x8000)
				global_platform_data.window_should_close = true;
		} break;
		
		case WM_LBUTTONUP: Win32_UpdateMouseButton(input, Engine_MouseButton_Left, false); break;
		case WM_LBUTTONDOWN: Win32_UpdateMouseButton(input, Engine_MouseButton_Left, true); break;
		case WM_MBUTTONUP: Win32_UpdateMouseButton(input, Engine_MouseButton_Middle, false); break;
		case WM_MBUTTONDOWN: Win32_UpdateMouseButton(input, Engine_MouseButton_Middle, true); break;
		case WM_RBUTTONUP: Win32_UpdateMouseButton(input, Engine_MouseButton_Right, false); break;
		case WM_RBUTTONDOWN: Win32_UpdateMouseButton(input, Engine_MouseButton_Right, true); break;
		
		case WM_MOUSEWHEEL:
		{
			int32 delta = GET_WHEEL_DELTA_WPARAM(wparam) / 120;
			input->mouse.scroll += delta;
		} break;
		
		case WM_DEVICECHANGE:
		{
			Win32_CheckForGamepads();
		} break;
		
		default:
		{
			result = DefWindowProcW(window, message, wparam, lparam);
		} break;
	}
	
	return result;
}

#ifdef INTERNAL_ENABLE_HOT
API
#endif
int WINAPI
WinMain(HINSTANCE instance, HINSTANCE previous, LPSTR args, int cmd_show)
{
	Trace();
	
	// NOTE(ljre): __argc and __argv - those are public symbols from the CRT
	int32 argc = __argc;
	char** argv = __argv;
	
	//- Init
	{
		LARGE_INTEGER value;
		QueryPerformanceFrequency(&value);
		global_time_frequency = value.QuadPart;
	}
	
	global_process_started_time = Win32_GetTimer();
	global_heap = GetProcessHeap();
	global_instance = instance;
	global_class_name = L"GameClassName";
	
	// NOTE(ljre): Register window class
	{
		WNDCLASSW window_class = {
			.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW,
			.lpfnWndProc = WindowProc,
			.lpszClassName = global_class_name,
			.hInstance = instance,
			.hCursor = LoadCursorA(NULL, IDC_ARROW),
		};
		
		if (!RegisterClassW(&window_class))
			Win32_ExitWithErrorMessage(L"Could not create window class.");
	}
	
	// NOTE(ljre): Get Monitor Size
	{
		HMONITOR monitor = MonitorFromWindow(global_window, MONITOR_DEFAULTTONEAREST);
		MONITORINFO info;
		info.cbSize = sizeof info;
		
		if (GetMonitorInfoA(monitor, &info))
		{
			global_monitor = info.rcWork;
		}
	}
	
	//- Run
	int32 result;
	
#if !defined(_MSC_VER) || !defined(ENABLE_SEH_IN_MAIN)
	result = Engine_Main(argc, argv);
#else
	MINIDUMP_EXCEPTION_INFORMATION info = { 0 };
	
	__try
	{
		result = Engine_Main(argc, argv);
	}
	__except (info.ExceptionPointers = GetExceptionInformation(),
		GetExceptionCode() == EXCEPTION_BREAKPOINT ? EXCEPTION_CONTINUE_SEARCH : EXCEPTION_EXECUTE_HANDLER)
	{
		HANDLE file = CreateFileW(L"minidump.dmp", GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
		
		if (file)
		{
			info.ThreadId = GetCurrentThreadId();
			info.ClientPointers = FALSE;
			
			MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), file,
				MiniDumpNormal, &info, NULL, NULL);
			
			CloseHandle(file);
		}
		
		ExitProcess(2);
	}
#endif
	
	// NOTE(ljre): Free resources... or nah :P
	//Win32_DeinitAudio();
	
	return result;
}

//~ API
API void
Platform_ExitWithErrorMessage(String message)
{
	wchar_t* msg = Win32_ConvertStringToWSTR(message, NULL, 0);
	Win32_ExitWithErrorMessage(msg);
	/* no return */
}

API void
Platform_MessageBox(String title, String message)
{
	Trace();
	
	wchar_t* wtitle = Win32_ConvertStringToWSTR(title, NULL, 0);
	wchar_t* wmessage = Win32_ConvertStringToWSTR(message, NULL, 0);
	
	MessageBoxW(NULL, wmessage, wtitle, MB_OK);
	
	Platform_HeapFree(wtitle);
	Platform_HeapFree(wmessage);
}

API void
Platform_DefaultData(Platform_Data* out_data)
{
	*out_data = (Platform_Data) {
		.show_cursor = true,
		.lock_cursor = false,
		.center_window = true,
		.fullscreen = false,
		.window_should_close = false,
		
		.window_x = 0,
		.window_y = 0,
		.window_width = 800,
		.window_height = 600,
		.window_title = StrInit(""),
		
		.graphics_api = Platform_GraphicsApi_OpenGL,
	};
}

API bool
Platform_CreateWindow(Platform_Data* config, const Platform_GraphicsContext** out_graphics, Engine_InputData* input_data)
{
	Trace();
	
	if (config->center_window)
	{
		config->window_x = (global_monitor.right - global_monitor.left) / 2 - config->window_width / 2;
		config->window_y = (global_monitor.bottom - global_monitor.top) / 2 - config->window_height / 2;
		
		config->center_window = false;
	}
	
	wchar_t window_name[1024];
	Win32_ConvertStringToWSTR(config->window_title, window_name, ArrayLength(window_name));
	
	bool ok = false;
	
	if (config->graphics_api & Platform_GraphicsApi_OpenGL)
		ok = ok || Win32_CreateOpenGLWindow(config, window_name);
	if (config->graphics_api & Platform_GraphicsApi_Direct3D)
		ok = ok || Win32_CreateDirect3DWindow(config, window_name);
	
	if (ok)
	{
		DEV_BROADCAST_DEVICEINTERFACE notification_filter = {
			sizeof(notification_filter),
			DBT_DEVTYP_DEVICEINTERFACE
		};
		
		if (!RegisterDeviceNotification(global_window, &notification_filter, DEVICE_NOTIFY_WINDOW_HANDLE))
		{
			// TODO
		}
		
		Win32_InitInput();
		Win32_InitAudio();
		config->user_resized_window = false;
		config->window_should_close = false;
		global_platform_data = *config;
		
		Mem_Set(input_data, 0, sizeof(*input_data));
		Platform_PollEvents(config, input_data);
		
		*out_graphics = &global_graphics_context;
		
		ShowWindow(global_window, SW_SHOWDEFAULT);
	}
	
	return ok;
}

API float64
Platform_GetTime(void)
{
	int64 time = Win32_GetTimer();
	return (float64)(time - global_process_started_time) / (float64)global_time_frequency;
}

API void
Platform_PollEvents(Platform_Data* inout_data, Engine_InputData* out_input_data)
{
	Trace();
	
	SetWindowLongPtrW(global_window, GWLP_USERDATA, (LONG_PTR)out_input_data);
	
	Win32_UpdateInputEarly(out_input_data);
	Win32_UpdatePlatformConfigIfNeeded(inout_data);
	
	MSG message;
	while (PeekMessageW(&message, 0, 0, 0, PM_REMOVE))
	{
		TranslateMessage(&message);
		DispatchMessageW(&message);
	}
	
	Win32_UpdateInputLate(out_input_data);
}

API void
Platform_FinishFrame(void)
{
	Trace();
	
	switch (global_graphics_context.api)
	{
		case Platform_GraphicsApi_OpenGL: Win32_OpenGLSwapBuffers(); break;
		case Platform_GraphicsApi_Direct3D: Win32_Direct3DSwapBuffers(); break;
		default: {} break;
	}
}

API void*
Platform_HeapAlloc(uintsize size)
{
	Trace(); TraceF(64, "%fKiB", (float64)size / 1024.0);
	
	void* result = HeapAlloc(global_heap, HEAP_ZERO_MEMORY, size);
	
	return result;
}

API void*
Platform_HeapRealloc(void* ptr, uintsize size)
{
	Trace(); TraceF(64, "%fKiB", (float64)size / 1024.0);
	
	void* result;
	
	if (ptr)
		result = HeapReAlloc(global_heap, HEAP_ZERO_MEMORY, ptr, size);
	else
		result = HeapAlloc(global_heap, HEAP_ZERO_MEMORY, size);
	
	return result;
}

API void
Platform_HeapFree(void* ptr)
{
	Trace();
	if (!ptr)
		return;
	
	HeapFree(global_heap, 0, ptr);
}

API void*
Platform_VirtualReserve(uintsize size)
{
	Trace();
	
	void* result = VirtualAlloc(NULL, size, MEM_RESERVE, PAGE_READWRITE);
	return result;
}

API void
Platform_VirtualCommit(void* ptr, uintsize size)
{
	Trace();
	VirtualAlloc(ptr, size, MEM_COMMIT, PAGE_READWRITE);
}

API void
Platform_VirtualDecommit(void* ptr, uintsize size)
{
	Trace();
	VirtualFree(ptr, size, MEM_DECOMMIT);
}

API void
Platform_VirtualFree(void* ptr, uintsize size)
{
	Trace();
	VirtualFree(ptr, 0, MEM_RELEASE);
	(void)size;
}

API void*
Platform_ReadEntireFile(String path, uintsize* out_size, Arena* opt_arena)
{
	Trace(); TraceText(path);
	wchar_t wpath[1024];
	Win32_ConvertStringToWSTR(path, wpath, ArrayLength(wpath));
	
	HANDLE handle = CreateFileW(wpath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
	if (handle == INVALID_HANDLE_VALUE)
		return NULL;
	
	LARGE_INTEGER sizeStruct;
	if (!GetFileSizeEx(handle, &sizeStruct))
	{
		CloseHandle(handle);
		return NULL;
	}
	
	uintsize size = sizeStruct.QuadPart;
	void* memory = opt_arena ? Arena_Push(opt_arena, size) : Platform_HeapAlloc(size);
	if (!memory)
	{
		CloseHandle(handle);
		return NULL;
	}
	
	DWORD bytes_read;
	if (!ReadFile(handle, memory, (DWORD)size, &bytes_read, NULL) || bytes_read != size)
	{
		if (!opt_arena)
			Platform_HeapFree(memory);
		memory = NULL;
		out_size = NULL;
	}
	
	if (out_size)
		*out_size = bytes_read;
	
	CloseHandle(handle);
	
	return memory;
}

API bool
Platform_WriteEntireFile(String path, const void* data, uintsize size)
{
	Trace(); TraceText(path);
	
	wchar_t wpath[1024];
	Win32_ConvertStringToWSTR(path, wpath, ArrayLength(wpath));
	
	bool32 result = true;
	DWORD bytes_written;
	
	HANDLE handle = CreateFileW(wpath, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
	if (handle == INVALID_HANDLE_VALUE)
		return false;
	
	if (!WriteFile(handle, data, (DWORD)size, &bytes_written, 0) ||
		bytes_written != size)
	{
		result = false;
	}
	
	CloseHandle(handle);
	
	return result;
}

API void
Platform_FreeFileMemory(void* ptr, uintsize size)
{
	// NOTE(ljre): Ignore parameter 'size' for this platform.
	(void)size;
	
	Platform_HeapFree(ptr);
}

API uint64
Platform_CurrentPosixTime(void)
{
	time_t result = time(NULL);
	if (result == -1)
		result = 0;
	
	return (uint64)result;
}

API void*
Platform_LoadDiscordLibrary(void)
{
	HMODULE library = Win32_LoadLibrary("discord_game_sdk.dll");
	if (!library)
		return NULL;
	
	void* result = GetProcAddress(library, "DiscordCreate");
	
	if (!result)
	{
		FreeLibrary(library);
		return NULL;
	}
	
	return result;
}

#ifdef INTERNAL_ENABLE_HOT
static uint64
GetFileLastWriteTime(const char* filename)
{
	FILETIME last_write_time = { 0 };
	WIN32_FIND_DATA find_data;
	HANDLE find_handle = FindFirstFileA(filename, &find_data);
	
	if(find_handle != INVALID_HANDLE_VALUE)
	{
		FindClose(find_handle);
		last_write_time = find_data.ftLastWriteTime;
	}
	
	uint64 result = 0;
	
	result |= (uint64)(last_write_time.dwLowDateTime);
	result |= (uint64)(last_write_time.dwHighDateTime) << 32;
	
	return result;
}

API void*
Platform_LoadGameLibrary(void)
{
	Trace();
	
	static HMODULE library = NULL;
	static uint64 saved_last_update;
	static void* saved_result;
	
	void* result = saved_result;
	
	if (!library)
	{
		const char* target_dll_path = "./build/game_.dll";
		if (!CopyFileA("./build/game.dll", target_dll_path, false))
		{
			// NOTE(ljre): fallback to the original file
			target_dll_path = "./build/game.dll";
			Platform_DebugLog("%x\n", (uint32)GetLastError());
		}
		else
			saved_last_update = GetFileLastWriteTime("./build/game.dll");
		
		library = Win32_LoadLibrary(target_dll_path);
		Assert(library);
		
		result = GetProcAddress(library, "Game_Main");
	}
	else
	{
		uint64 last_update = GetFileLastWriteTime("./build/game.dll");
		
		if (last_update > saved_last_update)
		{
			FreeLibrary(library);
			
			const char* target_dll_path = "./build/game_.dll";
			if (!CopyFileA("./build/game.dll", target_dll_path, false))
				// NOTE(ljre): fallback to the original file
				target_dll_path = "./build/game.dll";
			else
				saved_last_update = last_update;
			
			library = Win32_LoadLibrary(target_dll_path);
			Assert(library);
			
			result = GetProcAddress(library, "Game_Main");
			
			SetForegroundWindow(global_window);
		}
	}
	
	Assert(result);
	
	saved_result = result;
	return result;
}
#endif

//- Debug
#ifdef DEBUG
API void
Platform_DebugError(const char* msg)
{
	MessageBoxA(NULL, msg, "Debug Error", MB_OK);
	exit(1);
	/* no return */
}

API void
Platform_DebugMessageBox(const char* restrict format, ...)
{
	char buffer[16 << 10];
	
	va_list args;
	va_start(args, format);
	String_VPrintfBuffer(buffer, sizeof(buffer), format, args);
	MessageBoxA(NULL, buffer, "Debug", MB_OK | MB_TOPMOST);
	va_end(args);
}

API void
Platform_DebugLog(const char* restrict format, ...)
{
	char buffer[16 << 10];
	
	va_list args;
	va_start(args, format);
	String_VPrintfBuffer(buffer, sizeof(buffer), format, args);
	OutputDebugStringA(buffer);
	va_end(args);
}
#endif // DEBUG
