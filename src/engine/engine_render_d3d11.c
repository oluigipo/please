#define D3D (*global_engine.graphics_context->d3d)

#ifdef DEBUG
#   define D3DCall(...) do { HRESULT r=(__VA_ARGS__); Assert(!FAILED(r)); } while (0)
#else
#   define D3DCall(...) (__VA_ARGS__)
#endif

static bool
D3D11_Init(const Engine_RenderApi** out_api)
{
	Trace();
	
	return false;
}

static void
D3D11_Deinit(void)
{
	Trace();
}

#undef D3D
#undef D3DCall
