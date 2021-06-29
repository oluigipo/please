#include "internal.h"

#if defined(__clang__)
#   pragma clang diagnostic push
//#   pragma clang diagnostic ignored "-Wsign-conversion"
//#   pragma clang diagnostic ignored "-Wimplicit-int-conversion"
//#   pragma clang diagnostic ignored "-Wimplicit-int-float-conversion"
#   pragma clang diagnostic ignored "-Weverything"
#elif defined(__GNUC__)
#   pragma GCC diagnostic push
//#   pragma GCC diagnostic ignored "-Wsign-conversion"
//#   pragma GCC diagnostic ignored "-Wconversion"
#   pragma GCC diagnostic ignored "-Wall"
#   pragma GCC diagnostic ignored "-Wextra"
#else
#   pragma warning(push, 0)
#endif

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <dwmapi.h>
#include <versionhelpers.h>

//#define INITGUID
#define COBJMACROS
#include <dbt.h>
#include <xinput.h>
#include <dinput.h>

#include "platform_win32_guid.c"

#include <stdio.h>
#include <math.h>
#include <string.h>

#define __uuidof(x) &(IID_ ## x)

// #include "ext/ps_rawinput.h"

#if defined(__clang__)
#   pragma clang diagnostic pop
#elif defined(__GNUC__)
#   pragma GCC diagnostic pop
#else
#   pragma warning(pop, 0)
#endif

//~ Types and Macros

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

//~ Functions
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
		size = ((uintsize)MultiByteToWideChar(CP_UTF8, 0, str.data, (int32)str.len, NULL, 0) + 1);
		size = size * sizeof(wchar_t);
		
		buffer = HeapAlloc(global_heap, 0, size);
	}
	
	MultiByteToWideChar(CP_UTF8, 0, str.data, (int32)str.len, buffer, (int32)size);
	buffer[size-1] = 0;
	
	return buffer;
}

#include "platform_win32_input.c"
#include "platform_win32_opengl.c"

internal LRESULT CALLBACK
Win32_WindowProc(HWND window, UINT message, WPARAM wparam, LPARAM lparam)
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
		.lpfnWndProc = Win32_WindowProc,
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
	wchar_t* msg = Win32_ConvertStringToWSTR(message, NULL, 0);
	Win32_ExitWithErrorMessage(msg);
	/* no return */
}

API bool32
Platform_CreateWindow(int32 width, int32 height, String name, uint32 flags)
{
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
	Win32_UpdateInput();
	
	MSG message;
	while (PeekMessageW(&message, 0, 0, 0, PM_REMOVE))
	{
		TranslateMessage(&message);
		DispatchMessageW(&message);
	}
}

API void*
Platform_HeapAlloc(uintsize size)
{
	return HeapAlloc(global_heap, HEAP_ZERO_MEMORY, size);
}

API void*
Platform_HeapRealloc(void* ptr, uintsize size)
{
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
	HeapFree(global_heap, 0, ptr);
}

API const OpenGL_VTable*
Platform_GetOpenGLVTable(void)
{
	return &global_opengl.vtable;
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
