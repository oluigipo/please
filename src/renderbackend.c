static const OS_WindowGraphicsContext* g_graphics_context;

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
RenderBackend_ExecuteDrawCommands(Arena* scratch_arena, RenderBackend_DrawCommand* commands)
{
	Trace();
	
	if (!g_graphics_context)
		return;
	
	switch (g_graphics_context->api)
	{
#ifdef CONFIG_ENABLE_OPENGL
		case OS_WindowGraphicsApi_OpenGL: RenderBackend_DrawOpenGL_(scratch_arena, commands); break;
#endif
#ifdef CONFIG_ENABLE_D3D11
		case OS_WindowGraphicsApi_Direct3D11: RenderBackend_DrawD3d11_(scratch_arena, commands); break;
#endif
		default: break;
	}
}
