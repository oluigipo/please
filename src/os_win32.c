#include <math.h>
#include <ext/cglm/cglm.h>

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
#include <shellscalingapi.h>

// Input
#include <xinput.h>
#include <dinput.h>

// Audio
#include <audioclient.h>
#include <audiopolicy.h>
#include <mmdeviceapi.h>
#include <Functiondiscoverykeys_devpkey.h>

#undef near
#undef far

#include "os_win32_guid.c"

extern __declspec(dllimport) LONG WINAPI RtlGetVersion(RTL_OSVERSIONINFOW*);

ReenableWarnings();

#if defined(_MSC_VER)
#   pragma comment(lib, "kernel32.lib")
#   pragma comment(lib, "user32.lib")
#   pragma comment(lib, "gdi32.lib")
//#   pragma comment(lib, "hid.lib")
#   pragma comment(lib, "ntdll.lib")
#   if defined(CONFIG_ENABLE_STEAM)
#       if defined(CONFIG_M64)
#           pragma comment(lib, "lib\\steam_api64.lib")
#       else
#           pragma comment(lib, "lib\\steam_api.lib")
#       endif
#   endif
#endif

//~ NOTE(ljre): Globals
static OS_WindowGraphicsContext g_graphics_context;
static OS_State g_os;

struct
{
	HINSTANCE instance;
	uint64 process_started_time;
	uint64 time_frequency;
	RECT monitor;
	HWND window;
	bool lock_cursor;
	HDC hdc;
	LPCWSTR class_name;
	HANDLE worker_threads[E_Limits_MaxWorkerThreadCount];
	
	int32 user_thread_count;
	int32 argc;
	const char* const* argv;
	
	struct
	{
		OS_WindowGraphicsApi desired_graphics_api;
		int32 desired_d3d11_ft;
	}
	init_config;
}
static g_win32;

#if 0
int __declspec(dllexport) NvOptimusEnablement = 1;
int __declspec(dllexport) AmdPowerXpressRequestHighPerformance = 1;
#endif

//~ NOTE(ljre): Utils
static uint64
Win32_GetTimer(void)
{
	LARGE_INTEGER value;
	QueryPerformanceCounter(&value);
	return value.QuadPart;
}

static String
Win32_WideToString(Arena* output_arena, LPWSTR wide)
{
	Trace();
	
	int32 size = WideCharToMultiByte(CP_UTF8, 0, wide, -1, NULL, 0, NULL, NULL);
	uint8* str = Arena_PushDirtyAligned(output_arena, size, 1);
	WideCharToMultiByte(CP_UTF8, 0, wide, -1, (char*)str, size, NULL, NULL);
	
	return StrMake(size, str);
}

static LPWSTR
Win32_StringToWide(Arena* output_arena, String str)
{
	Trace();
	Assert(str.size < INT32_MAX);
	
	int32 size = MultiByteToWideChar(CP_UTF8, 0, (const char*)str.data, (int32)str.size, NULL, 0);
	wchar_t* wide = Arena_PushDirtyAligned(output_arena, (size + 1) * sizeof(wchar_t), alignof(wchar_t));
	MultiByteToWideChar(CP_UTF8, 0, (const char*)str.data, (int32)str.size, wide, size);
	wide[size] = 0;
	
	return wide;
}

static HMODULE
Win32_LoadLibrary(const char* name)
{
	Trace();
	HMODULE result = LoadLibraryA(name);
	
	if (result)
		OS_DebugLog("Loaded Library: %s\n", name);
	
	return result;
}

static Arena*
Win32_GetThreadScratchArena(void)
{
	static thread_local Arena* this_arena;
	
	if (!this_arena)
		this_arena = Arena_Create(64 << 10, 64 << 10);
	
	return this_arena;
}

//~ NOTE(ljre): Files
#include "os_win32_input.c"
#include "os_win32_audio.c"

#ifdef CONFIG_ENABLE_OPENGL
#   include "os_win32_opengl.c"
#else
static bool Win32_CreateOpenGLWindow(const OS_WindowState* config, const wchar_t* title)
{ return false; }
static void Win32_OpenGLResizeWindow(void)
{}
#endif

#ifdef CONFIG_ENABLE_D3D11
#   include "os_win32_d3d11.c"
#else
static bool Win32_CreateD3d11Window(const OS_WindowState* config, const wchar_t* title, int32 desired_feature_level)
{ return false; }
static void Win32_D3d11ResizeWindow(void)
{}
#endif

//~ NOTE(ljre): Functions
static LRESULT CALLBACK
WindowProc(HWND window, UINT message, WPARAM wparam, LPARAM lparam)
{
	Trace();
	
	LRESULT result = 0;
	
	switch (message)
	{
		case WM_CLOSE:
		case WM_DESTROY:
		case WM_QUIT:
		{
			if (g_win32.window == window)
				g_os.window.should_close = true;
		} break;
		
		case WM_SIZE:
		{
			g_os.window.width = LOWORD(lparam);
			g_os.window.height = HIWORD(lparam);
			
			g_os.window.resized_by_user = true;
			
			switch (g_graphics_context.api)
			{
				case OS_WindowGraphicsApi_Null: break;
				
				case OS_WindowGraphicsApi_OpenGL: Win32_OpenGLResizeWindow(); break;
				case OS_WindowGraphicsApi_Direct3D11: Win32_D3d11ResizeWindow(); break;
			}
		} break;
		
		case WM_CHAR:
		{
			if ((wparam < 32 && wparam != '\r' && wparam != '\t' && wparam != '\b') ||
			(wparam > 0x7f && wparam <= 0xa0))
				break;
			
			uint32 codepoint = (uint32)wparam;
			int32 repeat_count = (lparam & 0xffff);
			(void)repeat_count;
			
			if (g_os.text_codepoints_count < ArrayLength(g_os.text_codepoints))
				g_os.text_codepoints[g_os.text_codepoints_count++] = codepoint;
		} break;
		
		case WM_SYSKEYDOWN:
		case WM_SYSKEYUP:
		case WM_KEYUP:
		case WM_KEYDOWN:
		{
			uint32 vkcode = (uint32)wparam;
			bool was_down = ((lparam & (1u << 30)) != 0);
			bool is_down = ((lparam & (1u << 31)) == 0);
			
			(void)was_down;
			
			if (vkcode >= ArrayLength(global_keyboard_key_table))
				break;
			
			// NOTE(ljre): Checks for which side specifically.
			// TODO(ljre): This might fuck up a bit the buffering...
			if (vkcode == VK_SHIFT)
			{
				Win32_UpdateKeyboardKey(&g_os, VK_LSHIFT, !!(GetKeyState(VK_LSHIFT) & 0x8000));
				Win32_UpdateKeyboardKey(&g_os, VK_RSHIFT, !!(GetKeyState(VK_RSHIFT) & 0x8000));
			}
			else if (vkcode == VK_CONTROL)
			{
				Win32_UpdateKeyboardKey(&g_os, VK_LCONTROL, !!(GetKeyState(VK_LCONTROL) & 0x8000));
				Win32_UpdateKeyboardKey(&g_os, VK_RCONTROL, !!(GetKeyState(VK_RCONTROL) & 0x8000));
			}
			else
				Win32_UpdateKeyboardKey(&g_os, vkcode, is_down);
			
			// NOTE(ljre): Always close on Alt+F4
			if (vkcode == VK_F4 && GetKeyState(VK_MENU) & 0x8000)
				g_os.window.should_close = true;
		} break;
		
		case WM_LBUTTONUP: Win32_UpdateMouseButton(&g_os, OS_MouseButton_Left, false); break;
		case WM_LBUTTONDOWN: Win32_UpdateMouseButton(&g_os, OS_MouseButton_Left, true); break;
		case WM_MBUTTONUP: Win32_UpdateMouseButton(&g_os, OS_MouseButton_Middle, false); break;
		case WM_MBUTTONDOWN: Win32_UpdateMouseButton(&g_os, OS_MouseButton_Middle, true); break;
		case WM_RBUTTONUP: Win32_UpdateMouseButton(&g_os, OS_MouseButton_Right, false); break;
		case WM_RBUTTONDOWN: Win32_UpdateMouseButton(&g_os, OS_MouseButton_Right, true); break;
		
		case WM_MOUSEWHEEL:
		{
			int32 delta = GET_WHEEL_DELTA_WPARAM(wparam) / 120;
			g_os.mouse.scroll += delta;
		} break;
		
		case WM_DEVICECHANGE:
		{
			Win32_CheckForGamepads();
			Win32_UpdateAudioEndpointIfNeeded();
		} break;
		
		default:
		{
			result = DefWindowProcW(window, message, wparam, lparam);
		} break;
	}
	
	return result;
}

static DWORD WINAPI
Win32_ThreadProc_(void* user_data)
{
	OS_UserMainArgs user_args = {
		.thread_id = (int32)(uintptr)user_data,
		.thread_count = g_win32.user_thread_count,
		
		.argc = g_win32.argc,
		.argv = g_win32.argv,
	};
	
	return OS_UserMain(&user_args);
}

//~ NOTE(ljre): Entry point
int WINAPI
WinMain(HINSTANCE instance, HINSTANCE previous, LPSTR cmd_args, int cmd_show)
{
	//TraceInit();
	
	//- Init
	{
		HMODULE library = LoadLibraryA("shcore.dll");
		bool ok = false;
		
		if (library)
		{
			HRESULT (WINAPI* set_process_dpi_aware)(PROCESS_DPI_AWARENESS value);
			
			set_process_dpi_aware = (void*)GetProcAddress(library, "SetProcessDpiAwareness");
			if (set_process_dpi_aware)
				ok = (S_OK == set_process_dpi_aware(PROCESS_PER_MONITOR_DPI_AWARE));
		}
		
		if (!ok)
		{
			library = LoadLibraryA("user32.dll");
			
			if (library)
			{
				BOOL (WINAPI* set_process_dpi_aware)(void);
				
				set_process_dpi_aware = (void*)GetProcAddress(library, "SetProcessDPIAware");
				if (set_process_dpi_aware)
					ok = set_process_dpi_aware();
			}
		}
	}
	
	{
		LARGE_INTEGER value;
		QueryPerformanceFrequency(&value);
		g_win32.time_frequency = value.QuadPart;
	}
	
	{
		HMONITOR monitor = MonitorFromPoint((POINT) { 0, 0 }, MONITOR_DEFAULTTOPRIMARY);
		MONITORINFO info = {
			.cbSize = sizeof(info),
		};
		
		SafeAssert(GetMonitorInfoA(monitor, &info));
		g_win32.monitor = info.rcWork;
	}
	
#ifdef CONFIG_DEBUG
	{
		LPWSTR path;
		
#ifdef _WIN64
		path = L"./redist/x86_64-windows/";
#else
		path = L"./redist/i386-windows/";
#endif
		
		SetDllDirectoryW(path);
	}
#endif
	
	int32 cpu_core_count = 1;
	{
		SYSTEM_INFO system_info = { 0 };
		GetNativeSystemInfo(&system_info);
		
		cpu_core_count = system_info.dwNumberOfProcessors;
	}
	
	g_win32.process_started_time = Win32_GetTimer();
	g_win32.instance = instance;
	g_win32.user_thread_count = Clamp(cpu_core_count/2, 1, E_Limits_MaxWorkerThreadCount);
	g_win32.argc = __argc;
	g_win32.argv = (const char* const*)__argv;
	
	for (int32 i = 1; i < g_win32.argc; ++i)
	{
		String arg = StrMake(Mem_Strlen(g_win32.argv[i]), g_win32.argv[i]);
		
		if (String_Equals(arg, Str("-opengl")))
			g_win32.init_config.desired_graphics_api = OS_WindowGraphicsApi_OpenGL;
		else if (String_Equals(arg, Str("-d3d11")))
			g_win32.init_config.desired_graphics_api = OS_WindowGraphicsApi_Direct3D11;
		else if (String_Equals(arg, Str("-no-worker-threads")))
			g_win32.user_thread_count = 1;
		else if (String_Equals(arg, Str("-no-vsync")))
		{}
		else if (String_StartsWith(arg, Str("-d3d11=")))
		{
			int32 feature_level = 0;
			arg = String_Substr(arg, sizeof("-d3d11=")-1, -1);
			
			if (String_Equals(arg, Str("11_0")))
				feature_level = 110;
			else if (String_Equals(arg, Str("10_1")))
				feature_level = 101;
			else if (String_Equals(arg, Str("10_0")))
				feature_level = 100;
			else if (String_Equals(arg, Str("9_3")))
				feature_level = 93;
			else if (String_Equals(arg, Str("9_2")))
				feature_level = 92;
			else if (String_Equals(arg, Str("9_1")))
				feature_level = 91;
			
			g_win32.init_config.desired_graphics_api = OS_WindowGraphicsApi_Direct3D11;
			g_win32.init_config.desired_d3d11_ft = feature_level;
		}
	}
	
	//- Run
	int32 result = OS_UserMain(&(OS_UserMainArgs) {
		.thread_id = 0,
		.thread_count = g_win32.user_thread_count,
		
		.argc = g_win32.argc,
		.argv = g_win32.argv,
	});
	
	//- NOTE(ljre): Free resources... or nah :P
	//Win32_DeinitSimpleAudio();
	Win32_DeinitAudio(&g_os);
	
	for (int32 i = 0; i < ArrayLength(g_win32.worker_threads); ++i)
	{
		if (g_win32.worker_threads[i])
		{
			TerminateThread(g_win32.worker_threads[i], 0);
			g_win32.worker_threads[i] = NULL;
		}
	}
	
	switch (g_graphics_context.api)
	{
#ifdef CONFIG_ENABLE_OPENGL
		case OS_WindowGraphicsApi_OpenGL: Win32_DestroyOpenGLWindow(); break;
#endif
#ifdef CONFIG_ENABLE_D3D11
		case OS_WindowGraphicsApi_Direct3D11: Win32_DestroyD3d11Window(); break;
#endif
		default: break;
	}
	
	//TraceDeinit();
	//ExitProcess(result);
	
	return result;
}

//~ NOTE(ljre): API
API bool
OS_Init(const OS_InitDesc* desc, OS_State** out_state)
{
	Trace();
	
	// NOTE(ljre): Window class
	{
		g_win32.class_name = L"WindowClassName";
		WNDCLASSW window_class = {
			.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW,
			.lpfnWndProc = WindowProc,
			.lpszClassName = g_win32.class_name,
			.hInstance = g_win32.instance,
			.hCursor = LoadCursorA(NULL, IDC_ARROW),
			.hIcon = LoadIconA(g_win32.instance, MAKEINTRESOURCE(101)),
		};
		
		if (!RegisterClassW(&window_class))
			OS_ExitWithErrorMessage("Could not create window class.");
	}
	
	OS_WindowState window_state = {
		.show_cursor = true,
		.x = (g_win32.monitor.right - g_win32.monitor.left - 1280) / 2,
		.y = (g_win32.monitor.bottom - g_win32.monitor.top - 720) / 2,
		.width = 1280,
		.height = 720,
	};
	
	bool ok = true;
	
	{
		bool created_window = false;
		wchar_t* window_name = L"Title";
		
		switch (g_win32.init_config.desired_graphics_api)
		{
			default: break;
			
			case OS_WindowGraphicsApi_OpenGL: created_window = Win32_CreateOpenGLWindow(&window_state, window_name); break;
			case OS_WindowGraphicsApi_Direct3D11: created_window = Win32_CreateD3d11Window(&window_state, window_name, g_win32.init_config.desired_d3d11_ft); break;
		}
		
		if (!created_window)
			created_window = Win32_CreateD3d11Window(&window_state, window_name, 0);
		if (!created_window)
			created_window = Win32_CreateOpenGLWindow(&window_state, window_name);
		
		ok = ok && created_window;
	}
	
	if (g_win32.user_thread_count > 0)
	{
		SafeAssert(g_win32.user_thread_count < 16);
		
		for (int32 i = 1; i < g_win32.user_thread_count; ++i)
		{
			HANDLE handle = CreateThread(NULL, 0, Win32_ThreadProc_, (void*)(uintptr)i, 0, NULL);
			
			g_win32.worker_threads[i] = handle;
		}
	}
	
	if (ok)
	{
		DEV_BROADCAST_DEVICEINTERFACE notification_filter = {
			sizeof(notification_filter),
			DBT_DEVTYP_DEVICEINTERFACE
		};
		
		OS_State* os_state = &g_os;
		bool audio_initialized_successfully = false;
		
		if (!RegisterDeviceNotification(g_win32.window, &notification_filter, DEVICE_NOTIFY_WINDOW_HANDLE))
			OS_DebugLog("Failed to register input device notification.");
		if (desc->audiothread_proc && !(audio_initialized_successfully = Win32_InitAudio(desc, os_state)))
			OS_DebugLog("Failed to initialize audio.");
		
		ok = ok && Win32_InitInput();
		
		if (ok)
		{
			os_state->has_audio = audio_initialized_successfully;
			os_state->has_keyboard = true;
			os_state->has_mouse = true;
			os_state->has_gestures = false;
			
			os_state->window = window_state;
			os_state->graphics_context = &g_graphics_context;
			*out_state = os_state;
			
			ShowWindow(g_win32.window, SW_SHOWDEFAULT);
		}
	}
	
	return ok;
}

API void
OS_PollEvents(void)
{
	Trace();
	
	Win32_UpdateInputEarly(&g_os);
	
	MSG message;
	while (PeekMessageW(&message, 0, 0, 0, PM_REMOVE))
	{
		TranslateMessage(&message);
		DispatchMessageW(&message);
	}
	
	Win32_UpdateInputLate(&g_os);
}

API void
OS_ExitWithErrorMessage(const char* fmt, ...)
{
	Arena* scratch_arena = Win32_GetThreadScratchArena();
	
	va_list args;
	va_start(args, fmt);
	String str = Arena_VPrintf(scratch_arena, fmt, args);
	va_end(args);
	
	wchar_t* wstr = Win32_StringToWide(scratch_arena, str);
	MessageBoxW(NULL, wstr, L"Fatal Error!", MB_OK);
	exit(1);
	/* no return */
}

API void
OS_MessageBox(String title, String message)
{
	Trace();
	Arena* scratch_arena = Win32_GetThreadScratchArena();
	
	for Arena_TempScope(scratch_arena)
	{
		wchar_t* wtitle = Win32_StringToWide(scratch_arena, title);
		wchar_t* wmessage = Win32_StringToWide(scratch_arena, message);
		
		MessageBoxW(NULL, wmessage, wtitle, MB_OK);
	}
}

API bool
OS_WaitForVsync(void)
{
	Trace();
	
	static bool already_tried = false;
	static HRESULT (WINAPI* dwm_flush)(void);
	
	if (!already_tried && IsWindowsVistaOrGreater())
	{
		already_tried = true;
		
		HMODULE library = Win32_LoadLibrary("dwmapi.dll");
		if (library)
			dwm_flush = (void*)GetProcAddress(library, "DwmFlush");
	}
	
	if (dwm_flush)
		return dwm_flush() == S_OK;
	
	return false;
}

API void*
OS_HeapAlloc(uintsize size)
{
	Trace();
	
	return HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, size);
}

API void*
OS_HeapRealloc(void* ptr, uintsize size)
{
	Trace();
	
	void* result;
	
	if (ptr)
		result = HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, ptr, size);
	else
		result = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, size);
	
	return result;
}

API void
OS_HeapFree(void* ptr)
{
	Trace();
	
	if (ptr)
		HeapFree(GetProcessHeap(), 0, ptr);
}

API void*
OS_VirtualReserve(void* address, uintsize size)
{
	Trace();
	
	return VirtualAlloc(address, size, MEM_RESERVE, PAGE_READWRITE);
}

API bool
OS_VirtualCommit(void* ptr, uintsize size)
{
	Trace();
	
	return VirtualAlloc(ptr, size, MEM_COMMIT, PAGE_READWRITE);
}

API void
OS_VirtualDecommit(void* ptr, uintsize size)
{
	Trace();
	
	BOOL ok = VirtualFree(ptr, size, MEM_DECOMMIT);
	SafeAssert(ok);
}

API void
OS_VirtualRelease(void* ptr, uintsize size)
{
	Trace();
	(void)size;
	
	BOOL ok = VirtualFree(ptr, 0, MEM_RELEASE);
	SafeAssert(ok);
}

API uint64
OS_CurrentPosixTime(void)
{
	SYSTEMTIME system_time;
	FILETIME file_time;
	GetSystemTime(&system_time);
	SystemTimeToFileTime(&system_time, &file_time);
	
	FILETIME posix_time;
	SYSTEMTIME posix_system_time = {
		.wYear = 1970,
		.wMonth = 1,
		.wDayOfWeek = 4,
		.wDay = 1,
		.wHour = 0,
		.wMinute = 0,
		.wSecond = 0,
		.wMilliseconds = 0,
	};
	SystemTimeToFileTime(&posix_system_time, &posix_time);
	
	uint64 result = 0;
	result +=  file_time.dwLowDateTime | (uint64) file_time.dwHighDateTime << 32;
	result -= posix_time.dwLowDateTime | (uint64)posix_time.dwHighDateTime << 32;
	
	return result;
}

API uint64
OS_CurrentTick(uint64* out_ticks_per_second)
{
	*out_ticks_per_second = g_win32.time_frequency;
	return Win32_GetTimer() - g_win32.process_started_time;
}

API float64
OS_GetTimeInSeconds(void)
{
	int64 time = Win32_GetTimer();
	return (float64)(time - g_win32.process_started_time) / (float64)g_win32.time_frequency;
}

API bool
OS_ReadEntireFile(String path, Arena* output_arena, void** out_data, uintsize* out_size)
{
	Trace(); TraceText(path);
	
	Arena* scratch_arena = Win32_GetThreadScratchArena();
	HANDLE handle;
	
	for Arena_TempScope(scratch_arena)
	{
		LPWSTR wpath = Win32_StringToWide(scratch_arena, path);
		handle = CreateFileW(wpath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_READONLY, NULL);
	}
	
	if (handle == INVALID_HANDLE_VALUE)
		return false;
	
	LARGE_INTEGER large_int;
	if (!GetFileSizeEx(handle, &large_int))
	{
		CloseHandle(handle);
		return false;
	}
	
#ifndef _WIN64
	if (large_int.QuadPart > UINT32_MAX)
	{
		CloseHandle(handle);
		return false;
	}
#endif
	
	Arena_Savepoint output_arena_save = Arena_Save(output_arena);
	uintsize file_size = (uintsize)large_int.QuadPart;
	uint8* file_data = Arena_PushDirtyAligned(output_arena, file_size, 1);
	
	uintsize still_to_read = file_size;
	uint8* p = file_data;
	while (still_to_read > 0)
	{
		DWORD to_read = (DWORD)Min(still_to_read, UINT32_MAX);
		DWORD did_read;
		
		if (!ReadFile(handle, p, to_read, &did_read, NULL))
		{
			Arena_Restore(output_arena_save);
			CloseHandle(handle);
			return false;
		}
		
		still_to_read -= to_read;
		p += to_read;
	}
	
	*out_data = file_data;
	*out_size = file_size;
	
	CloseHandle(handle);
	return true;
}

API bool
OS_WriteEntireFile(String path, const void* data, uintsize size)
{
	Trace(); TraceText(path);
	
	Arena* scratch_arena = Win32_GetThreadScratchArena();
	HANDLE handle;
	
	for Arena_TempScope(scratch_arena)
	{
		LPWSTR wpath = Win32_StringToWide(scratch_arena, path);
		handle = CreateFileW(wpath, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
	}
	
	if (handle == INVALID_HANDLE_VALUE)
		return false;
	
	uintsize total_size = size;
	const uint8* head = data;
	
	while (size > 0)
	{
		DWORD bytes_written = 0;
		DWORD to_write = (uint32)Min(total_size, UINT32_MAX);
		
		if (!WriteFile(handle, head, to_write, &bytes_written, NULL))
		{
			CloseHandle(handle);
			return false;
		}
		
		total_size -= bytes_written;
		head += bytes_written;
	}
	
	CloseHandle(handle);
	return true;
}

API void*
OS_LoadDiscordLibrary(void)
{
	Trace();
	
	static HANDLE library;
	
	if (!library)
	{
		library = Win32_LoadLibrary("discord_game_sdk.dll");
		
		if (!library)
			return NULL;
	}
	
	return (void*)GetProcAddress(library, "DiscordCreate");
}

#ifdef CONFIG_ENABLE_HOT
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
OS_LoadGameLibrary(void)
{
	Trace();
	
	static HMODULE library = NULL;
	static uint64 saved_last_update;
	static void* saved_result;
	
	const char* const dll_name = "./build/x86_64-windows-gamehot.dll";
	void* result = saved_result;
	
	if (!library)
	{
		const char* target_dll_path = "./build/hot-game_.dll";
		if (!CopyFileA(dll_name, target_dll_path, false))
		{
			// NOTE(ljre): fallback to the original file
			target_dll_path = dll_name;
			OS_DebugLog("%x\n", (uint32)GetLastError());
		}
		else
			saved_last_update = GetFileLastWriteTime(dll_name);
		
		library = Win32_LoadLibrary(target_dll_path);
		SafeAssert(library);
		
		result = GetProcAddress(library, "G_Main");
		SafeAssert(result);
	}
	else
	{
		uint64 last_update = GetFileLastWriteTime(dll_name);
		
		if (last_update > saved_last_update)
		{
			FreeLibrary(library);
			
			const char* target_dll_path = "./build/hot-game_.dll";
			if (!CopyFileA(dll_name, target_dll_path, false))
				// NOTE(ljre): fallback to the original file
				target_dll_path = dll_name;
			else
				saved_last_update = last_update;
			
			library = Win32_LoadLibrary(target_dll_path);
			SafeAssert(library);
			
			result = GetProcAddress(library, "G_Main");
			SafeAssert(result);
			
			SetForegroundWindow(g_win32.window);
		}
	}
	
	Assert(result);
	
	saved_result = result;
	return result;
}
#endif //CONFIG_ENABLE_HOT

#if 1
static_assert(sizeof(OS_RWLock) == sizeof(SRWLOCK), "we want to just reinterpret_cast OS_RWLock to win32's SRWLOCK");

API void
OS_InitRWLock(OS_RWLock* lock)
{ Mem_Zero(lock, sizeof(*lock)); }

API void
OS_LockShared(OS_RWLock* lock)
{ AcquireSRWLockShared((SRWLOCK*)lock); }

API void
OS_LockExclusive(OS_RWLock* lock)
{ AcquireSRWLockExclusive((SRWLOCK*)lock); }

API bool
OS_TryLockShared(OS_RWLock* lock)
{ return TryAcquireSRWLockShared((SRWLOCK*)lock); }

API bool
OS_TryLockExclusive(OS_RWLock* lock)
{ return TryAcquireSRWLockExclusive((SRWLOCK*)lock); }

API void
OS_UnlockShared(OS_RWLock* lock)
{ ReleaseSRWLockShared((SRWLOCK*)lock); }

API void
OS_UnlockExclusive(OS_RWLock* lock)
{ ReleaseSRWLockExclusive((SRWLOCK*)lock); }

API void
OS_DeinitRWLock(OS_RWLock* lock)
{}
#else
API void
OS_InitRWLock(OS_RWLock* lock)
{
	CRITICAL_SECTION* csec = OS_HeapAlloc(sizeof(*csec));
	InitializeCriticalSection(csec);
	lock->ptr_ = csec;
}

API void
OS_LockShared(OS_RWLock* lock)
{ EnterCriticalSection(lock->ptr_); }

API void
OS_LockExclusive(OS_RWLock* lock)
{ EnterCriticalSection(lock->ptr_); }

API bool
OS_TryLockShared(OS_RWLock* lock)
{ return TryEnterCriticalSection(lock->ptr_); }

API bool
OS_TryLockExclusive(OS_RWLock* lock)
{ return TryEnterCriticalSection(lock->ptr_); }

API void
OS_UnlockShared(OS_RWLock* lock)
{ LeaveCriticalSection(lock->ptr_); }

API void
OS_UnlockExclusive(OS_RWLock* lock)
{ LeaveCriticalSection(lock->ptr_); }

API void
OS_DeinitRWLock(OS_RWLock* lock)
{
	DeleteCriticalSection(lock->ptr_);
	OS_HeapFree(lock->ptr_);
	lock->ptr_ = NULL;
}
#endif

API void
OS_InitSemaphore(OS_Semaphore* sem, int32 max_count)
{
	Trace();
	
	HANDLE handle = CreateSemaphoreA(NULL, max_count, max_count, NULL);
	sem->ptr = handle;
}

API bool
OS_WaitForSemaphore(OS_Semaphore* sem)
{
	//Trace();
	Assert(sem);
	
	if (sem->ptr)
	{
		DWORD result = WaitForSingleObjectEx(sem->ptr, INFINITE, false);
		return result == WAIT_OBJECT_0;
	}
	
	return false;
}

API void
OS_SignalSemaphore(OS_Semaphore* sem, int32 count)
{
	Trace();
	Assert(sem);
	
	if (sem->ptr)
		ReleaseSemaphore(sem->ptr, count, NULL);
}

API void
OS_DeinitSemaphore(OS_Semaphore* sem)
{
	Trace();
	Assert(sem);
	
	if (sem->ptr)
	{
		CloseHandle(sem->ptr);
		sem->ptr = NULL;
	}
}

API void
OS_InitEventSignal(OS_EventSignal* sig)
{
	Trace();
	
	HANDLE handle = CreateEvent(NULL, false, false, NULL);
	SafeAssert(handle);
	
	sig->ptr = handle;
}

API bool
OS_WaitEventSignal(OS_EventSignal* sig)
{
	Trace();
	SafeAssert(sig && sig->ptr);
	
	DWORD result = WaitForSingleObjectEx(sig->ptr, INFINITE, false);
	return result == WAIT_OBJECT_0;
}

API void
OS_SetEventSignal(OS_EventSignal* sig)
{
	Trace();
	SafeAssert(sig && sig->ptr);
	
	SetEvent(sig->ptr);
}

API void
OS_ResetEventSignal(OS_EventSignal* sig)
{
	Trace();
	SafeAssert(sig && sig->ptr);
	
	ResetEvent(sig->ptr);
}

API void
OS_DeinitEventSignal(OS_EventSignal* sig)
{
	Trace();
	SafeAssert(sig && sig->ptr);
	
	CloseHandle(sig->ptr);
	sig->ptr = NULL;
}

API int32
OS_InterlockedCompareExchange32(volatile int32* ptr, int32 new_value, int32 expected)
{ return (int32)InterlockedCompareExchange((volatile LONG*)ptr, (LONG)new_value, (LONG)expected); }

API int64
OS_InterlockedCompareExchange64(volatile int64* ptr, int64 new_value, int64 expected)
{ return (int64)InterlockedCompareExchange64((volatile LONG64*)ptr, (LONG64)new_value, (LONG64)expected); }

API void*
OS_InterlockedCompareExchangePtr(void* volatile* ptr, void* new_value, void* expected)
{ return (void*)InterlockedCompareExchangePointer((volatile PVOID*)ptr, (PVOID)new_value, (PVOID)expected); }

API int32
OS_InterlockedIncrement32(volatile int32* ptr)
{ return (int32)InterlockedIncrement((volatile LONG*)ptr); }

API int32
OS_InterlockedDecrement32(volatile int32* ptr)
{ return (int32)InterlockedDecrement((volatile LONG*)ptr); }

#ifdef CONFIG_DEBUG
API void
OS_DebugMessageBox(const char* fmt, ...)
{
	Arena* scratch_arena = Win32_GetThreadScratchArena();
	
	for Arena_TempScope(scratch_arena)
	{
		va_list args;
		va_start(args, fmt);
		String str = Arena_VPrintf(scratch_arena, fmt, args);
		va_end(args);
		
		wchar_t* wstr = Win32_StringToWide(scratch_arena, str);
		MessageBoxW(NULL, wstr, L"OS_DebugMessageBox", MB_OK);
	}
}

API void
OS_DebugLog(const char* fmt, ...)
{
	Arena* scratch_arena = Win32_GetThreadScratchArena();
	
	for Arena_TempScope(scratch_arena)
	{
		va_list args;
		va_start(args, fmt);
		String str = Arena_VPrintf(scratch_arena, fmt, args);
		Arena_PushData(scratch_arena, &""); // null terminator
		va_end(args);
		
		if (IsDebuggerPresent())
			OutputDebugStringA((const char*)str.data);
		else
			fwrite(str.data, 1, str.size, stderr);
	}
}

API int
OS_DebugLogPrintfFormat(const char* fmt, ...)
{
	Arena* scratch_arena = Win32_GetThreadScratchArena();
	int32 ret = 0;
	
	for Arena_TempScope(scratch_arena)
	{
		va_list args, args2;
		va_start(args, fmt);
		va_copy(args2, args);
		
		int32 len = vsnprintf(NULL, 0, fmt, args);
		char* buffer = Arena_PushDirtyAligned(scratch_arena, len+1, 1);
		vsnprintf(buffer, len+1, fmt, args2);
		
		va_end(args);
		va_end(args2);
		
		if (IsDebuggerPresent())
			OutputDebugStringA(buffer);
		else
			fwrite(buffer, 1, len, stderr);
		
		ret = len;
	}
	
	return ret;
}

#endif //CONFIG_DEBUG
