#ifndef API_OS_D3D9C_H
#define API_OS_D3D9C_H

#ifndef WIN32_LEAN_AND_MEAN
#   define WIN32_LEAN_AND_MEAN
#endif

#ifndef COBJMACROS
#   define COBJMACROS
#endif

#pragma push_macro("far")
#pragma push_macro("near")

#define far
#define near

#ifdef CONFIG_DEBUG
#   define D3D_DEBUG_INFO
#endif

//#include <d3d9.h>
#include <d3d9helper.h>

struct OS_D3d9cApi
{
	IDirect3D9* d3d9;
	IDirect3DDevice9* device;
};

#pragma pop_macro("far")
#pragma pop_macro("near")

#endif //API_OS_D3D9C_H
