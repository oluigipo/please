static const OS_WindowGraphicsContext* g_graphics_context;

#define RB_PoolOf_(T, size_) struct { uint32 size; uint32 first_free; T data[size_]; }

#define RB_PoolAlloc_(pool, out_index) RB_PoolAllocImpl_(pool, ArrayLength((pool)->data), sizeof((pool)->data[0]), out_index)
#define RB_PoolFetch_(pool, index) RB_PoolFetchImpl_(pool, ArrayLength((pool)->data), sizeof((pool)->data[0]), index)
#define RB_PoolFree_(pool, index) RB_PoolFreeImpl_(pool, ArrayLength((pool)->data), sizeof((pool)->data[0]), index)

RB_PoolOf_(uint8,) typedef RB_Pool_;

static void*
RB_PoolAllocImpl_(void* pool_ptr, uint32 max_size, uintsize obj_size, uint32* out_index)
{
	RB_Pool_* pool = pool_ptr;
	uint32 index = 0;
	
	if (pool->first_free != 0)
	{
		index = pool->first_free;
		
		uint32* next_free_ptr = (uint32*)(pool->data + (index-1) * obj_size);
		pool->first_free = *next_free_ptr;
	}
	else if (pool->size < max_size)
		index = ++pool->size;
	else
		SafeAssert(false);
	
	void* result = pool->data + (index-1) * obj_size;
	Mem_Zero(result, obj_size);
	
	*out_index = index;
	return result;
}

static void*
RB_PoolFetchImpl_(void* pool_ptr, uint32 max_size, uintsize obj_size, uint32 index)
{
	SafeAssert(index != 0);
	SafeAssert(index <= max_size);
	
	RB_Pool_* pool = pool_ptr;
	void* result = pool->data + (index-1) * obj_size;
	
	return result;
}

static void
RB_PoolFreeImpl_(void* pool_ptr, uint32 max_size, uintsize obj_size, uint32 index)
{
	SafeAssert(index != 0);
	SafeAssert(index <= max_size);
	
	RB_Pool_* pool = pool_ptr;
	uint32* next_free_ptr = (uint32*)(pool->data + (index-1) * obj_size);
	
	*next_free_ptr = pool->first_free;
	pool->first_free = index;
}

static const String RB_resource_cmd_names[] = {
	[RB_ResourceCommandKind_MakeTexture2D] = StrInit("RB_ResourceCommandKind_MakeTexture2D"),
	[RB_ResourceCommandKind_MakeVertexBuffer] = StrInit("RB_ResourceCommandKind_MakeVertexBuffer"),
	[RB_ResourceCommandKind_MakeIndexBuffer] = StrInit("RB_ResourceCommandKind_MakeIndexBuffer"),
	[RB_ResourceCommandKind_MakeUniformBuffer] = StrInit("RB_ResourceCommandKind_MakeUniformBuffer"),
	[RB_ResourceCommandKind_MakeShader] = StrInit("RB_ResourceCommandKind_MakeShader"),
	[RB_ResourceCommandKind_MakeRenderTarget] = StrInit("RB_ResourceCommandKind_MakeRenderTarget"),
	[RB_ResourceCommandKind_MakeBlendState] = StrInit("RB_ResourceCommandKind_MakeBlendState"),
	[RB_ResourceCommandKind_MakeRasterizerState] = StrInit("RB_ResourceCommandKind_MakeRasterizerState"),
	[RB_ResourceCommandKind_UpdateVertexBuffer] = StrInit("RB_ResourceCommandKind_UpdateVertexBuffer"),
	[RB_ResourceCommandKind_UpdateIndexBuffer] = StrInit("RB_ResourceCommandKind_UpdateIndexBuffer"),
	[RB_ResourceCommandKind_UpdateUniformBuffer] = StrInit("RB_ResourceCommandKind_UpdateUniformBuffer"),
	[RB_ResourceCommandKind_UpdateTexture2D] = StrInit("RB_ResourceCommandKind_UpdateTexture2D"),
	[RB_ResourceCommandKind_FreeTexture2D] = StrInit("RB_ResourceCommandKind_FreeTexture2D"),
	[RB_ResourceCommandKind_FreeVertexBuffer] = StrInit("RB_ResourceCommandKind_FreeVertexBuffer"),
	[RB_ResourceCommandKind_FreeIndexBuffer] = StrInit("RB_ResourceCommandKind_FreeIndexBuffer"),
	[RB_ResourceCommandKind_FreeUniformBuffer] = StrInit("RB_ResourceCommandKind_FreeUniformBuffer"),
	[RB_ResourceCommandKind_FreeShader] = StrInit("RB_ResourceCommandKind_FreeShader"),
	[RB_ResourceCommandKind_FreeRenderTarget] = StrInit("RB_ResourceCommandKind_FreeRenderTarget"),
	[RB_ResourceCommandKind_FreeBlendState] = StrInit("RB_ResourceCommandKind_FreeBlendState"),
	[RB_ResourceCommandKind_FreeRasterizerState] = StrInit("RB_ResourceCommandKind_FreeRasterizerState"),
};

static const String RB_draw_cmd_names[] = {
	[RB_DrawCommandKind_Clear] = StrInit("RB_DrawCommandKind_Clear"),
	[RB_DrawCommandKind_ApplyBlendState] = StrInit("RB_DrawCommandKind_ApplyBlendState"),
	[RB_DrawCommandKind_ApplyRasterizerState] = StrInit("RB_DrawCommandKind_ApplyRasterizerState"),
	[RB_DrawCommandKind_ApplyRenderTarget] = StrInit("RB_DrawCommandKind_ApplyRenderTarget"),
	[RB_DrawCommandKind_DrawCall] = StrInit("RB_DrawCommandKind_DrawCall"),
};

#ifdef CONFIG_ENABLE_OPENGL
#   include "api_os_opengl.h"
#   include "renderbackend_opengl.c"
#endif
#ifdef CONFIG_ENABLE_D3D11
#   include "api_os_d3d11.h"
#   include "renderbackend_d3d11.c"
#endif

API void
RB_Init(Arena* scratch_arena, const OS_WindowGraphicsContext* graphics_context)
{
	Trace();
	
	Assert(graphics_context);
	g_graphics_context = graphics_context;
	
	switch (g_graphics_context->api)
	{
#ifdef CONFIG_ENABLE_OPENGL
		case OS_WindowGraphicsApi_OpenGL: RB_InitOpenGL_(scratch_arena); break;
#endif
#ifdef CONFIG_ENABLE_D3D11
		case OS_WindowGraphicsApi_Direct3D11: RB_InitD3d11_(scratch_arena); break;
#endif
		default: break;
	}
}

API void
RB_Deinit(Arena* scratch_arena)
{
	Trace();
	
	if (!g_graphics_context)
		return;
	
	switch (g_graphics_context->api)
	{
#ifdef CONFIG_ENABLE_OPENGL
		case OS_WindowGraphicsApi_OpenGL: RB_DeinitOpenGL_(scratch_arena); break;
#endif
#ifdef CONFIG_ENABLE_D3D11
		case OS_WindowGraphicsApi_Direct3D11: RB_DeinitD3d11_(scratch_arena); break;
#endif
		default: break;
	}
	
	g_graphics_context = NULL;
}

API bool
RB_Present(Arena* scratch_arena, int32 vsync_count)
{
	Trace();
	
	if (!g_graphics_context)
		return false;
	
	return g_graphics_context->present_and_vsync(vsync_count);
}

API void
RB_QueryCapabilities(RB_Capabilities* out_capabilities)
{
	Trace();
	
	if (!g_graphics_context)
		return;
	
	switch (g_graphics_context->api)
	{
#ifdef CONFIG_ENABLE_OPENGL
		case OS_WindowGraphicsApi_OpenGL: RB_CapabilitiesOpenGL_(out_capabilities); break;
#endif
#ifdef CONFIG_ENABLE_D3D11
		case OS_WindowGraphicsApi_Direct3D11: RB_CapabilitiesD3d11_(out_capabilities); break;
#endif
		default: break;
	}
}

API void
RB_ExecuteResourceCommands(Arena* scratch_arena, RB_ResourceCommand* commands)
{
	Trace();
	
	if (!g_graphics_context)
		return;
	
	switch (g_graphics_context->api)
	{
#ifdef CONFIG_ENABLE_OPENGL
		case OS_WindowGraphicsApi_OpenGL: RB_ResourceOpenGL_(scratch_arena, commands); break;
#endif
#ifdef CONFIG_ENABLE_D3D11
		case OS_WindowGraphicsApi_Direct3D11: RB_ResourceD3d11_(scratch_arena, commands); break;
#endif
		default: break;
	}
}

API void
RB_ExecuteDrawCommands(Arena* scratch_arena, RB_DrawCommand* commands, int32 default_width, int32 default_height)
{
	Trace();
	
	if (!g_graphics_context)
		return;
	
	switch (g_graphics_context->api)
	{
#ifdef CONFIG_ENABLE_OPENGL
		case OS_WindowGraphicsApi_OpenGL: RB_DrawOpenGL_(scratch_arena, commands, default_width, default_height); break;
#endif
#ifdef CONFIG_ENABLE_D3D11
		case OS_WindowGraphicsApi_Direct3D11: RB_DrawD3d11_(scratch_arena, commands, default_width, default_height); break;
#endif
		default: break;
	}
}
