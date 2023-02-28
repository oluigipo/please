#define GL (*g_graphics_context->opengl)

struct RB_OpenGLBuffer_
{
	uint32 id;
	uint32 index_type; // used if index buffer.
}
typedef RB_OpenGLBuffer_;

struct RB_OpenGLShader_
{
	uint32 program_id;
	
	RB_LayoutDesc input_layout[8];
}
typedef RB_OpenGLShader_;

struct RB_OpenGLBlend_
{
	bool enable;
	uint32 source;
	uint32 dest;
	uint32 op;
	uint32 source_alpha;
	uint32 dest_alpha;
	uint32 op_alpha;
}
typedef RB_OpenGLBlend_;

struct RB_OpenGLRasterizer_
{
	uint32 polygon_mode;
	uint32 cull_mode;
	uint32 frontface;
	
	bool flag_enable_cullface : 1;
	bool flag_depth_test : 1;
	bool flag_scissor : 1;
}
typedef RB_OpenGLRasterizer_;

static RB_PoolOf_(RB_OpenGLShader_, 64) g_ogl_shaderpool;
static RB_PoolOf_(RB_OpenGLBlend_, 16) g_ogl_blendpool;
static RB_PoolOf_(RB_OpenGLRasterizer_, 16) g_ogl_rasterizerpool;
static RB_PoolOf_(RB_OpenGLBuffer_, 512) g_ogl_bufferpool;

#ifdef CONFIG_DEBUG
static void APIENTRY
RB_OpenGLDebugMessageCallback_(GLenum source, GLenum type, GLuint id, GLenum severity,
	GLsizei length, const GLchar* message, const void* user_param)
{
	const char* type_str = "Debug Message";
	
	if (type == GL_DEBUG_TYPE_ERROR)
	{
		type_str = "Error";
		
		if (Assert_IsDebuggerPresent_())
			Debugbreak();
	}
	
	OS_DebugLog("OpenGL %s:\n\tType: 0x%x\n\tID: %u\n\tSeverity: 0x%x\n\tMessage: %.*s\n", type_str, type, id, severity, (int)length, message);
}
#endif //CONFIG_DEBUG

static void
RB_InitOpenGL_(Arena* scratch_arena)
{
#ifdef CONFIG_DEBUG
	GL.glEnable(GL_DEBUG_OUTPUT);
	GL.glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
	GL.glDebugMessageCallback(RB_OpenGLDebugMessageCallback_, NULL);
#endif //CONFIG_DEBUG
	
	GL.glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	
	GL.glDisable(GL_DEPTH_TEST);
	GL.glDisable(GL_CULL_FACE);
	GL.glEnable(GL_BLEND);
	GL.glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

static void
RB_DeinitOpenGL_(Arena* scratch_arena)
{
}

static void
RB_CapabilitiesOpenGL_(RB_Capabilities* out_capabilities)
{
	int32 max_texture_size;
	String driver;
	
	GL.glGetIntegerv(GL_MAX_TEXTURE_SIZE, &max_texture_size);
	
	const uint8* driver_cstr = GL.glGetString(GL_VENDOR);
	driver = StrMake(Mem_Strlen((char*)driver_cstr), driver_cstr);
	
	*out_capabilities = (RB_Capabilities) {
		.backend_api = StrInit("OpenGL 3.3"),
		.driver = driver,
		.max_texture_size = max_texture_size,
	};
}

static void
RB_ResourceOpenGL_(Arena* scratch_arena, RB_ResourceCommand* commands)
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
				int32 width = cmd->texture_2d.width;
				int32 height = cmd->texture_2d.height;
				int32 channels = cmd->texture_2d.channels;
				const void* pixels = cmd->texture_2d.pixels;
				
				SafeAssert(width && height && channels > 0 && channels <= 4);
				
				uint32 id;
				const int32 formats[4] = { GL_RED, GL_RG, GL_RGB, GL_RGBA, };
				int32 format = formats[channels - 1];
				
				bool mag_linear = cmd->texture_2d.flag_linear_filtering;
				bool min_linear = mag_linear;
				
				GL.glGenTextures(1, &id);
				GL.glBindTexture(GL_TEXTURE_2D, id);
				
				GL.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, mag_linear ? GL_LINEAR : GL_NEAREST);
				GL.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, min_linear ? GL_LINEAR : GL_NEAREST);
				GL.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
				GL.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
				
				{
					Trace(); TraceName(Str("glTexImage2D"));
					GL.glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, pixels);
				}
				
				GL.glBindTexture(GL_TEXTURE_2D, 0);
				
				handle.id = id;
			} break;
			
			//case RB_ResourceCommandKind_MakeVertexBuffer:
			//case RB_ResourceCommandKind_MakeIndexBuffer:
			//case RB_ResourceCommandKind_MakeUniformBuffer:
			{
				uint32 kind;
				
				if (0) case RB_ResourceCommandKind_MakeVertexBuffer: kind = GL_ARRAY_BUFFER;
				if (0) case RB_ResourceCommandKind_MakeIndexBuffer: kind = GL_ELEMENT_ARRAY_BUFFER;
				if (0) case RB_ResourceCommandKind_MakeUniformBuffer: kind = GL_UNIFORM_BUFFER;
				
				uint32 id;
				uint32 usage = (cmd->flag_dynamic) ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW;
				
				GL.glGenBuffers(1, &id);
				GL.glBindBuffer(kind, id);
				GL.glBufferData(kind, cmd->buffer.size, cmd->buffer.memory, usage);
				GL.glBindBuffer(kind, 0);
				
				RB_OpenGLBuffer_* pool_data = RB_PoolAlloc_(&g_ogl_bufferpool, &handle.id);
				pool_data->id = id;
				
				if (cmd->kind == RB_ResourceCommandKind_MakeIndexBuffer)
				{
					switch (cmd->buffer.index_type)
					{
						case RB_IndexType_Uint16: pool_data->index_type = GL_UNSIGNED_SHORT; break;
						case RB_IndexType_Uint32: pool_data->index_type = GL_UNSIGNED_INT; break;
					}
				}
			} break;
			
			case RB_ResourceCommandKind_MakeShader:
			{
				char info[512];
				int32 success = true;
				
				const char* vertex_shader_source = Arena_PushCString(scratch_arena, cmd->shader.gl_vs_src);
				const char* fragment_shader_source = Arena_PushCString(scratch_arena, cmd->shader.gl_fs_src);
				
				// Compile Vertex Shader
				uint32 vertex_shader = GL.glCreateShader(GL_VERTEX_SHADER);
				GL.glShaderSource(vertex_shader, 1, &vertex_shader_source, NULL);
				GL.glCompileShader(vertex_shader);
				
				// Check for errors
				GL.glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &success);
				if (!success)
				{
					GL.glGetShaderInfoLog(vertex_shader, sizeof(info), NULL, info);
					OS_DebugLog("Vertex Shader Error:\n\n%s", info);
					
					GL.glDeleteShader(vertex_shader);
					break;
				}
				
				// Compile Fragment Shader
				uint32 fragment_shader = GL.glCreateShader(GL_FRAGMENT_SHADER);
				GL.glShaderSource(fragment_shader, 1, &fragment_shader_source, NULL);
				GL.glCompileShader(fragment_shader);
				
				// Check for errors
				GL.glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &success);
				if (!success)
				{
					GL.glGetShaderInfoLog(fragment_shader, sizeof(info), NULL, info);
					OS_DebugLog("Fragment Shader Error:\n\n%s", info);
					
					GL.glDeleteShader(vertex_shader);
					GL.glDeleteShader(fragment_shader);
					break;
				}
				
				// Link
				uint32 program = GL.glCreateProgram();
				GL.glAttachShader(program, vertex_shader);
				GL.glAttachShader(program, fragment_shader);
				GL.glLinkProgram(program);
				
				// Check for errors
				GL.glGetProgramiv(program, GL_LINK_STATUS, &success);
				if (!success)
				{
					GL.glGetProgramInfoLog(program, sizeof(info), NULL, info);
					OS_DebugLog("Shader Linking Error:\n\n%s", info);
					
					GL.glDeleteProgram(program);
					GL.glDeleteShader(vertex_shader);
					GL.glDeleteShader(fragment_shader);
					break;
				}
				
				// Clean-up
				GL.glDeleteShader(vertex_shader);
				GL.glDeleteShader(fragment_shader);
				
				RB_OpenGLShader_* pool_data = RB_PoolAlloc_(&g_ogl_shaderpool, &handle.id);
				pool_data->program_id = program;
				Mem_Copy(pool_data->input_layout, cmd->shader.input_layout, sizeof(pool_data->input_layout));
			} break;
			
			case RB_ResourceCommandKind_MakeRenderTarget:
			{
				SafeAssert(false);
			} break;
			
			case RB_ResourceCommandKind_MakeBlendState:
			{
				static const uint32 functable[] = {
					[RB_BlendFunc_Zero] = GL_ZERO,
					[RB_BlendFunc_One] = GL_ONE,
					[RB_BlendFunc_SrcColor] = GL_SRC_COLOR,
					[RB_BlendFunc_InvSrcColor] = GL_ONE_MINUS_SRC_COLOR,
					[RB_BlendFunc_DstColor] = GL_DST_COLOR,
					[RB_BlendFunc_InvDstColor] = GL_ONE_MINUS_DST_COLOR,
					[RB_BlendFunc_SrcAlpha] = GL_SRC_ALPHA,
					[RB_BlendFunc_InvSrcAlpha] = GL_ONE_MINUS_SRC_ALPHA,
					[RB_BlendFunc_DstAlpha] = GL_DST_ALPHA,
					[RB_BlendFunc_InvDstAlpha] = GL_ONE_MINUS_DST_ALPHA,
				};
				
				static const uint32 optable[] = {
					[RB_BlendOp_Add] = GL_FUNC_ADD,
					[RB_BlendOp_Subtract] = GL_FUNC_SUBTRACT,
				};
				
				SafeAssert(cmd->blend.source >= 0 && cmd->blend.source < ArrayLength(functable));
				SafeAssert(cmd->blend.dest >= 0 && cmd->blend.dest < ArrayLength(functable));
				SafeAssert(cmd->blend.source_alpha >= 0 && cmd->blend.source_alpha < ArrayLength(functable));
				SafeAssert(cmd->blend.dest_alpha >= 0 && cmd->blend.dest_alpha < ArrayLength(functable));
				SafeAssert(cmd->blend.op >= 0 && cmd->blend.op < ArrayLength(optable));
				SafeAssert(cmd->blend.op_alpha >= 0 && cmd->blend.op_alpha < ArrayLength(optable));
				
				RB_OpenGLBlend_* pool_data = RB_PoolAlloc_(&g_ogl_blendpool, &handle.id);
				pool_data->enable = cmd->blend.enable;
				pool_data->source = cmd->blend.source ? functable[cmd->blend.source] : GL_ONE;
				pool_data->dest = cmd->blend.dest ? functable[cmd->blend.dest] : GL_ZERO;
				pool_data->op = cmd->blend.op ? optable[cmd->blend.op] : GL_FUNC_ADD;
				pool_data->source = cmd->blend.source_alpha ? functable[cmd->blend.source_alpha] : GL_ONE;
				pool_data->dest = cmd->blend.dest_alpha ? functable[cmd->blend.dest_alpha] : GL_ZERO;
				pool_data->op = cmd->blend.op_alpha ? optable[cmd->blend.op_alpha] : GL_FUNC_ADD;
			} break;
			
			case RB_ResourceCommandKind_MakeRasterizerState:
			{
				static const uint32 filltable[] = {
					[0] = GL_FILL,
					[RB_FillMode_Solid] = GL_FILL,
					[RB_FillMode_Wireframe] = GL_LINE,
				};
				
				static const uint32 culltable[] = {
					[RB_CullMode_None] = 0,
					[RB_CullMode_Front] = GL_FRONT,
					[RB_CullMode_Back] = GL_BACK,
				};
				
				SafeAssert(cmd->rasterizer.fill_mode >= 0 && cmd->rasterizer.fill_mode < ArrayLength(filltable));
				SafeAssert(cmd->rasterizer.cull_mode >= 0 && cmd->rasterizer.cull_mode < ArrayLength(culltable));
				
				RB_OpenGLRasterizer_* pool_data = RB_PoolAlloc_(&g_ogl_rasterizerpool, &handle.id);
				pool_data->polygon_mode = filltable[cmd->rasterizer.fill_mode];
				pool_data->cull_mode = culltable[cmd->rasterizer.cull_mode];
				pool_data->frontface = cmd->rasterizer.flag_cw_backface ? GL_CCW : GL_CW;
				pool_data->flag_enable_cullface = (culltable[cmd->rasterizer.cull_mode] != 0);
				pool_data->flag_depth_test = cmd->rasterizer.flag_depth_test;
				pool_data->flag_scissor = cmd->rasterizer.flag_scissor;
			} break;
			
			//case RB_ResourceCommandKind_UpdateVertexBuffer:
			//case RB_ResourceCommandKind_UpdateIndexBuffer:
			//case RB_ResourceCommandKind_UpdateUniformBuffer:
			{
				uint32 kind;
				
				if (0) case RB_ResourceCommandKind_UpdateVertexBuffer: kind = GL_ARRAY_BUFFER;
				if (0) case RB_ResourceCommandKind_UpdateIndexBuffer: kind = GL_ELEMENT_ARRAY_BUFFER;
				if (0) case RB_ResourceCommandKind_UpdateUniformBuffer: kind = GL_UNIFORM_BUFFER;
				
				Assert(handle.id);
				
				RB_OpenGLBuffer_* pool_data = RB_PoolFetch_(&g_ogl_bufferpool, handle.id);
				
				uint32 id = pool_data->id;
				uint32 usage = GL_DYNAMIC_DRAW;
				
				GL.glBindBuffer(kind, id);
				if (!cmd->flag_subregion)
					GL.glBufferData(kind, cmd->buffer.size, cmd->buffer.memory, usage);
				else
					GL.glBufferSubData(kind, cmd->buffer.offset, cmd->buffer.size, cmd->buffer.memory);
				GL.glBindBuffer(kind, 0);
			} break;
			
			case RB_ResourceCommandKind_UpdateTexture2D:
			{
				int32 width = cmd->texture_2d.width;
				int32 height = cmd->texture_2d.height;
				int32 channels = cmd->texture_2d.channels;
				const void* pixels = cmd->texture_2d.pixels;
				
				Assert(width && height && channels > 0 && channels <= 4);
				
				uint32 id = handle.id;
				const int32 formats[4] = { GL_RED, GL_RG, GL_RGB, GL_RGBA, };
				int32 format = formats[channels - 1];
				
				GL.glBindTexture(GL_TEXTURE_2D, id);
				if (cmd->flag_subregion)
					GL.glTexSubImage2D(GL_TEXTURE_2D, 0, cmd->texture_2d.xoffset, cmd->texture_2d.yoffset, width, height, format, GL_UNSIGNED_BYTE, pixels);
				else
					GL.glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, pixels);
				GL.glBindTexture(GL_TEXTURE_2D, 0);
			} break;
			
			case RB_ResourceCommandKind_FreeTexture2D:
			{
				Assert(handle.id);
				
				uint32 id = handle.id;
				
				GL.glDeleteTextures(1, &id);
				
				handle.id = 0;
			} break;
			
			case RB_ResourceCommandKind_FreeVertexBuffer:
			case RB_ResourceCommandKind_FreeIndexBuffer:
			case RB_ResourceCommandKind_FreeUniformBuffer:
			{
				Assert(handle.id);
				
				RB_OpenGLBuffer_* pool_data = RB_PoolFetch_(&g_ogl_bufferpool, handle.id);
				
				uint32 id = pool_data->id;
				GL.glDeleteBuffers(1, &id);
				
				RB_PoolFree_(&g_ogl_bufferpool, handle.id);
				handle.id = 0;
			} break;
			
			case RB_ResourceCommandKind_FreeShader:
			{
				Assert(handle.id);
				RB_OpenGLShader_* pool_data = RB_PoolFetch_(&g_ogl_shaderpool, handle.id);
				
				GL.glDeleteProgram(pool_data->program_id);
				
				RB_PoolFree_(&g_ogl_shaderpool, handle.id);
				handle.id = 0;
			} break;
			
			case RB_ResourceCommandKind_FreeRenderTarget:
			{
				SafeAssert(false);
			} break;
			
			case RB_ResourceCommandKind_FreeBlendState:
			{
				Assert(handle.id);
				
				RB_PoolFree_(&g_ogl_blendpool, handle.id);
				handle.id = 0;
			} break;
			
			case RB_ResourceCommandKind_FreeRasterizerState:
			{
				Assert(handle.id);
				
				RB_PoolFree_(&g_ogl_rasterizerpool, handle.id);
				handle.id = 0;
			} break;
		}
		
		*cmd->handle = handle;
	}
}

static void
RB_DrawOpenGL_(Arena* scratch_arena, RB_DrawCommand* commands, int32 default_width, int32 default_height)
{
	Trace();
	
	GL.glViewport(0, 0, default_width, default_height);
	
	// default blend
	GL.glEnable(GL_BLEND);
	GL.glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	GL.glBlendEquation(GL_FUNC_ADD);
	
	// default rasterizer
	GL.glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	GL.glFrontFace(GL_CCW);
	GL.glDisable(GL_DEPTH_TEST);
	GL.glDisable(GL_SCISSOR_TEST);
	
	for (RB_DrawCommand* cmd = commands; cmd; cmd = cmd->next)
	{
		Trace(); TraceName(RB_draw_cmd_names[cmd->kind]);
		Assert(cmd->kind != 0);
		
		if (cmd->resources_cmd)
			RB_ResourceOpenGL_(scratch_arena, cmd->resources_cmd);
		
		switch (cmd->kind)
		{
			case 0: SafeAssert(false); break;
			
			case RB_DrawCommandKind_Clear:
			{
				uint32 bits = 0;
				
				if (cmd->clear.flag_color)
					bits |= GL_COLOR_BUFFER_BIT;
				if (cmd->clear.flag_depth)
					bits |= GL_DEPTH_BUFFER_BIT;
				if (cmd->clear.flag_stencil)
					bits |= GL_STENCIL_BUFFER_BIT;
				
				if (bits)
				{
					float32 r = cmd->clear.color[0];
					float32 g = cmd->clear.color[1];
					float32 b = cmd->clear.color[2];
					float32 a = cmd->clear.color[3];
					
					GL.glClearColor(r, g, b, a);
					GL.glClear(bits);
				}
			} break;
			
			case RB_DrawCommandKind_ApplyBlendState:
			{
				if (cmd->apply.handle)
				{
					RB_OpenGLBlend_* pool_data = RB_PoolFetch_(&g_ogl_blendpool, cmd->apply.handle->id);
					
					if (!pool_data->enable)
						GL.glDisable(GL_BLEND);
					else
					{
						GL.glEnable(GL_BLEND);
						GL.glBlendFuncSeparate(pool_data->source, pool_data->dest, pool_data->source_alpha, pool_data->dest_alpha);
						GL.glBlendEquationSeparate(pool_data->op, pool_data->op_alpha);
					}
				}
				else
				{
					GL.glEnable(GL_BLEND);
					GL.glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
					GL.glBlendEquation(GL_FUNC_ADD);
				}
			} break;
			
			case RB_DrawCommandKind_ApplyRasterizerState:
			{
				if (cmd->apply.handle)
				{
					RB_OpenGLRasterizer_* pool_data = RB_PoolFetch_(&g_ogl_rasterizerpool, cmd->apply.handle->id);
					
					GL.glPolygonMode(GL_FRONT_AND_BACK, pool_data->polygon_mode);
					GL.glFrontFace(pool_data->frontface);
					
					if (pool_data->flag_enable_cullface)
					{
						GL.glEnable(GL_CULL_FACE);
						GL.glCullFace(pool_data->cull_mode);
					}
					else
						GL.glDisable(GL_CULL_FACE);
					
					if (pool_data->flag_depth_test)
						GL.glEnable(GL_DEPTH_TEST);
					else
						GL.glDisable(GL_DEPTH_TEST);
					
					if (pool_data->flag_scissor)
						GL.glEnable(GL_SCISSOR_TEST);
					else
						GL.glDisable(GL_SCISSOR_TEST);
				}
				else
				{
					GL.glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
					GL.glFrontFace(GL_CCW);
					GL.glDisable(GL_DEPTH_TEST);
					GL.glDisable(GL_SCISSOR_TEST);
					GL.glDisable(GL_CULL_FACE);
				}
			} break;
			
			case RB_DrawCommandKind_ApplyRenderTarget:
			{
				SafeAssert(false);
			} break;
			
			case RB_DrawCommandKind_DrawCall:
			{
				// Buffers and Shader
				SafeAssert(cmd->drawcall.shader && cmd->drawcall.ibuffer);
				
				RB_OpenGLShader_* shader_pool_data = RB_PoolFetch_(&g_ogl_shaderpool, cmd->drawcall.shader->id);
				RB_OpenGLBuffer_* ibuffer_pool_data = RB_PoolFetch_(&g_ogl_bufferpool, cmd->drawcall.ibuffer->id);
				
				const uint32 shader = shader_pool_data->program_id;
				uint32 ibuffer = ibuffer_pool_data->id;
				uint32 vbuffers[ArrayLength(cmd->drawcall.vbuffers)] = { 0 };
				
				for (intsize i = 0; i < ArrayLength(cmd->drawcall.vbuffers); ++i)
				{
					if (cmd->drawcall.vbuffers[i])
					{
						uint32 index = cmd->drawcall.vbuffers[i]->id;
						RB_OpenGLBuffer_* pool_data = RB_PoolFetch_(&g_ogl_bufferpool, index);
						
						vbuffers[i] = pool_data->id;
					}
				}
				
				// Vertex Layout
				uint32 vao;
				
				GL.glGenVertexArrays(1, &vao);
				GL.glBindVertexArray(vao);
				GL.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibuffer);
				
				for (intsize i = 0; i < ArrayLength(shader_pool_data->input_layout); ++i)
				{
					const RB_LayoutDesc* layout = &shader_pool_data->input_layout[i];
					
					if (!layout->kind)
						break;
					
					SafeAssert(layout->vbuffer_index < ArrayLength(vbuffers));
					GL.glBindBuffer(GL_ARRAY_BUFFER, vbuffers[layout->vbuffer_index]);
					
					uint32 loc = layout->gl_location;
					uint32 stride = cmd->drawcall.vbuffer_strides[layout->vbuffer_index];
					uintptr offset = cmd->drawcall.vbuffer_offsets[layout->vbuffer_index] + layout->offset;
					
					switch (layout->kind)
					{
						//case RB_LayoutDescKind_Vec2:
						//case RB_LayoutDescKind_Vec3:
						//case RB_LayoutDescKind_Vec4:
						{
							uint32 count;
							
							if (0) case RB_LayoutDescKind_Float: count = 1;
							if (0) case RB_LayoutDescKind_Vec2: count = 2;
							if (0) case RB_LayoutDescKind_Vec3: count = 3;
							if (0) case RB_LayoutDescKind_Vec4: count = 4;
							
							GL.glEnableVertexAttribArray(loc);
							GL.glVertexAttribDivisor(loc, layout->divisor);
							GL.glVertexAttribPointer(loc, count, GL_FLOAT, false, stride, (void*)offset);
						} break;
						
						case RB_LayoutDescKind_Mat2:
						{
							GL.glEnableVertexAttribArray(loc+0);
							GL.glEnableVertexAttribArray(loc+1);
							
							GL.glVertexAttribDivisor(loc+0, layout->divisor);
							GL.glVertexAttribDivisor(loc+1, layout->divisor);
							
							GL.glVertexAttribPointer(loc+0, 2, GL_FLOAT, false, stride, (void*)(offset + sizeof(float32[2])*0));
							GL.glVertexAttribPointer(loc+1, 2, GL_FLOAT, false, stride, (void*)(offset + sizeof(float32[2])*1));
						} break;
						
						case RB_LayoutDescKind_Mat3:
						{
							GL.glEnableVertexAttribArray(loc+0);
							GL.glEnableVertexAttribArray(loc+1);
							GL.glEnableVertexAttribArray(loc+2);
							
							GL.glVertexAttribDivisor(loc+0, layout->divisor);
							GL.glVertexAttribDivisor(loc+1, layout->divisor);
							GL.glVertexAttribDivisor(loc+2, layout->divisor);
							
							GL.glVertexAttribPointer(loc+0, 3, GL_FLOAT, false, stride, (void*)(offset + sizeof(float32[3])*0));
							GL.glVertexAttribPointer(loc+1, 3, GL_FLOAT, false, stride, (void*)(offset + sizeof(float32[3])*1));
							GL.glVertexAttribPointer(loc+2, 3, GL_FLOAT, false, stride, (void*)(offset + sizeof(float32[3])*2));
						} break;
						
						case RB_LayoutDescKind_Mat4:
						{
							GL.glEnableVertexAttribArray(loc+0);
							GL.glEnableVertexAttribArray(loc+1);
							GL.glEnableVertexAttribArray(loc+2);
							GL.glEnableVertexAttribArray(loc+3);
							
							GL.glVertexAttribDivisor(loc+0, layout->divisor);
							GL.glVertexAttribDivisor(loc+1, layout->divisor);
							GL.glVertexAttribDivisor(loc+2, layout->divisor);
							GL.glVertexAttribDivisor(loc+3, layout->divisor);
							
							GL.glVertexAttribPointer(loc+0, 4, GL_FLOAT, false, stride, (void*)(offset + sizeof(float32[4])*0));
							GL.glVertexAttribPointer(loc+1, 4, GL_FLOAT, false, stride, (void*)(offset + sizeof(float32[4])*1));
							GL.glVertexAttribPointer(loc+2, 4, GL_FLOAT, false, stride, (void*)(offset + sizeof(float32[4])*2));
							GL.glVertexAttribPointer(loc+3, 4, GL_FLOAT, false, stride, (void*)(offset + sizeof(float32[4])*3));
						} break;
						
						default: Assert(false); break;
					}
					
					GL.glBindBuffer(GL_ARRAY_BUFFER, 0);
				}
				
				// Uniforms
				GL.glUseProgram(shader);
				
				if (cmd->drawcall.ubuffer)
				{
					uint32 index = cmd->drawcall.ubuffer->id;
					RB_OpenGLBuffer_* pool_data = RB_PoolFetch_(&g_ogl_bufferpool, index);
					
					GL.glBindBuffer(GL_UNIFORM_BUFFER, pool_data->id);
					
					int32 location = GL.glGetUniformBlockIndex(shader, "UniformBuffer");
					SafeAssert(location != -1);
					
					GL.glUniformBlockBinding(shader, location, 0);
					GL.glBindBufferBase(GL_UNIFORM_BUFFER, 0, pool_data->id);
					
					GL.glBindBuffer(GL_UNIFORM_BUFFER, 0);
				}
				
				// Samplers
				uint32 sampler_count = 0;
				
				for (intsize i = 0; i < ArrayLength(cmd->drawcall.samplers); ++i)
				{
					const RB_SamplerDesc* sampler = &cmd->drawcall.samplers[i];
					
					if (!sampler->handle)
						continue;
					
					uint32 id = sampler->handle->id;
					int32 location = -1;
					
					{
						SafeAssert(i < 10);
						char name[] = "uTexture[N]";
						name[sizeof(name)-3] = (char)('0' + i);
						
						location = GL.glGetUniformLocation(shader, name);
						SafeAssert(location != -1);
					}
					
					GL.glActiveTexture(GL_TEXTURE0 + sampler_count);
					GL.glBindTexture(GL_TEXTURE_2D, id);
					GL.glUniform1i(location, sampler_count);
					
					++sampler_count;
				}
				
				// Index type
				uint32 index_type = ibuffer_pool_data->index_type;
				
				// Draw Call
				uint32 index_count = cmd->drawcall.index_count;
				uint32 instance_count = cmd->drawcall.instance_count;
				
				GL.glDrawElementsInstanced(GL_TRIANGLES, index_count, index_type, NULL, instance_count);
				
				// Done
				GL.glBindVertexArray(0);
				GL.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
				GL.glUseProgram(0);
				
				GL.glDeleteVertexArrays(1, &vao);
			} break;
		}
		
		// end of loop
	}
}

#undef GL
