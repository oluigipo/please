#include "internal.h"

//- Disable Warnings
#if defined(__clang__)
#   pragma clang diagnostic push
#   pragma clang diagnostic ignored "-Weverything"
#else
#   pragma warning(push, 0)
#endif

//- Includes
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

//- Enable Warnings
#if defined(__clang__)
#   pragma clang diagnostic pop
#else
#   pragma warning(pop, 0)
#endif

#if defined(_MSC_VER)
#   pragma comment(lib, "user32.lib")
#   pragma comment(lib, "gdi32.lib")
#   pragma comment(lib, "hid.lib")
#endif

//~ Globals
internal HANDLE global_heap;
internal const wchar_t* global_class_name;
internal HINSTANCE global_instance;
internal int64 global_time_frequency;
internal int64 global_process_started_time;
internal HWND global_window;
internal HDC global_hdc;
internal bool32 global_window_should_close;
internal bool32 global_lock_cursor;
internal GraphicsContext global_graphics_context;
internal Platform_Config global_config = { 0 };

internal int32 global_window_width;
internal int32 global_window_height;
internal RECT global_monitor;

//~ Internal API
internal int64
Win32_GetTimer(void)
{
	LARGE_INTEGER value;
	QueryPerformanceCounter(&value);
	return value.QuadPart;
}

internal void
Win32_CheckForErrors(void)
{
	DWORD error = GetLastError();
	if (error != ERROR_SUCCESS)
	{
		char buffer[512];
		snprintf(buffer, sizeof buffer, "%llu", (uint64)error);
		MessageBoxA(NULL, buffer, "Error Code", MB_OK);
	}
}

internal void
Win32_ExitWithErrorMessage(const wchar_t* message)
{
	MessageBoxW(NULL, message, L"Error", MB_OK);
	exit(1);
}

internal wchar_t*
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

internal inline HMODULE
Win32_LoadLibrary(const char* name)
{
	HMODULE result = LoadLibraryA(name);
	
	if (result)
		Platform_DebugLog("Loaded Library: %s\n", name);
	
	return result;
}

//~ Functions
internal void
ProcessDeferredEvents(void)
{
	// NOTE(ljre): Window Position & Size
	{
		UINT flags = SWP_NOSIZE | SWP_NOMOVE;
		
		int32 x = global_config.window_x;
		int32 y = global_config.window_y;
		int32 width = global_config.window_width;
		int32 height = global_config.window_height;
		
		if (width == 0)
			width = global_window_width;
		else
			flags &=~ (UINT)SWP_NOSIZE;
		
		if (height == 0)
			height = global_window_height;
		else
			flags &=~ (UINT)SWP_NOSIZE;
		
		if (global_config.center_window > 0)
		{
			x = (global_monitor.right - global_monitor.left) / 2 - width / 2;
			y = (global_monitor.bottom - global_monitor.top) / 2 - height / 2;
			
			flags &=~ (UINT)SWP_NOMOVE;
		}
		else if (x != 0 || y != 0)
			flags &=~ (UINT)SWP_NOMOVE;
		
		if (flags != (SWP_NOSIZE | SWP_NOMOVE))
			SetWindowPos(global_window, NULL, x, y, width, height, flags | SWP_NOZORDER);
	}
	
	// NOTE(ljre): Window Title
	{
		if (global_config.window_title.size > 0)
		{
			wchar_t* name = Win32_ConvertStringToWSTR(global_config.window_title, NULL, 0);
			SetWindowTextW(global_window, name);
			HeapFree(global_heap, 0, name);
		}
	}
	
	// NOTE(ljre): Cursor
	{
		static bool32 previous_show = true;
		bool32 show = !!(global_config.show_cursor + 1);
		
		if (global_config.show_cursor && previous_show != show)
		{
			ShowCursor(show);
			previous_show = show;
		}
		
		if (global_config.lock_cursor)
		{
			global_lock_cursor = !!(global_config.lock_cursor + 1);
		}
	}
	
	// NOTE(ljre): Reset config.
	MemSet(&global_config, 0, sizeof(global_config));
}

//~ Entry Point
#include "platform_win32_input.c"
#include "platform_win32_audio.c"
#include "platform_win32_opengl.c"
#include "platform_win32_direct3d.c"

internal LRESULT CALLBACK
WindowProc(HWND window, UINT message, WPARAM wparam, LPARAM lparam)
{
	LRESULT result = 0;
	
	switch (message)
	{
		case WM_CLOSE:
		case WM_DESTROY:
		case WM_QUIT:
		{
			if (global_window == window)
				global_window_should_close = true;
		} break;
		
		case WM_SIZE:
		{
			global_window_width = LOWORD(lparam);
			global_window_height = HIWORD(lparam);
		} break;
		
		case WM_SYSKEYDOWN:
		case WM_SYSKEYUP:
		case WM_KEYUP:
		case WM_KEYDOWN:
		{
			uint32 vkcode = (uint32)wparam;
			bool32 was_down = ((lparam & (1 << 30)) != 0);
			bool32 is_down = ((lparam & (1 << 31)) == 0);
			
			if (was_down == is_down || vkcode >= ArrayLength(global_keyboard_key_table))
				break;
			
			Win32_UpdateKeyboardKey(vkcode, is_down);
			
			// NOTE(ljre): Checks for which side specifically.
			if (vkcode == VK_SHIFT)
			{
				Win32_UpdateKeyboardKey(VK_LSHIFT, !!(GetKeyState(VK_LSHIFT) & 0x8000));
				Win32_UpdateKeyboardKey(VK_RSHIFT, !!(GetKeyState(VK_RSHIFT) & 0x8000));
			}
			else if (vkcode == VK_CONTROL)
			{
				Win32_UpdateKeyboardKey(VK_LCONTROL, !!(GetKeyState(VK_LCONTROL) & 0x8000));
				Win32_UpdateKeyboardKey(VK_RCONTROL, !!(GetKeyState(VK_RCONTROL) & 0x8000));
			}
			
			// NOTE(ljre): Always close on Alt+F4
			if (vkcode == VK_F4 && GetKeyState(VK_MENU) & 0x8000)
				global_window_should_close = true;
		} break;
		
		case WM_LBUTTONUP: global_mouse.buttons[Input_MouseButton_Left] &=~ 1; break;
		case WM_LBUTTONDOWN: global_mouse.buttons[Input_MouseButton_Left] |= 1; break;
		case WM_MBUTTONUP: global_mouse.buttons[Input_MouseButton_Middle] &=~ 1; break;
		case WM_MBUTTONDOWN: global_mouse.buttons[Input_MouseButton_Middle] |= 1; break;
		case WM_RBUTTONUP: global_mouse.buttons[Input_MouseButton_Right] &=~ 1; break;
		case WM_RBUTTONDOWN: global_mouse.buttons[Input_MouseButton_Right] |= 1; break;
		
		case WM_MOUSEWHEEL:
		{
			int32 delta = GET_WHEEL_DELTA_WPARAM(wparam) / 120;
			global_mouse.scroll += delta;
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

#ifndef INTERNAL_ENABLE_HOT
int WINAPI
WinMain(HINSTANCE instance, HINSTANCE previous, LPSTR args, int cmd_show)
#else
API int WINAPI
WinMain(HINSTANCE instance, HINSTANCE previous, LPSTR args, int cmd_show)
#endif
{
	Trace("WinMain - Program Entry Point");
	
	// NOTE(ljre): __argc and __argv - those are public symbols
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
	global_window_should_close = false;
	global_class_name = L"GameClassName";
	
	WNDCLASSW window_class = {
		.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW,
		.lpfnWndProc = WindowProc,
		.lpszClassName = global_class_name,
		.hInstance = instance,
	};
	
	if (!RegisterClassW(&window_class))
		Win32_ExitWithErrorMessage(L"Could not create window class.");
	
	//- Run
	int32 result = Engine_Main(argc, argv);
	
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
	Trace("Platform_MessageBox");
	
	wchar_t* wtitle = Win32_ConvertStringToWSTR(title, NULL, 0);
	wchar_t* wmessage = Win32_ConvertStringToWSTR(message, NULL, 0);
	
	MessageBoxW(NULL, wmessage, wtitle, MB_OK);
	
	Platform_HeapFree(wtitle);
	Platform_HeapFree(wmessage);
}

API bool32
Platform_CreateWindow(const Platform_Config* config, const GraphicsContext** out_graphics)
{
	Trace("Platform_CreateWindow");
	
	wchar_t window_name[1024];
	Win32_ConvertStringToWSTR(config->window_title, window_name, ArrayLength(window_name));
	
	bool32 ok = false;
	
	if (config->graphics_api & GraphicsAPI_OpenGL)
		ok = ok || Win32_CreateOpenGLWindow(config->window_width, config->window_height, window_name);
	if (config->graphics_api & GraphicsAPI_Direct3D)
		ok = ok || Win32_CreateDirect3DWindow(config->window_width, config->window_height, window_name);
	
	if (ok)
	{
		DEV_BROADCAST_DEVICEINTERFACE notification_filter = {
			sizeof(notification_filter),
			DBT_DEVTYP_DEVICEINTERFACE
		};
		
		if (NULL == RegisterDeviceNotification(global_window, &notification_filter, DEVICE_NOTIFY_WINDOW_HANDLE))
		{
			// TODO
		}
		
		global_window_width = config->window_width;
		global_window_height = config->window_height;
		
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
		
		Win32_InitInput();
		Win32_InitAudio();
		global_config.center_window = config->center_window;
		global_config.show_cursor = config->show_cursor;
		global_config.lock_cursor = config->lock_cursor;
		Platform_PollEvents();
		
		*out_graphics = &global_graphics_context;
		ShowWindow(global_window, SW_SHOWDEFAULT);
		SetCursor(LoadCursorA(NULL, IDC_ARROW));
	}
	
	return ok;
}

API bool32
Platform_WindowShouldClose(void)
{
	return global_window_should_close;
}

API int32
Platform_WindowWidth(void)
{
	return global_window_width;
}

API int32
Platform_WindowHeight(void)
{
	return global_window_height;
}

API void
Platform_UpdateConfig(const Platform_Config* config)
{
	MemCopy(&global_config, config, sizeof(global_config));
}

API float64
Platform_GetTime(void)
{
	int64 time = Win32_GetTimer();
	return (float64)(time - global_process_started_time) / (float64)global_time_frequency;
}

API void
Platform_PollEvents(void)
{
	Trace("Platform_PollEvents");
	
	Win32_UpdateInputPre();
	ProcessDeferredEvents();
	
	MSG message;
	while (PeekMessageW(&message, 0, 0, 0, PM_REMOVE))
	{
		TranslateMessage(&message);
		DispatchMessageW(&message);
	}
	
	Win32_UpdateInputPos();
}

API void
Platform_FinishFrame(void)
{
	Trace("Platform_FinishFrame");
	
	switch (global_graphics_context.api)
	{
		case GraphicsAPI_OpenGL: Win32_OpenGLSwapBuffers(); break;
		case GraphicsAPI_Direct3D: Win32_Direct3DSwapBuffers(); break;
		default: {} break;
	}
}

API void*
Platform_HeapAlloc(uintsize size)
{
	Trace("Platform_HeapAlloc");
	
	void* result = HeapAlloc(global_heap, HEAP_ZERO_MEMORY, size);
	
	return result;
}

API void*
Platform_HeapRealloc(void* ptr, uintsize size)
{
	Trace("Platform_HeapRealloc");
	
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
	Trace("Platform_HeapFree");
	if (!ptr)
		return;
	
	HeapFree(global_heap, 0, ptr);
}

API void*
Platform_VirtualReserve(uintsize size)
{
	Trace("Platform_VirtualAlloc");
	
	void* result = VirtualAlloc(NULL, size, MEM_RESERVE, PAGE_READWRITE);
	return result;
}

API void
Platform_VirtualCommit(void* ptr, uintsize size)
{
	Trace("Platform_VirtualCommit");
	VirtualAlloc(ptr, size, MEM_COMMIT, PAGE_READWRITE);
}

API void
Platform_VirtualFree(void* ptr, uintsize size)
{
	Trace("Platform_VirtualFree");
	VirtualFree(ptr, 0, MEM_RELEASE);
	(void)size;
}

API void*
Platform_ReadEntireFile(String path, uintsize* out_size, Arena* opt_arena)
{
	Trace("Platform_ReadEntireFile");
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

API bool32
Platform_WriteEntireFile(String path, const void* data, uintsize size)
{
	Trace("Platform_WriteEntireFile");
	
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
internal uint64
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
	char buffer[Kilobytes(16)];
	
	va_list args;
	va_start(args, format);
	vsnprintf(buffer, sizeof buffer, format, args);
	MessageBoxA(NULL, buffer, "Debug", MB_OK | MB_TOPMOST);
	va_end(args);
}

API void
Platform_DebugLog(const char* restrict format, ...)
{
	char buffer[Kilobytes(16)];
	
	va_list args;
	va_start(args, format);
	vsnprintf(buffer, sizeof buffer, format, args);
	OutputDebugStringA(buffer);
	va_end(args);
}

API void
Platform_DebugDumpBitmap(const char* restrict path, const void* data, int32 width, int32 height, int32 channels)
{
	FILE* file = fopen(path, "w");
	if (!file)
		return;
	
	fprintf(file, "P3\n%i %i 255\n", width, height);
	int32 size = width*height;
	const uint8* pixels = data;
	
	for (int32 i = 0; i < size; ++i)
	{
		int32 r, g, b;
		
		switch (channels)
		{
			case 1:
			{
				r = *pixels;
				g = *pixels;
				b = *pixels;
				
				++pixels;
			} break;
			
			case 2:
			{
				r = pixels[0];
				g = pixels[1];
				b = 0;
				
				pixels += 2;
			} break;
			
			case 3:
			{
				r = pixels[0];
				g = pixels[1];
				b = pixels[2];
				
				pixels += 3;
			} break;
		}
		
		fprintf(file, "%i %i %i ", r, g, b);
	}
	
	fclose(file);
}
#endif // DEBUG
