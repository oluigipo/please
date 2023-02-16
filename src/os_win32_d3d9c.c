#include "api_os_d3d9c.h"

typedef IDirect3D9* WINAPI ProcDirect3DCreate9(UINT SDKVersion);

//~ Globals
static OS_D3d9cApi global_d3d9c;

//~ Internal API
static void
Win32_DestroyD3d9cWindow(void)
{
	Trace();
	
	if (global_d3d9c.device)
		IDirect3DDevice9_Release(global_d3d9c.device);
	if (global_d3d9c.d3d9)
		IDirect3D9_Release(global_d3d9c.d3d9);
	
	DestroyWindow(global_window);
}

static bool
Win32_D3d9cSwapBuffers(int32 vsync_count)
{
	Trace();
	SafeAssert(SUCCEEDED(IDirect3DDevice9_Present(global_d3d9c.device, NULL, NULL, NULL, NULL)));
	
	bool ok = true;
	while (ok && vsync_count--)
		ok = OS_WaitForVsync();
	
	return ok;
}

static bool
Win32_CreateD3d9cWindow(const OS_WindowState* config, const wchar_t* title)
{
	Trace();
	
	HMODULE library = Win32_LoadLibrary("d3d9.dll");
	if (!library)
		return false;
	
	ProcDirect3DCreate9* create_proc = (void*)GetProcAddress(library, "Direct3DCreate9");
	if (!create_proc)
		return false;
	
	IDirect3D9* d3d9 = create_proc(D3D_SDK_VERSION);
	if (!d3d9)
		return false;
	
	HWND window = CreateWindowExW(
		0, global_class_name, title, WS_OVERLAPPEDWINDOW,
		config->x, config->y, config->width, config->height,
		NULL, NULL, global_instance, NULL);
	if (!window)
	{
		IDirect3D9_Release(d3d9);
		return false;
	}
	
	D3DPRESENT_PARAMETERS present_params = {
		.BackBufferWidth = 0,
		.BackBufferHeight = 0,
		.BackBufferFormat = D3DFMT_UNKNOWN,
		.BackBufferCount = 2,
		.MultiSampleType = D3DMULTISAMPLE_NONE,
		.MultiSampleQuality = 0,
		.SwapEffect = D3DSWAPEFFECT_DISCARD,
		.hDeviceWindow = window,
		.Windowed = true,
		.EnableAutoDepthStencil = true,
		.AutoDepthStencilFormat = D3DFMT_D24S8,
		.Flags = D3DPRESENTFLAG_DISCARD_DEPTHSTENCIL,
		.FullScreen_RefreshRateInHz = 0,
		.PresentationInterval = 0,
	};
	
	IDirect3DDevice9* device;
	HRESULT hr = IDirect3D9_CreateDevice(
		d3d9, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, window,
		D3DCREATE_HARDWARE_VERTEXPROCESSING, &present_params, &device);
	if (!SUCCEEDED(hr))
	{
		IDirect3D9_Release(d3d9);
		DestroyWindow(window);
		return false;
	}
	
	global_d3d9c.d3d9 = d3d9;
	global_d3d9c.device = device;
	global_window = window;
	global_graphics_context.d3d9c = &global_d3d9c;
	global_graphics_context.api = OS_WindowGraphicsApi_Direct3D9c;
	global_graphics_context.present_and_vsync = Win32_D3d9cSwapBuffers;
	
	return true;
}
