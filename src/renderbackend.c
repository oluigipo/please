static const OS_WindowGraphicsContext* g_graphics_context;

#define RenderBackend_PoolOf(T, size_) struct { uint32 size; uint32 first_free; T data[size_]; }

#define RenderBackend_PoolAlloc(pool, out_index) RenderBackend_PoolAllocImpl_(pool, ArrayLength((pool)->data), sizeof((pool)->data[0]), out_index)
#define RenderBackend_PoolFetch(pool, index) RenderBackend_PoolFetchImpl_(pool, ArrayLength((pool)->data), sizeof((pool)->data[0]), index)
#define RenderBackend_PoolFree(pool, index) RenderBackend_PoolFreeImpl_(pool, ArrayLength((pool)->data), sizeof((pool)->data[0]), index)

RenderBackend_PoolOf(uint8,) typedef RenderBackend_Pool_;

static void*
RenderBackend_PoolAllocImpl_(void* pool_ptr, uint32 max_size, uintsize obj_size, uint32* out_index)
{
	RenderBackend_Pool_* pool = pool_ptr;
	uint32 index = 0;
	
	if (pool->first_free != 0)
	{
		index = pool->first_free;
		
		uint32* next_free_ptr = (uint32*)(pool->data + (index-1) * obj_size);
		pool->first_free = *next_free_ptr;
	}
	else if (pool->size < max_size)
	{
		index = ++pool->size;
	}
	else
		SafeAssert(false);
	
	void* result = pool->data + (index-1) * obj_size;
	Mem_Zero(result, obj_size);
	
	*out_index = index;
	return result;
}

static void*
RenderBackend_PoolFetchImpl_(void* pool_ptr, uint32 max_size, uintsize obj_size, uint32 index)
{
	SafeAssert(index != 0);
	
	RenderBackend_Pool_* pool = pool_ptr;
	void* result = pool->data + (index-1) * obj_size;
	
	return result;
}

static void
RenderBackend_PoolFreeImpl_(void* pool_ptr, uint32 max_size, uintsize obj_size, uint32 index)
{
	SafeAssert(index != 0);
	
	RenderBackend_Pool_* pool = pool_ptr;
	uint32* next_free_ptr = (uint32*)(pool->data + (index-1) * obj_size);
	
	*next_free_ptr = pool->first_free;
	pool->first_free = index;
}

#ifdef CONFIG_ENABLE_OPENGL
#   include "renderbackend_opengl.c"
#endif
#ifdef CONFIG_ENABLE_D3D11
#   include "renderbackend_d3d11.c"
#endif

API void
RenderBackend_Init(Arena* scratch_arena, const OS_WindowGraphicsContext* graphics_context)
{
	Trace();
	
	Assert(graphics_context);
	g_graphics_context = graphics_context;
	
	switch (g_graphics_context->api)
	{
#ifdef CONFIG_ENABLE_OPENGL
		case OS_WindowGraphicsApi_OpenGL: RenderBackend_InitOpenGL_(scratch_arena); break;
#endif
#ifdef CONFIG_ENABLE_D3D11
		case OS_WindowGraphicsApi_Direct3D11: RenderBackend_InitD3d11_(scratch_arena); break;
#endif
		default: break;
	}
}

API void
RenderBackend_Deinit(Arena* scratch_arena)
{
	Trace();
	
	if (!g_graphics_context)
		return;
	
	switch (g_graphics_context->api)
	{
#ifdef CONFIG_ENABLE_OPENGL
		case OS_WindowGraphicsApi_OpenGL: RenderBackend_DeinitOpenGL_(scratch_arena); break;
#endif
#ifdef CONFIG_ENABLE_D3D11
		case OS_WindowGraphicsApi_Direct3D11: RenderBackend_DeinitD3d11_(scratch_arena); break;
#endif
		default: break;
	}
	
	g_graphics_context = NULL;
}

API bool
RenderBackend_Present(Arena* scratch_arena, int32 vsync_count)
{
	Trace();
	
	if (!g_graphics_context)
		return false;
	
	return g_graphics_context->present_and_vsync(vsync_count);
}

API void
RenderBackend_ExecuteResourceCommands(Arena* scratch_arena, RenderBackend_ResourceCommand* commands)
{
	Trace();
	
	if (!g_graphics_context)
		return;
	
	switch (g_graphics_context->api)
	{
#ifdef CONFIG_ENABLE_OPENGL
		case OS_WindowGraphicsApi_OpenGL: RenderBackend_ResourceOpenGL_(scratch_arena, commands); break;
#endif
#ifdef CONFIG_ENABLE_D3D11
		case OS_WindowGraphicsApi_Direct3D11: RenderBackend_ResourceD3d11_(scratch_arena, commands); break;
#endif
		default: break;
	}
}

API void
RenderBackend_ExecuteDrawCommands(Arena* scratch_arena, RenderBackend_DrawCommand* commands, int32 default_width, int32 default_height)
{
	Trace();
	
	if (!g_graphics_context)
		return;
	
	switch (g_graphics_context->api)
	{
#ifdef CONFIG_ENABLE_OPENGL
		case OS_WindowGraphicsApi_OpenGL: RenderBackend_DrawOpenGL_(scratch_arena, commands, default_width, default_height); break;
#endif
#ifdef CONFIG_ENABLE_D3D11
		case OS_WindowGraphicsApi_Direct3D11: RenderBackend_DrawD3d11_(scratch_arena, commands, default_width, default_height); break;
#endif
		default: break;
	}
}
