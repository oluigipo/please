#include "api_os_d3d11.h"

#define near
#define far

#if defined(CONFIG_DEBUG)
#   include <dxgidebug.h>
#endif

#undef near
#undef far

#define D3D11CreateDeviceAndSwapChain global_proc_D3D11CreateDeviceAndSwapChain
typedef HRESULT WINAPI
ProcD3D11CreateDeviceAndSwapChain(IDXGIAdapter* pAdapter,
	D3D_DRIVER_TYPE DriverType,
	HMODULE Software,
	UINT Flags,
	const D3D_FEATURE_LEVEL* pFeatureLevels,
	UINT FeatureLevels,
	UINT SDKVersion,
	const DXGI_SWAP_CHAIN_DESC* pSwapChainDesc,
	IDXGISwapChain** ppSwapChain,
	ID3D11Device** ppDevice,
	D3D_FEATURE_LEVEL* pFeatureLevel,
	ID3D11DeviceContext** ppImmediateContext);

//~ Globals
static OS_D3d11Api global_direct3d;
static ProcD3D11CreateDeviceAndSwapChain* D3D11CreateDeviceAndSwapChain;

#if defined(CONFIG_DEBUG)
static IDXGIInfoQueue* global_direct3d_info_queue;
#endif

//~ Internal API
static void
Win32_DestroyD3d11Window(void)
{
	if (global_direct3d.context)
		ID3D11DeviceContext_Release(global_direct3d.context);
	if (global_direct3d.swapchain)
		IDXGISwapChain_Release(global_direct3d.swapchain);
	if (global_direct3d.device)
		ID3D11Device_Release(global_direct3d.device);
	
	DestroyWindow(global_window);
}

static bool
Win32_D3d11SwapBuffers(int32 vsync_count)
{
	int32 to_d3d11 = Min(vsync_count, 4);
	bool ok = IDXGISwapChain_Present(global_direct3d.swapchain, to_d3d11, 0);
	
#if defined(CONFIG_DEBUG)
	// NOTE(ljre): Check for debug messages
	if (global_direct3d_info_queue)
	{
		static uint64 next = 0;
		uint64 message_count = IDXGIInfoQueue_GetNumStoredMessages(global_direct3d_info_queue, DXGI_DEBUG_ALL);
		
		for (uint64 i = next; i < message_count; ++i)
		{
			SIZE_T message_length;
			if (S_OK == IDXGIInfoQueue_GetMessage(global_direct3d_info_queue, DXGI_DEBUG_ALL, i, NULL, &message_length))
			{
				for Arena_TempScope(global_scratch_arena)
				{
					DXGI_INFO_QUEUE_MESSAGE* message = Arena_Push(global_scratch_arena, message_length);
					
					IDXGIInfoQueue_GetMessage(global_direct3d_info_queue, DXGI_DEBUG_ALL, i, message, &message_length);
					OS_DebugMessageBox("%.*s", message->DescriptionByteLength, message->pDescription);
				}
			}
		}
	}
#endif
	
	if (ok)
	{
		ok = (vsync_count > 0);
		vsync_count -= to_d3d11;
		
		while (vsync_count --> 0)
		{
			ok = OS_WaitForVsync();
			
			if (!ok)
				break;
		}
	}
	
	return ok;
}

static bool
Win32_CreateD3d11Window(const OS_WindowState* config, const wchar_t* title)
{
	Trace();
	
	DWORD style = (WS_OVERLAPPEDWINDOW); // & ~(WS_THICKFRAME | WS_MAXIMIZEBOX);
	HWND window = CreateWindowExW(0, global_class_name, title, style,
		config->x, config->y,
		config->width, config->height,
		NULL, NULL, global_instance, NULL);
	
	if (!window)
		return false;
	
	HDC hdc = GetDC(window);
	
	HMODULE library = Win32_LoadLibrary("d3d11.dll");
	if (!library)
	{
		DestroyWindow(window);
		return false;
	}
	
	D3D11CreateDeviceAndSwapChain = (void*)GetProcAddress(library, "D3D11CreateDeviceAndSwapChain");
	if (!D3D11CreateDeviceAndSwapChain)
	{
		DestroyWindow(window);
		return false;
	}
	
	bool is_win10_or_later = IsWindows10OrGreater();
	
	DXGI_SWAP_CHAIN_DESC swapchain_desc = {
		.BufferDesc = {
			.Width = 0,
			.Height = 0,
			.RefreshRate = {
				.Numerator = 0,
				.Denominator = 1,
			},
			.Format = DXGI_FORMAT_B8G8R8A8_UNORM,
			.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED,
			.Scaling = DXGI_MODE_SCALING_UNSPECIFIED,
		},
		.SampleDesc = {
			.Count = 1,
			.Quality = 0,
		},
		.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT,
		.BufferCount = 3,
		.OutputWindow = window,
		.Windowed = true,
		.SwapEffect = is_win10_or_later ? DXGI_SWAP_EFFECT_FLIP_DISCARD : DXGI_SWAP_EFFECT_DISCARD,
		.Flags = 0,
	};
	
	UINT flags = 0;
	
#if defined(CONFIG_DEBUG)
	flags |= D3D11_CREATE_DEVICE_DEBUG;
	
	D3D_FEATURE_LEVEL feature_levels[] = {
		D3D_FEATURE_LEVEL_9_3
	};
#else
	D3D_FEATURE_LEVEL feature_levels[] = {
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_1,
		D3D_FEATURE_LEVEL_10_0,
		D3D_FEATURE_LEVEL_9_3,
	};
#endif
	
	HRESULT result = D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL,
		flags, feature_levels, ArrayLength(feature_levels),
		D3D11_SDK_VERSION, &swapchain_desc,
		&global_direct3d.swapchain,
		&global_direct3d.device,
		NULL,
		&global_direct3d.context);
	
	if (FAILED(result))
	{
		DestroyWindow(window);
		return false;
	}
	
	ID3D11Resource* backbuffer;
	IDXGISwapChain_GetBuffer(global_direct3d.swapchain, 0, &IID_ID3D11Resource, (void**)&backbuffer);
	ID3D11Device_CreateRenderTargetView(global_direct3d.device, backbuffer, NULL, &global_direct3d.target);
	ID3D11Resource_Release(backbuffer);
	
#if defined(CONFIG_DEBUG)
	// NOTE(ljre): Load debugging thingy
	{
		HMODULE library = Win32_LoadLibrary("dxgidebug.dll");
		
		if (library)
		{
			OS_DebugLog("Loaded Library: dxgidebug.dll\n");
			
			HRESULT (WINAPI* get_debug_interface)(REFIID, void**);
			get_debug_interface = (void*)GetProcAddress(library, "DXGIGetDebugInterface");
			
			if (get_debug_interface)
				get_debug_interface(&IID_IDXGIInfoQueue, (void**)&global_direct3d_info_queue);
		}
	}
#endif
	
	ShowWindow(window, SW_SHOWDEFAULT);
	
	global_window = window;
	global_hdc = hdc;
	global_graphics_context.d3d11 = &global_direct3d;
	global_graphics_context.api = OS_WindowGraphicsApi_Direct3D11;
	global_graphics_context.present_and_vsync = Win32_D3d11SwapBuffers;
	
	return true;
}
