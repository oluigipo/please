#define GL (*global_engine.graphics_context->opengl)

struct OpenGL_Vertex
{
	vec3 position;
	vec3 normal;
	vec2 texcoord;
}
typedef OpenGL_Vertex;

static uint32 global_ogl_quad_vbo, global_ogl_quad_ebo;
static Render_Shader global_ogl_shader_default2d;
static uint32 global_ogl_default_diffuse_texture;

//~ NOTE(ljre): Functions
#ifdef DEBUG
static void APIENTRY
OpenGL_DebugMessageCallback_(GLenum source, GLenum type, GLuint id, GLenum severity,
	GLsizei length, const GLchar* message, const void* userParam)
{
	if (type == GL_DEBUG_TYPE_ERROR)
		Platform_DebugMessageBox("OpenGL Error:\nType = 0x%x\nID = %u\nSeverity = 0x%x\nMessage= %s",
		type, id, severity, message);
	else
		Platform_DebugLog("OpenGL Debug Callback:\n\tType = 0x%x\n\tID = %u\n\tSeverity = 0x%x\n\tmessage = %s\n",
		type, id, severity, message);
}
#endif

// NOTE(ljre): flags =
//                 1 - has depth
//                 2 - has stencil
static bool
OpenGL_MakeDepthStencilTexture_(const Render_Texture2DDesc* desc, Render_Texture2D* out_texture, uint8 flags)
{
	Assert(desc->width && desc->height);
	Assert(flags != 0 && flags <= 3);
	
	int32 format = 0;
	switch (flags)
	{
		case 1:
		{
			const int32 int_formats[] = {
				GL_DEPTH_COMPONENT, GL_DEPTH_COMPONENT16, GL_DEPTH_COMPONENT24, GL_DEPTH_COMPONENT32
			};
			const int32 float_formats[] = {
				0, 0, 0, GL_DEPTH_COMPONENT32F,
			};
			
			format = (desc->float_components) ? float_formats[desc->channels] : int_formats[desc->channels];
		} break;
		
		case 2:
		{
			format = GL_RED;
		} break;
		
		case 3:
		{
			format = (desc->float_components) ? GL_DEPTH32F_STENCIL8 : GL_DEPTH24_STENCIL8;
		} break;
		
		default: Unreachable(); break;
	}
	
	Assert(format != 0);
	
	uint32 id;
	GL.glGenTextures(1, &id);
	GL.glBindTexture(GL_TEXTURE_2D, id);
	GL.glTexImage2D(GL_TEXTURE_2D, 0, format, desc->width, desc->height, 0, GL_RED, GL_UNSIGNED_BYTE, NULL);
	
	GL.glBindTexture(GL_TEXTURE_2D, 0);
	
	*out_texture = (Render_Texture2D) {
		.width = desc->width,
		.height = desc->height,
		
		.opengl.id = id,
	};
	
	return true;
}

//~ NOTE(ljre): Resource allocation
static bool
OpenGL_MakeTexture2D(const Render_Texture2DDesc* desc, Render_Texture2D* out_texture)
{
	if (!desc->encoded_image)
	{
		// NOTE(ljre): Load uncompressed image.
		//             desc->pixels can be NULL.
		
		if (!desc->width || !desc->height || desc->channels > 4 || desc->channels == 0)
			return false;
		
		uint32 id;
		GL.glGenTextures(1, &id);
		GL.glBindTexture(GL_TEXTURE_2D, id);
		
		GL.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, desc->mag_linear ? GL_LINEAR : GL_NEAREST);
		GL.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, desc->min_linear ? GL_LINEAR : GL_NEAREST);
		GL.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		GL.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		
		const int32 float_formats[4] = { GL_R32F, GL_RG32F, GL_RGB32F, GL_RGBA32F };
		const int32 int_formats[4] = { GL_RED, GL_RG, GL_RGB, GL_RGBA, };
		int32 format = (desc->float_components) ? float_formats[desc->channels - 1] : int_formats[desc->channels - 1];
		int32 load_format = int_formats[desc->channels - 1];
		
		GL.glTexImage2D(GL_TEXTURE_2D, 0, format, desc->width, desc->height, 0, load_format, (desc->float_components) ? GL_FLOAT : GL_UNSIGNED_BYTE, desc->pixels);
		GL.glBindTexture(GL_TEXTURE_2D, 0);
		
		*out_texture = (Render_Texture2D) {
			.width = desc->width,
			.height = desc->height,
			.opengl.id = id,
		};
		
		return true;
	}
	else
	{
		// NOTE(ljre): Load compressed PNG image.
		if (!desc->encoded_image_size)
			return false;
		
		Assert(desc->encoded_image_size <= INT32_MAX); // NOTE(ljre): stb_image limitation...
		
		int32 width, height, channels;
		void* pixels = stbi_load_from_memory(desc->encoded_image, (int32)desc->encoded_image_size, &width, &height, &channels, 0);
		
		if (!pixels)
		{
			stbi_image_free(pixels);
			return false;
		}
		
		uint32 id;
		GL.glGenTextures(1, &id);
		GL.glBindTexture(GL_TEXTURE_2D, id);
		
		GL.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, desc->mag_linear ? GL_LINEAR : GL_NEAREST);
		GL.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, desc->min_linear ? GL_LINEAR : GL_NEAREST);
		GL.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		GL.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		
		const int32 float_formats[4] = { GL_R32F, GL_RG32F, GL_RGB32F, GL_RGBA32F };
		const int32 int_formats[4] = { GL_RED, GL_RG, GL_RGB, GL_RGBA, };
		int32 format = (desc->float_components) ? float_formats[channels - 1] : int_formats[channels - 1];
		int32 load_format = int_formats[channels - 1];
		
		GL.glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, load_format, GL_UNSIGNED_BYTE, pixels);
		GL.glBindTexture(GL_TEXTURE_2D, 0);
		
		stbi_image_free(pixels);
		
		*out_texture = (Render_Texture2D) {
			.width = width,
			.height = height,
			.opengl.id = id,
		};
		
		return true;
	}
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
	int32 u_view_matrix = GL.glGetUniformLocation(program, desc->opengl.uniform_view_matrix);
	int32 u_texture_sampler = GL.glGetUniformLocation(program, desc->opengl.uniform_texture_sampler);
	
	if (u_view_matrix == -1 || u_texture_sampler == -1)
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

static bool
OpenGL_MakeFramebuffer(const Render_FramebufferDesc* desc, Render_Framebuffer* out_fb)
{
	bool ok = true;
	int32 width = desc->width;
	int32 height = desc->height;
	Assert(width && height);
	
	if (!desc->color_attachment.encoded_image && !desc->color_attachment.width)
	{
		Assert((!desc->depth_attachment.encoded_image && desc->depth_attachment.width) ^
			(!desc->depth_stencil_attachment.encoded_image && desc->depth_stencil_attachment.width) ^
			(!desc->stencil_attachment.encoded_image && desc->stencil_attachment.width));
	}
	
	Render_Framebuffer fb = {
		.width = width,
		.height = height,
	};
	
	GL.glGenFramebuffers(1, &fb.opengl.id);
	GL.glBindFramebuffer(GL_FRAMEBUFFER, fb.opengl.id);
	
	if (desc->color_attachment.encoded_image || desc->color_attachment.width)
	{
		ok = ok && OpenGL_MakeTexture2D(&desc->color_attachment, &fb.color_attachment);
		
		GL.glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fb.color_attachment.opengl.id, 0);
	}
	
	if (desc->depth_attachment.width)
	{
		ok = ok && OpenGL_MakeDepthStencilTexture_(&desc->depth_attachment, &fb.depth_attachment, 1);
		
		GL.glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, fb.depth_attachment.opengl.id, 0);
	}
	else if (desc->depth_stencil_attachment.width)
	{
		ok = ok && OpenGL_MakeDepthStencilTexture_(&desc->depth_attachment, &fb.depth_attachment, 3);
		GL.glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, fb.depth_stencil_attachment.opengl.id, 0);
	}
	else if (desc->stencil_attachment.width)
	{
		ok = ok && OpenGL_MakeDepthStencilTexture_(&desc->depth_attachment, &fb.depth_attachment, 2);
		GL.glFramebufferTexture2D(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_TEXTURE_2D, fb.stencil_attachment.opengl.id, 0);
	}
	
	if (GL.glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		Assert(false);
	
	GL.glBindFramebuffer(GL_FRAMEBUFFER, 0);
	
	*out_fb = fb;
	return true;
}

//~ NOTE(ljre): Render functions
static void
OpenGL_ClearColor(const vec4 color)
{
	GL.glClearColor(color[0], color[1], color[2], color[3]);
	GL.glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
}

static void
OpenGL_ClearFramebuffer(Render_Framebuffer* fb, const vec4 color)
{
	GL.glBindFramebuffer(GL_FRAMEBUFFER, fb->opengl.id);
	
	uint32 bits = 0;
	if (fb->color_attachment.opengl.id)
		bits |= GL_COLOR_BUFFER_BIT;
	if (fb->depth_attachment.opengl.id)
		bits |= GL_DEPTH_BUFFER_BIT;
	if (fb->depth_stencil_attachment.opengl.id)
		bits |= GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT;
	if (fb->stencil_attachment.opengl.id)
		bits |= GL_STENCIL_BUFFER_BIT;
	
	GL.glClearColor(color[0], color[1], color[2], color[3]);
	GL.glClear(bits);
	
	GL.glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

static void
OpenGL_Draw2D(const Render_Data2D* data)
{
	Trace();
	
	uint32 vbo, vao;
	uintsize count = data->instance_count;
	uintsize size = count * sizeof(Render_Data2DInstance);
	Assert(count <= INT32_MAX);
	
	uint32 texture_id = global_ogl_default_diffuse_texture;
	if (data->texture)
	{
		texture_id = data->texture->opengl.id;
		Assert(texture_id);
	}
	
	const Render_Shader* shader = data->shader;
	if (!shader)
		shader = &global_ogl_shader_default2d;
	
	// NOTE(ljre): Config OpenGL State
	GL.glDisable(GL_DEPTH_TEST);
	GL.glDisable(GL_CULL_FACE);
	GL.glEnable(GL_BLEND);
	
	int32 width, height;
	
	if (data->framebuffer)
	{
		const Render_Framebuffer* fb = data->framebuffer;
		Assert(fb->opengl.id);
		
		GL.glBindFramebuffer(GL_FRAMEBUFFER, fb->opengl.id);
		
		width = fb->width;
		height = fb->height;
	}
	else
	{
		width = global_engine.platform->window_width;
		height = global_engine.platform->window_height;
	}
	
	GL.glViewport(0, 0, width, height);
	
	mat4 view;
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
		
		if (data->framebuffer)
			GL.glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
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
	Mem_Set(font, 0, sizeof(*font));
}

static void
OpenGL_FreeShader(Render_Shader* shader)
{
	GL.glDeleteProgram(shader->opengl.id);
	Mem_Set(shader, 0, sizeof(*shader));
}

static void
OpenGL_FreeFramebuffer(Render_Framebuffer* fb)
{
	GL.glDeleteFramebuffers(1, &fb->opengl.id);
	if (fb->color_attachment.opengl.id)
		GL.glDeleteTextures(1, &fb->color_attachment.opengl.id);
	if (fb->depth_attachment.opengl.id)
		GL.glDeleteTextures(1, &fb->depth_attachment.opengl.id);
	if (fb->depth_stencil_attachment.opengl.id)
		GL.glDeleteTextures(1, &fb->depth_stencil_attachment.opengl.id);
	if (fb->stencil_attachment.opengl.id)
		GL.glDeleteTextures(1, &fb->stencil_attachment.opengl.id);
	
	Mem_Set(fb, 0, sizeof(*fb));
}

//~ NOTE(ljre): Init
static bool
OpenGL_Init(const Engine_RenderApi** out_api)
{
	static const Engine_RenderApi api = {
		.make_texture_2d = OpenGL_MakeTexture2D,
		.make_font = OpenGL_MakeFont,
		.make_shader = OpenGL_MakeShader,
		.make_framebuffer = OpenGL_MakeFramebuffer,
		
		.clear_color = OpenGL_ClearColor,
		.draw_2d = OpenGL_Draw2D,
		.draw_text = OpenGL_DrawText,
		
		.calc_text_size = OpenGL_CalcTextSize,
		
		.free_texture_2d = OpenGL_FreeTexture2D,
		.free_font = OpenGL_FreeFont,
		.free_shader = OpenGL_FreeShader,
		.free_framebuffer = OpenGL_FreeFramebuffer,
	};
	
	const OpenGL_Vertex quad[] = {
		// Positions         // Normals           // Texcoords
		{ 0.0f, 0.0f, 0.0f,    0.0f, 0.0f, 1.0f,    0.0f, 0.0f, },
		{ 1.0f, 0.0f, 0.0f,    0.0f, 0.0f, 1.0f,    1.0f, 0.0f, },
		{ 1.0f, 1.0f, 0.0f,    0.0f, 0.0f, 1.0f,    1.0f, 1.0f, },
		{ 0.0f, 1.0f, 0.0f,    0.0f, 0.0f, 1.0f,    0.0f, 1.0f, },
	};
	
	const uint32 quad_indices[] = {
		0, 1, 2,
		2, 3, 0,
	};
	
	const uint32 default_diffuse[] = {
		0xFFFFFFFFu, 0xFFFFFFFFu,
		0xFFFFFFFFu, 0xFFFFFFFFu,
	};
	
	const Render_ShaderDesc default2d_shader = {
		.opengl = {
			.vertex_shader_source =
				"#version 330 core\n"
				"layout (location = 0) in vec3 aPosition;\n"
				"layout (location = 1) in vec3 aNormal;\n"
				"layout (location = 2) in vec2 aTexCoord;\n"
				
				"layout (location = 4) in vec4 aColor;\n"
				"layout (location = 5) in vec4 aTexCoords;\n"
				"layout (location = 6) in mat4 aTransform;\n"
				
				"out vec2 vTexCoord;\n"
				"out vec4 vColor;\n"
				
				"uniform mat4 uView;\n"
				
				"void main() {\n"
				"    gl_Position = uView * aTransform * vec4(aPosition, 1.0);\n"
				//"    gl_Position = vec4(aPosition, 1.0);\n"
				"    vTexCoord = aTexCoords.xy + aTexCoords.zw * vec2(aTexCoord.x, aTexCoord.y);\n"
				"    vColor = aColor;\n"
				"}\n",
			
			.fragment_shader_source =
				"#version 330 core\n"
				
				"in vec2 vTexCoord;\n"
				"in vec4 vColor;\n"
				
				"out vec4 oFragColor;\n"
				
				"uniform sampler2D uTexture;\n"
				
				"void main() {\n"
				"    oFragColor = vColor * texture(uTexture, vTexCoord);\n"
				"}\n",
			
			.attrib_vertex_position = 0,
			.attrib_vertex_normal = 1,
			.attrib_vertex_texcoord = 2,
			
			.attrib_color = 4,
			.attrib_texcoords = 5,
			.attrib_transform = 6,
			
			.uniform_view_matrix = "uView",
			.uniform_texture_sampler = "uTexture",
		},
	};
	
	GL.glGenBuffers(1, &global_ogl_quad_vbo);
	GL.glBindBuffer(GL_ARRAY_BUFFER, global_ogl_quad_vbo);
	GL.glBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_STATIC_DRAW);
	GL.glBindBuffer(GL_ARRAY_BUFFER, 0);
	
	GL.glGenBuffers(1, &global_ogl_quad_ebo);
	GL.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, global_ogl_quad_ebo);
	GL.glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(quad_indices), quad_indices, GL_STATIC_DRAW);
	GL.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	
	GL.glGenTextures(1, &global_ogl_default_diffuse_texture);
	GL.glBindTexture(GL_TEXTURE_2D, global_ogl_default_diffuse_texture);
	
	GL.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	GL.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	GL.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	GL.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	
	GL.glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 2, 2, 0, GL_RGBA, GL_UNSIGNED_BYTE, default_diffuse);
	GL.glBindTexture(GL_TEXTURE_2D, 0);
	
	{
		bool ok = OpenGL_MakeShader(&default2d_shader, &global_ogl_shader_default2d);
		Assert(ok);
	}
	
#ifdef DEBUG
	GL.glEnable(GL_DEBUG_OUTPUT);
	GL.glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
	GL.glDebugMessageCallback(OpenGL_DebugMessageCallback_, NULL);
#endif
	
	GL.glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	
	*out_api = &api;
	return true;
}

static void
OpenGL_Deinit(void)
{
	// TODO
}

#undef GL
