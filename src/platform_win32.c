#include "internal.h"

//- Disable Warnings
#if defined(__clang__)
#   pragma clang diagnostic push
#   pragma clang diagnostic ignored "-Weverything"
#elif defined(__GNUC__)
#   pragma GCC diagnostic push
#   pragma GCC diagnostic ignored "-Wall"
#   pragma GCC diagnostic ignored "-Wextra"
#else
#   pragma warning(push, 0)
#endif

//- Includes
#define WIN32_LEAN_AND_MEAN
#define COBJMACROS

// General
#include <windows.h>
#include <dwmapi.h>
#include <versionhelpers.h>
#include <dbt.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

// Input
#include <xinput.h>
#include <dinput.h>

// Audio
#include <audioclient.h>
#include <audiopolicy.h>
#include <mmdeviceapi.h>

#include "platform_win32_guid.c"

//- Enable Warnings
#if defined(__clang__)
#   pragma clang diagnostic pop
#elif defined(__GNUC__)
#   pragma GCC diagnostic pop
#else
#   pragma warning(pop, 0)
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
internal GraphicsAPI global_graphics_api;

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
		MessageBoxA(NULL, buffer, "Error Code", MB_OK | MB_TOPMOST);
	}
}

internal void
Win32_ExitWithErrorMessage(const wchar_t* message)
{
	MessageBoxW(NULL, message, L"Error", MB_OK | MB_TOPMOST);
	exit(1);
}

internal wchar_t*
Win32_ConvertStringToWSTR(String str, wchar_t* buffer, uintsize size)
{
	if (!buffer)
	{
		size = 1 + (uintsize)MultiByteToWideChar(CP_UTF8, 0, str.data, (int32)str.len, NULL, 0);
		buffer = HeapAlloc(global_heap, 0, size * sizeof(wchar_t));
	}
	
	MultiByteToWideChar(CP_UTF8, 0, str.data, (int32)str.len, buffer, (int32)size);
	buffer[size-1] = 0;
	
	return buffer;
}

//~ Entry Point
#include "platform_win32_input.c"
#include "platform_win32_audio.c"
#include "platform_win32_opengl.c"

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
			global_window_should_close = true;
		} break;
		
		case WM_SYSKEYDOWN:
		case WM_SYSKEYUP:
		case WM_KEYUP:
		case WM_KEYDOWN:
		{
			uint32 vkcode = (uint32)wparam;
			bool32 was_down = ((lparam & (1 << 30)) != 0);
			bool32 is_down = ((lparam & (1 << 31)) == 0);
			
			if (was_down == is_down || vkcode >= Input_KeyboardKey_Count)
				break;
			
			Input_KeyboardKey key = global_keyboard_key_table[vkcode];
			if (key)
			{
				global_keyboard_keys[key] &= ~1;
				global_keyboard_keys[key] |= is_down;
			}
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

int WINAPI
WinMain(HINSTANCE instance, HINSTANCE previous, LPSTR args, int cmd_show)
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
	return Engine_Main(argc, argv);
}

//~ API
API void
Platform_ExitWithErrorMessage(String message)
{
	Trace("Platform_ExitWithErrorMessage");
	
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
	
	MessageBoxW(NULL, wmessage, wtitle, MB_OK | MB_TOPMOST);
	
	Platform_HeapFree(wtitle);
	Platform_HeapFree(wmessage);
}

API bool32
Platform_CreateWindow(int32 width, int32 height, String name, uint32 flags)
{
	Trace("Platform_CreateWindow");
	
	wchar_t window_name[1024];
	Win32_ConvertStringToWSTR(name, window_name, ArrayLength(window_name));
	
	bool32 ok = false;
	
	if (flags & GraphicsAPI_OpenGL)
		ok = ok || Win32_CreateOpenGLWindow(width, height, window_name);
	
	if (ok)
	{
		DEV_BROADCAST_DEVICEINTERFACE notification_filter = {
			sizeof notification_filter,
			DBT_DEVTYP_DEVICEINTERFACE
		};
		
		if (NULL == RegisterDeviceNotification(global_window, &notification_filter, DEVICE_NOTIFY_WINDOW_HANDLE))
		{
			// TODO
		}
		
		Win32_InitInput();
		Win32_InitAudio();
		Platform_PollEvents();
	}
	
	return ok;
}

API bool32
Platform_WindowShouldClose(void)
{
	return global_window_should_close;
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
	Win32_UpdateInputPre();
	
	MSG message;
	while (PeekMessageW(&message, 0, 0, 0, PM_REMOVE))
	{
		TranslateMessage(&message);
		DispatchMessageW(&message);
	}
	
	Win32_UpdateInputPos();
	Win32_UpdateAudio();
}

API void
Platform_FinishFrame(void)
{
	Win32_FillAudioBuffer();
	global_opengl.vtable.glSwapBuffers();
}

API void*
Platform_HeapAlloc(uintsize size)
{
	Trace("Platform_HeapAlloc");
	
	return HeapAlloc(global_heap, HEAP_ZERO_MEMORY, size);
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
	
	HeapFree(global_heap, 0, ptr);
}

API void*
Platform_VirtualAlloc(uintsize size)
{
	Trace("Platform_VirtualAlloc");
	return VirtualAlloc(NULL, size, MEM_RESERVE, PAGE_READWRITE);
}

API void
Platform_VirtualCommit(void* ptr, uintsize size)
{
	Trace("Platform_VirtualCommit");
	VirtualAlloc(ptr, size, MEM_COMMIT, PAGE_READWRITE);
}

API void
Platform_VirtualFree(void* ptr)
{
	Trace("Platform_VirtualFree");
	VirtualFree(ptr, 0, MEM_RELEASE);
}

API const OpenGL_VTable*
Platform_GetOpenGLVTable(void)
{
	return &global_opengl.vtable;
}

API uint64
Platform_CurrentPosixTime(void)
{
	time_t result = time(NULL);
	if (result == -1)
		result = 0;
	
	return (uint64)result;
}

//- Debug
#ifdef DEBUG
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
	va_list args;
	va_start(args, format);
	vprintf(format, args);
	va_end(args);
}
#endif // DEBUG
