#define D3d11 (*ctx->graphics_context->d3d11)
#define D3d11Call(...) do { \
HRESULT hr = (__VA_ARGS__); \
if (FAILED(hr)) { \
if (Assert_IsDebuggerPresent_()) \
Debugbreak(); \
SafeAssert_OnFailure(#__VA_ARGS__, __FILE__, __LINE__, __func__); \
} \
} while (0)

struct RB_D3d11Texture2D_
{
	ID3D11Texture2D* texture;
	ID3D11ShaderResourceView* resource_view;
	ID3D11SamplerState* sampler_state;
	DXGI_FORMAT format;
	RB_TexFormat rb_format;
	int32 width, height;
}
typedef RB_D3d11Texture2D_;

struct RB_D3d11Shader_
{
	ID3D11VertexShader* vertex_shader;
	ID3D11PixelShader* pixel_shader;
	
	Buffer vs_blob;
}
typedef RB_D3d11Shader_;

struct RB_D3d11ComputeShader_
{
	ID3D11ComputeShader* shader;
	ID3D11ClassLinkage* linkage;
}
typedef RB_D3d11ComputeShader_;

struct RB_D3d11Buffer_
{
	ID3D11Buffer* buffer;
	ID3D11UnorderedAccessView* uav; // only if structured buffer
	DXGI_FORMAT index_type; // only if index buffer
}
typedef RB_D3d11Buffer_;

struct RB_D3d11Pipeline_
{
	RB_Shader shader_handle;
	ID3D11BlendState* blend_state;
	ID3D11RasterizerState* rasterizer_state;
	ID3D11DepthStencilState* depth_stencil_state;
	ID3D11InputLayout* input_layout;
}
typedef RB_D3d11Pipeline_;

struct RB_D3d11RenderTarget_
{
	intsize color_count;
	ID3D11RenderTargetView* color_views[RB_Limits_RenderTargetMaxColorAttachments];
	ID3D11DepthStencilView* depth_stencil_view;
}
typedef RB_D3d11RenderTarget_;

struct RB_D3d11Runtime_
{
	ID3D11RasterizerState* rasterizer_state;
	ID3D11SamplerState* linear_sampler;
	ID3D11SamplerState* nearest_sampler;
	ID3D11BlendState* blend_state;
	ID3D11DepthStencilState* depth_state;
	
	ID3D11VertexShader* curr_vertex_shader;
	ID3D11PixelShader* curr_pixel_shader;
	ID3D11InputLayout* curr_input_layout;
	ID3D11RasterizerState* curr_raststate;
	ID3D11BlendState* curr_blendstate;
	ID3D11DepthStencilState* curr_depthstate;
	
	struct { uint32 size, first_free; RB_D3d11Texture2D_ data[512]; } texpool;
	struct { uint32 size, first_free; RB_D3d11Buffer_ data[512]; } bufferpool;
	struct { uint32 size, first_free; RB_D3d11Shader_ data[64]; } shaderpool;
	struct { uint32 size, first_free; RB_D3d11Pipeline_ data[64]; } pipelinepool;
	struct { uint32 size, first_free; RB_D3d11RenderTarget_ data[64]; } rendertargetpool;
	struct { uint32 size, first_free; RB_D3d11ComputeShader_ data[64]; } compshaderpool;
}
typedef RB_D3d11Runtime_;

static DXGI_FORMAT
RB_D3d11TexFormatToDxgi_(RB_TexFormat texfmt, uint32* out_pixel_size)
{
	DXGI_FORMAT format = 0;
	uint32 pixel_size = 0;
	
	switch (texfmt)
	{
		case 0: case RB_TexFormat_Count: break;
		
		case RB_TexFormat_D16: format = DXGI_FORMAT_D16_UNORM; pixel_size = 2; break;
		case RB_TexFormat_D24S8: format = DXGI_FORMAT_D24_UNORM_S8_UINT; pixel_size = 4; break;
		case RB_TexFormat_A8: format = DXGI_FORMAT_A8_UNORM; pixel_size = 1; break;
		case RB_TexFormat_R8: format = DXGI_FORMAT_R8_UNORM; pixel_size = 1; break;
		case RB_TexFormat_RG8: format = DXGI_FORMAT_R8G8_UNORM; pixel_size = 2; break;
		case RB_TexFormat_RGB8: break;
		case RB_TexFormat_RGBA8: format = DXGI_FORMAT_R8G8B8A8_UNORM; pixel_size = 4; break;
	}
	
	if (out_pixel_size)
		*out_pixel_size = pixel_size;
	
	return format;
}

static bool
RB_D3d11TexFormatIsDepthStencil_(RB_TexFormat texfmt)
{
	switch (texfmt)
	{
		case 0: case RB_TexFormat_Count: return false;
		
		case RB_TexFormat_D16:
		case RB_TexFormat_D24S8: return true;
		
		case RB_TexFormat_A8:
		case RB_TexFormat_R8:
		case RB_TexFormat_RG8:
		case RB_TexFormat_RGB8:
		case RB_TexFormat_RGBA8: return false;
	}
}

static void
RB_D3d11FreeCtx_(RB_Ctx* ctx)
{
	
}

static bool
RB_D3d11IsValidHandle_(RB_Ctx* ctx, uint32 handle)
{ return handle != 0; }

static void
RB_D3d11Resource_(RB_Ctx* ctx, const RB_ResourceCall_* resc)
{
	RB_D3d11Runtime_* rt = ctx->rt;
	uint32 handle = *resc->handle;
	
	switch (resc->kind)
	{
		case RB_ResourceKind_Null_: Assert(false); break;
		
		case RB_ResourceKind_MakeTexture2D_:
		{
			Assert(!handle);
			
			const void* pixels = resc->tex2d.pixels;
			int32 width = resc->tex2d.width;
			int32 height = resc->tex2d.height;
			bool dynamic = resc->tex2d.flag_dynamic;
			uint32 pixel_size;
			DXGI_FORMAT format = RB_D3d11TexFormatToDxgi_(resc->tex2d.format, &pixel_size);
			uint32 pitch = width * pixel_size;
			
			Assert(width && height && format);
			
			UINT bind_flags = D3D11_BIND_SHADER_RESOURCE;
			
			if (resc->tex2d.flag_not_used_in_shader)
				bind_flags &= ~D3D11_BIND_SHADER_RESOURCE;
			if (resc->tex2d.flag_render_target)
			{
				if (RB_D3d11TexFormatIsDepthStencil_(resc->tex2d.format))
					bind_flags |= D3D11_BIND_DEPTH_STENCIL;
				else
					bind_flags |= D3D11_BIND_RENDER_TARGET;
			}
			
			const D3D11_TEXTURE2D_DESC tex_desc = {
				.Width = width,
				.Height = height,
				.MipLevels = 1,
				.ArraySize = 1,
				.Format = format,
				.SampleDesc = {
					.Count = 1,
					.Quality = 0,
				},
				.Usage = (dynamic) ? D3D11_USAGE_DYNAMIC : D3D11_USAGE_IMMUTABLE,
				.BindFlags = bind_flags,
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
			ID3D11ShaderResourceView* resource_view;
			ID3D11SamplerState* sampler_state = resc->tex2d.flag_linear_filtering ? rt->linear_sampler : rt->nearest_sampler;
			
			D3d11Call(ID3D11Device_CreateTexture2D(D3d11.device, &tex_desc, tex_initial, &texture));
			D3d11Call(ID3D11Device_CreateShaderResourceView(D3d11.device, (ID3D11Resource*)texture, &resource_view_desc, &resource_view));
			
			RB_D3d11Texture2D_* pool_data = RB_PoolAlloc_(&rt->texpool, &handle);
			pool_data->texture = texture;
			pool_data->resource_view = resource_view;
			pool_data->sampler_state = sampler_state;
			pool_data->format = format;
			pool_data->rb_format = resc->tex2d.format;
			pool_data->width = width;
			pool_data->height = height;
		} break;
		
		//case RB_ResourceKind_MakeVertexBuffer_:
		//case RB_ResourceKind_MakeIndexBuffer_:
		//case RB_ResourceKind_MakeUniformBuffer_:
		{
			uint32 bind_flags;
			D3D11_USAGE usage;
			uintsize size;
			const void* data;
			RB_IndexType index_type;
			UINT misc;
			
			if (0) case RB_ResourceKind_MakeVertexBuffer_:
			{
				bind_flags = D3D11_BIND_VERTEX_BUFFER;
				size = resc->vbuffer.size;
				data = resc->vbuffer.initial_data;
				usage = resc->vbuffer.flag_dynamic ? D3D11_USAGE_DYNAMIC : D3D11_USAGE_IMMUTABLE;
				misc = 0;
			}
			if (0) case RB_ResourceKind_MakeIndexBuffer_:
			{
				bind_flags = D3D11_BIND_INDEX_BUFFER;
				size = resc->ibuffer.size;
				data = resc->ibuffer.initial_data;
				usage = resc->ibuffer.flag_dynamic ? D3D11_USAGE_DYNAMIC : D3D11_USAGE_IMMUTABLE;
				index_type = resc->ibuffer.index_type;
				misc = 0;
			}
			if (0) case RB_ResourceKind_MakeUniformBuffer_:
			{
				bind_flags = D3D11_BIND_CONSTANT_BUFFER;
				size = resc->ubuffer.size;
				data = resc->ubuffer.initial_data;
				usage = D3D11_USAGE_DYNAMIC;
				misc = 0;
			}
			if (0) case RB_ResourceKind_MakeStructuredBuffer_:
			{
				bind_flags = D3D11_BIND_CONSTANT_BUFFER;
				size = resc->sbuffer.size;
				data = resc->sbuffer.initial_data;
				misc = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
				switch (resc->sbuffer.kind)
				{
					case RB_SBufferKind_Immutable: usage = D3D11_USAGE_IMMUTABLE; break;
					case RB_SBufferKind_Dynamic: usage = D3D11_USAGE_DYNAMIC; break;
					case RB_SBufferKind_GpuReadWrite: usage = D3D11_USAGE_DEFAULT; break;
				}
			}
			
			Assert(!handle);
			SafeAssert(size <= UINT32_MAX);
			
			const D3D11_BUFFER_DESC buffer_desc = {
				.Usage = usage,
				.ByteWidth = (uint32)size,
				.BindFlags = bind_flags,
				.CPUAccessFlags = (usage == D3D11_USAGE_DYNAMIC) ? D3D11_CPU_ACCESS_WRITE : 0,
				.MiscFlags = misc,
			};
			
			const D3D11_SUBRESOURCE_DATA* buf_initial = (!data) ? NULL : &(D3D11_SUBRESOURCE_DATA) {
				.pSysMem = data,
			};
			
			ID3D11Buffer* buffer;
			
			D3d11Call(ID3D11Device_CreateBuffer(D3d11.device, &buffer_desc, buf_initial, &buffer));
			
			RB_D3d11Buffer_* pool_data = RB_PoolAlloc_(&rt->bufferpool, &handle);
			pool_data->buffer = buffer;
			
			if (resc->kind == RB_ResourceKind_MakeIndexBuffer_)
			{
				switch (index_type)
				{
					default: SafeAssert(false); break;
					case RB_IndexType_Uint16: pool_data->index_type = DXGI_FORMAT_R16_UINT; break;
					case RB_IndexType_Uint32: pool_data->index_type = DXGI_FORMAT_R32_UINT; break;
				}
			}
			else if (resc->kind == RB_ResourceKind_MakeStructuredBuffer_)
			{
				const D3D11_UNORDERED_ACCESS_VIEW_DESC uav_desc = {
					.Format = DXGI_FORMAT_UNKNOWN,
					.ViewDimension = D3D11_UAV_DIMENSION_BUFFER,
					.Buffer = {
						.FirstElement = 0,
						.NumElements = (UINT)(resc->sbuffer.size / resc->sbuffer.stride),
						.Flags = 0,
					},
				};
				
				ID3D11UnorderedAccessView* uav;
				D3d11Call(ID3D11Device_CreateUnorderedAccessView(D3d11.device, (void*)buffer, &uav_desc, &uav));
				
				pool_data->uav = uav;
			}
		} break;
		
		case RB_ResourceKind_MakeShader_:
		{
			Assert(!handle);
			
			Buffer vs_blob = resc->shader.hlsl40_91.vs;
			Buffer ps_blob = resc->shader.hlsl40_91.ps;
			
			if (resc->shader.hlsl40_93.vs.size && ctx->caps.shader_type >= RB_ShaderType_Hlsl40Level93)
			{
				vs_blob = resc->shader.hlsl40_93.vs;
				ps_blob = resc->shader.hlsl40_93.ps;
			}
			
			if (resc->shader.hlsl40.vs.size && ctx->caps.shader_type >= RB_ShaderType_Hlsl40)
			{
				vs_blob = resc->shader.hlsl40.vs;
				ps_blob = resc->shader.hlsl40.ps;
			}
			
			SafeAssert(vs_blob.size && ps_blob.size);
			ID3D11VertexShader* vs;
			ID3D11PixelShader* ps;
			
			D3d11Call(ID3D11Device_CreateVertexShader(D3d11.device, vs_blob.data, vs_blob.size, NULL, &vs));
			D3d11Call(ID3D11Device_CreatePixelShader(D3d11.device, ps_blob.data, ps_blob.size, NULL, &ps));
			
			RB_D3d11Shader_* pool_data = RB_PoolAlloc_(&rt->shaderpool, &handle);
			pool_data->vertex_shader = vs;
			pool_data->pixel_shader = ps;
			pool_data->vs_blob = vs_blob;
		} break;
		
		case RB_ResourceKind_MakeComputeShader_:
		{
			Assert(!handle);
			
			Buffer cs_blob = resc->compute_shader.hlsl40;
			
			SafeAssert(cs_blob.size);
			ID3D11ComputeShader* cs;
			ID3D11ClassLinkage* linkage;
			
			D3d11Call(ID3D11Device_CreateClassLinkage(D3d11.device, &linkage));
			D3d11Call(ID3D11Device_CreateComputeShader(D3d11.device, cs_blob.data, cs_blob.size, linkage, &cs));
			
			RB_D3d11ComputeShader_* pool_data = RB_PoolAlloc_(&rt->compshaderpool, &handle);
			pool_data->shader = cs;
			pool_data->linkage = linkage;
		} break;
		
		case RB_ResourceKind_MakePipeline_:
		{
			SafeAssert(resc->pipeline.shader.id);
			RB_Shader shader_handle = resc->pipeline.shader;
			RB_D3d11Shader_* shader_pool_data = RB_PoolFetch_(&rt->shaderpool, shader_handle.id);
			Buffer vs_blob = shader_pool_data->vs_blob;
			
			int32 layout_size = 0;
			D3D11_INPUT_ELEMENT_DESC layout_desc[RB_Limits_PipelineMaxVertexInputs*4] = { 0 };
			
			for (int32 i = 0; i < ArrayLength(resc->pipeline.input_layout); ++i)
			{
				if (!resc->pipeline.input_layout[i].format)
					break;
				
				RB_LayoutDesc curr_layout = resc->pipeline.input_layout[i];
				
				switch (curr_layout.format)
				{
					case 0: Assert(false); break;
					
					//case RB_VertexFormat_Vec2:
					//case RB_VertexFormat_Vec3:
					//case RB_VertexFormat_Vec4:
					{
						DXGI_FORMAT format;
						
						if (0) case RB_VertexFormat_Scalar: format = DXGI_FORMAT_R32_FLOAT;
						if (0) case RB_VertexFormat_Vec2: format = DXGI_FORMAT_R32G32_FLOAT;
						if (0) case RB_VertexFormat_Vec3: format = DXGI_FORMAT_R32G32B32_FLOAT;
						if (0) case RB_VertexFormat_Vec4: format = DXGI_FORMAT_R32G32B32A32_FLOAT;
						if (0) case RB_VertexFormat_Vec2I16Norm: format = DXGI_FORMAT_R16G16_SNORM;
						if (0) case RB_VertexFormat_Vec2I16: format = DXGI_FORMAT_R16G16_SINT;
						if (0) case RB_VertexFormat_Vec4I16Norm: format = DXGI_FORMAT_R16G16B16A16_SNORM;
						if (0) case RB_VertexFormat_Vec4I16: format = DXGI_FORMAT_R16G16B16A16_SINT;
						if (0) case RB_VertexFormat_Vec4U8Norm: format = DXGI_FORMAT_R8G8B8A8_UNORM;
						if (0) case RB_VertexFormat_Vec4U8: format = DXGI_FORMAT_R8G8B8A8_UINT;
						if (0) case RB_VertexFormat_Vec2F16: format = DXGI_FORMAT_R16G16_FLOAT;
						if (0) case RB_VertexFormat_Vec4F16: format = DXGI_FORMAT_R16G16B16A16_FLOAT;
						
						D3D11_INPUT_ELEMENT_DESC element_desc = {
							.SemanticName = "VINPUT",
							.SemanticIndex = (UINT)layout_size,
							.Format = format,
							.InputSlot = curr_layout.buffer_slot,
							.AlignedByteOffset = curr_layout.offset,
							.InputSlotClass = (curr_layout.divisor == 0) ? D3D11_INPUT_PER_VERTEX_DATA : D3D11_INPUT_PER_INSTANCE_DATA,
							.InstanceDataStepRate = curr_layout.divisor,
						};
						
						layout_desc[layout_size++] = element_desc;
					} break;
					
					case RB_VertexFormat_Mat2:
					{
						D3D11_INPUT_ELEMENT_DESC element_desc = {
							.SemanticName = "VINPUT",
							.SemanticIndex = (UINT)layout_size,
							.Format = DXGI_FORMAT_R32G32_FLOAT,
							.InputSlot = curr_layout.buffer_slot,
							.AlignedByteOffset = curr_layout.offset,
							.InputSlotClass = (curr_layout.divisor == 0) ? D3D11_INPUT_PER_VERTEX_DATA : D3D11_INPUT_PER_INSTANCE_DATA,
							.InstanceDataStepRate = curr_layout.divisor,
						};
						
						layout_desc[layout_size++] = element_desc; ++element_desc.SemanticIndex; element_desc.AlignedByteOffset += sizeof(float32[2]);
						layout_desc[layout_size++] = element_desc;
					} break;
					
					case RB_VertexFormat_Mat3:
					{
						D3D11_INPUT_ELEMENT_DESC element_desc = {
							.SemanticName = "VINPUT",
							.SemanticIndex = (UINT)layout_size,
							.Format = DXGI_FORMAT_R32G32B32_FLOAT,
							.InputSlot = curr_layout.buffer_slot,
							.AlignedByteOffset = curr_layout.offset,
							.InputSlotClass = (curr_layout.divisor == 0) ? D3D11_INPUT_PER_VERTEX_DATA : D3D11_INPUT_PER_INSTANCE_DATA,
							.InstanceDataStepRate = curr_layout.divisor,
						};
						
						layout_desc[layout_size++] = element_desc; ++element_desc.SemanticIndex; element_desc.AlignedByteOffset += sizeof(float32[3]);
						layout_desc[layout_size++] = element_desc; ++element_desc.SemanticIndex; element_desc.AlignedByteOffset += sizeof(float32[3]);
						layout_desc[layout_size++] = element_desc;
					} break;
					
					case RB_VertexFormat_Mat4:
					{
						D3D11_INPUT_ELEMENT_DESC element_desc = {
							.SemanticName = "VINPUT",
							.SemanticIndex = (UINT)layout_size,
							.Format = DXGI_FORMAT_R32G32B32A32_FLOAT,
							.InputSlot = curr_layout.buffer_slot,
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
			
			SafeAssert(resc->pipeline.blend_source >= 0 && resc->pipeline.blend_source < ArrayLength(functable));
			SafeAssert(resc->pipeline.blend_dest >= 0 && resc->pipeline.blend_dest < ArrayLength(functable));
			SafeAssert(resc->pipeline.blend_source_alpha >= 0 && resc->pipeline.blend_source_alpha < ArrayLength(functable));
			SafeAssert(resc->pipeline.blend_dest_alpha >= 0 && resc->pipeline.blend_dest_alpha < ArrayLength(functable));
			
			SafeAssert(resc->pipeline.blend_op >= 0 && resc->pipeline.blend_op < ArrayLength(optable));
			SafeAssert(resc->pipeline.blend_op_alpha >= 0 && resc->pipeline.blend_op_alpha < ArrayLength(optable));
			
			D3D11_BLEND_DESC blend_desc = {
				.AlphaToCoverageEnable = false,
				.IndependentBlendEnable = false,
				
				.RenderTarget[0] = {
					.BlendEnable = resc->pipeline.flag_blend,
					.SrcBlend = resc->pipeline.blend_source ? functable[resc->pipeline.blend_source] : D3D11_BLEND_ONE,
					.DestBlend = resc->pipeline.blend_dest ? functable[resc->pipeline.blend_dest] : D3D11_BLEND_ZERO,
					.BlendOp = resc->pipeline.blend_op ? optable[resc->pipeline.blend_op] : D3D11_BLEND_OP_ADD,
					.SrcBlendAlpha = resc->pipeline.blend_source_alpha ? functable[resc->pipeline.blend_source_alpha] : D3D11_BLEND_ONE,
					.DestBlendAlpha = resc->pipeline.blend_dest_alpha ? functable[resc->pipeline.blend_dest_alpha] : D3D11_BLEND_ZERO,
					.BlendOpAlpha = resc->pipeline.blend_op_alpha ? optable[resc->pipeline.blend_op_alpha] : D3D11_BLEND_OP_ADD,
					.RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL,
				},
			};
			
			static const D3D11_FILL_MODE filltable[] = {
				[RB_FillMode_Solid]     = D3D11_FILL_SOLID,
				[RB_FillMode_Wireframe] = D3D11_FILL_WIREFRAME,
			};
			
			static const D3D11_CULL_MODE culltable[] = {
				[RB_CullMode_None]  = D3D11_CULL_NONE,
				[RB_CullMode_Front] = D3D11_CULL_FRONT,
				[RB_CullMode_Back]  = D3D11_CULL_BACK,
			};
			
			SafeAssert(resc->pipeline.fill_mode >= 0 && resc->pipeline.fill_mode < ArrayLength(filltable));
			SafeAssert(resc->pipeline.cull_mode >= 0 && resc->pipeline.cull_mode < ArrayLength(culltable));
			
			D3D11_RASTERIZER_DESC rasterizer_desc = {
				.FillMode = filltable[resc->pipeline.fill_mode],
				.CullMode = culltable[resc->pipeline.cull_mode],
				.FrontCounterClockwise = resc->pipeline.flag_cw_backface,
				.DepthBias = 0,
				.SlopeScaledDepthBias = 0.0f,
				.DepthBiasClamp = 0.0f,
				.DepthClipEnable = true,
				.ScissorEnable = false, //resc->pipeline.flag_scissor,
				.MultisampleEnable = false,
				.AntialiasedLineEnable = false,
			};
			
			D3D11_DEPTH_STENCIL_DESC depth_stencil_desc = {
				.DepthEnable = resc->pipeline.flag_depth_test,
				.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL,
				.DepthFunc = D3D11_COMPARISON_LESS,
			};
			
			ID3D11InputLayout* input_layout;
			ID3D11BlendState* blend_state;
			ID3D11RasterizerState* rasterizer_state;
			ID3D11DepthStencilState* depth_stencil_state;
			
			D3d11Call(ID3D11Device_CreateInputLayout(D3d11.device, layout_desc, layout_size, vs_blob.data, vs_blob.size, &input_layout));
			D3d11Call(ID3D11Device_CreateBlendState(D3d11.device, &blend_desc, &blend_state));
			D3d11Call(ID3D11Device_CreateRasterizerState(D3d11.device, &rasterizer_desc, &rasterizer_state));
			D3d11Call(ID3D11Device_CreateDepthStencilState(D3d11.device, &depth_stencil_desc, &depth_stencil_state));
			
			RB_D3d11Pipeline_* pool_data = RB_PoolAlloc_(&rt->pipelinepool, &handle);
			pool_data->shader_handle = shader_handle;
			pool_data->input_layout = input_layout;
			pool_data->blend_state = blend_state;
			pool_data->rasterizer_state = rasterizer_state;
			pool_data->depth_stencil_state = depth_stencil_state;
		} break;
		
		case RB_ResourceKind_MakeRenderTarget_:
		{
			Assert(!handle);
			
			intsize color_count = 0;
			ID3D11RenderTargetView* color_views[ArrayLength(resc->render_target.color)] = { 0 };
			ID3D11DepthStencilView* depth_stencil_view = NULL;
			
			for (intsize i = 0; i < ArrayLength(resc->render_target.color); ++i)
			{
				RB_Tex2d texhandle = resc->render_target.color[i];
				if (!texhandle.id)
					break;
				
				RB_D3d11Texture2D_* tex_pool_data = RB_PoolFetch_(&rt->texpool, texhandle.id);
				
				ID3D11Resource* resource = (ID3D11Resource*)tex_pool_data->texture;
				ID3D11RenderTargetView* view;
				D3D11_RENDER_TARGET_VIEW_DESC desc = {
					.Format = tex_pool_data->format,
					.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D,
					.Texture2D = {
						.MipSlice = 0,
					},
				};
				
				D3d11Call(ID3D11Device_CreateRenderTargetView(D3d11.device, resource, &desc, &view));
				
				color_views[i] = view;
				color_count = i;
			}
			
			if (resc->render_target.depth_stencil.id)
			{
				RB_D3d11Texture2D_* tex_pool_data = RB_PoolFetch_(&rt->texpool, resc->render_target.depth_stencil.id);
				ID3D11Resource* resource = (ID3D11Resource*)tex_pool_data->texture;
				ID3D11DepthStencilView* view;
				D3D11_DEPTH_STENCIL_VIEW_DESC desc = {
					.Format = tex_pool_data->format,
					.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D,
					.Flags = 0,
					.Texture2D = {
						.MipSlice = 0,
					},
				};
				
				D3d11Call(ID3D11Device_CreateDepthStencilView(D3d11.device, resource, &desc, &view));
				
				depth_stencil_view = view;
			}
			
			RB_D3d11RenderTarget_* pool_data = RB_PoolAlloc_(&rt->rendertargetpool, &handle);
			pool_data->color_count = color_count;
			pool_data->depth_stencil_view = depth_stencil_view;
			MemoryCopy(pool_data->color_views, color_views, sizeof(color_views[0]) * color_count);
		} break;
		
		case RB_ResourceKind_UpdateVertexBuffer_:
		case RB_ResourceKind_UpdateIndexBuffer_:
		case RB_ResourceKind_UpdateUniformBuffer_:
		case RB_ResourceKind_UpdateStructuredBuffer_:
		{
			Assert(handle);
			Buffer new_data = resc->update.new_data;
			SafeAssert(new_data.size <= UINT32_MAX);
			
			RB_D3d11Buffer_* pool_data = RB_PoolFetch_(&rt->bufferpool, handle);
			ID3D11Buffer* buffer = pool_data->buffer;
			
			// Grow if needed
			D3D11_BUFFER_DESC desc = { 0 };
			ID3D11Buffer_GetDesc(buffer, &desc);
			
			if (new_data.size != desc.ByteWidth)
			{
				ID3D11Buffer_Release(buffer);
				desc.ByteWidth = (uint32)new_data.size;
				
				D3d11Call(ID3D11Device_CreateBuffer(D3d11.device, &desc, NULL, &buffer));
				pool_data->buffer = buffer;
				
				// TODO(ljre): Re-create UAV if structured buffer
			}
			
			D3D11_MAPPED_SUBRESOURCE map;
			D3d11Call(ID3D11DeviceContext_Map(D3d11.context, (ID3D11Resource*)buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &map));
			
			MemoryCopy(map.pData, new_data.data, new_data.size);
			
			ID3D11DeviceContext_Unmap(D3d11.context, (ID3D11Resource*)buffer, 0);
		} break;
		
		case RB_ResourceKind_UpdateTexture2D_:
		{
			Assert(handle);
			
			RB_D3d11Texture2D_* pool_data = RB_PoolFetch_(&rt->texpool, handle);
			Buffer new_data = resc->update.new_data;
			int32 width = pool_data->width;
			int32 height = pool_data->height;
			uint32 pixel_size;
			RB_D3d11TexFormatToDxgi_(pool_data->rb_format, &pixel_size);
			
			SafeAssert(new_data.size == (uintsize)width*height*pixel_size);
			
			ID3D11Texture2D* texture = pool_data->texture;
			D3D11_MAPPED_SUBRESOURCE map;
			
			D3d11Call(ID3D11DeviceContext_Map(D3d11.context, (ID3D11Resource*)texture, 0, D3D11_MAP_WRITE_DISCARD, 0, &map));
			
			for (intsize y = 0; y < height; ++y)
			{
				void* dst = (uint8*)map.pData + map.RowPitch*y;
				const void* src = new_data.data + width*pixel_size*y;
				uintsize size = width * pixel_size;
				
				MemoryCopy(dst, src, size);
			}
			
			ID3D11DeviceContext_Unmap(D3d11.context, (ID3D11Resource*)texture, 0);
		} break;
		
		case RB_ResourceKind_FreeTexture2D_:
		{
			Assert(handle);
			
			RB_D3d11Texture2D_* pool_data = RB_PoolFetch_(&rt->texpool, handle);
			
			ID3D11Texture2D_Release(pool_data->texture);
			ID3D11ShaderResourceView_Release(pool_data->resource_view);
			
			RB_PoolFree_(&rt->texpool, handle);
			handle = 0;
		} break;
		
		case RB_ResourceKind_FreeVertexBuffer_:
		case RB_ResourceKind_FreeIndexBuffer_:
		case RB_ResourceKind_FreeUniformBuffer_:
		case RB_ResourceKind_FreeStructuredBuffer_:
		{
			Assert(handle);
			
			RB_D3d11Buffer_* pool_data = RB_PoolFetch_(&rt->bufferpool, handle);
			
			ID3D11Buffer_Release(pool_data->buffer);
			if (pool_data->uav)
				ID3D11UnorderedAccessView_Release(pool_data->uav);
			
			RB_PoolFree_(&rt->bufferpool, handle);
			handle = 0;
		} break;
		
		case RB_ResourceKind_FreeShader_:
		{
			Assert(handle);
			
			RB_D3d11Shader_* pool_data = RB_PoolFetch_(&rt->shaderpool, handle);
			
			ID3D11VertexShader_Release(pool_data->vertex_shader);
			ID3D11PixelShader_Release(pool_data->pixel_shader);
			
			RB_PoolFree_(&rt->shaderpool, handle);
			handle = 0;
		} break;
		
		case RB_ResourceKind_FreeComputeShader_:
		{
			Assert(handle);
			
			RB_D3d11ComputeShader_* pool_data = RB_PoolFetch_(&rt->compshaderpool, handle);
			
			ID3D11ClassLinkage_Release(pool_data->linkage);
			ID3D11ComputeShader_Release(pool_data->shader);
			
			RB_PoolFree_(&rt->compshaderpool, handle);
			handle = 0;
		} break;
		
		case RB_ResourceKind_FreePipeline_:
		{
			Assert(handle);
			
			RB_D3d11Pipeline_* pool_data = RB_PoolFetch_(&rt->pipelinepool, handle);
			
			ID3D11BlendState_Release(pool_data->blend_state);
			ID3D11RasterizerState_Release(pool_data->rasterizer_state);
			ID3D11DepthStencilState_Release(pool_data->depth_stencil_state);
			ID3D11InputLayout_Release(pool_data->input_layout);
			
			RB_PoolFree_(&rt->pipelinepool, handle);
			handle = 0;
		} break;
		
		case RB_ResourceKind_FreeRenderTarget_:
		{
			SafeAssert(false);
		} break;
	}
	
	*resc->handle = handle;
}

static void
RB_D3d11Command_(RB_Ctx* ctx, const RB_CommandCall_* cmd)
{
	RB_D3d11Runtime_* rt = ctx->rt;
	
	switch (cmd->kind)
	{
		case RB_CommandKind_Null_: break;
		
		case RB_CommandKind_Begin_:
		{
			rt->curr_vertex_shader = NULL;
			rt->curr_pixel_shader = NULL;
			rt->curr_input_layout = NULL;
			rt->curr_raststate = rt->rasterizer_state;
			rt->curr_blendstate = rt->blend_state;
			rt->curr_depthstate = rt->depth_state;
			
			D3D11_VIEWPORT viewport = {
				.TopLeftX = 0.0f,
				.TopLeftY = 0.0f,
				.Width = (float32)cmd->begin.viewport_width,
				.Height = (float32)cmd->begin.viewport_height,
				.MinDepth = 0.0f,
				.MaxDepth = 1.0f,
			};
			
			ID3D11DeviceContext_IASetPrimitiveTopology(D3d11.context, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			ID3D11DeviceContext_RSSetViewports(D3d11.context, 1, &viewport);
			ID3D11DeviceContext_RSSetState(D3d11.context, rt->curr_raststate);
			ID3D11DeviceContext_OMSetRenderTargets(D3d11.context, 1, &D3d11.target, D3d11.depth_stencil);
			ID3D11DeviceContext_OMSetBlendState(D3d11.context, rt->curr_blendstate, NULL, 0xFFFFFFFF);
			ID3D11DeviceContext_OMSetDepthStencilState(D3d11.context, rt->curr_depthstate, 1);
		} break;
		
		case RB_CommandKind_End_:
		{
			ID3D11DeviceContext_ClearState(D3d11.context);
		} break;
		
		case RB_CommandKind_Clear_:
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
		
		case RB_CommandKind_ApplyPipeline_:
		{
			SafeAssert(cmd->apply_pipeline.handle.id);
			RB_D3d11Pipeline_* pool_data = RB_PoolFetch_(&rt->pipelinepool, cmd->apply_pipeline.handle.id);
			RB_D3d11Shader_* shader_pool_data = RB_PoolFetch_(&rt->shaderpool, pool_data->shader_handle.id);
			
			ID3D11BlendState* blend_state = pool_data->blend_state;
			ID3D11RasterizerState* rasterizer_state = pool_data->rasterizer_state;
			ID3D11DepthStencilState* depth_stencil_state = pool_data->depth_stencil_state;
			ID3D11InputLayout* input_layout = pool_data->input_layout;
			ID3D11VertexShader* vertex_shader = shader_pool_data->vertex_shader;
			ID3D11PixelShader* pixel_shader = shader_pool_data->pixel_shader;
			
			if (rt->curr_blendstate != blend_state)
				ID3D11DeviceContext_OMSetBlendState(D3d11.context, blend_state, NULL, 0xFFFFFFFF);
			if (rt->curr_raststate != rasterizer_state)
				ID3D11DeviceContext_RSSetState(D3d11.context, rasterizer_state);
			if (rt->curr_depthstate != depth_stencil_state)
				ID3D11DeviceContext_OMSetDepthStencilState(D3d11.context, depth_stencil_state, 1);
			if (rt->curr_input_layout != input_layout)
				ID3D11DeviceContext_IASetInputLayout(D3d11.context, input_layout);
			if (rt->curr_vertex_shader != vertex_shader)
				ID3D11DeviceContext_VSSetShader(D3d11.context, vertex_shader, NULL, 0);
			if (rt->curr_pixel_shader != pixel_shader)
				ID3D11DeviceContext_PSSetShader(D3d11.context, pixel_shader, NULL, 0);
			
			rt->curr_blendstate = blend_state;
			rt->curr_raststate = rasterizer_state;
			rt->curr_depthstate = depth_stencil_state;
			rt->curr_input_layout = input_layout;
			rt->curr_vertex_shader = vertex_shader;
			rt->curr_pixel_shader = pixel_shader;
		} break;
		
		case RB_CommandKind_ApplyRenderTarget_:
		{
			intsize color_count = 1;
			ID3D11RenderTargetView* color_views[RB_Limits_RenderTargetMaxColorAttachments] = { D3d11.target };
			ID3D11DepthStencilView* depth_stencil_view = D3d11.depth_stencil;
			
			if (cmd->apply_render_target.handle.id)
			{
				RB_D3d11RenderTarget_* pool_data = RB_PoolFetch_(&rt->rendertargetpool, cmd->apply_render_target.handle.id);
				color_count = pool_data->color_count;
				depth_stencil_view = pool_data->depth_stencil_view;
				MemoryCopy(color_views, pool_data->color_views, sizeof(color_views[0]) * color_count);
			}
			
			ID3D11DeviceContext_OMSetRenderTargets(D3d11.context, color_count, color_views, depth_stencil_view);
		} break;
		
		case RB_CommandKind_Draw_:
		{
			int32 base_vertex = 0; // = cmd->draw_instanced.base_vertex;
			uint32 base_index = cmd->draw.base_index;
			uint32 index_count = cmd->draw.index_count;
			uint32 instance_count = cmd->draw.instance_count;
			
			// Buffers
			SafeAssert(cmd->draw.ibuffer.id);
			RB_D3d11Buffer_* ibuffer_pool_data = RB_PoolFetch_(&rt->bufferpool, cmd->draw.ibuffer.id);
			
			ID3D11Buffer* ibuffer = ibuffer_pool_data->buffer;
			
			uint32 vbuffer_count = 0;
			ID3D11Buffer* vbuffers[ArrayLength(cmd->draw.vbuffers)] = { 0 };
			
			for (int32 i = 0; i < ArrayLength(vbuffers); ++i)
			{
				if (!cmd->draw.vbuffers[i].id)
					break;
				
				RB_D3d11Buffer_* vbuffer = RB_PoolFetch_(&rt->bufferpool, cmd->draw.vbuffers[i].id);
				
				vbuffers[i] = vbuffer->buffer;
				vbuffer_count = i+1;
			}
			
			uint32 strides[ArrayLength(vbuffers)] = { 0 };
			uint32 offsets[ArrayLength(vbuffers)] = { 0 };
			MemoryCopy(strides, cmd->draw.strides, sizeof(strides));
			MemoryCopy(offsets, cmd->draw.offsets, sizeof(offsets));
			
			// Samplers
			uint32 sampler_count = 0;
			ID3D11SamplerState* sampler_states[ArrayLength(cmd->draw.textures)] = { 0 };
			ID3D11ShaderResourceView* shader_resources[ArrayLength(sampler_states)] = { 0 };
			
			for (int32 i = 0; i < ArrayLength(cmd->draw.textures); ++i)
			{
				RB_Tex2d handle = cmd->draw.textures[i];
				
				if (!handle.id)
					break;
				
				RB_D3d11Texture2D_* tex_pool_data = RB_PoolFetch_(&rt->texpool, handle.id);
				sampler_states[i] = tex_pool_data->sampler_state;
				shader_resources[i] = tex_pool_data->resource_view;
				
				++sampler_count;
			}
			
			// Constant buffer
			ID3D11Buffer* cbuffer = NULL;
			
			if (cmd->draw.ubuffer.id)
			{
				RB_D3d11Buffer_* cbuffer_pool_data = RB_PoolFetch_(&rt->bufferpool, cmd->draw.ubuffer.id);
				cbuffer = cbuffer_pool_data->buffer;
			}
			
			// Index Type
			DXGI_FORMAT index_type = ibuffer_pool_data->index_type;
			
			// Draw
			ID3D11DeviceContext_IASetVertexBuffers(D3d11.context, 0, vbuffer_count, vbuffers, strides, offsets);
			ID3D11DeviceContext_IASetIndexBuffer(D3d11.context, ibuffer, index_type, 0);
			ID3D11DeviceContext_VSSetConstantBuffers(D3d11.context, 0, !!cbuffer, cbuffer ? &cbuffer : NULL);
			ID3D11DeviceContext_PSSetConstantBuffers(D3d11.context, 0, !!cbuffer, cbuffer ? &cbuffer : NULL);
			ID3D11DeviceContext_PSSetSamplers(D3d11.context, 0, sampler_count, sampler_states);
			ID3D11DeviceContext_PSSetShaderResources(D3d11.context, 0, sampler_count, shader_resources);
			
			if (instance_count)
				ID3D11DeviceContext_DrawIndexedInstanced(D3d11.context, index_count, instance_count, base_index, base_vertex, 0);
			else
				ID3D11DeviceContext_DrawIndexed(D3d11.context, index_count, base_index, base_vertex);
		} break;
		
		case RB_CommandKind_Dispatch_:
		{
			ID3D11ComputeShader* shader = RB_PoolFetch_(&rt->compshaderpool, cmd->dispatch.shader.id);
			ID3D11UnorderedAccessView* sbuffer = NULL;
			ID3D11Buffer* cbuffer = NULL;
			ID3D11ShaderResourceView* textures[RB_Limits_DrawMaxTextures] = { 0 };
			ID3D11SamplerState*  samplers[RB_Limits_DrawMaxTextures] = { 0 };
			UINT texture_count = 0;
			
			if (!RB_IsNull(cmd->dispatch.ubuffer))
			{
				RB_D3d11Buffer_* pool_data = RB_PoolFetch_(&rt->bufferpool, cmd->dispatch.ubuffer.id);
				cbuffer = pool_data->buffer;
			}
			if (!RB_IsNull(cmd->dispatch.sbuffer))
			{
				RB_D3d11Buffer_* pool_data = RB_PoolFetch_(&rt->bufferpool, cmd->dispatch.sbuffer.id);
				sbuffer = pool_data->uav;
			}
			for (intsize i = 0; i < ArrayLength(textures); ++i)
			{
				if (!RB_IsNull(cmd->dispatch.textures[i]))
				{
					RB_D3d11Texture2D_* pool_data = RB_PoolFetch_(&rt->texpool, cmd->dispatch.textures[i].id);
					textures[i] = pool_data->resource_view;
					samplers[i] = pool_data->sampler_state;
					
					texture_count = (UINT)(i+1);
				}
			}
			
			UINT count_x = cmd->dispatch.count_x;
			UINT count_y = cmd->dispatch.count_y;
			UINT count_z = cmd->dispatch.count_z;
			
			ID3D11DeviceContext_CSSetShader(D3d11.context, shader, NULL, 0);
			ID3D11DeviceContext_CSSetConstantBuffers(D3d11.context, 0, !!cbuffer, cbuffer ? &cbuffer : NULL);
			ID3D11DeviceContext_CSSetUnorderedAccessViews(D3d11.context, 0, !!sbuffer, sbuffer ? &sbuffer : NULL, (UINT[1]) { 0 });
			ID3D11DeviceContext_CSSetShaderResources(D3d11.context, 0, texture_count, textures);
			ID3D11DeviceContext_CSSetSamplers(D3d11.context, 0, texture_count, samplers);
			ID3D11DeviceContext_Dispatch(D3d11.context, count_x, count_y, count_z);
		} break;
	}
}

static void
RB_SetupD3d11Runtime_(RB_Ctx* ctx)
{
	RB_D3d11Runtime_* rt = ArenaPushStruct(ctx->arena, RB_D3d11Runtime_);
	ctx->rt = rt;
	ctx->rt_is_valid_handle = RB_D3d11IsValidHandle_;
	ctx->rt_free_ctx = RB_D3d11FreeCtx_;
	ctx->rt_resource = RB_D3d11Resource_;
	ctx->rt_cmd = RB_D3d11Command_;
	
	//- Default resources
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
	
	D3d11Call(ID3D11Device_CreateRasterizerState(D3d11.device, &rasterizer_desc, &rt->rasterizer_state));
	
	D3D11_SAMPLER_DESC linear_sampler_desc = {
		.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR,
		.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP,
		.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP,
		.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP,
		.MinLOD = -FLT_MAX,
		.MaxLOD = FLT_MAX,
		.ComparisonFunc = D3D11_COMPARISON_NEVER,
	};
	
	D3d11Call(ID3D11Device_CreateSamplerState(D3d11.device, &linear_sampler_desc, &rt->linear_sampler));
	
	D3D11_SAMPLER_DESC nearest_sampler_desc = {
		.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT,
		.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP,
		.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP,
		.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP,
		.MinLOD = -FLT_MAX,
		.MaxLOD = FLT_MAX,
		.ComparisonFunc = D3D11_COMPARISON_NEVER,
	};
	
	D3d11Call(ID3D11Device_CreateSamplerState(D3d11.device, &nearest_sampler_desc, &rt->nearest_sampler));
	
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
	
	D3d11Call(ID3D11Device_CreateBlendState(D3d11.device, &blend_desc, &rt->blend_state));
	
	D3D11_DEPTH_STENCIL_DESC depth_stencil_desc = {
		.DepthEnable = false,
		.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL,
		.DepthFunc = D3D11_COMPARISON_LESS,
	};
	
	D3d11Call(ID3D11Device_CreateDepthStencilState(D3d11.device, &depth_stencil_desc, &rt->depth_state));
	
	//- Capabilities
	D3D_FEATURE_LEVEL feature_level = ID3D11Device_GetFeatureLevel(D3d11.device);
	RB_Capabilities caps = {
		.backend_api = StrInit("Direct3D 11"),
		.driver_renderer = StrInit("Unknown"),
		.driver_vendor = StrInit("TODO"),
		.driver_version = StrInit("TODO"),
	};
	
	switch (feature_level)
	{
		default: break;
		//case D3D_FEATURE_LEVEL_12_2: caps.driver_renderer = Str("Feature Level 12_2"); break;
		case D3D_FEATURE_LEVEL_12_1: caps.driver_renderer = Str("Feature Level 12_1"); break;
		case D3D_FEATURE_LEVEL_12_0: caps.driver_renderer = Str("Feature Level 12_0"); break;
		case D3D_FEATURE_LEVEL_11_1: caps.driver_renderer = Str("Feature Level 11_1"); break;
		case D3D_FEATURE_LEVEL_11_0: caps.driver_renderer = Str("Feature Level 11_0"); break;
		case D3D_FEATURE_LEVEL_10_1: caps.driver_renderer = Str("Feature Level 10_1"); break;
		case D3D_FEATURE_LEVEL_10_0: caps.driver_renderer = Str("Feature Level 10_0"); break;
		case D3D_FEATURE_LEVEL_9_3: caps.driver_renderer = Str("Feature Level 9_3"); break;
		case D3D_FEATURE_LEVEL_9_2: caps.driver_renderer = Str("Feature Level 9_2"); break;
		case D3D_FEATURE_LEVEL_9_1: caps.driver_renderer = Str("Feature Level 9_1"); break;
	}
	
	//-
	if (feature_level >= D3D_FEATURE_LEVEL_9_1) // NOTE(ljre): This should always be true, really
	{
		caps.max_texture_size = 2048;
		caps.max_render_target_textures = 1;
		caps.max_textures_per_drawcall = 8;
		caps.shader_type = RB_ShaderType_Hlsl40Level91;
		caps.supported_texture_formats[0] |= (1 << RB_TexFormat_D16);
		caps.supported_texture_formats[0] |= (1 << RB_TexFormat_D24S8);
		caps.supported_texture_formats[0] |= (1 << RB_TexFormat_R8);
		caps.supported_texture_formats[0] |= (1 << RB_TexFormat_RGBA8);
	}
	
	if (feature_level >= D3D_FEATURE_LEVEL_9_2)
	{
		caps.has_32bit_index = true;
		caps.has_separate_alpha_blend = true;
		caps.supported_texture_formats[0] |= (1 << RB_TexFormat_A8);
	}
	
	if (feature_level >= D3D_FEATURE_LEVEL_9_3)
	{
		caps.max_texture_size = 4096;
		caps.max_render_target_textures = 4;
		caps.shader_type = RB_ShaderType_Hlsl40Level93;
		caps.has_instancing = true;
		caps.has_f16_formats = true;
	}
	
	if (feature_level >= D3D_FEATURE_LEVEL_10_0)
	{
		caps.max_texture_size = 8192;
		caps.max_render_target_textures = 8;
		caps.shader_type = RB_ShaderType_Hlsl40;
		caps.supported_texture_formats[0] |= (1 << RB_TexFormat_RG8);
	}
	
	if (feature_level >= D3D_FEATURE_LEVEL_10_1)
	{
		
	}
	
	if (feature_level >= D3D_FEATURE_LEVEL_11_0)
	{
		caps.max_texture_size = 16384;
		caps.max_render_target_textures = 8;
		caps.has_compute_shaders = true;
		caps.has_structured_buffer = true;
	}
	
	if (feature_level >= D3D_FEATURE_LEVEL_11_1)
	{
		
	}
	
	//- Extras
	D3D11_FEATURE_DATA_SHADER_MIN_PRECISION_SUPPORT shader_min_precision_support = { 0 };
	if (SUCCEEDED(ID3D11Device_CheckFeatureSupport(D3d11.device, D3D11_FEATURE_SHADER_MIN_PRECISION_SUPPORT, &shader_min_precision_support, sizeof(shader_min_precision_support))))
	{
		caps.has_f16_shader_ops = !!(shader_min_precision_support.PixelShaderMinPrecision & D3D11_SHADER_MIN_PRECISION_16_BIT);
	}
	
	ctx->caps = caps;
}

#undef D3d11
#undef D3d11Call
