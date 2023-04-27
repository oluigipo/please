#define GL (*ctx->graphics_context->opengl)

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

struct RB_OpenGLTexture2D_
{
	uint32 id;
	RB_TexFormat format;
	int32 width;
	int32 height;
}
typedef RB_OpenGLTexture2D_;

struct RB_OpenGLPipeline_
{
	bool flag_blend : 1;
	bool flag_enable_cullface : 1;
	bool flag_depth_test : 1;
	bool flag_scissor : 1;
	
	RB_Shader shader_handle;
	
	uint32 source;
	uint32 dest;
	uint32 op;
	uint32 source_alpha;
	uint32 dest_alpha;
	uint32 op_alpha;
	
	uint32 polygon_mode;
	uint32 cull_mode;
	uint32 frontface;
	
	RB_LayoutDesc input_layout[RB_Limits_PipelineMaxVertexInputs];
}
typedef RB_OpenGLPipeline_;

struct RB_OpenGLRuntime_
{
	RB_LayoutDesc curr_input_layout[RB_Limits_PipelineMaxVertexInputs];
	uint32 curr_program_id;
	int32 curr_uniform_block_location;
	int32 curr_samplers_locations[RB_Limits_DrawMaxTextures];
	
	struct { uint32 size, last_free; RB_OpenGLShader_ data[64]; } shaderpool;
	struct { uint32 size, last_free; RB_OpenGLPipeline_ data[64]; } pipelinepool;
	struct { uint32 size, last_free; RB_OpenGLTexture2D_ data[128]; } texpool;
	struct { uint32 size, last_free; RB_OpenGLBuffer_ data[512]; } bufferpool;
}
typedef RB_OpenGLRuntime_;

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

static GLenum
RB_OpenGLTexFormatToGLEnum_(RB_TexFormat texfmt, GLenum* out_unsized_format, GLenum* out_datatype)
{
	GLenum format = 0;
	GLenum unsized_format = 0;
	GLenum datatype = GL_UNSIGNED_BYTE;
	
	switch (texfmt)
	{
		case 0: case RB_TexFormat_Count: break;
		
		case RB_TexFormat_D16:
		{
			format = GL_DEPTH_COMPONENT16;
			unsized_format = GL_DEPTH_COMPONENT;
			datatype = GL_UNSIGNED_SHORT;
		} break;
		case RB_TexFormat_D24S8:
		{
			format = GL_DEPTH24_STENCIL8;
			unsized_format = GL_DEPTH_STENCIL;
			datatype = GL_UNSIGNED_INT_24_8;
		} break;
		case RB_TexFormat_A8: format = GL_ALPHA; unsized_format = GL_ALPHA; break;
		case RB_TexFormat_R8: format = GL_R8; unsized_format = GL_RED; break;
		case RB_TexFormat_RG8: format = GL_RG8; unsized_format = GL_RG; break;
		case RB_TexFormat_RGB8: format = GL_RGB8; unsized_format = GL_RGB; break;
		case RB_TexFormat_RGBA8: format = GL_RGBA8; unsized_format = GL_RGBA; break;
	}
	
	if (out_unsized_format)
		*out_unsized_format = unsized_format;
	if (out_datatype)
		*out_datatype = datatype;
	
	return format;
}

static void
RB_OpenGLFreeCtx_(RB_Ctx* ctx)
{
	
}

static bool
RB_OpenGLIsValidHandle_(RB_Ctx* ctx, uint32 handle)
{
	return handle != 0;
}

static void
RB_OpenGLResource_(RB_Ctx* ctx, const RB_ResourceCall_* resc)
{
	RB_OpenGLRuntime_* rt = ctx->rt;
	uint32 handle = *resc->handle;
	Arena* scratch_arena = ctx->arena;
	Arena_Savepoint scratch_arena_save = Arena_Save(scratch_arena);
	
	switch (resc->kind)
	{
		case 0: Assert(false); break;
		
		case RB_ResourceKind_MakeTexture2D_:
		{
			int32 width = resc->tex2d.width;
			int32 height = resc->tex2d.height;
			const void* pixels = resc->tex2d.pixels;
			
			SafeAssert(width && height);
			
			uint32 id;
			GLenum unsized_format, datatype;
			GLenum format = RB_OpenGLTexFormatToGLEnum_(resc->tex2d.format, &unsized_format, &datatype);
			
			SafeAssert(format && unsized_format && datatype);
			
			bool mag_linear = resc->tex2d.flag_linear_filtering;
			bool min_linear = mag_linear;
			
			GL.glGenTextures(1, &id);
			GL.glBindTexture(GL_TEXTURE_2D, id);
			
			GL.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, mag_linear ? GL_LINEAR : GL_NEAREST);
			GL.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, min_linear ? GL_LINEAR : GL_NEAREST);
			GL.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			GL.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
			
			{
				Trace(); TraceName(Str("glTexImage2D"));
				GL.glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, unsized_format, datatype, pixels);
			}
			
			GL.glBindTexture(GL_TEXTURE_2D, 0);
			
			RB_OpenGLTexture2D_* pool_data = RB_PoolAlloc_(&rt->texpool, &handle);
			pool_data->id = id;
			pool_data->format = resc->tex2d.format;
		} break;
		
		//case RB_ResourceKind_MakeVertexBuffer_:
		//case RB_ResourceKind_MakeIndexBuffer_:
		//case RB_ResourceKind_MakeUniformBuffer_:
		{
			uint32 kind;
			const void* initial_data;
			uintsize size;
			bool dynamic;
			
			if (0) case RB_ResourceKind_MakeVertexBuffer_:
			{
				kind = GL_ARRAY_BUFFER;
				initial_data = resc->vbuffer.initial_data;
				size = resc->vbuffer.size;
				dynamic = resc->vbuffer.flag_dynamic;
			}
			if (0) case RB_ResourceKind_MakeIndexBuffer_:
			{
				kind = GL_ELEMENT_ARRAY_BUFFER;
				initial_data = resc->ibuffer.initial_data;
				size = resc->ibuffer.size;
				dynamic = resc->ibuffer.flag_dynamic;
			}
			if (0) case RB_ResourceKind_MakeUniformBuffer_:
			{
				kind = GL_UNIFORM_BUFFER;
				initial_data = resc->ubuffer.initial_data;
				size = resc->ubuffer.size;
				dynamic = true;
			}
			
			uint32 id;
			uint32 usage = (dynamic) ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW;
			
			GL.glGenBuffers(1, &id);
			GL.glBindBuffer(kind, id);
			GL.glBufferData(kind, size, initial_data, usage);
			GL.glBindBuffer(kind, 0);
			
			RB_OpenGLBuffer_* pool_data = RB_PoolAlloc_(&rt->bufferpool, &handle);
			pool_data->id = id;
			
			if (resc->kind == RB_ResourceKind_MakeIndexBuffer_)
			{
				switch (resc->ibuffer.index_type)
				{
					case RB_IndexType_Null: Assert(false); break;
					case RB_IndexType_Uint16: pool_data->index_type = GL_UNSIGNED_SHORT; break;
					case RB_IndexType_Uint32: pool_data->index_type = GL_UNSIGNED_INT; break;
				}
			}
		} break;
		
		case RB_ResourceKind_MakeShader_:
		{
			char info[512];
			int32 success = true;
			
			const char* vertex_shader_source = Arena_PushCString(scratch_arena, resc->shader.glsl.vs);
			const char* fragment_shader_source = Arena_PushCString(scratch_arena, resc->shader.glsl.fs);
			
			const char* vertex_lines[] = {
				"#version 330 core\n",
				vertex_shader_source,
			};
			const char* fragment_lines[] = {
				"#version 330 core\n",
				"\n",
				fragment_shader_source,
			};
			
			if (GL.is_es)
			{
				vertex_lines[0] = "#version 300 es\n";
				fragment_lines[0] = "#version 300 es\n";
				fragment_lines[1] = "precision mediump float;\n";
			}
			
			// Compile Vertex Shader
			uint32 vertex_shader = GL.glCreateShader(GL_VERTEX_SHADER);
			GL.glShaderSource(vertex_shader, ArrayLength(vertex_lines), vertex_lines, NULL);
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
			GL.glShaderSource(fragment_shader, ArrayLength(fragment_lines), fragment_lines, NULL);
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
			
			RB_OpenGLShader_* pool_data = RB_PoolAlloc_(&rt->shaderpool, &handle);
			pool_data->program_id = program;
		} break;
		
		case RB_ResourceKind_MakeRenderTarget_:
		{
			SafeAssert(false);
		} break;
		
		case RB_ResourceKind_MakePipeline_:
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
			
			SafeAssert(resc->pipeline.blend_source >= 0 && resc->pipeline.blend_source < ArrayLength(functable));
			SafeAssert(resc->pipeline.blend_dest >= 0 && resc->pipeline.blend_dest < ArrayLength(functable));
			SafeAssert(resc->pipeline.blend_source_alpha >= 0 && resc->pipeline.blend_source_alpha < ArrayLength(functable));
			SafeAssert(resc->pipeline.blend_dest_alpha >= 0 && resc->pipeline.blend_dest_alpha < ArrayLength(functable));
			SafeAssert(resc->pipeline.blend_op >= 0 && resc->pipeline.blend_op < ArrayLength(optable));
			SafeAssert(resc->pipeline.blend_op_alpha >= 0 && resc->pipeline.blend_op_alpha < ArrayLength(optable));
			
			static const uint32 filltable[] = {
				[RB_FillMode_Solid] = GL_FILL,
				[RB_FillMode_Wireframe] = GL_LINE,
			};
			
			static const uint32 culltable[] = {
				[RB_CullMode_None] = 0,
				[RB_CullMode_Front] = GL_FRONT,
				[RB_CullMode_Back] = GL_BACK,
			};
			
			SafeAssert(resc->pipeline.fill_mode >= 0 && resc->pipeline.fill_mode < ArrayLength(filltable));
			SafeAssert(resc->pipeline.cull_mode >= 0 && resc->pipeline.cull_mode < ArrayLength(culltable));
			
			RB_OpenGLPipeline_* pool_data = RB_PoolAlloc_(&rt->pipelinepool, &handle);
			pool_data->flag_blend = resc->pipeline.flag_blend;
			pool_data->source = resc->pipeline.blend_source ? functable[resc->pipeline.blend_source] : GL_ONE;
			pool_data->dest = resc->pipeline.blend_dest ? functable[resc->pipeline.blend_dest] : GL_ZERO;
			pool_data->op = resc->pipeline.blend_op ? optable[resc->pipeline.blend_op] : GL_FUNC_ADD;
			pool_data->source_alpha = resc->pipeline.blend_source_alpha ? functable[resc->pipeline.blend_source_alpha] : GL_ONE;
			pool_data->dest_alpha = resc->pipeline.blend_dest_alpha ? functable[resc->pipeline.blend_dest_alpha] : GL_ZERO;
			pool_data->op_alpha = resc->pipeline.blend_op_alpha ? optable[resc->pipeline.blend_op_alpha] : GL_FUNC_ADD;
			
			pool_data->polygon_mode = filltable[resc->pipeline.fill_mode];
			pool_data->cull_mode = culltable[resc->pipeline.cull_mode];
			pool_data->frontface = resc->pipeline.flag_cw_backface ? GL_CCW : GL_CW;
			pool_data->flag_enable_cullface = (culltable[resc->pipeline.cull_mode] != 0);
			pool_data->flag_depth_test = resc->pipeline.flag_depth_test;
			//pool_data->flag_scissor = resc->pipeline.flag_scissor;
			
			SafeAssert(resc->pipeline.shader.id);
			pool_data->shader_handle = resc->pipeline.shader;
			
			Mem_Copy(pool_data->input_layout, resc->pipeline.input_layout, sizeof(pool_data->input_layout));
		} break;
		
		//case RB_ResourceKind_UpdateVertexBuffer_:
		//case RB_ResourceKind_UpdateIndexBuffer_:
		//case RB_ResourceKind_UpdateUniformBuffer_:
		{
			uint32 kind;
			
			if (0) case RB_ResourceKind_UpdateVertexBuffer_: kind = GL_ARRAY_BUFFER;
			if (0) case RB_ResourceKind_UpdateIndexBuffer_: kind = GL_ELEMENT_ARRAY_BUFFER;
			if (0) case RB_ResourceKind_UpdateUniformBuffer_: kind = GL_UNIFORM_BUFFER;
			
			Assert(handle);
			
			RB_OpenGLBuffer_* pool_data = RB_PoolFetch_(&rt->bufferpool, handle);
			
			uint32 id = pool_data->id;
			uint32 usage = GL_DYNAMIC_DRAW;
			
			GL.glBindBuffer(kind, id);
			GL.glBufferData(kind, resc->update.new_data.size, resc->update.new_data.data, usage);
			GL.glBindBuffer(kind, 0);
		} break;
		
		case RB_ResourceKind_UpdateTexture2D_:
		{
			Assert(handle);
			RB_OpenGLTexture2D_* pool_data = RB_PoolFetch_(&rt->texpool, handle);
			
			uint32 id = pool_data->id;
			int32 width = pool_data->width;
			int32 height = pool_data->height;
			const void* pixels = resc->update.new_data.data;
			Assert(width && height);
			//SafeAssert(resc->update.new_data.size == (uintsize)width*height*pixel_size);
			
			GLenum unsized_format, datatype;
			GLenum format = RB_OpenGLTexFormatToGLEnum_(pool_data->format, &unsized_format, &datatype);
			Assert(format && unsized_format && datatype);
			
			GL.glBindTexture(GL_TEXTURE_2D, id);
			GL.glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, unsized_format, datatype, pixels);
			GL.glBindTexture(GL_TEXTURE_2D, 0);
		} break;
		
		case RB_ResourceKind_FreeTexture2D_:
		{
			Assert(handle);
			RB_OpenGLTexture2D_* pool_data = RB_PoolFetch_(&rt->texpool, handle);
			
			GL.glDeleteTextures(1, &pool_data->id);
			
			RB_PoolFree_(&rt->texpool, handle);
			handle = 0;
		} break;
		
		case RB_ResourceKind_FreeVertexBuffer_:
		case RB_ResourceKind_FreeIndexBuffer_:
		case RB_ResourceKind_FreeUniformBuffer_:
		{
			Assert(handle);
			
			RB_OpenGLBuffer_* pool_data = RB_PoolFetch_(&rt->bufferpool, handle);
			
			uint32 id = pool_data->id;
			GL.glDeleteBuffers(1, &id);
			
			RB_PoolFree_(&rt->bufferpool, handle);
			handle = 0;
		} break;
		
		case RB_ResourceKind_FreeShader_:
		{
			Assert(handle);
			RB_OpenGLShader_* pool_data = RB_PoolFetch_(&rt->shaderpool, handle);
			
			GL.glDeleteProgram(pool_data->program_id);
			
			RB_PoolFree_(&rt->shaderpool, handle);
			handle = 0;
		} break;
		
		case RB_ResourceKind_FreeRenderTarget_:
		{
			SafeAssert(false);
		} break;
		
		case RB_ResourceKind_FreePipeline_:
		{
			Assert(handle);
			
			RB_PoolFree_(&rt->pipelinepool, handle);
			handle = 0;
		} break;
	}
	
	Arena_Restore(scratch_arena_save);
	*resc->handle = handle;
}

static void
RB_OpenGLCommand_(RB_Ctx* ctx, const RB_CommandCall_* cmd)
{
	RB_OpenGLRuntime_* rt = ctx->rt;
	
	switch (cmd->kind)
	{
		case 0: Assert(false); break;
		
		case RB_CommandKind_Begin_:
		{
			Mem_Zero(rt->curr_input_layout, sizeof(rt->curr_input_layout));
			rt->curr_program_id = 0;
			rt->curr_uniform_block_location = -1;
			for (intsize i = 0; i < ArrayLength(rt->curr_samplers_locations); ++i)
				rt->curr_samplers_locations[i] = -1;
			
			GL.glViewport(0, 0, cmd->begin.viewport_width, cmd->begin.viewport_height);
			
			// default blend
			GL.glEnable(GL_BLEND);
			GL.glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			GL.glBlendEquation(GL_FUNC_ADD);
			
			// default rasterizer
			GL.glFrontFace(GL_CCW);
			GL.glDisable(GL_DEPTH_TEST);
			GL.glDisable(GL_SCISSOR_TEST);
			if (!GL.is_es)
				GL.glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		} break;
		
		case RB_CommandKind_End_:
		{
			
		} break;
		
		case RB_CommandKind_Clear_:
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
		
		case RB_CommandKind_ApplyPipeline_:
		{
			SafeAssert(cmd->apply_pipeline.handle.id);
			RB_OpenGLPipeline_* pool_data = RB_PoolFetch_(&rt->pipelinepool, cmd->apply_pipeline.handle.id);
			RB_OpenGLShader_* shader_pool_data = RB_PoolFetch_(&rt->shaderpool, pool_data->shader_handle.id);
			
			Mem_Copy(rt->curr_input_layout, pool_data->input_layout, sizeof(rt->curr_input_layout));
			
			if (rt->curr_program_id != shader_pool_data->program_id)
			{
				rt->curr_program_id = shader_pool_data->program_id;
				GL.glUseProgram(shader_pool_data->program_id);
				
				rt->curr_uniform_block_location = GL.glGetUniformBlockIndex(rt->curr_program_id, "UniformBuffer");
				
				for (intsize i = 0; i < ArrayLength(rt->curr_samplers_locations); ++i)
				{
					static_assert(ArrayLength(rt->curr_samplers_locations) < 10, "this code needs to be updated if we ever use more than 10 textures");
					char name[] = "uTexture[N]";
					name[sizeof(name)-3] = (char)('0' + i);
					
					rt->curr_samplers_locations[i] = GL.glGetUniformLocation(rt->curr_program_id, name);
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
			
			if (!GL.is_es)
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
		} break;
		
		case RB_CommandKind_ApplyRenderTarget_:
		{
			SafeAssert(false);
		} break;
		
		case RB_CommandKind_Draw_:
		{
			// Buffers
			SafeAssert(cmd->draw.ibuffer.id);
			
			RB_OpenGLBuffer_* ibuffer_pool_data = RB_PoolFetch_(&rt->bufferpool, cmd->draw.ibuffer.id);
			
			uint32 ibuffer = ibuffer_pool_data->id;
			uint32 vbuffers[ArrayLength(cmd->draw.vbuffers)] = { 0 };
			
			for (intsize i = 0; i < ArrayLength(cmd->draw.vbuffers); ++i)
			{
				if (cmd->draw.vbuffers[i].id)
				{
					uint32 index = cmd->draw.vbuffers[i].id;
					RB_OpenGLBuffer_* pool_data = RB_PoolFetch_(&rt->bufferpool, index);
					
					vbuffers[i] = pool_data->id;
				}
			}
			
			// Vertex Layout
			uint32 vao;
			
			GL.glGenVertexArrays(1, &vao);
			GL.glBindVertexArray(vao);
			GL.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibuffer);
			
			uint32 location = 0;
			
			for (intsize i = 0; i < ArrayLength(rt->curr_input_layout); ++i)
			{
				const RB_LayoutDesc* layout = &rt->curr_input_layout[i];
				
				if (!layout->format)
					break;
				
				SafeAssert(layout->buffer_slot < ArrayLength(vbuffers));
				GL.glBindBuffer(GL_ARRAY_BUFFER, vbuffers[layout->buffer_slot]);
				
				uint32 loc = location;
				uint32 stride = cmd->draw.strides[layout->buffer_slot];
				uintptr offset = cmd->draw.offsets[layout->buffer_slot] + layout->offset;
				
				switch (layout->format)
				{
					//case RB_VertexFormat_Scalar:
					//case RB_VertexFormat_Vec2:
					//case RB_VertexFormat_Vec3:
					//case RB_VertexFormat_Vec4:
					//case RB_VertexFormat_Vec2F16:
					//case RB_VertexFormat_Vec4F16:
					{
						uint32 count;
						bool half;
						
						if (0) case RB_VertexFormat_Scalar: { count = 1; half = false; }
						if (0) case RB_VertexFormat_Vec2: { count = 2; half = false; }
						if (0) case RB_VertexFormat_Vec3: { count = 3; half = false; }
						if (0) case RB_VertexFormat_Vec4: { count = 4; half = false; }
						if (0) case RB_VertexFormat_Vec2F16: { count = 2; half = true; }
						if (0) case RB_VertexFormat_Vec4F16: { count = 2; half = true; }
						
						GLenum type = half ? GL_HALF_FLOAT : GL_FLOAT;
						
						GL.glEnableVertexAttribArray(loc);
						GL.glVertexAttribDivisor(loc, layout->divisor);
						GL.glVertexAttribPointer(loc, count, type, false, stride, (void*)offset);
						
						location += 1;
					} break;
					
					//case RB_VertexFormat_Vec2I16Norm:
					//case RB_VertexFormat_Vec4I16Norm:
					//case RB_VertexFormat_Vec4U16Norm:
					//case RB_VertexFormat_Vec2I16:
					//case RB_VertexFormat_Vec4I16:
					//case RB_VertexFormat_Vec4U16:
					{
						uint32 count;
						bool unsig;
						bool norm;
						
						if (0) case RB_VertexFormat_Vec2I16Norm: { count = 2; unsig = false; norm = true; }
						if (0) case RB_VertexFormat_Vec4I16Norm: { count = 4; unsig = false; norm = true; }
						if (0) case RB_VertexFormat_Vec2I16: { count = 2; unsig = false; norm = false; }
						if (0) case RB_VertexFormat_Vec4I16: { count = 4; unsig = false; norm = false; }
						if (0) case RB_VertexFormat_Vec4U8Norm: { count = 4; unsig = true; norm = true; }
						if (0) case RB_VertexFormat_Vec4U8: { count = 4; unsig = true; norm = false; }
						
						uint32 type = unsig ? GL_UNSIGNED_BYTE : GL_SHORT;
						
						GL.glEnableVertexAttribArray(loc);
						GL.glVertexAttribDivisor(loc, layout->divisor);
						if (norm)
							GL.glVertexAttribPointer(loc, count, type, true, stride, (void*)offset);
						else
							GL.glVertexAttribIPointer(loc, count, type, stride, (void*)offset);
						
						location += 1;
					} break;
					
					case RB_VertexFormat_Mat2:
					{
						GL.glEnableVertexAttribArray(loc+0);
						GL.glEnableVertexAttribArray(loc+1);
						
						GL.glVertexAttribDivisor(loc+0, layout->divisor);
						GL.glVertexAttribDivisor(loc+1, layout->divisor);
						
						GL.glVertexAttribPointer(loc+0, 2, GL_FLOAT, false, stride, (void*)(offset + sizeof(float32[2])*0));
						GL.glVertexAttribPointer(loc+1, 2, GL_FLOAT, false, stride, (void*)(offset + sizeof(float32[2])*1));
						
						location += 2;
					} break;
					
					case RB_VertexFormat_Mat3:
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
					
					case RB_VertexFormat_Mat4:
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
			if (cmd->draw.ubuffer.id)
			{
				uint32 index = cmd->draw.ubuffer.id;
				RB_OpenGLBuffer_* pool_data = RB_PoolFetch_(&rt->bufferpool, index);
				
				GL.glBindBuffer(GL_UNIFORM_BUFFER, pool_data->id);
				
				SafeAssert(rt->curr_uniform_block_location != -1);
				
				GL.glUniformBlockBinding(rt->curr_program_id, rt->curr_uniform_block_location, 0);
				GL.glBindBufferBase(GL_UNIFORM_BUFFER, 0, pool_data->id);
				
				GL.glBindBuffer(GL_UNIFORM_BUFFER, 0);
			}
			
			// Samplers
			uint32 sampler_count = 0;
			
			for (intsize i = 0; i < ArrayLength(cmd->draw.textures); ++i)
			{
				RB_Tex2d handle = cmd->draw.textures[i];
				
				if (!handle.id)
					continue;
				
				uint32 id = handle.id;
				int32 location = rt->curr_samplers_locations[i];
				SafeAssert(location != -1);
				
				GL.glActiveTexture(GL_TEXTURE0 + (int32)i);
				GL.glBindTexture(GL_TEXTURE_2D, id);
				GL.glUniform1i(location, (int32)i);
				
				++sampler_count;
			}
			
			// Index type
			uint32 index_type = ibuffer_pool_data->index_type;
			uintsize index_size = (index_type == GL_UNSIGNED_SHORT) ? 2 : 4;
			
			// Draw Call
			void* base_index = (void*)(cmd->draw.base_index * index_size);
			uint32 index_count = cmd->draw.index_count;
			uint32 instance_count = cmd->draw.instance_count;
			
			if (instance_count)
				GL.glDrawElementsInstanced(GL_TRIANGLES, index_count, index_type, base_index, instance_count);
			else
				GL.glDrawElements(GL_TRIANGLES, index_count, index_type, base_index);
			
			// Done
			GL.glBindVertexArray(0);
			GL.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
			
			GL.glDeleteVertexArrays(1, &vao);
		} break;
	}
}

static void
RB_SetupOpenGLRuntime_(RB_Ctx* ctx)
{
	RB_OpenGLRuntime_* rt = Arena_PushStruct(ctx->arena, RB_OpenGLRuntime_);
	ctx->rt = rt;
	ctx->rt_free_ctx = RB_OpenGLFreeCtx_;
	ctx->rt_is_valid_handle = RB_OpenGLIsValidHandle_;
	ctx->rt_resource = RB_OpenGLResource_;
	ctx->rt_cmd = RB_OpenGLCommand_;
	
#ifdef CONFIG_DEBUG
	if (GL.glDebugMessageCallback)
	{
		GL.glEnable(GL_DEBUG_OUTPUT);
		GL.glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
		GL.glDebugMessageCallback(RB_OpenGLDebugMessageCallback_, NULL);
	}
#endif //CONFIG_DEBUG
	
	GL.glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	GL.glDisable(GL_DEPTH_TEST);
	GL.glDisable(GL_CULL_FACE);
	GL.glEnable(GL_BLEND);
	GL.glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	
	//- Capabilities
	RB_Capabilities caps = {
		.backend_api = StrInit("OpenGL 3.3"),
		.shader_type = RB_ShaderType_Glsl,
	};
	
	if (GL.is_es)
		caps.backend_api = Str("OpenGL ES 3.0"),
	
	GL.glGetIntegerv(GL_MAX_TEXTURE_SIZE, &caps.max_texture_size);
	GL.glGetIntegerv(GL_MAX_COLOR_ATTACHMENTS, &caps.max_render_target_textures);
	
	caps.max_texture_size = ClampMin(caps.max_texture_size, 2048);
	caps.max_render_target_textures = ClampMin(caps.max_render_target_textures, 4);
	
	const uint8* vendor_cstr = GL.glGetString(GL_VENDOR);
	const uint8* renderer_cstr = GL.glGetString(GL_RENDERER);
	const uint8* version_cstr = GL.glGetString(GL_VERSION);
	caps.driver_vendor = StrMake(Mem_Strlen((char*)vendor_cstr), vendor_cstr);
	caps.driver_renderer = StrMake(Mem_Strlen((char*)renderer_cstr), renderer_cstr);
	caps.driver_version = StrMake(Mem_Strlen((char*)version_cstr), version_cstr);
	
	caps.has_instancing = true;
	caps.has_32bit_index = true;
	caps.has_separate_alpha_blend = true;
	caps.has_compute_shaders = false;
	caps.has_16bit_float = false;
	caps.has_wireframe_fillmode = !GL.is_es;
	
	caps.supported_texture_formats[0] |= (1 << RB_TexFormat_D16);
	caps.supported_texture_formats[0] |= (1 << RB_TexFormat_D24S8);
	caps.supported_texture_formats[0] |= (1 << RB_TexFormat_A8);
	caps.supported_texture_formats[0] |= (1 << RB_TexFormat_R8);
	caps.supported_texture_formats[0] |= (1 << RB_TexFormat_RG8);
	caps.supported_texture_formats[0] |= (1 << RB_TexFormat_RGB8);
	caps.supported_texture_formats[0] |= (1 << RB_TexFormat_RGBA8);
	
	ctx->caps = caps;
}

#undef GL
