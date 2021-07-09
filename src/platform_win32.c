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

// Direct3D
#include <d3d11.h>

// Input
#include <xinput.h>
#include <dinput.h>

// Audio
#include <audioclient.h>
#include <audiopolicy.h>
#include <mmdeviceapi.h>

#define INTERNAL_COMPLETE_D3D_CONTEXT
#include "internal_direct3d.h"

// NOTE(ljre): Last time I tried, MinGW didn't have 'dxgidebug.h' header.
#if defined(DEBUG) && !defined(__GNUC__)
#   include <dxgidebug.h>
#endif

#include "platform_win32_guid.c"

//- Enable Warnings
#if defined(__clang__)
#   pragma clang diagnostic pop
#elif defined(__GNUC__)
#   pragma GCC diagnostic pop
#else
#   pragma warning(pop, 0)
#endif

struct Win32_DeferredEvents
{
    int32 window_x, window_y;
    int32 window_width, window_height;
} typedef Win32_DeferredEvents;

//~ Globals
internal HANDLE global_heap;
internal const wchar_t* global_class_name;
internal HINSTANCE global_instance;
internal int64 global_time_frequency;
internal int64 global_process_started_time;
internal HWND global_window;
internal HDC global_hdc;
internal bool32 global_window_should_close;
internal GraphicsContext global_graphics_context;
internal Win32_DeferredEvents global_deferred_events = { -1, -1, -1, -1 };

internal int32 global_window_width;
internal int32 global_window_height;
internal RECT global_monitor;

//~ Functions
internal void
ProcessDeferredEvents(void)
{
    // NOTE(ljre): Window Position & Size
    {
        UINT flags = SWP_NOSIZE | SWP_NOMOVE;
        
        int32 x = global_deferred_events.window_x;
        int32 y = global_deferred_events.window_y;
        int32 width = global_deferred_events.window_width;
        int32 height = global_deferred_events.window_height;
        
        if (x != -1 && y != -1)
            flags &=~ (UINT)SWP_NOMOVE;
        
        if (width != -1 && height != -1)
        {
            flags &=~ (UINT)SWP_NOSIZE;
            
            global_window_width = width;
            global_window_height = height;
        }
        
        if (flags != (SWP_NOSIZE | SWP_NOMOVE))
            SetWindowPos(global_window, NULL, x, y, width, height, flags | SWP_NOZORDER);
        
        global_deferred_events.window_x = -1;
        global_deferred_events.window_y = -1;
        global_deferred_events.window_width = -1;
        global_deferred_events.window_height = -1;
    }
}

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
    int32 result = Engine_Main(argc, argv);
    
    return result;
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
Platform_CreateWindow(int32 width, int32 height, String name, uint32 flags, const GraphicsContext** out_graphics)
{
    Trace("Platform_CreateWindow");
    
    wchar_t window_name[1024];
    Win32_ConvertStringToWSTR(name, window_name, ArrayLength(window_name));
    
    bool32 ok = false;
    
    if (flags & GraphicsAPI_OpenGL)
        ok = ok || Win32_CreateOpenGLWindow(width, height, window_name);
    if (flags & GraphicsAPI_Direct3D)
        ok = ok || Win32_CreateDirect3DWindow(width, height, window_name);
    
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
        
        global_window_width = width;
        global_window_height = height;
        
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
        Platform_CenterWindow();
        Platform_PollEvents();
        
        *out_graphics = &global_graphics_context;
        ShowWindow(global_window, SW_SHOWDEFAULT);
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
Platform_SetWindow(int32 x, int32 y, int32 width, int32 height)
{
    if (x != -1)
        global_deferred_events.window_x = x;
    if (y != -1)
        global_deferred_events.window_y = y;
    if (width != -1)
        global_deferred_events.window_width = width;
    if (height != -1)
        global_deferred_events.window_height = height;
}

API void
Platform_CenterWindow(void)
{
    int32 monitor_width = global_monitor.right - global_monitor.left;
    int32 monitor_height = global_monitor.bottom - global_monitor.top;
    
    global_deferred_events.window_x = monitor_width / 2 - global_window_width / 2 + global_monitor.left;
    global_deferred_events.window_y = monitor_height / 2 - global_window_height / 2 + global_monitor.top;
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
    ProcessDeferredEvents();
    
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
Platform_VirtualFree(void* ptr, uintsize size)
{
    Trace("Platform_VirtualFree");
    VirtualFree(ptr, 0, MEM_RELEASE);
}

API void*
Platform_ReadEntireFile(String path, uintsize* out_size)
{
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
	
	uintsize size = (uintsize)sizeStruct.QuadPart;
	void* memory = Platform_HeapAlloc(size);
	if (!memory)
	{
		CloseHandle(handle);
		return NULL;
	}
	
	DWORD bytes_read;
	if (!ReadFile(handle, memory, (DWORD)size, &bytes_read, NULL) ||
		(uintsize)bytes_read != size)
	{
		Platform_HeapFree(memory);
		memory = NULL;
		out_size = NULL;
	}
	
	if (out_size)
		*out_size = (uintsize)bytes_read;
	
	CloseHandle(handle);
	return memory;
}

API bool32
Platform_WriteEntireFile(String path, const void* data, uintsize size)
{
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
    HMODULE library = LoadLibraryA("discord_game_sdk.dll");
    if (!library)
        return NULL;
    
    void* result = GetProcAddress(library, "DiscordCreate");
    
    if (!result)
    {
        FreeLibrary(library);
        return NULL;
    }
    
    Platform_DebugLog("Loaded Library: discord_game_sdk.dll\n");
    return result;
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
    fflush(stdout);
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
