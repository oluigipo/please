#define GL (*global_engine.graphics_context->opengl)

//~ NOTE(ljre): Resource allocation
static bool
OpenGL_MakeTexture2D(const Render_Texture2DDesc* desc, Render_Texture2D* out_texture)
{
	if (!desc->pixels || !desc->width || !desc->height || desc->channels > 4 || desc->channels == 2)
		return false;
	
	uint32 id;
	GL.glGenTextures(1, &id);
	GL.glBindTexture(GL_TEXTURE_2D, id);
	
	GL.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, desc->mag_linear ? GL_LINEAR : GL_NEAREST);
	GL.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, desc->min_linear ? GL_LINEAR : GL_NEAREST);
	GL.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	GL.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	
	const int32 formats[4] = { GL_RED, 0, GL_RGB, GL_RGBA, };
	int32 format = formats[desc->channels - 1];
	
	GL.glTexImage2D(GL_TEXTURE_2D, 0, format, desc->width, desc->height, 0, format, GL_UNSIGNED_BYTE, desc->pixels);
	GL.glBindTexture(GL_TEXTURE_2D, 0);
	
	*out_texture = (Render_Texture) {
		.width = desc->width,
		.height = desc->height,
		.opengl.id = id,
	};
	
	return true;
}

static bool
OpenGL_MakeTexture2DFromFile(const Render_Texture2DDesc* desc, Render_Texture2D* out_texture)
{
	if (!desc->encoded_image || !desc->encoded_image_size)
		return false;
	
	Assert(desc->encoded_image_size < INT32_MAX); // NOTE(ljre): stb_image limitation...
	int32 width, height, channels;
	void* pixels = stbi_load_from_memory(desc->encoded_image, desc->encoded_image_size, &width, &height, &channels, 0);
	
	if (!pixels)
		return false;
	
	uint32 id;
	GL.glGenTextures(1, &id);
	GL.glBindTexture(GL_TEXTURE_2D, id);
	
	GL.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, desc->mag_linear ? GL_LINEAR : GL_NEAREST);
	GL.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, desc->min_linear ? GL_LINEAR : GL_NEAREST);
	GL.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	GL.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	
	const int32 formats[4] = { GL_RED, 0, GL_RGB, GL_RGBA, };
	int32 format = formats[channels - 1];
	
	GL.glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, pixels);
	GL.glBindTexture(GL_TEXTURE_2D, 0);
	
	stbi_image_free(pixels);
	
	*out_texture = (Render_Texture) {
		.width = width,
		.height = height,
		.opengl.id = id,
	};
	
	return true;
}

static bool
OpenGL_MakeShader(const Render_ShaderDesc* desc, Render_Shader* out_shader)
{
	char info[512];
	int32 success = true;
	
	// Compile Vertex Shader
	uint32 vertex_shader = GL.glCreateShader(GL_VERTEX_SHADER);
	GL.glShaderSource(vertex_shader, 1, &desc->opengl.vertex_shader_source, NULL);
	GL.glCompileShader(vertex_shader);
	
	// Check for errors
	GL.glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &success);
	if (!success) {
		GL.glGetShaderInfoLog(vertex_shader, sizeof(info), NULL, info);
		Platform_DebugMessageBox("Vertex Shader Error:\n\n%s", info);
		
		GL.glDeleteShader(vertex_shader);
		return false;
	}
	
	// Compile Fragment Shader
	uint32 fragment_shader = GL.glCreateShader(GL_FRAGMENT_SHADER);
	GL.glShaderSource(fragment_shader, 1, &desc->opengl.fragment_shader_source, NULL);
	GL.glCompileShader(fragment_shader);
	
	// Check for errors
	GL.glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &success);
	if (!success) {
		GL.glGetShaderInfoLog(fragment_shader, sizeof(info), NULL, info);
		Platform_DebugMessageBox("Fragment Shader Error:\n\n%s", info);
		
		GL.glDeleteShader(vertex_shader);
		GL.glDeleteShader(fragment_shader);
		return false;
	}
	
	// Link
	uint32 program = GL.glCreateProgram();
	GL.glAttachShader(program, vertex_shader);
	GL.glAttachShader(program, fragment_shader);
	GL.glLinkProgram(program);
	
	// Check for errors
	GL.glGetProgramiv(program, GL_LINK_STATUS, &success);
	if (!success) {
		GL.glGetProgramInfoLog(program, sizeof(info), NULL, info);
		Platform_DebugMessageBox("Shader Linking Error:\n\n%s", info);
		
		GL.glDeleteProgram(program);
		GL.glDeleteShader(vertex_shader);
		GL.glDeleteShader(fragment_shader);
		return false;
	}
	
	// Clean-up
	GL.glDeleteShader(vertex_shader);
	GL.glDeleteShader(fragment_shader);
	
	// Uniforms
	uint32 u_view_matrix = GL.glGetUniformLocation(program, desc->opengl.uniform_view_matrix);
	uint32 u_texture_sampler = GL.glGetUniformLocation(program, desc->opengl.uniform_texture_sampler);
	
	if (!u_view_matrix || !u_texture_sampler)
	{
		GL.glDeleteProgram(program);
		return false;
	}
	
	*out_shader = (Render_Shader) {
		.opengl = {
			.id = program,
			.uniform_view_matrix_loc = u_view_matrix,
			.uniform_texture_sampler_loc = u_texture_sampler,
			
			.attrib_vertex_position = desc->opengl.attrib_vertex_position,
			.attrib_vertex_normal = desc->opengl.attrib_vertex_normal,
			.attrib_vertex_texcoord = desc->opengl.attrib_vertex_texcoord,
			
			.attrib_color = desc->opengl.attrib_color,
			.attrib_texcoords = desc->opengl.attrib_texcoords,
			.attrib_transform = desc->opengl.attrib_transform,
		},
	};
	
	return true;
}

static bool
OpenGL_MakeFont(const Render_FontDesc* desc, Render_Font* out_font)
{
	return false;
}

//~ NOTE(ljre): Render functions
static void
OpenGL_ClearColor(const vec4 color)
{
	GL.glClearColor(color[0], color[1], color[2], color[3]);
	GL.glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
}

static void
OpenGL_Draw2D(const Render_Data2D* batch)
{
	Trace();
	
	mat4 view;
	Engine_CalcViewMatrix2D(&data->camera, view);
	
	uint32 vbo, vao;
	uintsize count = data->instance_count;
	uintsize size = count * sizeof(Render_Data2DInstance);
	Assert(count <= INT32_MAX);
	
	uint32 texture_id = global_ogl_default_diffuse_texture;
	if (data->texture)
		texture_id = data->texture->opengl.id;
	
	const Render_Shader* shader = &global_ogl_shader_default2d;
	if (data->shader)
		shader = data->shader;
	
	// NOTE(ljre): Config OpenGL State
	GL.glViewport(0, 0, global_engine.platform->window_width, global_engine.platform->window_height);
	GL.glDisable(GL_DEPTH_TEST);
	GL.glDisable(GL_CULL_FACE);
	GL.glEnable(GL_BLEND);
	
	switch (data->blendmode)
	{
		case Render_BlendMode_Normal: GL.glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); break;
		case Render_BlendMode_Add: GL.glBlendFunc(GL_SRC_ALPHA, GL_ONE); break;
		case Render_BlendMode_Subtract: GL.glBlendFunc(GL_ZERO, GL_ONE_MINUS_SRC_COLOR); break;
		
		case Render_BlendMode_None: GL.glDisable(GL_BLEND); break;
		default: Unreachable(); break;
	}
	
	//- NOTE(ljre): Build Batch
	{
		GL.glGenVertexArrays(1, &vao);
		GL.glBindVertexArray(vao);
		
		uint8 av_pos = shader->opengl.attrib_vertex_position;
		uint8 av_norm = shader->opengl.attrib_vertex_normal;
		uint8 av_texc = shader->opengl.attrib_vertex_texcoord;
		
		GL.glBindBuffer(GL_ARRAY_BUFFER, global_ogl_quad_vbo);
		GL.glEnableVertexAttribArray(av_pos);
		GL.glVertexAttribPointer(av_pos, 3, GL_FLOAT, false, sizeof(OpenGL_Vertex), (void*)offsetof(OpenGL_Vertex, position));
		GL.glEnableVertexAttribArray(av_norm);
		GL.glVertexAttribPointer(av_norm, 3, GL_FLOAT, false, sizeof(OpenGL_Vertex), (void*)offsetof(OpenGL_Vertex, normal));
		GL.glEnableVertexAttribArray(av_texc);
		GL.glVertexAttribPointer(av_texc, 2, GL_FLOAT, false, sizeof(OpenGL_Vertex), (void*)offsetof(OpenGL_Vertex, texcoord));
		
		GL.glGenBuffers(1, &vbo);
		
		GL.glBindBuffer(GL_ARRAY_BUFFER, vbo);
		GL.glBufferData(GL_ARRAY_BUFFER, size, data->instances, GL_STATIC_DRAW);
		
		uint8 a_color = shader->opengl.attrib_color;
		uint8 a_texc = shader->opengl.attrib_texcoords;
		uint8 a_trans = shader->opengl.attrib_transform;
		
		GL.glEnableVertexAttribArray(a_color);
		GL.glVertexAttribDivisor(a_color, 1);
		GL.glVertexAttribPointer(a_color, 4, GL_FLOAT, false, sizeof(Render_Data2DInstance), (void*)offsetof(Render_Data2DInstance, color));
		GL.glEnableVertexAttribArray(a_texc);
		GL.glVertexAttribDivisor(a_texc, 1);
		GL.glVertexAttribPointer(a_texc, 4, GL_FLOAT, false, sizeof(Render_Data2DInstance), (void*)offsetof(Render_Data2DInstance, texcoords));
		GL.glEnableVertexAttribArray(a_trans+0);
		GL.glVertexAttribDivisor(a_trans+0, 1);
		GL.glVertexAttribPointer(a_trans+0, 4, GL_FLOAT, false, sizeof(Render_Data2DInstance), (void*)offsetof(Render_Data2DInstance, transform[0]));
		GL.glEnableVertexAttribArray(a_trans+1);
		GL.glVertexAttribDivisor(a_trans+1, 1);
		GL.glVertexAttribPointer(a_trans+1, 4, GL_FLOAT, false, sizeof(Render_Data2DInstance), (void*)offsetof(Render_Data2DInstance, transform[1]));
		GL.glEnableVertexAttribArray(a_trans+2);
		GL.glVertexAttribDivisor(a_trans+2, 1);
		GL.glVertexAttribPointer(a_trans+2, 4, GL_FLOAT, false, sizeof(Render_Data2DInstance), (void*)offsetof(Render_Data2DInstance, transform[2]));
		GL.glEnableVertexAttribArray(a_trans+3);
		GL.glVertexAttribDivisor(a_trans+3, 1);
		GL.glVertexAttribPointer(a_trans+3, 4, GL_FLOAT, false, sizeof(Render_Data2DInstance), (void*)offsetof(Render_Data2DInstance, transform[3]));
		
		GL.glBindBuffer(GL_ARRAY_BUFFER, 0);
	}
	
	//- NOTE(ljre): Render Batch
	{
		GL.glUseProgram(shader->opengl.id);
		GL.glUniformMatrix4fv(shader->opengl.uniform_view_matrix_loc, 1, false, (float32*)view);
		GL.glUniform1i(shader->opengl.uniform_texture_sampler_loc, 0);
		
		GL.glActiveTexture(GL_TEXTURE0);
		GL.glBindTexture(GL_TEXTURE_2D, texture_id);
		
		GL.glBindVertexArray(vao);
		GL.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, global_ogl_quad_ebo);
		
		GL.glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0, (int32)count);
		
		GL.glBindVertexArray(0);
		GL.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
		GL.glUseProgram(0);
	}
	
	//- NOTE(ljre): Free Batch
	{
		GL.glDeleteBuffers(1, &vbo);
		GL.glDeleteVertexArrays(1, &vao);
	}
}

static void
OpenGL_DrawText(const Render_DrawTextData* data)
{
	
}

static void
OpenGL_CalcTextSize(const Render_Font* font, String text, vec2* out_size)
{
	
}

//~ NOTE(ljre): Resource deallocation
static void
OpenGL_FreeTexture2D(Render_Texture2D* texture)
{
	GL.glDeleteTextures(1, &texture->opengl.id);
	Mem_Set(texture, 0, sizeof(*texture));
}

static void
OpenGL_FreeFont(Render_Font* font)
{
	// ...
	Mem_Set(shader, 0, sizeof(*font));
}

static void
OpenGL_FreeShader(Render_Shader* shader)
{
	GL.glDeleteProgram(&shader->opengl.id);
	Mem_Set(shader, 0, sizeof(*shader));
}

//~ NOTE(ljre): Init
static bool
OpenGL_Init(const Engine_RenderApi** out_api)
{
	static const Engine_RenderApi = {
		.make_texture_2d = OpenGL_MakeTexture2D,
		.make_texture_2d_from_file = OpenGL_MakeTexture2DFromFile,
		.make_font = OpenGL_MakeFont,
		.make_shader = OpenGL_MakeShader,
		
		.clear_color = OpenGL_ClearColor,
		.draw_2d = OpenGL_Draw2D,
		.draw_text = OpenGL_DrawText,
		
		.calc_text_size = OpenGL_CalcTextSize,
		
		.free_texture_2d = OpenGL_FreeTexture2D,
		.free_font = OpenGL_FreeFont,
		.free_shader = OpenGL_FreeShader,
	};
	
	*out_api = &api;
	
	return true;
}

static void
OpenGL_Deinit(void)
{
	// TODO
}

#undef GL
