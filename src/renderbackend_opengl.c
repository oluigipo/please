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
}
typedef RB_OpenGLShader_;

struct RB_OpenGLPipeline_
{
	bool flag_blend : 1;
	bool flag_enable_cullface : 1;
	bool flag_depth_test : 1;
	bool flag_scissor : 1;
	
	RB_Handle shader_handle;
	
	uint32 source;
	uint32 dest;
	uint32 op;
	uint32 source_alpha;
	uint32 dest_alpha;
	uint32 op_alpha;
	
	uint32 polygon_mode;
	uint32 cull_mode;
	uint32 frontface;
	
	RB_LayoutDesc input_layout[RB_Limits_MaxVertexInputs];
}
typedef RB_OpenGLPipeline_;

static RB_PoolOf_(RB_OpenGLShader_, 64) g_ogl_shaderpool;
static RB_PoolOf_(RB_OpenGLPipeline_, 16) g_ogl_pipelinepool;
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
	RB_Capabilities caps = {
		.backend_api = StrInit("OpenGL 3.3"),
		.shader_type = RB_ShaderType_Glsl33,
	};
	
	GL.glGetIntegerv(GL_MAX_TEXTURE_SIZE, &caps.max_texture_size);
	GL.glGetIntegerv(GL_MAX_COLOR_ATTACHMENTS, &caps.max_render_target_textures);
	
	const uint8* vendor_cstr = GL.glGetString(GL_VENDOR);
	caps.driver_vendor = StrMake(Mem_Strlen((char*)vendor_cstr), vendor_cstr);
	const uint8* renderer_cstr = GL.glGetString(GL_RENDERER);
	caps.driver_renderer = StrMake(Mem_Strlen((char*)renderer_cstr), renderer_cstr);
	const uint8* version_cstr = GL.glGetString(GL_VERSION);
	caps.driver_version = StrMake(Mem_Strlen((char*)version_cstr), version_cstr);
	
	caps.has_instancing = true;
	caps.has_32bit_index = true;
	caps.has_separate_alpha_blend = true;
	caps.has_compute_shaders = false;
	caps.has_16bit_float = false;
	
	*out_capabilities = caps;
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
			} break;
			
			case RB_ResourceCommandKind_MakeRenderTarget:
			{
				SafeAssert(false);
			} break;
			
			case RB_ResourceCommandKind_MakePipeline:
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
				
				SafeAssert(cmd->pipeline.blend_source >= 0 && cmd->pipeline.blend_source < ArrayLength(functable));
				SafeAssert(cmd->pipeline.blend_dest >= 0 && cmd->pipeline.blend_dest < ArrayLength(functable));
				SafeAssert(cmd->pipeline.blend_source_alpha >= 0 && cmd->pipeline.blend_source_alpha < ArrayLength(functable));
				SafeAssert(cmd->pipeline.blend_dest_alpha >= 0 && cmd->pipeline.blend_dest_alpha < ArrayLength(functable));
				SafeAssert(cmd->pipeline.blend_op >= 0 && cmd->pipeline.blend_op < ArrayLength(optable));
				SafeAssert(cmd->pipeline.blend_op_alpha >= 0 && cmd->pipeline.blend_op_alpha < ArrayLength(optable));
				
				static const uint32 filltable[] = {
					[RB_FillMode_Solid] = GL_FILL,
					[RB_FillMode_Wireframe] = GL_LINE,
				};
				
				static const uint32 culltable[] = {
					[RB_CullMode_None] = 0,
					[RB_CullMode_Front] = GL_FRONT,
					[RB_CullMode_Back] = GL_BACK,
				};
				
				SafeAssert(cmd->pipeline.fill_mode >= 0 && cmd->pipeline.fill_mode < ArrayLength(filltable));
				SafeAssert(cmd->pipeline.cull_mode >= 0 && cmd->pipeline.cull_mode < ArrayLength(culltable));
				
				RB_OpenGLPipeline_* pool_data = RB_PoolAlloc_(&g_ogl_pipelinepool, &handle.id);
				pool_data->flag_blend = cmd->pipeline.flag_blend;
				pool_data->source = cmd->pipeline.blend_source ? functable[cmd->pipeline.blend_source] : GL_ONE;
				pool_data->dest = cmd->pipeline.blend_dest ? functable[cmd->pipeline.blend_dest] : GL_ZERO;
				pool_data->op = cmd->pipeline.blend_op ? optable[cmd->pipeline.blend_op] : GL_FUNC_ADD;
				pool_data->source = cmd->pipeline.blend_source_alpha ? functable[cmd->pipeline.blend_source_alpha] : GL_ONE;
				pool_data->dest = cmd->pipeline.blend_dest_alpha ? functable[cmd->pipeline.blend_dest_alpha] : GL_ZERO;
				pool_data->op = cmd->pipeline.blend_op_alpha ? optable[cmd->pipeline.blend_op_alpha] : GL_FUNC_ADD;
				
				pool_data->polygon_mode = filltable[cmd->pipeline.fill_mode];
				pool_data->cull_mode = culltable[cmd->pipeline.cull_mode];
				pool_data->frontface = cmd->pipeline.flag_cw_backface ? GL_CCW : GL_CW;
				pool_data->flag_enable_cullface = (culltable[cmd->pipeline.cull_mode] != 0);
				pool_data->flag_depth_test = cmd->pipeline.flag_depth_test;
				pool_data->flag_scissor = cmd->pipeline.flag_scissor;
				
				SafeAssert(cmd->pipeline.shader);
				pool_data->shader_handle = *cmd->pipeline.shader;
				
				Mem_Copy(pool_data->input_layout, cmd->pipeline.input_layout, sizeof(pool_data->input_layout));
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
			
			case RB_ResourceCommandKind_FreePipeline:
			{
				Assert(handle.id);
				
				RB_PoolFree_(&g_ogl_pipelinepool, handle.id);
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
	
	RB_LayoutDesc input_layout[RB_Limits_MaxVertexInputs] = { 0 };
	uint32 current_program_id = 0;
	
	int32 uniform_block_location = -1;
	int32 samplers_locations[RB_Limits_MaxTexturesPerDrawCall];
	for (intsize i = 0; i < ArrayLength(samplers_locations); ++i)
		samplers_locations[i] = -1;
	
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
			
			case RB_DrawCommandKind_ApplyPipeline:
			{
				SafeAssert(cmd->apply.handle);
				RB_OpenGLPipeline_* pool_data = RB_PoolFetch_(&g_ogl_pipelinepool, cmd->apply.handle->id);
				RB_OpenGLShader_* shader_pool_data = RB_PoolFetch_(&g_ogl_shaderpool, pool_data->shader_handle.id);
				
				Mem_Copy(input_layout, pool_data->input_layout, sizeof(input_layout));
				
				if (current_program_id != shader_pool_data->program_id)
				{
					current_program_id = shader_pool_data->program_id;
					GL.glUseProgram(shader_pool_data->program_id);
					
					uniform_block_location = GL.glGetUniformBlockIndex(current_program_id, "UniformBuffer");
					
					for (intsize i = 0; i < ArrayLength(samplers_locations); ++i)
					{
						static_assert(ArrayLength(samplers_locations) < 10);
						char name[] = "uTexture[N]";
						name[sizeof(name)-3] = (char)('0' + i);
						
						samplers_locations[i] = GL.glGetUniformLocation(current_program_id, name);
					}
				}
				
				if (!pool_data->flag_blend)
					GL.glDisable(GL_BLEND);
				else
				{
					GL.glEnable(GL_BLEND);
					GL.glBlendFuncSeparate(pool_data->source, pool_data->dest, pool_data->source_alpha, pool_data->dest_alpha);
					GL.glBlendEquationSeparate(pool_data->op, pool_data->op_alpha);
				}
				
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
			} break;
			
			case RB_DrawCommandKind_ApplyRenderTarget:
			{
				SafeAssert(false);
			} break;
			
			//case RB_DrawCommandKind_DrawIndexed:
			//case RB_DrawCommandKind_DrawInstanced:
			{
				bool instanced;
				
				if (0) case RB_DrawCommandKind_DrawIndexed: instanced = false;
				if (0) case RB_DrawCommandKind_DrawInstanced: instanced = true;
				
				// Buffers
				SafeAssert(cmd->draw_instanced.ibuffer);
				
				RB_OpenGLBuffer_* ibuffer_pool_data = RB_PoolFetch_(&g_ogl_bufferpool, cmd->draw_instanced.ibuffer->id);
				
				uint32 ibuffer = ibuffer_pool_data->id;
				uint32 vbuffers[ArrayLength(cmd->draw_instanced.vbuffers)] = { 0 };
				
				for (intsize i = 0; i < ArrayLength(cmd->draw_instanced.vbuffers); ++i)
				{
					if (cmd->draw_instanced.vbuffers[i])
					{
						uint32 index = cmd->draw_instanced.vbuffers[i]->id;
						RB_OpenGLBuffer_* pool_data = RB_PoolFetch_(&g_ogl_bufferpool, index);
						
						vbuffers[i] = pool_data->id;
					}
				}
				
				// Vertex Layout
				uint32 vao;
				
				GL.glGenVertexArrays(1, &vao);
				GL.glBindVertexArray(vao);
				GL.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibuffer);
				
				uint32 location = 0;
				
				for (intsize i = 0; i < ArrayLength(input_layout); ++i)
				{
					const RB_LayoutDesc* layout = &input_layout[i];
					
					if (!layout->kind)
						break;
					
					SafeAssert(layout->vbuffer_index < ArrayLength(vbuffers));
					GL.glBindBuffer(GL_ARRAY_BUFFER, vbuffers[layout->vbuffer_index]);
					
					uint32 loc = location;
					uint32 stride = cmd->draw_instanced.vbuffer_strides[layout->vbuffer_index];
					uintptr offset = cmd->draw_instanced.vbuffer_offsets[layout->vbuffer_index] + layout->offset;
					
					switch (layout->kind)
					{
						//case RB_LayoutDescKind_Scalar:
						//case RB_LayoutDescKind_Vec2:
						//case RB_LayoutDescKind_Vec3:
						//case RB_LayoutDescKind_Vec4:
						{
							uint32 count;
							
							if (0) case RB_LayoutDescKind_Scalar: count = 1;
							if (0) case RB_LayoutDescKind_Vec2: count = 2;
							if (0) case RB_LayoutDescKind_Vec3: count = 3;
							if (0) case RB_LayoutDescKind_Vec4: count = 4;
							
							GL.glEnableVertexAttribArray(loc);
							GL.glVertexAttribDivisor(loc, layout->divisor);
							GL.glVertexAttribPointer(loc, count, GL_FLOAT, false, stride, (void*)offset);
							
							location += 1;
						} break;
						
						//case RB_LayoutDescKind_Vec2I16Norm:
						//case RB_LayoutDescKind_Vec4I16Norm:
						//case RB_LayoutDescKind_Vec4U16Norm:
						//case RB_LayoutDescKind_Vec2I16:
						//case RB_LayoutDescKind_Vec4I16:
						//case RB_LayoutDescKind_Vec4U16:
						{
							uint32 count;
							bool unsig;
							bool norm;
							
							if (0) case RB_LayoutDescKind_Vec2I16Norm: { count = 2; unsig = false; norm = true; }
							if (0) case RB_LayoutDescKind_Vec4I16Norm: { count = 4; unsig = false; norm = true; }
							if (0) case RB_LayoutDescKind_Vec2I16: { count = 2; unsig = false; norm = false; }
							if (0) case RB_LayoutDescKind_Vec4I16: { count = 4; unsig = false; norm = false; }
							if (0) case RB_LayoutDescKind_Vec4U8Norm: { count = 4; unsig = true; norm = true; }
							if (0) case RB_LayoutDescKind_Vec4U8: { count = 4; unsig = true; norm = false; }
							
							uint32 type = unsig ? GL_UNSIGNED_BYTE : GL_SHORT;
							
							GL.glEnableVertexAttribArray(loc);
							GL.glVertexAttribDivisor(loc, layout->divisor);
							GL.glVertexAttribPointer(loc, count, type, norm, stride, (void*)offset);
							
							location += 1;
						} break;
						
						case RB_LayoutDescKind_Mat2:
						{
							GL.glEnableVertexAttribArray(loc+0);
							GL.glEnableVertexAttribArray(loc+1);
							
							GL.glVertexAttribDivisor(loc+0, layout->divisor);
							GL.glVertexAttribDivisor(loc+1, layout->divisor);
							
							GL.glVertexAttribPointer(loc+0, 2, GL_FLOAT, false, stride, (void*)(offset + sizeof(float32[2])*0));
							GL.glVertexAttribPointer(loc+1, 2, GL_FLOAT, false, stride, (void*)(offset + sizeof(float32[2])*1));
							
							location += 2;
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
							
							location += 3;
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
							
							location += 4;
						} break;
						
						default: Assert(false); break;
					}
					
					GL.glBindBuffer(GL_ARRAY_BUFFER, 0);
				}
				
				// Uniforms
				if (cmd->draw_instanced.ubuffer)
				{
					uint32 index = cmd->draw_instanced.ubuffer->id;
					RB_OpenGLBuffer_* pool_data = RB_PoolFetch_(&g_ogl_bufferpool, index);
					
					GL.glBindBuffer(GL_UNIFORM_BUFFER, pool_data->id);
					
					SafeAssert(uniform_block_location != -1);
					
					GL.glUniformBlockBinding(current_program_id, uniform_block_location, 0);
					GL.glBindBufferBase(GL_UNIFORM_BUFFER, 0, pool_data->id);
					
					GL.glBindBuffer(GL_UNIFORM_BUFFER, 0);
				}
				
				// Samplers
				uint32 sampler_count = 0;
				
				for (intsize i = 0; i < ArrayLength(cmd->draw_instanced.textures); ++i)
				{
					RB_Handle* handle = cmd->draw_instanced.textures[i];
					
					if (!handle)
						continue;
					
					uint32 id = handle->id;
					int32 location = samplers_locations[i];
					SafeAssert(location != -1);
					
					GL.glActiveTexture(GL_TEXTURE0 + sampler_count);
					GL.glBindTexture(GL_TEXTURE_2D, id);
					GL.glUniform1i(location, sampler_count);
					
					++sampler_count;
				}
				
				// Index type
				uint32 index_type = ibuffer_pool_data->index_type;
				uintsize index_size = (index_type == GL_UNSIGNED_SHORT) ? 2 : 4;
				
				// Draw Call
				int32 base_vertex = cmd->draw_instanced.base_vertex;
				void* base_index = (void*)(cmd->draw_instanced.base_index * index_size);
				uint32 index_count = cmd->draw_instanced.index_count;
				uint32 instance_count = cmd->draw_instanced.instance_count;
				
				if (instanced)
					GL.glDrawElementsInstancedBaseVertex(GL_TRIANGLES, index_count, index_type, base_index, instance_count, base_vertex);
				else
					GL.glDrawElementsBaseVertex(GL_TRIANGLES, index_count, index_type, base_index, base_vertex);
				
				// Done
				GL.glBindVertexArray(0);
				GL.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
				
				GL.glDeleteVertexArrays(1, &vao);
			} break;
		}
		
		// end of loop
	}
}

#undef GL
