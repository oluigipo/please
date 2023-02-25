#include "api_os_d3d11.h"

#define near
#define far

#if defined(CONFIG_DEBUG) && !defined(__GNUC__)
#   include <dxgidebug.h>
#endif

#undef near
#undef far

#define D3D11CreateDeviceAndSwapChain global_proc_D3D11CreateDeviceAndSwapChain
typedef HRESULT WINAPI
ProcD3D11CreateDeviceAndSwapChain(
	IDXGIAdapter* pAdapter,
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

#if defined(CONFIG_DEBUG) && !defined(__GNUC__)
static IDXGIInfoQueue* global_direct3d_info_queue;
#endif

//~ Internal API
static void
Win32_DestroyD3d11Window(void)
{
	Trace();
	
	if (global_direct3d.context)
		ID3D11DeviceContext_Release(global_direct3d.context);
	if (global_direct3d.swapchain)
		IDXGISwapChain_Release(global_direct3d.swapchain);
	if (global_direct3d.device)
		ID3D11Device_Release(global_direct3d.device);
	
	DestroyWindow(g_win32.window);
}

static bool
Win32_D3d11SwapBuffers(int32 vsync_count)
{
	Trace();
	
	int32 to_d3d11 = Min(vsync_count, 0);
	IDXGISwapChain_Present(global_direct3d.swapchain, 0, 0);
	bool ok = true;
	
#if defined(CONFIG_DEBUG) && !defined(__GNUC__)
	// NOTE(ljre): Check for debug messages
	if (global_direct3d_info_queue)
	{
		static uint64 next = 0;
		
		Arena* scratch_arena = Win32_GetThreadScratchArena();
		uint64 message_count = IDXGIInfoQueue_GetNumStoredMessages(global_direct3d_info_queue, DXGI_DEBUG_ALL);
		
		for (uint64 i = next; i < message_count; ++i)
		{
			SIZE_T message_length;
			if (S_OK == IDXGIInfoQueue_GetMessage(global_direct3d_info_queue, DXGI_DEBUG_ALL, i, NULL, &message_length))
			{
				for Arena_TempScope(scratch_arena)
				{
					DXGI_INFO_QUEUE_MESSAGE* message = Arena_Push(scratch_arena, message_length);
					
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

static void
Win32_D3d11ResizeWindow(void)
{
	ID3D11DeviceContext_ClearState(global_direct3d.context);
	ID3D11RenderTargetView_Release(global_direct3d.target);
	global_direct3d.target = NULL;
	
	HRESULT hr;
	
	hr = IDXGISwapChain_ResizeBuffers(global_direct3d.swapchain, 0, 0, 0, DXGI_FORMAT_UNKNOWN, 0);
	SafeAssert(SUCCEEDED(hr));
	
	ID3D11Texture2D* backbuffer;
	hr = IDXGISwapChain_GetBuffer(global_direct3d.swapchain, 0, &IID_ID3D11Texture2D, (void**)&backbuffer);
	SafeAssert(SUCCEEDED(hr));
	
	D3D11_TEXTURE2D_DESC backbuffer_desc;
	ID3D11Texture2D_GetDesc(backbuffer, &backbuffer_desc);
	
	hr = ID3D11Device_CreateRenderTargetView(global_direct3d.device, (ID3D11Resource*)backbuffer, NULL, &global_direct3d.target);
	SafeAssert(SUCCEEDED(hr));
	
	ID3D11Resource_Release(backbuffer);
	
	ID3D11DepthStencilView_Release(global_direct3d.depth_stencil);
	global_direct3d.depth_stencil = NULL;
	
	D3D11_TEXTURE2D_DESC depth_stencil_desc = {
		.Width = backbuffer_desc.Width,
		.Height = backbuffer_desc.Height,
		.MipLevels = 1,
		.ArraySize = 1,
		.Format = DXGI_FORMAT_D24_UNORM_S8_UINT,
		.SampleDesc = {
			.Count = 1,
			.Quality = 0,
		},
		.Usage = D3D11_USAGE_DEFAULT,
		.BindFlags = D3D11_BIND_DEPTH_STENCIL,
		.CPUAccessFlags = 0,
		.MiscFlags = 0,
	};
	
	ID3D11Texture2D* depth_stencil;
	hr = ID3D11Device_CreateTexture2D(global_direct3d.device, &depth_stencil_desc, NULL, &depth_stencil);
	SafeAssert(SUCCEEDED(hr));
	hr = ID3D11Device_CreateDepthStencilView(global_direct3d.device, (ID3D11Resource*)depth_stencil, NULL, &global_direct3d.depth_stencil);
	SafeAssert(SUCCEEDED(hr));
	
	ID3D11Texture2D_Release(depth_stencil);
}

static bool
Win32_CreateD3d11Window(const OS_WindowState* config, const wchar_t* title)
{
	Trace();
	
	DWORD style = (WS_OVERLAPPEDWINDOW); // & ~(WS_THICKFRAME | WS_MAXIMIZEBOX);
	HWND window = CreateWindowExW(
		0, g_win32.class_name, title, style,
		config->x, config->y,
		config->width, config->height,
		NULL, NULL, g_win32.instance, NULL);
	
	if (!window)
		return false;
	
	HDC hdc = NULL;//GetDC(window);
	
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
	
	RTL_OSVERSIONINFOW version_info = { sizeof(version_info), };
	RtlGetVersion(&version_info);
	bool is_win10_or_later = (version_info.dwMajorVersion >= 10);
	
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
	
#if defined(CONFIG_DEBUG) && !defined(__GNUC__)
	flags |= D3D11_CREATE_DEVICE_DEBUG;
	
	D3D_FEATURE_LEVEL feature_levels[] = {
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_1,
		D3D_FEATURE_LEVEL_10_0,
		D3D_FEATURE_LEVEL_9_3,
		//D3D_FEATURE_LEVEL_9_2,
		//D3D_FEATURE_LEVEL_9_1,
	};
#else
	D3D_FEATURE_LEVEL feature_levels[] = {
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_1,
		D3D_FEATURE_LEVEL_10_0,
		D3D_FEATURE_LEVEL_9_3,
		//D3D_FEATURE_LEVEL_9_2,
		//D3D_FEATURE_LEVEL_9_1,
	};
#endif
	
	HRESULT hr = D3D11CreateDeviceAndSwapChain(
		NULL, D3D_DRIVER_TYPE_HARDWARE, NULL,
		flags, feature_levels, ArrayLength(feature_levels),
		D3D11_SDK_VERSION, &swapchain_desc,
		&global_direct3d.swapchain,
		&global_direct3d.device,
		NULL,
		&global_direct3d.context);
	if (FAILED(hr))
	{
		DestroyWindow(window);
		return false;
	}
	
	// Make render target view and get backbuffer desc
	ID3D11Texture2D* backbuffer;
	hr = IDXGISwapChain_GetBuffer(global_direct3d.swapchain, 0, &IID_ID3D11Texture2D, (void**)&backbuffer);
	SafeAssert(SUCCEEDED(hr));
	
	D3D11_TEXTURE2D_DESC backbuffer_desc;
	ID3D11Texture2D_GetDesc(backbuffer, &backbuffer_desc);
	hr = ID3D11Device_CreateRenderTargetView(global_direct3d.device, (ID3D11Resource*)backbuffer, NULL, &global_direct3d.target);
	SafeAssert(SUCCEEDED(hr));
	
	ID3D11Resource_Release(backbuffer);
	
	// Make depth stencil view
	D3D11_TEXTURE2D_DESC depth_stencil_desc = {
		.Width = backbuffer_desc.Width,
		.Height = backbuffer_desc.Height,
		.MipLevels = 1,
		.ArraySize = 1,
		.Format = DXGI_FORMAT_D24_UNORM_S8_UINT,
		.SampleDesc = {
			.Count = 1,
			.Quality = 0,
		},
		.Usage = D3D11_USAGE_DEFAULT,
		.BindFlags = D3D11_BIND_DEPTH_STENCIL,
		.CPUAccessFlags = 0,
		.MiscFlags = 0,
	};
	
	ID3D11Texture2D* depth_stencil;
	hr = ID3D11Device_CreateTexture2D(global_direct3d.device, &depth_stencil_desc, NULL, &depth_stencil);
	SafeAssert(SUCCEEDED(hr));
	hr = ID3D11Device_CreateDepthStencilView(global_direct3d.device, (ID3D11Resource*)depth_stencil, NULL, &global_direct3d.depth_stencil);
	SafeAssert(SUCCEEDED(hr));
	ID3D11Texture2D_Release(depth_stencil);
	
#if defined(CONFIG_DEBUG) && !defined(__GNUC__)
	// NOTE(ljre): Load debugging thingy
	{
		HMODULE library = Win32_LoadLibrary("dxgidebug.dll");
		
		if (library)
		{
			HRESULT (WINAPI* get_debug_interface)(REFIID, void**);
			get_debug_interface = (void*)GetProcAddress(library, "DXGIGetDebugInterface");
			
			if (get_debug_interface)
				get_debug_interface(&IID_IDXGIInfoQueue, (void**)&global_direct3d_info_queue);
		}
	}
#endif
	
	g_win32.window = window;
	g_win32.hdc = hdc;
	g_graphics_context.d3d11 = &global_direct3d;
	g_graphics_context.api = OS_WindowGraphicsApi_Direct3D11;
	g_graphics_context.present_and_vsync = Win32_D3d11SwapBuffers;
	
	return true;
}
