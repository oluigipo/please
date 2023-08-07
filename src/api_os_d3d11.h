#ifndef API_OS_D3D11_H
#define API_OS_D3D11_H

#ifndef WIN32_LEAN_AND_MEAN
#	define WIN32_LEAN_AND_MEAN
#endif

#ifndef COBJMACROS
#	define COBJMACROS
#endif

#pragma push_macro("far")
#pragma push_macro("near")

#define far
#define near

#include <d3d11.h>

struct OS_D3d11Api
{
	ID3D11Device* device;
	ID3D11DeviceContext* context;
	IDXGISwapChain* swapchain;
	ID3D11RenderTargetView* target;
	ID3D11DepthStencilView* depth_stencil;
};

#pragma pop_macro("far")
#pragma pop_macro("near")

#endif //API_OS_D3D11_H
