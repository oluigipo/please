#include "config.h"
#include "api_os.h"

#if defined(CONFIG_OS_WIN32) || defined(CONFIG_OS_WIN32LINUX)
#	if defined(_MSC_VER)
#		pragma comment(lib, "kernel32.lib")
#		pragma comment(lib, "user32.lib")
#		pragma comment(lib, "gdi32.lib")
//#		pragma comment(lib, "hid.lib")
#		pragma comment(lib, "ntdll.lib")
#		if defined(CONFIG_ENABLE_STEAM)
#			if defined(CONFIG_M64)
#				pragma comment(lib, "lib\\steam_api64.lib")
#			else
#				pragma comment(lib, "lib\\steam_api.lib")
#			endif
#		endif
#	endif

#	include <math.h>
#	include <ext/cglm/cglm.h>
DisableWarnings();
#	define WIN32_LEAN_AND_MEAN
#	define COBJMACROS
#	define DIRECTINPUT_VERSION 0x0800
#	define near
#	define far
#	include <windows.h>
#	include <dwmapi.h>
#	include <versionhelpers.h>
#	include <dbt.h>
#	include <stdio.h>
#	include <stdlib.h>
#	include <time.h>
#	include <synchapi.h>
#	include <shellscalingapi.h>
#	include <xinput.h>
#	include <dinput.h>
#	include <audioclient.h>
#	include <audiopolicy.h>
#	include <mmdeviceapi.h>
#	include <Functiondiscoverykeys_devpkey.h>
#	include <ext/guid_utils.h>
#	if defined(CONFIG_ENABLE_D3D11) && defined(CONFIG_DEBUG) && !defined(__GNUC__)
#		include <dxgidebug.h>
#	endif
#	undef near
#	undef far
ReenableWarnings();

static uint64 Win32_GetTimer(void);
static String Win32_WideToString(Arena* output_arena, LPWSTR wide);
static LPWSTR Win32_StringToWide(Arena* output_arena, String str);
static HMODULE Win32_LoadLibrary(const char* name);
static HMODULE Win32_LoadLibraryWide(LPCWSTR name);
static Arena* Win32_GetThreadScratchArena(void);
static bool Win32_WaitForVsync(void);
static bool Win32_InitAudio(const OS_InitDesc* init_desc, OS_State* os_state);
static void Win32_DeinitAudio(OS_State* os_state);
static void Win32_UpdateAudioEndpointIfNeeded(void);
static bool Win32_CreateOpenGLWindow(const OS_WindowState* config, const wchar_t* title);
static void Win32_OpenGLResizeWindow(void);
static void Win32_DestroyOpenGLWindow(void);
static bool Win32_CreateD3d11Window(const OS_WindowState* config, const wchar_t* title, int32 desired_feature_level);
static void Win32_D3d11ResizeWindow(void);
static void Win32_DestroyD3d11Window(void);
static void Win32_CheckForGamepads(void);
static bool Win32_InitInput(void);
static void Win32_UpdateKeyboardKey(OS_State* os_state, uint32 vkcode, bool is_down);
static void Win32_UpdateMouseButton(OS_State* os_state, OS_MouseButton button, bool is_down);
static inline void Win32_UpdateInputEarly(OS_State* os_state);
static inline void Win32_UpdateInputLate(OS_State* os_state);

static const OS_KeyboardKey global_keyboard_key_table[256];
#	if 0
int __declspec(dllexport) NvOptimusEnablement = 1;
int __declspec(dllexport) AmdPowerXpressRequestHighPerformance = 1;
#	endif

#	include "os_internal_gamepad_map.h"
#	include "os_win32_guid.c"
#	include "os_win32.c"
#	include "os_win32_input.c"
#	include "os_win32_audio.c"
#	ifdef CONFIG_ENABLE_OPENGL
#		include "api_os_opengl.h"
#		include "os_win32_opengl.c"
#	endif
#	ifdef CONFIG_ENABLE_D3D11
#		include "api_os_d3d11.h"
#		include "os_win32_d3d11.c"
#	endif
#elif defined(CONFIG_OS_LINUX)
#	include "os_linux.c"
#elif defined(CONFIG_OS_ANDROID)
#	include "os_android.c"
#endif
