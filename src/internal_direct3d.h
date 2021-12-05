#ifndef INTERNAL_DIRECT3D_H
#define INTERNAL_DIRECT3D_H

typedef struct GraphicsContext_Direct3D GraphicsContext_Direct3D;

#endif // INTERNAL_DIRECT3D_H

//~ Windows Stuff
#if defined(INTERNAL_COMPLETE_GRAPHICS_CONTEXT) && !defined(INTERNAL_DIRECT3D_H_COMPLETE)
#define INTERNAL_DIRECT3D_H_COMPLETE

#ifndef WIN32_LEAN_AND_MEAN
#   define WIN32_LEAN_AND_MEAN
#endif

#ifndef COBJMACROS
#   define COBJMACROS
#endif

#pragma push_macro("far")
#pragma push_macro("near")

//-
#include <d3d11.h>

typedef HRESULT WINAPI ProcD3DCompile(LPCVOID pSrcData, SIZE_T SrcDataSize, LPCSTR pSourceName,
									  const D3D_SHADER_MACRO* pDefines, ID3DInclude* pInclude,
									  LPCSTR pEntrypoint, LPCSTR pTarget, UINT Flags1,
									  UINT Flags2, ID3DBlob** ppCode, ID3DBlob** ppErrorMsgs);

struct GraphicsContext_Direct3D
{
	ID3D11Device* device;
	ID3D11DeviceContext* context;
	IDXGISwapChain* swapchain;
	ID3D11RenderTargetView* target;
	
	// Optional
	ProcD3DCompile* compile_shader;
}
typedef GraphicsContext_Direct3D;
//-

#pragma pop_macro("far")
#pragma pop_macro("near")

#endif // INTERNAL_COMPLETE_D3D_CONTEXT
