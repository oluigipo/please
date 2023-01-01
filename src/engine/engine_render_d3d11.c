#define D3d11 (*global_engine.graphics_context->d3d)

#ifdef DEBUG
#   define D3d11_Call(...) do { HRESULT r=(__VA_ARGS__); Assert(!FAILED(r)); } while (0)
#else
#   define D3d11_Call(...) (__VA_ARGS__)
#endif

#include "internal_d3d11_shader_default_vs.inc"
#include "internal_d3d11_shader_default_ps.inc"

static Render_Shader global_d3d11_default_shader;

//~ NOTE(ljre): API
static bool
D3d11_MakeTexture2D(const Render_Texture2DDesc* desc, Render_Texture2D* out_texture)
{
	Trace();
	
	const void* initial_data;
	int32 width, height;
	uint32 channels;
	uint32 pitch;
	bool should_free_initial_data = false;
	
	if (desc->encoded_image)
	{
		Assert(desc->encoded_image_size <= INT32_MAX);
		
		int32 ch;
		void* data = stbi_load_from_memory(desc->encoded_image, (int32)desc->encoded_image_size, &width, &height, &ch, 4);
		
		channels = 4;
		pitch = width * sizeof(uint32);
		initial_data = data;
		should_free_initial_data = true;
	}
	else
	{
		initial_data = desc->pixels;
		width = desc->width;
		height = desc->height;
		channels = desc->channels;
		
		if (desc->float_components)
			pitch = width * channels * sizeof(float32);
		else
			pitch = width * channels;
	}
	
	Assert(initial_data && width && height && channels && channels <= 4);
	
	static const DXGI_FORMAT formats_float[] = {
		DXGI_FORMAT_R32_FLOAT,
		DXGI_FORMAT_R32G32_FLOAT,
		DXGI_FORMAT_R32G32B32_FLOAT,
		DXGI_FORMAT_R32G32B32A32_FLOAT,
	};
	
	static const DXGI_FORMAT formats_int[] = {
		DXGI_FORMAT_R8_UNORM,
		DXGI_FORMAT_R8G8_UNORM,
		0,
		DXGI_FORMAT_R8G8B8A8_UNORM,
	};
	
	const D3D11_TEXTURE2D_DESC tex_desc = {
		.Width = width,
		.Height = height,
		.MipLevels = (desc->generate_mipmaps) ? 0 : 1,
		.ArraySize = 1,
		.Format = (desc->float_components) ? formats_float[channels-1] : formats_int[channels-1],
		.SampleDesc = {
			.Count = 1,
			.Quality = 0,
		},
		.Usage = (initial_data) ? D3D11_USAGE_IMMUTABLE : D3D11_USAGE_DYNAMIC,
		.BindFlags = D3D11_BIND_SHADER_RESOURCE,
		.CPUAccessFlags = (initial_data) ? D3D11_CPU_ACCESS_WRITE : 0,
		.MiscFlags = 0,
	};
	
	const D3D11_SUBRESOURCE_DATA tex_initial = {
		.pSysMem = initial_data,
		.SysMemPitch = (uint32)pitch,
	};
	
	ID3D11Texture2D* handle;
	bool result = ID3D11Device_CreateTexture2D(D3d11.device, &tex_desc, (initial_data) ? &tex_initial : NULL, &handle);
	if (result)
	{
		*out_texture = (Render_Texture2D) {
			.width = width,
			.height = height,
			
			.d3d11.handle = handle,
		};
	}
	
	if (should_free_initial_data)
		stbi_image_free((void*)initial_data);
	
	return result;
}

static bool
D3d11_MakeShader(const Render_ShaderDesc* desc, Render_Shader* out_shader)
{
	const void* ps_blob_data = desc->d3d11.ps_blob_data;
	uintsize ps_blob_size = desc->d3d11.ps_blob_size;
	const void* vs_blob_data = desc->d3d11.vs_blob_data;
	uintsize vs_blob_size = desc->d3d11.vs_blob_size;
	
	Assert(ps_blob_data && ps_blob_size && vs_blob_data && vs_blob_size);
	
	ID3D11VertexShader* vs_handle;
	ID3D11PixelShader* ps_handle;
	
	if (ID3D11Device_CreateVertexShader(D3d11.device, vs_blob_data, vs_blob_size, NULL, &vs_handle) &&
		ID3D11Device_CreatePixelShader(D3d11.device, ps_blob_data, ps_blob_size, NULL, &ps_handle))
	{
		*out_shader = (Render_Shader) {
			.d3d11 = {
				.vs_handle = vs_handle,
				.ps_handle = ps_handle,
			},
		};
		
		return true;
	}
	
	return false;
}

static void
D3d11_ClearColor(const vec4 color)
{
	ID3D11DeviceContext_ClearRenderTargetView(D3d11.context, D3d11.target, color);
}

static void
D3d11_Draw2D(const Render_Data2D* data)
{
	
}

//~ NOTE(ljre): Internal API
static bool
D3d11_Init(const Engine_RenderApi** out_api)
{
	Trace();
	static const Engine_RenderApi api = {
		.make_texture_2d = D3d11_MakeTexture2D,
		.make_shader = D3d11_MakeShader,
		.clear_color = D3d11_ClearColor,
		.draw_2d = D3d11_Draw2D,
	};
	
	Render_ShaderDesc default_shader_desc = {
		.d3d11 = {
			.ps_blob_data = g_Shader_DefaultPixel,
			.ps_blob_size = sizeof(g_Shader_DefaultPixel),
			.vs_blob_data = g_Shader_DefaultVertex,
			.vs_blob_size = sizeof(g_Shader_DefaultVertex),
		},
	};
	
	if (!D3d11_MakeShader(&default_shader_desc, &global_d3d11_default_shader))
		return false;
	
	*out_api = &api;
	return true;
}

static void
D3d11_Deinit(void)
{
	Trace();
}

#undef D3d11
#undef D3d11_Call
