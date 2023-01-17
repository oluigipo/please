#define GL (*g_graphics_context->opengl)

static uint32 g_ogl_uniform_buffer;

#ifdef CONFIG_DEBUG
static void APIENTRY
RenderBackend_OpenGLDebugMessageCallback_(GLenum source, GLenum type, GLuint id, GLenum severity,
	GLsizei length, const GLchar* message, const void* user_param)
{
	const char* type_str = "Debug Message";
	
	if (type == GL_DEBUG_TYPE_ERROR)
		type_str = "Error";
	
	OS_DebugLog("OpenGL %s:\n\tType: 0x%x\n\tID: %u\n\tSeverity: 0x%x\n\tMessage: %.*s\n", type_str, type, id, severity, (int)length, message);
}
#endif //CONFIG_DEBUG

static void
RenderBackend_InitOpenGL_(Arena* scratch_arena)
{
#ifdef CONFIG_DEBUG
	GL.glEnable(GL_DEBUG_OUTPUT);
	GL.glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
	GL.glDebugMessageCallback(RenderBackend_OpenGLDebugMessageCallback_, NULL);
#endif //CONFIG_DEBUG
	
	GL.glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
}

static void
RenderBackend_DeinitOpenGL_(Arena* scratch_arena)
{
}

static void
RenderBackend_ResourceOpenGL_(Arena* scratch_arena, RenderBackend_ResourceCommand* commands)
{
	for (RenderBackend_ResourceCommand* cmd = commands; cmd; cmd = cmd->next)
	{
		Assert(cmd->kind);
		Assert(cmd->handle);
		
		RenderBackend_Handle handle = *cmd->handle;
		
		switch (cmd->kind)
		{
			case 0: Assert(false); break;
			
			case RenderBackend_ResourceCommandKind_MakeTexture2D:
			{
				int32 width = cmd->texture_2d.width;
				int32 height = cmd->texture_2d.height;
				int32 channels = cmd->texture_2d.channels;
				const void* pixels = cmd->texture_2d.pixels;
				
				Assert(width && height && channels > 0 && channels <= 4);
				
				uint32 id;
				const int32 formats[4] = { GL_RED, GL_RG, GL_RGB, GL_RGBA, };
				int32 format = formats[channels - 1];
				
				bool mag_linear = true;
				bool min_linear = true;
				
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
				
				handle.ref = (void*)id;
			} break;
			
			case RenderBackend_ResourceCommandKind_MakeVertexBuffer:
			{
				uint32 id;
				uint32 usage = (cmd->flag_dynamic) ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW;
				
				GL.glGenBuffers(1, &id);
				GL.glBindBuffer(GL_ARRAY_BUFFER, id);
				GL.glBufferData(GL_ARRAY_BUFFER, cmd->buffer.size, cmd->buffer.data, usage);
				GL.glBindBuffer(GL_ARRAY_BUFFER, 0);
				
				handle.id = id;
			} break;
			
			case RenderBackend_ResourceCommandKind_MakeIndexBuffer:
			{
				uint32 id;
				uint32 usage = (cmd->flag_dynamic) ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW;
				
				GL.glGenBuffers(1, &id);
				GL.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, id);
				GL.glBufferData(GL_ELEMENT_ARRAY_BUFFER, cmd->buffer.size, cmd->buffer.data, usage);
				GL.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
				
				handle.id = id;
			} break;
			
			case RenderBackend_ResourceCommandKind_MakeShader:
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
				
				handle.id = program;
			} break;
			
			case RenderBackend_ResourceCommandKind_MakeRenderTarget:
			{
				SafeAssert(false);
			} break;
			
			case RenderBackend_ResourceCommandKind_UpdateVertexBuffer:
			{
				Assert(handle.ref);
				
				uint32 id = handle.id;
				uint32 usage = (cmd->flag_dynamic) ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW;
				
				GL.glBindBuffer(GL_ARRAY_BUFFER, id);
				if (cmd->flag_subregion)
					GL.glBufferData(GL_ARRAY_BUFFER, cmd->buffer.size, cmd->buffer.data, usage);
				else
					GL.glBufferSubData(GL_ARRAY_BUFFER, cmd->buffer.offset, cmd->buffer.size, cmd->buffer.data);
				GL.glBindBuffer(GL_ARRAY_BUFFER, 0);
			} break;
			
			case RenderBackend_ResourceCommandKind_UpdateIndexBuffer:
			{
				Assert(handle.ref);
				
				uint32 id = handle.id;
				uint32 usage = (cmd->flag_dynamic) ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW;
				
				GL.glBindBuffer(GL_ARRAY_BUFFER, id);
				if (cmd->flag_subregion)
					GL.glBufferData(GL_ARRAY_BUFFER, cmd->buffer.size, cmd->buffer.data, usage);
				else
					GL.glBufferSubData(GL_ARRAY_BUFFER, cmd->buffer.offset, cmd->buffer.size, cmd->buffer.data);
				GL.glBindBuffer(GL_ARRAY_BUFFER, 0);
			} break;
			
			case RenderBackend_ResourceCommandKind_UpdateTexture2D:
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
			
			case RenderBackend_ResourceCommandKind_FreeTexture2D:
			{
				Assert(handle.ref);
				
				uint32 id = handle.id;
				
				GL.glDeleteTextures(1, &id);
				
				handle.id = 0;
			} break;
			
			case RenderBackend_ResourceCommandKind_FreeVertexBuffer:
			case RenderBackend_ResourceCommandKind_FreeIndexBuffer:
			{
				Assert(handle.ref);
				
				uint32 id = handle.id;
				
				GL.glDeleteBuffers(1, &id);
				
				handle.id = 0;
			} break;
			
			case RenderBackend_ResourceCommandKind_FreeShader:
			{
				Assert(handle.ref);
				
				uint32 id = handle.id;
				
				GL.glDeleteProgram(id);
				
				handle.id = 0;
			} break;
			
			case RenderBackend_ResourceCommandKind_FreeRenderTarget:
			{
				SafeAssert(false);
			} break;
		}
		
		*cmd->handle = handle;
	}
}

static void
RenderBackend_DrawOpenGL_(Arena* scratch_arena, RenderBackend_DrawCommand* commands)
{
	uint32 ubo;
	GL.glGenBuffers(1, &ubo);
	
	for (RenderBackend_DrawCommand* cmd = commands; cmd; cmd = cmd->next)
	{
		if (cmd->resources_cmd)
			RenderBackend_ResourceOpenGL_(scratch_arena, cmd->resources_cmd);
		
		switch (cmd->kind)
		{
			case 0: Assert(false); break;
			
			case RenderBackend_DrawCommandKind_Clear:
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
			
			case RenderBackend_DrawCommandKind_SetRenderTarget:
			{
				uint32 id = cmd->set_target.handle->id;
				
				GL.glBindFramebuffer(GL_FRAMEBUFFER, id);
			} break;
			
			case RenderBackend_DrawCommandKind_ResetRenderTarget:
			{
				GL.glBindFramebuffer(GL_FRAMEBUFFER, 0);
			} break;
			
			case RenderBackend_DrawCommandKind_DrawCall:
			{
				// Buffers and Shader
				uint32 shader = 0;
				uint32 ibuffer = 0;
				uint32 vbuffers[ArrayLength(cmd->drawcall.vbuffers)] = { 0 };
				
				Assert(cmd->drawcall.shader);
				Assert(cmd->drawcall.ibuffer);
				
				shader = (uint32)cmd->drawcall.shader->ref;
				ibuffer = (uint32)cmd->drawcall.ibuffer->ref;
				for (intsize i = 0; i < ArrayLength(cmd->drawcall.vbuffers); ++i)
				{
					if (cmd->drawcall.vbuffers[i])
						vbuffers[i] = (uint32)cmd->drawcall.vbuffers[i]->ref;
				}
				
				// Vertex Layout
				uint32 vao;
				
				GL.glGenVertexArrays(1, &vao);
				GL.glBindVertexArray(vao);
				GL.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibuffer);
				
				for (intsize i = 0; i < ArrayLength(cmd->drawcall.vertex_layout); ++i)
				{
					const RenderBackend_LayoutDesc* layout = &cmd->drawcall.vertex_layout[i];
					
					if (!layout->kind)
						continue;
					
					uint32 loc = layout->location;
					
					Assert(layout->vbuffer_index < ArrayLength(vbuffers));
					GL.glBindBuffer(GL_ARRAY_BUFFER, vbuffers[layout->vbuffer_index]);
					
					switch (layout->kind)
					{
						//case RenderBackend_LayoutDescKind_Vec2:
						//case RenderBackend_LayoutDescKind_Vec3:
						//case RenderBackend_LayoutDescKind_Vec4:
						{
							uint32 count;
							
							if (0) case RenderBackend_LayoutDescKind_Vec2: count = 2;
							if (0) case RenderBackend_LayoutDescKind_Vec3: count = 3;
							if (0) case RenderBackend_LayoutDescKind_Vec4: count = 4;
							
							GL.glEnableVertexAttribArray(loc);
							GL.glVertexAttribDivisor(loc, layout->divisor);
							GL.glVertexAttribPointer(loc, count, GL_FLOAT, false, layout->stride, (void*)layout->offset);
						} break;
						
						case RenderBackend_LayoutDescKind_Mat4:
						{
							GL.glEnableVertexAttribArray(loc+0);
							GL.glEnableVertexAttribArray(loc+1);
							GL.glEnableVertexAttribArray(loc+2);
							GL.glEnableVertexAttribArray(loc+3);
							
							GL.glVertexAttribDivisor(loc+0, layout->divisor);
							GL.glVertexAttribDivisor(loc+1, layout->divisor);
							GL.glVertexAttribDivisor(loc+2, layout->divisor);
							GL.glVertexAttribDivisor(loc+3, layout->divisor);
							
							GL.glVertexAttribPointer(loc+0, 4, GL_FLOAT, false, layout->stride, (void*)(layout->offset + sizeof(float32[4])*0));
							GL.glVertexAttribPointer(loc+1, 4, GL_FLOAT, false, layout->stride, (void*)(layout->offset + sizeof(float32[4])*1));
							GL.glVertexAttribPointer(loc+2, 4, GL_FLOAT, false, layout->stride, (void*)(layout->offset + sizeof(float32[4])*2));
							GL.glVertexAttribPointer(loc+3, 4, GL_FLOAT, false, layout->stride, (void*)(layout->offset + sizeof(float32[4])*3));
						} break;
						
						default: Assert(false); break;
					}
					
					GL.glBindBuffer(GL_ARRAY_BUFFER, 0);
				}
				
				// Uniforms
				GL.glUseProgram(shader);
				
				if (cmd->drawcall.uniform_buffer.size)
				{
					GL.glBindBuffer(GL_UNIFORM_BUFFER, ubo);
					GL.glBufferData(GL_UNIFORM_BUFFER, cmd->drawcall.uniform_buffer.size, cmd->drawcall.uniform_buffer.data, GL_DYNAMIC_DRAW);
					GL.glBindBuffer(GL_UNIFORM_BUFFER, 0);
					
					int32 location = GL.glGetUniformBlockIndex(shader, "UniformBuffer");
					GL.glUniformBlockBinding(shader, location, 0);
					GL.glBindBufferBase(shader, 0, ubo);
				}
				
				// Samplers
				uint32 sampler_count = 0;
				
				for (intsize i = 0; i < ArrayLength(cmd->drawcall.samplers); ++i)
				{
					const RenderBackend_SamplerDesc* sampler = &cmd->drawcall.samplers[i];
					
					if (!sampler->handle)
						continue;
					
					uint32 id = (uint32)sampler->handle->ref;
					int32 location = 0;
					
					for Arena_TempScope(scratch_arena)
					{
						const char* name = Arena_PushCString(scratch_arena, sampler->gl_name);
						location = GL.glGetUniformLocation(shader, name);
						SafeAssert(location != -1);
					}
					
					GL.glActiveTexture(GL_TEXTURE0 + sampler_count);
					GL.glBindTexture(GL_TEXTURE_2D, id);
					GL.glUniform1i(location, sampler_count);
					
					++sampler_count;
				}
				
				// Draw Call
				uint32 index_count = cmd->drawcall.index_count;
				uint32 instance_count = cmd->drawcall.instance_count;
				
				GL.glDrawElementsInstanced(GL_TRIANGLES, index_count, GL_UNSIGNED_INT, NULL, instance_count);
				
				// Done
				GL.glBindVertexArray(0);
				GL.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
				GL.glUseProgram(0);
				
				GL.glDeleteVertexArrays(1, &vao);
			} break;
		}
		
		// end of loop
	}
	
	GL.glDeleteBuffers(1, &ubo);
}

#undef GL
