#define D3d11 (*g_graphics_context->d3d11)
#define D3d11Call(...) do { \
if (FAILED(__VA_ARGS__)) { \
if (Assert_IsDebuggerPresent_()) \
Debugbreak(); \
SafeAssert_OnFailure(#__VA_ARGS__, __FILE__, __LINE__, __func__); \
} \
} while (0)

//~ GUIDs
#ifndef IID_ID3D11Resource
#define IID_ID3D11Resource _def2_IID_ID3D11Resource
static const IID IID_ID3D11Resource =
{0xdc8e63f3,0xd12b,0x4952,0xb4,0x7b,0x5e,0x45,0x02,0x6a,0x86,0x2d};
#endif //IID_ID3D11Texture2D

//~ Globals
struct RB_D3d11Texture2D_
{
	ID3D11Texture2D* texture;
	ID3D11ShaderResourceView* resource_view;
	ID3D11SamplerState* sampler_state;
}
typedef RB_D3d11Texture2D_;

struct RB_D3d11Shader_
{
	ID3D11VertexShader* vertex_shader;
	ID3D11PixelShader* pixel_shader;
	ID3D11InputLayout* input_layout;
	
	uint32 strides[RB_Limits_InputsPerShader];
	uint32 offsets[RB_Limits_InputsPerShader];
}
typedef RB_D3d11Shader_;

struct RB_D3d11Buffer_
{
	ID3D11Buffer* buffer;
}
typedef RB_D3d11Buffer_;

struct RB_D3d11Blend_
{
	ID3D11BlendState* blend_state;
}
typedef RB_D3d11Blend_;

struct RB_D3d11Rasterizer_
{
	ID3D11RasterizerState* rasterizer_state;
	ID3D11DepthStencilState* depth_stencil_state;
}
typedef RB_D3d11Rasterizer_;

static RB_PoolOf_(RB_D3d11Texture2D_, 512) g_d3d11_texpool;
static RB_PoolOf_(RB_D3d11Shader_, 64) g_d3d11_shaderpool;
static RB_PoolOf_(RB_D3d11Buffer_, 512) g_d3d11_bufferpool;
static RB_PoolOf_(RB_D3d11Blend_, 16) g_d3d11_blendpool;
static RB_PoolOf_(RB_D3d11Rasterizer_, 16) g_d3d11_rasterizerpool;

static ID3D11RasterizerState* g_d3d11_rasterizer_state;
static ID3D11SamplerState* g_d3d11_linear_sampler_state;
static ID3D11SamplerState* g_d3d11_nearest_sampler_state;
static ID3D11BlendState* g_d3d11_blend_state;
static ID3D11DepthStencilState* g_d3d11_depth_state;

static void
RB_InitD3d11_(Arena* scratch_arena)
{
	D3D11_RASTERIZER_DESC rasterizer_desc = {
		.FillMode = D3D11_FILL_SOLID,
		.CullMode = D3D11_CULL_NONE,
		.FrontCounterClockwise = true,
		.DepthBias = 0,
		.SlopeScaledDepthBias = 0.0f,
		.DepthBiasClamp = 0.0f,
		.DepthClipEnable = false,
		.ScissorEnable = false,
		.MultisampleEnable = false,
		.AntialiasedLineEnable = false,
	};
	
	D3d11Call(ID3D11Device_CreateRasterizerState(D3d11.device, &rasterizer_desc, &g_d3d11_rasterizer_state));
	
	D3D11_SAMPLER_DESC linear_sampler_desc = {
		.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR,
		.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP,
		.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP,
		.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP,
		.MinLOD = -FLT_MAX,
		.MaxLOD = FLT_MAX,
		.ComparisonFunc = D3D11_COMPARISON_NEVER,
	};
	
	D3d11Call(ID3D11Device_CreateSamplerState(D3d11.device, &linear_sampler_desc, &g_d3d11_linear_sampler_state));
	
	D3D11_SAMPLER_DESC nearest_sampler_desc = {
		.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT,
		.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP,
		.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP,
		.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP,
		.MinLOD = -FLT_MAX,
		.MaxLOD = FLT_MAX,
		.ComparisonFunc = D3D11_COMPARISON_NEVER,
	};
	
	D3d11Call(ID3D11Device_CreateSamplerState(D3d11.device, &nearest_sampler_desc, &g_d3d11_nearest_sampler_state));
	
	D3D11_BLEND_DESC blend_desc = {
		.AlphaToCoverageEnable = false,
		.IndependentBlendEnable = false,
		
		.RenderTarget[0] = {
			.BlendEnable = true,
			.SrcBlend = D3D11_BLEND_SRC_ALPHA,
			.DestBlend = D3D11_BLEND_INV_SRC_ALPHA,
			.BlendOp = D3D11_BLEND_OP_ADD,
			.SrcBlendAlpha = D3D11_BLEND_SRC_ALPHA,
			.DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA,
			.BlendOpAlpha = D3D11_BLEND_OP_ADD,
			.RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL,
		},
	};
	
	D3d11Call(ID3D11Device_CreateBlendState(D3d11.device, &blend_desc, &g_d3d11_blend_state));
	
	D3D11_DEPTH_STENCIL_DESC depth_stencil_desc = {
		.DepthEnable = false,
		.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL,
		.DepthFunc = D3D11_COMPARISON_LESS,
	};
	
	D3d11Call(ID3D11Device_CreateDepthStencilState(D3d11.device, &depth_stencil_desc, &g_d3d11_depth_state));
}

static void
RB_DeinitD3d11_(Arena* scratch_arena)
{
}

static void
RB_ResourceD3d11_(Arena* scratch_arena, RB_ResourceCommand* commands)
{
	Trace();
	
	for (RB_ResourceCommand* cmd = commands; cmd; cmd = cmd->next)
	{
		Trace(); TraceName(RB_resource_cmd_names[cmd->kind]);
		
		Assert(cmd->kind);
		Assert(cmd->handle);
		
		RB_Handle handle = *cmd->handle;
		
		switch (cmd->kind)
		{
			case 0: Assert(false); break;
			
			case RB_ResourceCommandKind_MakeTexture2D:
			{
				Assert(!handle.id);
				
				const void* pixels = cmd->texture_2d.pixels;
				int32 width = cmd->texture_2d.width;
				int32 height = cmd->texture_2d.height;
				uint32 channels = cmd->texture_2d.channels;
				bool dynamic = cmd->flag_dynamic;
				uint32 pitch = width * channels;
				
				Assert(width && height && channels);
				
				const DXGI_FORMAT formats[] = {
					DXGI_FORMAT_R8_UNORM,
					DXGI_FORMAT_R8G8_UNORM,
					0,
					DXGI_FORMAT_R8G8B8A8_UNORM,
				};
				
				Assert(channels <= 4 && channels != 3);
				
				const D3D11_TEXTURE2D_DESC tex_desc = {
					.Width = width,
					.Height = height,
					.MipLevels = 1,
					.ArraySize = 1,
					.Format = formats[channels - 1],
					.SampleDesc = {
						.Count = 1,
						.Quality = 0,
					},
					.Usage = (dynamic) ? D3D11_USAGE_DYNAMIC : D3D11_USAGE_IMMUTABLE,
					.BindFlags = D3D11_BIND_SHADER_RESOURCE,
					.CPUAccessFlags = (dynamic) ? D3D11_CPU_ACCESS_WRITE : 0,
					.MiscFlags = 0,
				};
				
				const D3D11_SUBRESOURCE_DATA* tex_initial = (!pixels) ? NULL : &(D3D11_SUBRESOURCE_DATA) {
					.pSysMem = pixels,
					.SysMemPitch = (uint32)pitch,
				};
				
				const D3D11_SHADER_RESOURCE_VIEW_DESC resource_view_desc = {
					.Format = tex_desc.Format,
					.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D,
					.Texture2D = {
						.MostDetailedMip = 0,
						.MipLevels = 1,
					},
				};
				
				ID3D11Texture2D* texture;
				ID3D11Resource* as_resource;
				ID3D11ShaderResourceView* resource_view;
				ID3D11SamplerState* sampler_state = cmd->texture_2d.flag_linear_filtering ? g_d3d11_linear_sampler_state : g_d3d11_nearest_sampler_state;
				
				D3d11Call(ID3D11Device_CreateTexture2D(D3d11.device, &tex_desc, tex_initial, &texture));
				D3d11Call(ID3D11Texture2D_QueryInterface(texture, &IID_ID3D11Resource, (void**)&as_resource));
				D3d11Call(ID3D11Device_CreateShaderResourceView(D3d11.device, as_resource, &resource_view_desc, &resource_view));
				
				RB_D3d11Texture2D_* pool_data = RB_PoolAlloc_(&g_d3d11_texpool, &handle.id);
				pool_data->texture = texture;
				pool_data->resource_view = resource_view;
				pool_data->sampler_state = sampler_state;
			} break;
			
			//case RB_ResourceCommandKind_MakeVertexBuffer:
			//case RB_ResourceCommandKind_MakeIndexBuffer:
			//case RB_ResourceCommandKind_MakeUniformBuffer:
			{
				uint32 bind_flags;
				
				if (0) case RB_ResourceCommandKind_MakeVertexBuffer: bind_flags = D3D11_BIND_VERTEX_BUFFER;
				if (0) case RB_ResourceCommandKind_MakeIndexBuffer: bind_flags = D3D11_BIND_INDEX_BUFFER;
				if (0) case RB_ResourceCommandKind_MakeUniformBuffer: bind_flags = D3D11_BIND_CONSTANT_BUFFER;
				
				Assert(!handle.id);
				SafeAssert(cmd->buffer.size <= UINT32_MAX);
				
				const void* memory = cmd->buffer.memory;
				
				const D3D11_BUFFER_DESC buffer_desc = {
					.Usage = (cmd->flag_dynamic) ? D3D11_USAGE_DYNAMIC : D3D11_USAGE_IMMUTABLE,
					.ByteWidth = (uint32)cmd->buffer.size,
					.BindFlags = bind_flags,
					.CPUAccessFlags = (cmd->flag_dynamic) ? D3D11_CPU_ACCESS_WRITE : 0,
				};
				
				const D3D11_SUBRESOURCE_DATA* buf_initial = (!memory) ? NULL : &(D3D11_SUBRESOURCE_DATA) {
					.pSysMem = memory,
				};
				
				ID3D11Buffer* buffer;
				
				D3d11Call(ID3D11Device_CreateBuffer(D3d11.device, &buffer_desc, buf_initial, &buffer));
				
				RB_D3d11Buffer_* pool_data = RB_PoolAlloc_(&g_d3d11_bufferpool, &handle.id);
				pool_data->buffer = buffer;
			} break;
			
			case RB_ResourceCommandKind_MakeShader:
			{
				Assert(!handle.id);
				
				Buffer vs_blob = cmd->shader.d3d_vs_blob;
				Buffer ps_blob = cmd->shader.d3d_ps_blob;
				
				ID3D11VertexShader* vs;
				ID3D11PixelShader* ps;
				ID3D11InputLayout* input_layout;
				uint32 layout_size = 0;
				D3D11_INPUT_ELEMENT_DESC layout_desc[RB_Limits_InputsPerShader*4] = { 0 };
				
				for (int32 i = 0; i < ArrayLength(cmd->shader.input_layout); ++i)
				{
					if (!cmd->shader.input_layout[i].kind)
						break;
					
					RB_LayoutDesc curr_layout = cmd->shader.input_layout[i];
					
					switch (curr_layout.kind)
					{
						case 0: Assert(false); break;
						
						//case RB_LayoutDescKind_Vec2:
						//case RB_LayoutDescKind_Vec3:
						//case RB_LayoutDescKind_Vec4:
						{
							DXGI_FORMAT format;
							
							if (0) case RB_LayoutDescKind_Float: format = DXGI_FORMAT_R32_FLOAT;
							if (0) case RB_LayoutDescKind_Vec2: format = DXGI_FORMAT_R32G32_FLOAT;
							if (0) case RB_LayoutDescKind_Vec3: format = DXGI_FORMAT_R32G32B32_FLOAT;
							if (0) case RB_LayoutDescKind_Vec4: format = DXGI_FORMAT_R32G32B32A32_FLOAT;
							
							D3D11_INPUT_ELEMENT_DESC element_desc = {
								.SemanticName = "VINPUT",
								.SemanticIndex = layout_size,
								.Format = format,
								.InputSlot = curr_layout.vbuffer_index,
								.AlignedByteOffset = curr_layout.offset,
								.InputSlotClass = (curr_layout.divisor == 0) ? D3D11_INPUT_PER_VERTEX_DATA : D3D11_INPUT_PER_INSTANCE_DATA,
								.InstanceDataStepRate = curr_layout.divisor,
							};
							
							layout_desc[layout_size++] = element_desc;
						} break;
						
						case RB_LayoutDescKind_Mat2:
						{
							D3D11_INPUT_ELEMENT_DESC element_desc = {
								.SemanticName = "VINPUT",
								.SemanticIndex = layout_size,
								.Format = DXGI_FORMAT_R32G32_FLOAT,
								.InputSlot = curr_layout.vbuffer_index,
								.AlignedByteOffset = curr_layout.offset,
								.InputSlotClass = (curr_layout.divisor == 0) ? D3D11_INPUT_PER_VERTEX_DATA : D3D11_INPUT_PER_INSTANCE_DATA,
								.InstanceDataStepRate = curr_layout.divisor,
							};
							
							layout_desc[layout_size++] = element_desc; ++element_desc.SemanticIndex; element_desc.AlignedByteOffset += sizeof(float32[2]);
							layout_desc[layout_size++] = element_desc;
						} break;
						
						case RB_LayoutDescKind_Mat3:
						{
							D3D11_INPUT_ELEMENT_DESC element_desc = {
								.SemanticName = "VINPUT",
								.SemanticIndex = layout_size,
								.Format = DXGI_FORMAT_R32G32B32_FLOAT,
								.InputSlot = curr_layout.vbuffer_index,
								.AlignedByteOffset = curr_layout.offset,
								.InputSlotClass = (curr_layout.divisor == 0) ? D3D11_INPUT_PER_VERTEX_DATA : D3D11_INPUT_PER_INSTANCE_DATA,
								.InstanceDataStepRate = curr_layout.divisor,
							};
							
							layout_desc[layout_size++] = element_desc; ++element_desc.SemanticIndex; element_desc.AlignedByteOffset += sizeof(float32[3]);
							layout_desc[layout_size++] = element_desc; ++element_desc.SemanticIndex; element_desc.AlignedByteOffset += sizeof(float32[3]);
							layout_desc[layout_size++] = element_desc;
						} break;
						
						case RB_LayoutDescKind_Mat4:
						{
							D3D11_INPUT_ELEMENT_DESC element_desc = {
								.SemanticName = "VINPUT",
								.SemanticIndex = layout_size,
								.Format = DXGI_FORMAT_R32G32B32A32_FLOAT,
								.InputSlot = curr_layout.vbuffer_index,
								.AlignedByteOffset = curr_layout.offset,
								.InputSlotClass = (curr_layout.divisor == 0) ? D3D11_INPUT_PER_VERTEX_DATA : D3D11_INPUT_PER_INSTANCE_DATA,
								.InstanceDataStepRate = curr_layout.divisor,
							};
							
							layout_desc[layout_size++] = element_desc; ++element_desc.SemanticIndex; element_desc.AlignedByteOffset += sizeof(float32[4]);
							layout_desc[layout_size++] = element_desc; ++element_desc.SemanticIndex; element_desc.AlignedByteOffset += sizeof(float32[4]);
							layout_desc[layout_size++] = element_desc; ++element_desc.SemanticIndex; element_desc.AlignedByteOffset += sizeof(float32[4]);
							layout_desc[layout_size++] = element_desc;
						} break;
					}
				}
				
				D3d11Call(ID3D11Device_CreateVertexShader(D3d11.device, vs_blob.data, vs_blob.size, NULL, &vs));
				D3d11Call(ID3D11Device_CreatePixelShader(D3d11.device, ps_blob.data, ps_blob.size, NULL, &ps));
				D3d11Call(ID3D11Device_CreateInputLayout(D3d11.device, layout_desc, layout_size, vs_blob.data, vs_blob.size, &input_layout));
				
				RB_D3d11Shader_* pool_data = RB_PoolAlloc_(&g_d3d11_shaderpool, &handle.id);
				pool_data->vertex_shader = vs;
				pool_data->pixel_shader = ps;
				pool_data->input_layout = input_layout;
			} break;
			
			case RB_ResourceCommandKind_MakeRenderTarget:
			{
				SafeAssert(false);
			} break;
			
			case RB_ResourceCommandKind_MakeBlendState:
			{
				static const D3D11_BLEND functable[] = {
					[RB_BlendFunc_Zero]        = D3D11_BLEND_ZERO,
					[RB_BlendFunc_One]         = D3D11_BLEND_ONE,
					[RB_BlendFunc_SrcColor]    = D3D11_BLEND_SRC_COLOR,
					[RB_BlendFunc_InvSrcColor] = D3D11_BLEND_INV_SRC_COLOR,
					[RB_BlendFunc_DstColor]    = D3D11_BLEND_DEST_COLOR,
					[RB_BlendFunc_InvDstColor] = D3D11_BLEND_INV_DEST_COLOR,
					[RB_BlendFunc_SrcAlpha]    = D3D11_BLEND_SRC_ALPHA,
					[RB_BlendFunc_InvSrcAlpha] = D3D11_BLEND_INV_SRC_ALPHA,
					[RB_BlendFunc_DstAlpha]    = D3D11_BLEND_DEST_ALPHA,
					[RB_BlendFunc_InvDstAlpha] = D3D11_BLEND_INV_DEST_ALPHA,
				};
				
				static const D3D11_BLEND_OP optable[] = {
					[RB_BlendOp_Add]      = D3D11_BLEND_OP_ADD,
					[RB_BlendOp_Subtract] = D3D11_BLEND_OP_SUBTRACT,
				};
				
				SafeAssert(cmd->blend.source >= 0 && cmd->blend.source < ArrayLength(functable));
				SafeAssert(cmd->blend.dest >= 0 && cmd->blend.dest < ArrayLength(functable));
				SafeAssert(cmd->blend.source_alpha >= 0 && cmd->blend.source_alpha < ArrayLength(functable));
				SafeAssert(cmd->blend.dest_alpha >= 0 && cmd->blend.dest_alpha < ArrayLength(functable));
				
				SafeAssert(cmd->blend.op >= 0 && cmd->blend.op < ArrayLength(optable));
				SafeAssert(cmd->blend.op_alpha >= 0 && cmd->blend.op_alpha < ArrayLength(optable));
				
				D3D11_BLEND_DESC blend_desc = {
					.AlphaToCoverageEnable = false,
					.IndependentBlendEnable = false,
					
					.RenderTarget[0] = {
						.BlendEnable = cmd->blend.enable,
						.SrcBlend = cmd->blend.source ? functable[cmd->blend.source] : D3D11_BLEND_ONE,
						.DestBlend = cmd->blend.dest ? functable[cmd->blend.dest] : D3D11_BLEND_ZERO,
						.BlendOp = cmd->blend.op ? optable[cmd->blend.op] : D3D11_BLEND_OP_ADD,
						.SrcBlendAlpha = cmd->blend.source_alpha ? functable[cmd->blend.source_alpha] : D3D11_BLEND_ONE,
						.DestBlendAlpha = cmd->blend.dest_alpha ? functable[cmd->blend.dest_alpha] : D3D11_BLEND_ZERO,
						.BlendOpAlpha = cmd->blend.op_alpha ? optable[cmd->blend.op_alpha] : D3D11_BLEND_OP_ADD,
						.RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL,
					},
				};
				
				ID3D11BlendState* blend_state;
				D3d11Call(ID3D11Device_CreateBlendState(D3d11.device, &blend_desc, &blend_state));
				
				RB_D3d11Blend_* pool_data = RB_PoolAlloc_(&g_d3d11_blendpool, &handle.id);
				pool_data->blend_state = blend_state;
			} break;
			
			case RB_ResourceCommandKind_MakeRasterizerState:
			{
				static const D3D11_FILL_MODE filltable[] = {
					[0] = D3D11_FILL_SOLID,
					[RB_FillMode_Solid]     = D3D11_FILL_SOLID,
					[RB_FillMode_Wireframe] = D3D11_FILL_WIREFRAME,
				};
				
				static const D3D11_CULL_MODE culltable[] = {
					[0] = D3D11_CULL_NONE,
					[RB_CullMode_None]  = D3D11_CULL_NONE,
					[RB_CullMode_Front] = D3D11_CULL_FRONT,
					[RB_CullMode_Back]  = D3D11_CULL_BACK,
				};
				
				SafeAssert(cmd->rasterizer.fill_mode >= 0 && cmd->rasterizer.fill_mode < ArrayLength(filltable));
				SafeAssert(cmd->rasterizer.cull_mode >= 0 && cmd->rasterizer.cull_mode < ArrayLength(culltable));
				
				D3D11_RASTERIZER_DESC rasterizer_desc = {
					.FillMode = filltable[cmd->rasterizer.fill_mode],
					.CullMode = culltable[cmd->rasterizer.cull_mode],
					.FrontCounterClockwise = cmd->rasterizer.flag_cw_backface,
					.DepthBias = 0,
					.SlopeScaledDepthBias = 0.0f,
					.DepthBiasClamp = 0.0f,
					.DepthClipEnable = false, //cmd->rasterizer.flag_depth_test,
					.ScissorEnable = cmd->rasterizer.flag_scissor,
					.MultisampleEnable = false,
					.AntialiasedLineEnable = false,
				};
				
				D3D11_DEPTH_STENCIL_DESC depth_stencil_desc = {
					.DepthEnable = cmd->rasterizer.flag_depth_test,
					.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL,
					.DepthFunc = D3D11_COMPARISON_LESS,
				};
				
				ID3D11RasterizerState* rasterizer_state;
				D3d11Call(ID3D11Device_CreateRasterizerState(D3d11.device, &rasterizer_desc, &rasterizer_state));
				ID3D11DepthStencilState* depth_stencil_state;
				D3d11Call(ID3D11Device_CreateDepthStencilState(D3d11.device, &depth_stencil_desc, &depth_stencil_state));
				
				RB_D3d11Rasterizer_* pool_data = RB_PoolAlloc_(&g_d3d11_rasterizerpool, &handle.id);
				pool_data->rasterizer_state = rasterizer_state;
				pool_data->depth_stencil_state = depth_stencil_state;
			} break;
			
			case RB_ResourceCommandKind_UpdateVertexBuffer:
			case RB_ResourceCommandKind_UpdateIndexBuffer:
			case RB_ResourceCommandKind_UpdateUniformBuffer:
			{
				Assert(handle.id);
				SafeAssert(cmd->buffer.size <= UINT32_MAX);
				
				RB_D3d11Buffer_* pool_data = RB_PoolFetch_(&g_d3d11_bufferpool, handle.id);
				ID3D11Buffer* buffer = pool_data->buffer;
				
				// Grow if needed
				D3D11_BUFFER_DESC desc = { 0 };
				ID3D11Buffer_GetDesc(buffer, &desc);
				
				if (desc.ByteWidth < cmd->buffer.size)
				{
					ID3D11Buffer_Release(buffer);
					
					desc.ByteWidth = (uint32)cmd->buffer.size;
					
					D3D11_SUBRESOURCE_DATA initial_data = {
						.pSysMem = cmd->buffer.memory,
					};
					
					D3d11Call(ID3D11Device_CreateBuffer(D3d11.device, &desc, &initial_data, &buffer));
				}
				else
				{
					// TODO(ljre): cmd->flag_subregion
					ID3D11Resource* resource;
					D3D11_MAPPED_SUBRESOURCE map;
					
					D3d11Call(ID3D11Buffer_QueryInterface(buffer, &IID_ID3D11Resource, (void**)&resource));
					D3d11Call(ID3D11DeviceContext_Map(D3d11.context, resource, 0, D3D11_MAP_WRITE_DISCARD, 0, &map));
					Mem_Copy(map.pData, cmd->buffer.memory, cmd->buffer.size);
					ID3D11DeviceContext_Unmap(D3d11.context, resource, 0);
				}
				
				pool_data->buffer = buffer;
			} break;
			
			case RB_ResourceCommandKind_UpdateTexture2D:
			{
				Assert(handle.id);
				
				RB_D3d11Texture2D_* pool_data = RB_PoolFetch_(&g_d3d11_texpool, handle.id);
				
				// TODO(ljre): cmd->flag_subregion
				
				const uint8* pixels = cmd->texture_2d.pixels;
				int32 width = cmd->texture_2d.width;
				int32 height = cmd->texture_2d.height;
				uint32 channels = cmd->texture_2d.channels;
				
				ID3D11Texture2D* texture = pool_data->texture;
				ID3D11Resource* resource;
				D3D11_MAPPED_SUBRESOURCE map;
				
				D3d11Call(ID3D11Texture2D_QueryInterface(texture, &IID_ID3D11Resource, (void**)&resource));
				D3d11Call(ID3D11DeviceContext_Map(D3d11.context, resource, 0, D3D11_MAP_WRITE_DISCARD, 0, &map));
				
				for (intsize y = 0; y < height; ++y)
					Mem_Copy((uint8*)map.pData + map.RowPitch*y, pixels + width*channels*y, width * channels);
				
				ID3D11DeviceContext_Unmap(D3d11.context, resource, 0);
			} break;
			
			case RB_ResourceCommandKind_FreeTexture2D:
			{
				Assert(handle.id);
				
				RB_D3d11Texture2D_* pool_data = RB_PoolFetch_(&g_d3d11_texpool, handle.id);
				
				ID3D11Texture2D_Release(pool_data->texture);
				ID3D11ShaderResourceView_Release(pool_data->resource_view);
				
				RB_PoolFree_(&g_d3d11_texpool, handle.id);
				handle.id = 0;
			} break;
			
			case RB_ResourceCommandKind_FreeVertexBuffer:
			case RB_ResourceCommandKind_FreeIndexBuffer:
			case RB_ResourceCommandKind_FreeUniformBuffer:
			{
				Assert(handle.id);
				
				RB_D3d11Buffer_* pool_data = RB_PoolFetch_(&g_d3d11_bufferpool, handle.id);
				
				ID3D11Buffer_Release(pool_data->buffer);
				
				RB_PoolFree_(&g_d3d11_bufferpool, handle.id);
				handle.id = 0;
			} break;
			
			case RB_ResourceCommandKind_FreeShader:
			{
				SafeAssert(false);
			} break;
			
			case RB_ResourceCommandKind_FreeBlendState:
			{
				Assert(handle.id);
				
				RB_D3d11Blend_* pool_data = RB_PoolFetch_(&g_d3d11_blendpool, handle.id);
				
				ID3D11BlendState_Release(pool_data->blend_state);
				
				RB_PoolFree_(&g_d3d11_blendpool, handle.id);
				handle.id = 0;
			} break;
			
			case RB_ResourceCommandKind_FreeRasterizerState:
			{
				Assert(handle.id);
				
				RB_D3d11Rasterizer_* pool_data = RB_PoolFetch_(&g_d3d11_rasterizerpool, handle.id);
				
				ID3D11RasterizerState_Release(pool_data->rasterizer_state);
				
				RB_PoolFree_(&g_d3d11_rasterizerpool, handle.id);
				handle.id = 0;
			} break;
			
			case RB_ResourceCommandKind_FreeRenderTarget:
			{
				SafeAssert(false);
			} break;
		}
		
		*cmd->handle = handle;
	}
}

static void
RB_DrawD3d11_(Arena* scratch_arena, RB_DrawCommand* commands, int32 default_width, int32 default_height)
{
	Trace();
	
	ID3D11VertexShader* curr_vertex_shader = NULL;
	ID3D11PixelShader* curr_pixel_shader = NULL;
	ID3D11InputLayout* curr_input_layout = NULL;
	ID3D11Buffer* curr_cbuffer = NULL;
	ID3D11RasterizerState* curr_raststate = g_d3d11_rasterizer_state;
	ID3D11BlendState* curr_blendstate = g_d3d11_blend_state;
	ID3D11DepthStencilState* curr_depthstate = g_d3d11_depth_state;
	
	D3D11_VIEWPORT viewport = {
		.TopLeftX = 0,
		.TopLeftY = 0,
		.Width = default_width,
		.Height = default_height,
		.MinDepth = 0.0f,
		.MaxDepth = 1.0f,
	};
	
	ID3D11DeviceContext_IASetPrimitiveTopology(D3d11.context, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	ID3D11DeviceContext_RSSetViewports(D3d11.context, 1, &viewport);
	ID3D11DeviceContext_RSSetState(D3d11.context, curr_raststate);
	ID3D11DeviceContext_OMSetRenderTargets(D3d11.context, 1, &D3d11.target, D3d11.depth_stencil);
	ID3D11DeviceContext_OMSetBlendState(D3d11.context, curr_blendstate, NULL, 0xFFFFFFFF);
	ID3D11DeviceContext_OMSetDepthStencilState(D3d11.context, curr_depthstate, 1);
	
	for (RB_DrawCommand* cmd = commands; cmd; cmd = cmd->next)
	{
		Trace(); TraceName(RB_draw_cmd_names[cmd->kind]);
		
		Assert(cmd->kind);
		
		if (cmd->resources_cmd)
			RB_ResourceD3d11_(scratch_arena, cmd->resources_cmd);
		
		switch (cmd->kind)
		{
			case 0: SafeAssert(false); break;
			
			case RB_DrawCommandKind_Clear:
			{
				if (cmd->clear.flag_color)
					ID3D11DeviceContext_ClearRenderTargetView(D3d11.context, D3d11.target, cmd->clear.color);
				
				uint32 ds_flags = 0;
				if (cmd->clear.flag_depth)
					ds_flags |= D3D11_CLEAR_DEPTH;
				if (cmd->clear.flag_stencil)
					ds_flags |= D3D11_CLEAR_STENCIL;
				
				if (ds_flags)
					ID3D11DeviceContext_ClearDepthStencilView(D3d11.context, D3d11.depth_stencil, ds_flags, 1.0f, 0);
			} break;
			
			case RB_DrawCommandKind_ApplyBlendState:
			{
				ID3D11BlendState* desired = g_d3d11_blend_state;
				
				if (cmd->apply.handle)
				{
					SafeAssert(cmd->apply.handle->id);
					RB_D3d11Blend_* pool_data = RB_PoolFetch_(&g_d3d11_blendpool, cmd->apply.handle->id);
					
					desired = pool_data->blend_state;
				}
				
				if (curr_blendstate != desired)
					ID3D11DeviceContext_OMSetBlendState(D3d11.context, desired, NULL, 0xFFFFFFFF);
				
				curr_blendstate = desired;
			} break;
			
			case RB_DrawCommandKind_ApplyRasterizerState:
			{
				ID3D11RasterizerState* desired_ras = g_d3d11_rasterizer_state;
				ID3D11DepthStencilState* desired_ds = g_d3d11_depth_state;
				
				if (cmd->apply.handle)
				{
					SafeAssert(cmd->apply.handle->id);
					RB_D3d11Rasterizer_* pool_data = RB_PoolFetch_(&g_d3d11_rasterizerpool, cmd->apply.handle->id);
					
					desired_ras = pool_data->rasterizer_state;
					desired_ds = pool_data->depth_stencil_state;
				}
				
				if (curr_raststate != desired_ras)
					ID3D11DeviceContext_RSSetState(D3d11.context, desired_ras);
				if (curr_depthstate != desired_ds)
					ID3D11DeviceContext_OMSetDepthStencilState(D3d11.context, desired_ds, 1);
				
				curr_raststate = desired_ras;
				curr_depthstate = desired_ds;
			} break;
			
			case RB_DrawCommandKind_ApplyRenderTarget:
			{
				SafeAssert(false);
			} break;
			
			case RB_DrawCommandKind_DrawCall:
			{
				uint32 index_count = cmd->drawcall.index_count;
				uint32 instance_count = cmd->drawcall.instance_count;
				
				// Shaders
				SafeAssert(cmd->drawcall.shader && cmd->drawcall.shader->id);
				RB_D3d11Shader_* shader_pool_data = RB_PoolFetch_(&g_d3d11_shaderpool, cmd->drawcall.shader->id);
				
				ID3D11VertexShader* vertex_shader = shader_pool_data->vertex_shader;
				ID3D11PixelShader* pixel_shader = shader_pool_data->pixel_shader;
				ID3D11InputLayout* input_layout = shader_pool_data->input_layout;
				
				// Buffers
				SafeAssert(cmd->drawcall.ibuffer && cmd->drawcall.ibuffer->id);
				RB_D3d11Buffer_* ibuffer_pool_data = RB_PoolFetch_(&g_d3d11_bufferpool, cmd->drawcall.ibuffer->id);
				
				ID3D11Buffer* ibuffer = ibuffer_pool_data->buffer;;
				
				uint32 vbuffer_count = 0;
				ID3D11Buffer* vbuffers[ArrayLength(cmd->drawcall.vbuffers)] = { 0 };
				
				for (int32 i = 0; i < ArrayLength(vbuffers); ++i)
				{
					if (!cmd->drawcall.vbuffers[i])
						break;
					
					SafeAssert(cmd->drawcall.vbuffers[i]->id);
					RB_D3d11Buffer_* vbuffer = RB_PoolFetch_(&g_d3d11_bufferpool, cmd->drawcall.vbuffers[i]->id);
					
					vbuffers[i] = vbuffer->buffer;
					vbuffer_count = i+1;
				}
				
				uint32 strides[ArrayLength(vbuffers)] = { 0 };
				uint32 offsets[ArrayLength(vbuffers)] = { 0 };
				Mem_Copy(strides, cmd->drawcall.vbuffer_strides, sizeof(strides));
				Mem_Copy(offsets, cmd->drawcall.vbuffer_offsets, sizeof(offsets));
				
				// Samplers
				uint32 sampler_count = 0;
				ID3D11SamplerState* sampler_states[ArrayLength(cmd->drawcall.samplers)] = { 0 };
				ID3D11ShaderResourceView* shader_resources[ArrayLength(sampler_states)] = { 0 };
				
				for (int32 i = 0; i < ArrayLength(cmd->drawcall.samplers); ++i)
				{
					RB_SamplerDesc desc = cmd->drawcall.samplers[i];
					
					if (!desc.handle)
						break;
					
					SafeAssert(desc.handle->id);
					RB_D3d11Texture2D_* tex_pool_data = RB_PoolFetch_(&g_d3d11_texpool, desc.handle->id);
					
					sampler_states[i] = tex_pool_data->sampler_state;
					shader_resources[i] = tex_pool_data->resource_view;
					
					++sampler_count;
				}
				
				// Constant buffer
				ID3D11Buffer* cbuffer = NULL;
				
				if (cmd->drawcall.ubuffer)
				{
					RB_D3d11Buffer_* cbuffer_pool_data = RB_PoolFetch_(&g_d3d11_bufferpool, cmd->drawcall.ubuffer->id);
					cbuffer = cbuffer_pool_data->buffer;
				}
				
				// Index Type
				DXGI_FORMAT index_type = DXGI_FORMAT_R32_UINT;
				switch (cmd->drawcall.index_type)
				{
					case RB_IndexType_Uint16: index_type = DXGI_FORMAT_R16_UINT; break;
					case RB_IndexType_Uint32: index_type = DXGI_FORMAT_R32_UINT; break;
					default: SafeAssert(false);
				}
				
				// Draw
				if (input_layout != curr_input_layout)
					ID3D11DeviceContext_IASetInputLayout(D3d11.context, input_layout);
				ID3D11DeviceContext_IASetVertexBuffers(D3d11.context, 0, vbuffer_count, vbuffers, strides, offsets);
				ID3D11DeviceContext_IASetIndexBuffer(D3d11.context, ibuffer, index_type, 0);
				if (vertex_shader != curr_vertex_shader)
					ID3D11DeviceContext_VSSetShader(D3d11.context, vertex_shader, NULL, 0);
				if (cbuffer != curr_cbuffer)
					ID3D11DeviceContext_VSSetConstantBuffers(D3d11.context, 0, !!cbuffer, cbuffer ? &cbuffer : NULL);
				if (pixel_shader != curr_pixel_shader)
					ID3D11DeviceContext_PSSetShader(D3d11.context, pixel_shader, NULL, 0);
				ID3D11DeviceContext_PSSetSamplers(D3d11.context, 0, sampler_count, sampler_states);
				ID3D11DeviceContext_PSSetShaderResources(D3d11.context, 0, sampler_count, shader_resources);
				
				{
					Trace(); TraceName(Str("ID3D11DeviceContext::DrawIndexedInstanced"));
					ID3D11DeviceContext_DrawIndexedInstanced(D3d11.context, index_count, instance_count, 0, 0, 0);
				}
				
				curr_vertex_shader = vertex_shader;
				curr_pixel_shader = pixel_shader;
				curr_input_layout = input_layout;
				curr_cbuffer = cbuffer;
			} break;
		}
	}
	
	ID3D11DeviceContext_ClearState(D3d11.context);
}

#undef D3d11
#undef D3d11Call
