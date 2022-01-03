#define near
#define far

#include "internal_direct3d.h"

#if defined(DEBUG)
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
internal GraphicsContext_Direct3D global_direct3d;
internal ProcD3D11CreateDeviceAndSwapChain* D3D11CreateDeviceAndSwapChain;

#if defined(DEBUG)
internal IDXGIInfoQueue* global_direct3d_info_queue;
#endif

//~ Internal API
internal void
Win32_DestroyDirect3DWindow(void)
{
	if (global_direct3d.context)
		ID3D11DeviceContext_Release(global_direct3d.context);
	if (global_direct3d.swapchain)
		IDXGISwapChain_Release(global_direct3d.swapchain);
	if (global_direct3d.device)
		ID3D11Device_Release(global_direct3d.device);
	
	DestroyWindow(global_window);
}

internal void
Win32_Direct3DSwapBuffers(void)
{
	IDXGISwapChain_Present(global_direct3d.swapchain, 1, 0);
	
#if defined(DEBUG)
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
				
				DXGI_INFO_QUEUE_MESSAGE* message = Platform_HeapAlloc(message_length);
				
				IDXGIInfoQueue_GetMessage(global_direct3d_info_queue, DXGI_DEBUG_ALL, i, message, &message_length);
				Platform_DebugMessageBox("%.*s", message->DescriptionByteLength, message->pDescription);
				
				Platform_HeapFree(message);
			}
		}
	}
#endif
}

internal bool32
Win32_CreateDirect3DWindow(int32 width, int32 height, const wchar_t* title)
{
	Trace("Win32_CreateDirect3DWindow");
	
	DWORD style = (WS_OVERLAPPEDWINDOW) & ~(WS_THICKFRAME | WS_MAXIMIZEBOX);
	HWND window = CreateWindowExW(0, global_class_name, title, style,
								  CW_USEDEFAULT, CW_USEDEFAULT,
								  width, height,
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
		.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD,
		.Flags = 0,
	};
	
	UINT flags = 0;
	
#if defined(DEBUG)
	flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
	
	HRESULT result = D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL,
												   flags, NULL, 0,
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
	
	// NOTE(ljre): Load D3D Compiler.
	{
		HMODULE library = Win32_LoadLibrary("d3dcompiler_47.dll");
		
		if (library)
		{
			Platform_DebugLog("Loaded Library: d3dcompiler_47.dll\n");
			global_direct3d.compile_shader = (void*)GetProcAddress(library, "D3DCompile");
		}
	}
	
#if defined(DEBUG)
	// NOTE(ljre): Load debugging thingy
	{
		HMODULE library = Win32_LoadLibrary("dxgidebug.dll");
		
		if (library)
		{
			Platform_DebugLog("Loaded Library: dxgidebug.dll\n");
			
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
	global_graphics_context.d3d = &global_direct3d;
	global_graphics_context.api = GraphicsAPI_Direct3D;
	
	return true;
}
