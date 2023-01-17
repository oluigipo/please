#define D3d11 (*global_engine.graphics_context->d3d11)

#ifdef CONFIG_DEBUG
#   define D3d11_Call(...) do { HRESULT r=(__VA_ARGS__); Assert(!FAILED(r)); } while (0)
#else
#   define D3d11_Call(...) (__VA_ARGS__)
#endif

#include "engine_internal_d3d11_shader_default_vs.inc"
#include "engine_internal_d3d11_shader_default_ps.inc"

static Render_Shader global_d3d11_default_shader;
static Render_Texture2D global_d3d11_default_texture;
static ID3D11InputLayout* global_d3d11_default_input_layout;
static ID3D11SamplerState* global_d3d11_default_sampler_state;
static ID3D11Buffer* global_d3d11_default_vertex_buffer;
static ID3D11RasterizerState* global_d3d11_rasterizer_state;

//~ GUIDs
#define IID_ID3D11Texture2D _def_IID_ID3D11Texture2D
static const IID IID_ID3D11Texture2D =
{0x6f15aaf2,0xd208,0x4e89,{0x9a,0xb4,0x48,0x95,0x35,0xd3,0x4f,0x9c}};

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
		.CPUAccessFlags = (initial_data) ? 0 : D3D11_CPU_ACCESS_WRITE,
		.MiscFlags = 0,
	};
	
	const D3D11_SUBRESOURCE_DATA* tex_initial = (!initial_data) ? NULL : &(D3D11_SUBRESOURCE_DATA) {
		.pSysMem = initial_data,
		.SysMemPitch = (uint32)pitch,
	};
	
	const D3D11_SHADER_RESOURCE_VIEW_DESC view_desc = {
		.Format = tex_desc.Format,
		.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D,
		.Texture2D = {
			.MostDetailedMip = 0,
			.MipLevels = 1,
		},
	};
	
	ID3D11Texture2D* texture_handle;
	ID3D11Resource* as_resource;
	ID3D11ShaderResourceView* view_handle;
	bool ok = true;
	
	if (ok)
		ok = S_OK == ID3D11Device_CreateTexture2D(D3d11.device, &tex_desc, tex_initial, &texture_handle);
	if (ok)
		ok = S_OK == ID3D11Texture2D_QueryInterface(texture_handle, &IID_ID3D11Texture2D, (void**)&as_resource);
	if (ok)
		ok = S_OK == ID3D11Device_CreateShaderResourceView(D3d11.device, as_resource, &view_desc, &view_handle);
	if (ok)
	{
		*out_texture = (Render_Texture2D) {
			.width = width,
			.height = height,
			
			.d3d11 = {
				.texture_handle = texture_handle,
				.view_handle = view_handle,
			}
		};
	}
	
	if (should_free_initial_data)
		stbi_image_free((void*)initial_data);
	
	return ok;
}

static bool
D3d11_MakeShader(const Render_ShaderDesc* desc, Render_Shader* out_shader)
{
	Trace();
	
	const void* ps_blob_data = desc->d3d11.ps_blob_data;
	uintsize ps_blob_size = desc->d3d11.ps_blob_size;
	const void* vs_blob_data = desc->d3d11.vs_blob_data;
	uintsize vs_blob_size = desc->d3d11.vs_blob_size;
	
	Assert(ps_blob_data && ps_blob_size && vs_blob_data && vs_blob_size);
	
	ID3D11VertexShader* vs_handle;
	ID3D11PixelShader* ps_handle;
	
	if (!ID3D11Device_CreateVertexShader(D3d11.device, vs_blob_data, vs_blob_size, NULL, &vs_handle) &&
		!ID3D11Device_CreatePixelShader(D3d11.device, ps_blob_data, ps_blob_size, NULL, &ps_handle))
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
	Trace();
	
	ID3D11DeviceContext_ClearRenderTargetView(D3d11.context, D3d11.target, color);
}

static void
D3d11_Draw2D(const Render_Data2D* data)
{
	Trace();
	
	const Render_Texture2D* texture = data->texture;
	if (!texture)
		texture = &global_d3d11_default_texture;
	
	const Render_Shader* shader = data->shader;
	if (!shader)
		shader = &global_d3d11_default_shader;
	
	int32 width, height;
	if (data->framebuffer)
	{
		// TODO
		width = 0;
		height = 0;
	}
	else
	{
		width = global_engine.window_state->width;
		height = global_engine.window_state->height;
	}
	
	mat4 view = GLM_MAT4_IDENTITY_INIT;
	Render_Camera2D camera = data->camera;
	
	if (Mem_Compare(&camera, &(Render_Camera2D) { 0 }, sizeof(camera)) == 0)
	{
		camera = (Render_Camera2D) {
			.pos = { 0.0f, 0.0f },
			.size = { (float32)width, (float32)height },
			.zoom = 1.0f,
			.angle = 0.0f,
		};
	}
	
	Engine_CalcViewMatrix2D(&camera, view);
	
	D3D11_BUFFER_DESC buffer_desc = {
		.Usage = D3D11_USAGE_IMMUTABLE,
		.ByteWidth = sizeof(*data->instances) * (UINT)data->instance_count,
		.BindFlags = D3D11_BIND_VERTEX_BUFFER,
	};
	
	D3D11_SUBRESOURCE_DATA buffer_data_desc = {
		.pSysMem = data->instances,
	};
	
	ID3D11Buffer* batch_buffer;
	D3d11_Call(ID3D11Device_CreateBuffer(D3d11.device, &buffer_desc, &buffer_data_desc, &batch_buffer));
	
	D3D11_BUFFER_DESC cbuffer_desc = {
		.Usage = D3D11_USAGE_DYNAMIC,
		.ByteWidth = sizeof(mat4),
		.BindFlags = D3D11_BIND_CONSTANT_BUFFER,
		.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE,
	};
	
	D3D11_SUBRESOURCE_DATA cbuffer_data_desc = {
		.pSysMem = view,
	};
	
	ID3D11Buffer* cbuffer;
	D3d11_Call(ID3D11Device_CreateBuffer(D3d11.device, &cbuffer_desc, &cbuffer_data_desc, &cbuffer));
	
	ID3D11Buffer* buffers[] = {
		global_d3d11_default_vertex_buffer,
		batch_buffer,
	};
	
	uint32 strides[] = {
		sizeof(float32[2]),
		sizeof(Render_Data2DInstance),
	};
	
	uint32 offsets[] = {
		0,
		0
	};
	
	D3D11_VIEWPORT viewport = {
		.TopLeftX = 0,
		.TopLeftY = 0,
		.Width = width,
		.Height = height,
	};
	
	ID3D11ShaderResourceView* resources[] = {
		texture->d3d11.view_handle,
	};
	
	ID3D11DeviceContext_IASetPrimitiveTopology(D3d11.context, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	ID3D11DeviceContext_IASetInputLayout(D3d11.context, global_d3d11_default_input_layout);
	ID3D11DeviceContext_IASetVertexBuffers(D3d11.context, 0, 2, buffers, strides, offsets);
	ID3D11DeviceContext_VSSetShader(D3d11.context, global_d3d11_default_shader.d3d11.vs_handle, NULL, 0);
	ID3D11DeviceContext_VSSetConstantBuffers(D3d11.context, 0, 1, &cbuffer);
	ID3D11DeviceContext_RSSetViewports(D3d11.context, 1, &viewport);
	ID3D11DeviceContext_RSSetState(D3d11.context, global_d3d11_rasterizer_state);
	ID3D11DeviceContext_PSSetShader(D3d11.context, global_d3d11_default_shader.d3d11.ps_handle, NULL, 0);
	ID3D11DeviceContext_PSSetSamplers(D3d11.context, 0, 1, &global_d3d11_default_sampler_state);
	ID3D11DeviceContext_PSSetShaderResources(D3d11.context, 0, 1, resources);
	ID3D11DeviceContext_OMSetRenderTargets(D3d11.context, 1, &D3d11.target, NULL);
	
	ID3D11DeviceContext_DrawInstanced(D3d11.context, 6, data->instance_count, 0, 0);
	ID3D11DeviceContext_ClearState(D3d11.context);
	
	ID3D11Buffer_Release(batch_buffer);
	ID3D11Buffer_Release(cbuffer);
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
	
	*out_api = &api;
	
	float32 buffer_data[] = {
		0.0f, 0.0f,
		1.0f, 0.0f,
		0.0f, 1.0f,
		0.0f, 1.0f,
		1.0f, 0.0f,
		1.0f, 1.0f,
	};
	
	D3D11_BUFFER_DESC buffer_desc = {
		.Usage = D3D11_USAGE_IMMUTABLE,
		.ByteWidth = sizeof(buffer_data),
		.BindFlags = D3D11_BIND_VERTEX_BUFFER,
	};
	
	D3D11_SUBRESOURCE_DATA buffer_data_desc = {
		.pSysMem = buffer_data,
	};
	
	D3d11_Call(ID3D11Device_CreateBuffer(D3d11.device, &buffer_desc, &buffer_data_desc, &global_d3d11_default_vertex_buffer));
	
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
	
	D3D11_INPUT_ELEMENT_DESC layout[] = {
		{ "V_POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "I_TRANSFORM", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, offsetof(Render_Data2DInstance, transform[0]), D3D11_INPUT_PER_INSTANCE_DATA, 1 },
		{ "I_TRANSFORM", 1, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, offsetof(Render_Data2DInstance, transform[1]), D3D11_INPUT_PER_INSTANCE_DATA, 1 },
		{ "I_TRANSFORM", 2, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, offsetof(Render_Data2DInstance, transform[2]), D3D11_INPUT_PER_INSTANCE_DATA, 1 },
		{ "I_TRANSFORM", 3, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, offsetof(Render_Data2DInstance, transform[3]), D3D11_INPUT_PER_INSTANCE_DATA, 1 },
		{ "I_TEXCOORDS", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, offsetof(Render_Data2DInstance, texcoords), D3D11_INPUT_PER_INSTANCE_DATA, 1 },
		{ "I_COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, offsetof(Render_Data2DInstance, color), D3D11_INPUT_PER_INSTANCE_DATA, 1 },
	};
	
	D3d11_Call(ID3D11Device_CreateInputLayout(D3d11.device, layout, ArrayLength(layout), g_Shader_DefaultVertex, sizeof(g_Shader_DefaultVertex), &global_d3d11_default_input_layout));
	
	D3D11_SAMPLER_DESC sampler_desc = {
		.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR,
		.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP,
		.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP,
		.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP,
	};
	
	D3d11_Call(ID3D11Device_CreateSamplerState(D3d11.device, &sampler_desc, &global_d3d11_default_sampler_state));
	
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
	
	D3d11_Call(ID3D11Device_CreateRasterizerState(D3d11.device, &rasterizer_desc, &global_d3d11_rasterizer_state));
	
	Render_Texture2DDesc default_texture_desc = {
		.pixels = (uint32[]) {
			0xFFFFFFFF, 0xFFFFFFFF,
			0xFFFFFFFF, 0xFFFFFFFF,
		},
		.width = 2,
		.height = 2,
		.channels = 4,
	};
	
	if (!D3d11_MakeTexture2D(&default_texture_desc, &global_d3d11_default_texture))
		Assert(false);
	
	return true;
}

static void
D3d11_Deinit(void)
{
	Trace();
}

#undef D3d11
#undef D3d11_Call
