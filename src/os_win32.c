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

#undef near
#undef far

#include "os_win32_guid.c"

extern __declspec(dllimport) LONG WINAPI RtlGetVersion(RTL_OSVERSIONINFOW*);

ReenableWarnings();

#if defined(_MSC_VER)
#   pragma comment(lib, "kernel32.lib")
#   pragma comment(lib, "user32.lib")
#   pragma comment(lib, "gdi32.lib")
#   pragma comment(lib, "hid.lib")
#   pragma comment(lib, "ntdll.lib")
#endif

//~ NOTE(ljre): Globals
static Arena* global_scratch_arena;
static OS_WindowState global_window_state;
static OS_WindowGraphicsContext global_graphics_context;
static HINSTANCE global_instance;
static uint64 global_process_started_time;
static uint64 global_time_frequency;
static OS_InputState global_input_state;
static RECT global_monitor;
static HWND global_window;
static bool global_lock_cursor;
static HDC global_hdc;
static LPWSTR global_class_name = L"WindowClassName";

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
	HMODULE result = LoadLibraryA(name);
	
	if (result)
		OS_DebugLog("Loaded Library: %s\n", name);
	
	return result;
}

static void
Win32_UpdateWindowStateIfNeeded(OS_WindowState* inout_state)
{
	Trace();
	
	//- NOTE(ljre): Simple-idk-how-to-call-it data.
	{
		inout_state->should_close = global_window_state.should_close;
		inout_state->resized_by_user = global_window_state.resized_by_user;
		
		if (global_window_state.resized_by_user)
		{
			inout_state->width = global_window_state.width;
			inout_state->height = global_window_state.height;
		}
	}
	
	//- NOTE(ljre): Window Position & Size
	{
		UINT flags = SWP_NOSIZE | SWP_NOMOVE;
		
		int32 x = inout_state->x;
		int32 y = inout_state->y;
		int32 width = inout_state->width;
		int32 height = inout_state->height;
		
		if (width != global_window_state.width || height != global_window_state.height)
			flags &= ~(UINT)SWP_NOSIZE;
		
		if (inout_state->center_window)
		{
			inout_state->x = x = (global_monitor.right - global_monitor.left) / 2 - width / 2;
			inout_state->y = y = (global_monitor.bottom - global_monitor.top) / 2 - height / 2;
			inout_state->center_window = false;
			
			flags &= ~(UINT)SWP_NOMOVE;
		}
		else if (x != global_window_state.x || y != global_window_state.y)
			flags &= ~(UINT)SWP_NOMOVE;
		
		if (flags)
			SetWindowPos(global_window, NULL, x, y, width, height, flags | SWP_NOZORDER);
	}
	
	//- NOTE(ljre): Window Title
	{
		uintsize new_size = 0;
		for (int32 i = 0; i < ArrayLength(inout_state->title); ++i)
		{
			if (!inout_state->title[i])
			{
				new_size = i;
				break;
			}
		}
		
		uintsize old_size = 0;
		for (int32 i = 0; i < ArrayLength(global_window_state.title); ++i)
		{
			if (!global_window_state.title[i])
			{
				old_size = i;
				break;
			}
		}
		
		String new_title = StrMake(new_size, inout_state->title);
		String old_title = StrMake(old_size, global_window_state.title);
		
		if (!String_Equals(old_title, new_title))
		{
			for Arena_TempScope(global_scratch_arena)
			{
				wchar_t* name = Win32_StringToWide(global_scratch_arena, new_title);
				SetWindowTextW(global_window, name);
			}
		}
	}
	
	//- NOTE(ljre): Cursor
	{
		if (inout_state->show_cursor != global_window_state.show_cursor)
			ShowCursor(inout_state->show_cursor);
		
		global_lock_cursor = inout_state->lock_cursor;
	}
	
	//- NOTE(ljre): Sync
	global_window_state = *inout_state;
	global_window_state.resized_by_user = false;
}

//~ NOTE(ljre): Files
#include "os_win32_input.c"
#include "os_win32_simpleaudio.c"

#ifdef CONFIG_ENABLE_OPENGL
#   include "os_win32_opengl.c"
#else
static bool Win32_CreateOpenGLWindow(const OS_WindowState* config, const wchar_t* title)
{ return false; }
#endif

#ifdef CONFIG_ENABLE_D3D11
#   include "os_win32_d3d11.c"
#else
static bool Win32_CreateD3d11Window(const OS_WindowState* config, const wchar_t* title)
{ return false; }
#endif

//~ NOTE(ljre): Functions
static LRESULT CALLBACK
WindowProc(HWND window, UINT message, WPARAM wparam, LPARAM lparam)
{
	Trace();
	
	LRESULT result = 0;
	OS_InputState* input = (void*)GetWindowLongPtrW(window, GWLP_USERDATA);
	
	switch (message)
	{
		case WM_CLOSE:
		case WM_DESTROY:
		case WM_QUIT:
		{
			if (global_window == window)
				global_window_state.should_close = true;
		} break;
		
		case WM_SIZE:
		{
			global_window_state.width = LOWORD(lparam);
			global_window_state.height = HIWORD(lparam);
			
			global_window_state.resized_by_user = true;
			
#ifdef CONFIG_ENABLE_D3D11
			if (global_graphics_context.api == OS_WindowGraphicsApi_Direct3D11)
			{
				ID3D11DeviceContext_ClearState(global_direct3d.context);
				ID3D11RenderTargetView_Release(global_direct3d.target);
				global_direct3d.target = NULL;
				
				HRESULT hr;
				
				hr = IDXGISwapChain_ResizeBuffers(global_direct3d.swapchain, 0, 0, 0, DXGI_FORMAT_UNKNOWN, 0);
				SafeAssert(SUCCEEDED(hr));
				
				ID3D11Resource* backbuffer;
				hr = IDXGISwapChain_GetBuffer(global_direct3d.swapchain, 0, &IID_ID3D11Resource, (void**)&backbuffer);
				SafeAssert(SUCCEEDED(hr));
				
				hr = ID3D11Device_CreateRenderTargetView(global_direct3d.device, backbuffer, NULL, &global_direct3d.target);
				SafeAssert(SUCCEEDED(hr));
				
				ID3D11Resource_Release(backbuffer);
			}
#endif
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
				global_window_state.should_close = true;
		} break;
		
		case WM_LBUTTONUP: Win32_UpdateMouseButton(input, OS_MouseButton_Left, false); break;
		case WM_LBUTTONDOWN: Win32_UpdateMouseButton(input, OS_MouseButton_Left, true); break;
		case WM_MBUTTONUP: Win32_UpdateMouseButton(input, OS_MouseButton_Middle, false); break;
		case WM_MBUTTONDOWN: Win32_UpdateMouseButton(input, OS_MouseButton_Middle, true); break;
		case WM_RBUTTONUP: Win32_UpdateMouseButton(input, OS_MouseButton_Right, false); break;
		case WM_RBUTTONDOWN: Win32_UpdateMouseButton(input, OS_MouseButton_Right, true); break;
		
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

//~ NOTE(ljre): Entry point
#ifdef CONFIG_ENABLE_HOT
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
			SetProcessDPIAware();
	}
	
	{
		LARGE_INTEGER value;
		QueryPerformanceFrequency(&value);
		global_time_frequency = value.QuadPart;
	}
	
	{
		HMONITOR monitor = MonitorFromWindow(global_window, MONITOR_DEFAULTTONEAREST);
		MONITORINFO info = {
			.cbSize = sizeof(info),
		};
		
		SafeAssert(GetMonitorInfoA(monitor, &info));
		global_monitor = info.rcWork;
	}
	
	global_process_started_time = Win32_GetTimer();
	global_instance = instance;
	global_scratch_arena = Arena_Create(32 << 10, 32 << 10);
	
	//- Run
	int32 result = OS_UserMain(argc, argv);
	
	// NOTE(ljre): Free resources... or nah :P
	//Win32_DeinitAudio();
	
	return result;
}

//~ NOTE(ljre): API
API bool
OS_Init(const OS_InitDesc* desc, OS_InitOutput* out_output)
{
	Trace();
	
	WNDCLASSW window_class = {
		.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW,
		.lpfnWndProc = WindowProc,
		.lpszClassName = global_class_name,
		.hInstance = global_instance,
		.hCursor = LoadCursorA(NULL, IDC_ARROW),
	};
	
	if (!RegisterClassW(&window_class))
		OS_ExitWithErrorMessage("Could not create window class.");
	
	OS_WindowState config = desc->window_initial_state;
	
	if (config.center_window)
	{
		config.x = (global_monitor.right - global_monitor.left - config.width) / 2;
		config.y = (global_monitor.bottom - global_monitor.top - config.height) / 2;
		
		config.center_window = false;
	}
	
	bool ok = true;
	
	if (desc->flags & OS_InitFlags_WindowAndGraphics)
	{
		bool created_window = false;
		
		for Arena_TempScope(global_scratch_arena)
		{
			uintsize size = 0;
			for (int32 i = 0; i < ArrayLength(config.title); ++i)
			{
				if (!config.title[i])
				{
					size = i;
					break;
				}
			}
			
			wchar_t* window_name = Win32_StringToWide(global_scratch_arena, StrMake(size, config.title));
			
			switch (desc->window_desired_api)
			{
				default: break;
				
				case OS_WindowGraphicsApi_OpenGL: created_window = Win32_CreateOpenGLWindow(&config, window_name); break;
				case OS_WindowGraphicsApi_Direct3D11: created_window = Win32_CreateD3d11Window(&config, window_name); break;
			}
			
			if (!created_window)
				created_window = Win32_CreateD3d11Window(&config, window_name);
			if (!created_window)
				created_window = Win32_CreateOpenGLWindow(&config, window_name);
		}
		
		ok = ok && created_window;
	}
	
	if (ok)
	{
		DEV_BROADCAST_DEVICEINTERFACE notification_filter = {
			sizeof(notification_filter),
			DBT_DEVTYP_DEVICEINTERFACE
		};
		
		if (!RegisterDeviceNotification(global_window, &notification_filter, DEVICE_NOTIFY_WINDOW_HANDLE))
			OS_DebugLog("Failed to register input device notification.");
		if ((desc->flags & OS_InitFlags_SimpleAudio) && !Win32_InitSimpleAudio())
			OS_DebugLog("Failed to initialize SimpleAudio API.");
		
		ok = ok && Win32_InitInput();
		
		if (ok)
		{
			config.resized_by_user = false;
			config.should_close = false;
			global_window_state = config;
			
			out_output->window_state = config;
			out_output->input_state = global_input_state;
			
			if (desc->flags & OS_InitFlags_WindowAndGraphics)
			{
				out_output->graphics_context = &global_graphics_context;
				ShowWindow(global_window, SW_SHOWDEFAULT);
			}
		}
	}
	
	return ok;
}

API void
OS_PollEvents(OS_WindowState* inout_state, OS_InputState* out_input)
{
	Trace();
	
	SetWindowLongPtrW(global_window, GWLP_USERDATA, (LONG_PTR)out_input);
	
	Win32_UpdateInputEarly(out_input);
	Win32_UpdateWindowStateIfNeeded(inout_state);
	
	MSG message;
	while (PeekMessageW(&message, 0, 0, 0, PM_REMOVE))
	{
		TranslateMessage(&message);
		DispatchMessageW(&message);
	}
	
	Win32_UpdateInputLate(out_input);
}

API void
OS_ExitWithErrorMessage(const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	String str = Arena_VPrintf(global_scratch_arena, fmt, args);
	va_end(args);
	
	wchar_t* wstr = Win32_StringToWide(global_scratch_arena, str);
	MessageBoxW(NULL, wstr, L"Fatal Error!", MB_OK);
	exit(1);
	/* no return */
}

API void
OS_MessageBox(String title, String message)
{
	Trace();
	
	for Arena_TempScope(global_scratch_arena)
	{
		wchar_t* wtitle = Win32_StringToWide(global_scratch_arena, title);
		wchar_t* wmessage = Win32_StringToWide(global_scratch_arena, message);
		
		MessageBoxW(NULL, wmessage, wtitle, MB_OK);
	}
}

API void
OS_DefaultWindowState(OS_WindowState* out_state)
{
	*out_state = (OS_WindowState) {
		.show_cursor = true,
		.lock_cursor = false,
		.center_window = true,
		.fullscreen = false,
		.should_close = false,
		
		.x = 0,
		.y = 0,
		.width = 800,
		.height = 600,
	};
}

API bool
OS_WaitForVsync(void)
{
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
OS_VirtualReserve(uintsize size)
{
	Trace();
	
	return VirtualAlloc(NULL, size, MEM_RESERVE, PAGE_READWRITE);
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
	Assert(ok);
}

API void
OS_VirtualRelease(void* ptr, uintsize size)
{
	Trace();
	(void)size;
	
	BOOL ok = VirtualFree(ptr, 0, MEM_RELEASE);
	Assert(ok);
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
	*out_ticks_per_second = global_time_frequency;
	return Win32_GetTimer() - global_process_started_time;
}

API float64
OS_GetTimeInSeconds(void)
{
	int64 time = Win32_GetTimer();
	return (float64)(time - global_process_started_time) / (float64)global_time_frequency;
}

API bool
OS_ReadEntireFile(String path, Arena* output_arena, void** out_data, uintsize* out_size)
{
	Trace(); TraceText(path);
	
	HANDLE handle;
	
	for Arena_TempScope(global_scratch_arena)
	{
		LPWSTR wpath = Win32_StringToWide(global_scratch_arena, path);
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
	
	HANDLE handle;
	
	for Arena_TempScope(global_scratch_arena)
	{
		LPWSTR wpath = Win32_StringToWide(global_scratch_arena, path);
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
	
	const char* const dll_name = "./build/hot-game.dll";
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
		Assert(library);
		
		result = GetProcAddress(library, "G_Main");
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
			Assert(library);
			
			result = GetProcAddress(library, "G_Main");
			
			SetForegroundWindow(global_window);
		}
	}
	
	Assert(result);
	
	saved_result = result;
	return result;
}
#endif //CONFIG_ENABLE_HOT

#ifdef CONFIG_DEBUG
API void
OS_DebugMessageBox(const char* fmt, ...)
{
	for Arena_TempScope(global_scratch_arena)
	{
		va_list args;
		va_start(args, fmt);
		String str = Arena_VPrintf(global_scratch_arena, fmt, args);
		va_end(args);
		
		wchar_t* wstr = Win32_StringToWide(global_scratch_arena, str);
		MessageBoxW(NULL, wstr, L"OS_DebugMessageBox", MB_OK);
	}
}

API void
OS_DebugLog(const char* fmt, ...)
{
	for Arena_TempScope(global_scratch_arena)
	{
		va_list args;
		va_start(args, fmt);
		String str = Arena_VPrintf(global_scratch_arena, fmt, args);
		Arena_PushData(global_scratch_arena, &""); // null terminator
		va_end(args);
		
		if (IsDebuggerPresent())
			OutputDebugStringA((const char*)str.data);
		else
			fwrite(str.data, 1, str.size, stderr);
	}
}
#endif //CONFIG_DEBUG
