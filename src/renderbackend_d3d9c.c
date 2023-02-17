#define D3d9c (*g_graphics_context->d3d9c)
#define D3d9cCall(...) do { \
if (FAILED(__VA_ARGS__)) { \
if (Assert_IsDebuggerPresent_()) \
Debugbreak(); \
SafeAssert_OnFailure(#__VA_ARGS__, __FILE__, __LINE__, __func__); \
} \
} while (0)

//~ Globals
struct RB_D3d9cTexture2D_
{
	IDirect3DTexture9* texture;
}
typedef RB_D3d9cTexture2D_;

struct RB_D3d9cShader_
{
	IDirect3DVertexShader9* vertex_shader;
	IDirect3DPixelShader9* pixel_shader;
	IDirect3DVertexDeclaration9* vertex_declaration;
	uint32 divisor_count;
	uint32 divisors[RB_Limits_InputsPerShader];
}
typedef RB_D3d9cShader_;

struct RB_D3d9cVertexBuffer_
{
	IDirect3DVertexBuffer9* vertex_buffer;
}
typedef RB_D3d9cVertexBuffer_;

struct RB_D3d9cIndexBuffer_
{
	IDirect3DIndexBuffer9* index_buffer;
}
typedef RB_D3d9cIndexBuffer_;

struct RB_D3d9cUniformBuffer_
{
	vec4 floats[32];
}
typedef RB_D3d9cUniformBuffer_;

static RB_PoolOf_(RB_D3d9cTexture2D_, 512) g_d3d9c_texpool;
static RB_PoolOf_(RB_D3d9cShader_, 64) g_d3d9c_shaderpool;
static RB_PoolOf_(RB_D3d9cVertexBuffer_, 512) g_d3d9c_vbufpool;
static RB_PoolOf_(RB_D3d9cIndexBuffer_, 512) g_d3d9c_ibufpool;
static RB_PoolOf_(RB_D3d9cUniformBuffer_, 512) g_d3d9c_ubufpool;

static void
RB_InitD3d9c_(Arena* scratch_arena)
{
	Trace();
	
	
}

static void
RB_DeinitD3d9c_(Arena* scratch_arena)
{
	Trace();
	
	
}

static void
RB_CapabilitiesD3d9c_(RB_Capabilities* out_capabilities)
{
	*out_capabilities = (RB_Capabilities) {
		.backend_api = StrInit("Direct3D 9.0c"),
		.driver = StrInit("TODO: get driver info"),
		.max_texture_size = 2048,
	};
}

static void
RB_ResourceD3d9c_(Arena* scratch_arena, RB_ResourceCommand* commands)
{
	Trace();
	
	for (RB_ResourceCommand* cmd = commands; cmd; cmd = cmd->next)
	{
		Trace(); TraceName(RB_resource_cmd_names[cmd->kind]);
		
		Assert(cmd->handle);
		
		RB_Handle handle = *cmd->handle;
		
		switch (cmd->kind)
		{
			case 0: SafeAssert(false); break;
			
			case RB_ResourceCommandKind_MakeTexture2D:
			{
				const uint8* pixels = cmd->texture_2d.pixels;
				UINT width = (UINT)cmd->texture_2d.width;
				UINT height = (UINT)cmd->texture_2d.height;
				DWORD usage = (cmd->flag_dynamic) ? D3DUSAGE_DYNAMIC : 0;
				D3DPOOL pool = (cmd->flag_dynamic) ? D3DPOOL_DEFAULT : D3DPOOL_MANAGED;
				
				D3DFORMAT format = 0;
				uintsize format_size = 0;
				
				switch (cmd->texture_2d.channels)
				{
					default: SafeAssert(false); break;
					case 1: format = D3DFMT_A8; format_size = 1; break;
					case 4: format = D3DFMT_A8B8G8R8; format_size = 4; break;
				}
				
				IDirect3DTexture9* texture;
				D3d9cCall(IDirect3DDevice9_CreateTexture(D3d9c.device, width, height, 1, usage, format, pool, &texture, NULL));
				
				D3DLOCKED_RECT locked_rect;
				D3d9cCall(IDirect3DTexture9_LockRect(texture, 0, &locked_rect, NULL, 0));
				
				uintsize row_size = width * format_size;
				for (UINT i = 0; i < height; ++i)
					Mem_Copy((uint8*)locked_rect.pBits + i * locked_rect.Pitch, pixels + i * row_size, row_size);
				
				D3d9cCall(IDirect3DTexture9_UnlockRect(texture, 0));
				
				RB_D3d9cTexture2D_* pool_data = RB_PoolAlloc_(&g_d3d9c_texpool, &handle.id);
				pool_data->texture = texture;
			} break;
			
			case RB_ResourceCommandKind_MakeVertexBuffer:
			{
				DWORD usage = (cmd->flag_dynamic) ? D3DUSAGE_DYNAMIC : 0;
				UINT length = (UINT)cmd->buffer.size;
				D3DPOOL pool = (cmd->flag_dynamic) ? D3DPOOL_DEFAULT : D3DPOOL_MANAGED;
				
				IDirect3DVertexBuffer9* vbuffer;
				D3d9cCall(IDirect3DDevice9_CreateVertexBuffer(D3d9c.device, length, usage, 0, pool, &vbuffer, NULL));
				
				if (cmd->buffer.memory)
				{
					void* mem;
					D3d9cCall(IDirect3DVertexBuffer9_Lock(vbuffer, 0, 0, &mem, 0));
					Mem_Copy(mem, cmd->buffer.memory, length);
					D3d9cCall(IDirect3DVertexBuffer9_Unlock(vbuffer));
				}
				
				RB_D3d9cVertexBuffer_* pool_data = RB_PoolAlloc_(&g_d3d9c_vbufpool, &handle.id);
				pool_data->vertex_buffer = vbuffer;
			} break;
			
			case RB_ResourceCommandKind_MakeIndexBuffer:
			{
				DWORD usage = (cmd->flag_dynamic) ? D3DUSAGE_DYNAMIC : 0;
				D3DPOOL pool = (cmd->flag_dynamic) ? D3DPOOL_DEFAULT : D3DPOOL_MANAGED;
				UINT length = (UINT)cmd->buffer.size;
				D3DFORMAT format = 0;
				
				switch (cmd->buffer.index_type)
				{
					default: SafeAssert(false); break;
					case RB_IndexType_Uint16: format = D3DFMT_INDEX16; break;
					case RB_IndexType_Uint32: format = D3DFMT_INDEX32; break;
				}
				
				IDirect3DIndexBuffer9* ibuffer;
				D3d9cCall(IDirect3DDevice9_CreateIndexBuffer(D3d9c.device, length, usage, format, pool, &ibuffer, NULL));
				
				if (cmd->buffer.memory)
				{
					void* mem;
					D3d9cCall(IDirect3DIndexBuffer9_Lock(ibuffer, 0, 0, &mem, 0));
					Mem_Copy(mem, cmd->buffer.memory, length);
					D3d9cCall(IDirect3DIndexBuffer9_Unlock(ibuffer));
				}
				
				RB_D3d9cIndexBuffer_* pool_data = RB_PoolAlloc_(&g_d3d9c_ibufpool, &handle.id);
				pool_data->index_buffer = ibuffer;
			} break;
			
			case RB_ResourceCommandKind_MakeUniformBuffer:
			{
				RB_D3d9cUniformBuffer_* pool_data = RB_PoolAlloc_(&g_d3d9c_ubufpool, &handle.id);
				
				// TODO
				(void)pool_data;
			} break;
			
			case RB_ResourceCommandKind_MakeShader:
			{
				IDirect3DVertexShader9* vshader = NULL;
				IDirect3DPixelShader9* pshader = NULL;
				const DWORD* vshader_tokens = (const DWORD*)cmd->shader.d3d9c_vs_blob.data;
				const DWORD* pshader_tokens = (const DWORD*)cmd->shader.d3d9c_ps_blob.data;
				
				SafeAssert(vshader_tokens && pshader_tokens);
				D3d9cCall(IDirect3DDevice9_CreateVertexShader(D3d9c.device, vshader_tokens, &vshader));
				D3d9cCall(IDirect3DDevice9_CreatePixelShader(D3d9c.device, pshader_tokens, &pshader));
				
				IDirect3DVertexDeclaration9* vdecl = NULL;
				int32 element_count = 0, max_buffer_index = 0;
				D3DVERTEXELEMENT9 elements[RB_Limits_InputsPerShader*4+1] = { 0 };
				uint32 divisors[RB_Limits_InputsPerShader] = { 0 };
				
				for (int32 i = 0; i < ArrayLength(cmd->shader.input_layout); ++i)
				{
					if (!cmd->shader.input_layout[i].kind)
						break;
					
					RB_LayoutDesc curr_layout = cmd->shader.input_layout[i];
					
					SafeAssert(curr_layout.vbuffer_index < RB_Limits_InputsPerShader);
					max_buffer_index = Max(max_buffer_index, curr_layout.vbuffer_index);
					divisors[curr_layout.vbuffer_index] = curr_layout.divisor;
					
					switch (curr_layout.kind)
					{
						case 0: Assert(false); break;
						
						//case RB_LayoutDescKind_Float:
						//case RB_LayoutDescKind_Vec2:
						//case RB_LayoutDescKind_Vec3:
						//case RB_LayoutDescKind_Vec4:
						{
							D3DDECLTYPE decl_type;
							
							if (0) case RB_LayoutDescKind_Float: decl_type = D3DDECLTYPE_FLOAT1;
							if (0) case RB_LayoutDescKind_Vec2: decl_type = D3DDECLTYPE_FLOAT2;
							if (0) case RB_LayoutDescKind_Vec3: decl_type = D3DDECLTYPE_FLOAT3;
							if (0) case RB_LayoutDescKind_Vec4: decl_type = D3DDECLTYPE_FLOAT4;
							
							D3DVERTEXELEMENT9 element = {
								.Stream = (WORD)curr_layout.vbuffer_index,
								.Offset = (WORD)curr_layout.offset,
								.Type = (BYTE)decl_type,
								.Method = D3DDECLMETHOD_DEFAULT,
								.Usage = D3DDECLUSAGE_TEXCOORD,
								.UsageIndex = 0,
							};
							
							elements[element_count++] = element;
						} break;
						
						case RB_LayoutDescKind_Mat2:
						{
							D3DVERTEXELEMENT9 element = {
								.Stream = (WORD)curr_layout.vbuffer_index,
								.Offset = (WORD)curr_layout.offset,
								.Type = D3DDECLTYPE_FLOAT2,
								.Method = D3DDECLMETHOD_DEFAULT,
								.Usage = D3DDECLUSAGE_TEXCOORD,
								.UsageIndex = 1,
							};
							
							elements[element_count++] = element; ++element.UsageIndex; element.Offset += sizeof(float32[2]);
							elements[element_count++] = element;
						} break;
						
						case RB_LayoutDescKind_Mat3:
						{
							D3DVERTEXELEMENT9 element = {
								.Stream = (WORD)curr_layout.vbuffer_index,
								.Offset = (WORD)curr_layout.offset,
								.Type = D3DDECLTYPE_FLOAT3,
								.Method = D3DDECLMETHOD_DEFAULT,
								.Usage = D3DDECLUSAGE_TEXCOORD,
								.UsageIndex = 1,
							};
							
							elements[element_count++] = element; ++element.UsageIndex; element.Offset += sizeof(float32[3]);
							elements[element_count++] = element; ++element.UsageIndex; element.Offset += sizeof(float32[3]);
							elements[element_count++] = element;
						} break;
						
						case RB_LayoutDescKind_Mat4:
						{
							D3DVERTEXELEMENT9 element = {
								.Stream = (WORD)curr_layout.vbuffer_index,
								.Offset = (WORD)curr_layout.offset,
								.Type = D3DDECLTYPE_FLOAT3,
								.Method = D3DDECLMETHOD_DEFAULT,
								.Usage = D3DDECLUSAGE_TEXCOORD,
								.UsageIndex = 1,
							};
							
							elements[element_count++] = element; ++element.UsageIndex; element.Offset += sizeof(float32[4]);
							elements[element_count++] = element; ++element.UsageIndex; element.Offset += sizeof(float32[4]);
							elements[element_count++] = element; ++element.UsageIndex; element.Offset += sizeof(float32[4]);
							elements[element_count++] = element;
						} break;
					}
				}
				
				elements[element_count++] = (D3DVERTEXELEMENT9) D3DDECL_END();
				
				D3d9cCall(IDirect3DDevice9_CreateVertexDeclaration(D3d9c.device, elements, &vdecl));
				
				RB_D3d9cShader_* pool_data = RB_PoolAlloc_(&g_d3d9c_shaderpool, &handle.id);
				pool_data->vertex_shader = vshader;
				pool_data->pixel_shader = pshader;
				pool_data->vertex_declaration = vdecl;
				pool_data->divisor_count = max_buffer_index + 1;
				Mem_Copy(pool_data->divisors, divisors, sizeof(uint32) * (max_buffer_index + 1));
			} break;
			
			case RB_ResourceCommandKind_MakeRenderTarget:
			{
				SafeAssert(false);
			} break;
			
			case RB_ResourceCommandKind_MakeBlendState:
			{
				
			} break;
			
			case RB_ResourceCommandKind_MakeRasterizerState:
			{
				
			} break;
			
			//-
			
			case RB_ResourceCommandKind_UpdateVertexBuffer:
			{
				
			} break;
			
			case RB_ResourceCommandKind_UpdateIndexBuffer:
			{
				
			} break;
			
			case RB_ResourceCommandKind_UpdateUniformBuffer:
			{
				
			} break;
			
			case RB_ResourceCommandKind_UpdateTexture2D:
			{
				
			} break;
			
			//-
			
			case RB_ResourceCommandKind_FreeTexture2D:
			{
				
			} break;
			
			case RB_ResourceCommandKind_FreeVertexBuffer:
			{
				
			} break;
			
			case RB_ResourceCommandKind_FreeIndexBuffer:
			{
				
			} break;
			
			case RB_ResourceCommandKind_FreeUniformBuffer:
			{
				
			} break;
			
			case RB_ResourceCommandKind_FreeShader:
			{
				
			} break;
			
			case RB_ResourceCommandKind_FreeRenderTarget:
			{
				SafeAssert(false);
			} break;
			
			case RB_ResourceCommandKind_FreeBlendState:
			{
				
			} break;
			
			case RB_ResourceCommandKind_FreeRasterizerState:
			{
				
			} break;
			
		}
		
		*cmd->handle = handle;
	}
}

static void
RB_DrawD3d9c_(Arena* scratch_arena, RB_DrawCommand* commands, int32 default_width, int32 default_height)
{
	Trace();
	
	D3DVIEWPORT9 viewport = {
		.X = 0,
		.Y = 0,
		.Width = default_width,
		.Height= default_height,
		.MinZ = 0.0f,
		.MaxZ = 1.0f,
	};
	
	D3d9cCall(IDirect3DDevice9_BeginScene(D3d9c.device));
	D3d9cCall(IDirect3DDevice9_SetViewport(D3d9c.device, &viewport));
	
	for (RB_DrawCommand* cmd = commands; cmd; cmd = cmd->next)
	{
		Trace(); TraceName(RB_draw_cmd_names[cmd->kind]);
		
		if (cmd->resources_cmd)
			RB_ResourceD3d9c_(scratch_arena, cmd->resources_cmd);
		
		switch (cmd->kind)
		{
			case 0: SafeAssert(false); break;
			
			case RB_DrawCommandKind_Clear:
			{
				SafeAssert(cmd->clear.flag_color);
				
				DWORD ds_flags = 0;
				if (cmd->clear.flag_depth)
					ds_flags |= D3DCLEAR_ZBUFFER;
				if (cmd->clear.flag_stencil)
					ds_flags |= D3DCLEAR_STENCIL;
				
				D3DCOLOR color = D3DCOLOR_RGBA(
				(int32)(cmd->clear.color[0] * 255.0f),
				(int32)(cmd->clear.color[1] * 255.0f),
				(int32)(cmd->clear.color[2] * 255.0f),
				(int32)(cmd->clear.color[3] * 255.0f));
				
				D3d9cCall(IDirect3DDevice9_Clear(D3d9c.device, 0, NULL, D3DCLEAR_TARGET | ds_flags, color, 1.0f, 0));
			} break;
			
			case RB_DrawCommandKind_ApplyBlendState:
			{
				
			} break;
			
			case RB_DrawCommandKind_ApplyRasterizerState:
			{
				
			} break;
			
			case RB_DrawCommandKind_ApplyRenderTarget:
			{
				
			} break;
			
			case RB_DrawCommandKind_DrawCall:
			{
				//- Stuff we're going to fetch
				IDirect3DVertexShader9* vshader = NULL;
				IDirect3DPixelShader9* pshader = NULL;
				IDirect3DIndexBuffer9* ibuffer = NULL;
				IDirect3DVertexDeclaration9* vdecl = NULL;
				IDirect3DBaseTexture9* textures[RB_Limits_SamplersPerDrawCall] = { 0 };
				IDirect3DVertexBuffer9* vbuffers[RB_Limits_MaxVertexBuffersPerDrawCall] = { 0 };
				UINT offsets[RB_Limits_MaxVertexBuffersPerDrawCall] = { 0 };
				UINT strides[RB_Limits_MaxVertexBuffersPerDrawCall] = { 0 };
				UINT divisors[RB_Limits_InputsPerShader] = { 0 };
				
				//- Shader
				SafeAssert(cmd->drawcall.shader);
				
				RB_D3d9cShader_* shader_pool_data = RB_PoolFetch_(&g_d3d9c_shaderpool, cmd->drawcall.shader->id);
				vshader = shader_pool_data->vertex_shader;
				pshader = shader_pool_data->pixel_shader;
				vdecl = shader_pool_data->vertex_declaration;
				
				for (int32 i = 0; i < shader_pool_data->divisor_count; ++i)
				{
					if (shader_pool_data->divisors[i])
						divisors[i] = shader_pool_data->divisors[i] | (D3DSTREAMSOURCE_INSTANCEDATA);
					else
						divisors[i] = (UINT)cmd->drawcall.index_count | (D3DSTREAMSOURCE_INDEXEDDATA);
				}
				
				//- Buffers
				SafeAssert(cmd->drawcall.ibuffer);
				
				RB_D3d9cIndexBuffer_* ibuffer_pool_data = RB_PoolFetch_(&g_d3d9c_ibufpool, cmd->drawcall.ibuffer->id);
				ibuffer = ibuffer_pool_data->index_buffer;
				
				for (int32 i = 0; i < ArrayLength(vbuffers); ++i)
				{
					if (!cmd->drawcall.vbuffers[i])
						break;
					
					RB_D3d9cVertexBuffer_* vbuffer_pool_data = RB_PoolFetch_(&g_d3d9c_vbufpool, cmd->drawcall.vbuffers[i]->id);
					vbuffers[i] = vbuffer_pool_data->vertex_buffer;
					offsets[i] = cmd->drawcall.vbuffer_offsets[i];
					strides[i] = cmd->drawcall.vbuffer_strides[i];
				}
				
				//- Textures
				for (int32 i = 0; i < ArrayLength(textures); ++i)
				{
					if (!cmd->drawcall.samplers[i].handle)
						break;
					
					RB_D3d9cTexture2D_* pool_data = RB_PoolFetch_(&g_d3d9c_texpool, cmd->drawcall.samplers[i].handle->id);
					textures[i] = (IDirect3DBaseTexture9*)pool_data->texture;
				}
				
				//- Draw call
				UINT index_count = (UINT)cmd->drawcall.index_count;
				UINT instance_count = (UINT)cmd->drawcall.instance_count;
				
				D3d9cCall(IDirect3DDevice9_SetVertexShader(D3d9c.device, vshader));
				D3d9cCall(IDirect3DDevice9_SetPixelShader(D3d9c.device, pshader));
				D3d9cCall(IDirect3DDevice9_SetVertexDeclaration(D3d9c.device, vdecl));
				for (int32 i = 0; i < ArrayLength(vbuffers) && vbuffers[i]; ++i)
					D3d9cCall(IDirect3DDevice9_SetStreamSource(D3d9c.device, (UINT)i, vbuffers[i], offsets[i], strides[i]));
				for (int32 i = 0; i < ArrayLength(divisors) && divisors[i]; ++i)
					D3d9cCall(IDirect3DDevice9_SetStreamSourceFreq(D3d9c.device, (UINT)i, divisors[i]));
				for (int32 i = 0; i < ArrayLength(textures) && textures[i]; ++i)
					D3d9cCall(IDirect3DDevice9_SetTexture(D3d9c.device, (UINT)i, textures[i]));
				D3d9cCall(IDirect3DDevice9_SetIndices(D3d9c.device, ibuffer));
				D3d9cCall(IDirect3DDevice9_DrawIndexedPrimitive(D3d9c.device, D3DPT_TRIANGLELIST, 0, 0, index_count, 0, instance_count));
			} break;
		}
	}
	
	D3d9cCall(IDirect3DDevice9_EndScene(D3d9c.device));
}

#undef D3d9c
#undef D3d9cCall
