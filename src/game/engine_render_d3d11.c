#define D3D (*global_engine.graphics_context->d3d)

#ifdef DEBUG
#   define D3DCall(...) do{HRESULT r=(__VA_ARGS__);if(FAILED(r)){Platform_DebugMessageBox("D3DCall Failed\nFile: " __FILE__ "\nLine: %i\nError code: %lx",__LINE__,r);exit(1);}}while(0)
#else
#   define D3DCall(...) (__VA_ARGS__)
#endif


internal bool
D3D11_Init(const Engine_RendererApi** out_api)
{
	Trace();
	
	return false;
}

internal void
D3D11_Deinit(void)
{
	Trace();
}

#undef D3D
#undef D3DCall
