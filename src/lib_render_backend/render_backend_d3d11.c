#define D3d11 (*g_graphics_context->d3d11)

#ifdef CONFIG_DEBUG
#   define D3d11Call(...) do { HRESULT r=(__VA_ARGS__); Assert(!FAILED(r)); } while (0)
#else
#   define D3d11Call(...) (__VA_ARGS__)
#endif

// GUIDs
#ifndef IID_ID3D11Texture2D
#define IID_ID3D11Texture2D _def_IID_ID3D11Texture2D
static const IID IID_ID3D11Texture2D =
{0x6f15aaf2,0xd208,0x4e89,{0x9a,0xb4,0x48,0x95,0x35,0xd3,0x4f,0x9c}};
#endif //IID_ID3D11Texture2D

static ID3D11RasterizerState* g_d3d11_rasterizer_state;

static void
RenderBackend_InitD3d11_(Arena* scratch_arena)
{
	D3D11_RASTERIZER_DESC rasterizer_desc = {
		.FillMode = D3D11_FILL_SOLID,
		.CullMode = D3D11_CULL_NONE,
		.FrontCounterClockwise = true,
		.DepthBias = 0,
		.SlopeScaledDepthBias = 0.0f,
		.DepthBiasClamp = 0.0f,
		.DepthClipEnable = true,
		.ScissorEnable = false,
		.MultisampleEnable = false,
		.AntialiasedLineEnable = false,
	};
	
	D3d11Call(ID3D11Device_CreateRasterizerState(D3d11.device, &rasterizer_desc, &g_d3d11_rasterizer_state));
}

static void
RenderBackend_DeinitD3d11_(Arena* scratch_arena)
{
}

static void
RenderBackend_ResourceD3d11_(Arena* scratch_arena, RenderBackend_ResourceCommand* commands)
{
	
}

static void
RenderBackend_DrawD3d11_(Arena* scratch_arena, RenderBackend_DrawCommand* commands)
{
	
}

#undef D3d11
#undef D3d11Call
