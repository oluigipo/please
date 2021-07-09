#define GL (*global_graphics->opengl)
#define D3D (*global_graphics->d3d)

#ifdef DEBUG
#   define D3DCall(x) do{HRESULT r=(x);if(FAILED(r)){Platform_DebugMessageBox("D3DCall Failed\nFile: " __FILE__ "\nLine: %i\nError code: %lx",__LINE__,r);exit(1);}}while(0)
#else
#   define D3DCall(x) (x)
#endif

//~ Globals
internal const GraphicsContext* global_graphics;
internal const float32 global_quad_vertices[] = {
    // Position          // Normals           // Texcoords
    0.0f, 0.0f, 0.0f,    0.0f, 0.0f, 1.0f,    0.0f, 0.0f,
    0.0f, 1.0f, 0.0f,    0.0f, 0.0f, 1.0f,    0.0f, 1.0f,
    1.0f, 0.0f, 0.0f,    0.0f, 0.0f, 1.0f,    1.0f, 0.0f,
    1.0f, 0.0f, 0.0f,    0.0f, 0.0f, 1.0f,    1.0f, 0.0f,
    0.0f, 1.0f, 0.0f,    0.0f, 0.0f, 1.0f,    0.0f, 1.0f,
    1.0f, 1.0f, 0.0f,    0.0f, 0.0f, 1.0f,    1.0f, 1.0f,
};

internal float32 global_width;
internal float32 global_height;
internal int32 global_uniform_color;
internal int32 global_uniform_matrix;
internal int32 global_uniform_texture;
internal mat4 global_proj;
internal uint32 global_white_texture;

internal const char* const global_vertex_shader =
"#version 330 core\n"
"layout (location = 0) in vec3 aPosition;\n"
"layout (location = 1) in vec3 aNormal;\n"
"layout (location = 2) in vec2 aTexCoord;\n"

"out vec2 vTexCoord;"

"uniform mat4 uMatrix;\n"

"void main() {"
"    gl_Position = uMatrix * vec4(aPosition, 1.0);"
"    vTexCoord = aTexCoord;"
"}"
;

internal const char* const global_fragment_shader =
"#version 330 core\n"

"in vec2 vTexCoord;"

"out vec4 oFragColor;"

"uniform vec4 uColor;"
"uniform sampler2D uTexture;"

"void main() {"
"    oFragColor = uColor * texture(uTexture, vTexCoord);"
"}"
;

//~ Functions
#ifdef DEBUG
internal void APIENTRY
OpenGLDebugMessageCallback(GLenum source, GLenum type, GLuint id, GLenum severity,
                           GLsizei length, const GLchar* message, const void* userParam)
{
    if (type == GL_DEBUG_TYPE_ERROR)
    {
        Platform_DebugMessageBox("OpenGL Error:\n\nType = 0x%x\nID = %u\nSeverity = 0x%x\nMessage= %s",
                                 type, id, severity, message);
    }
    else
    {
        Platform_DebugLog("OpenGL Debug Callback:\n\tType = 0x%x\n\tID = %u\n\tSeverity = 0x%x\n\tmessage = %s\n",
                          type, id, severity, message);
    }
}
#endif

internal uint32
CompileShader(const char* vertex_source, const char* fragment_source)
{
    char info[512];
    int32 success;
    
    // Compile Vertex Shader
    uint32 vertex_shader = GL.glCreateShader(GL_VERTEX_SHADER);
    GL.glShaderSource(vertex_shader, 1, &vertex_source, NULL);
    GL.glCompileShader(vertex_shader);
    
    // Check for errors
    GL.glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        GL.glGetShaderInfoLog(vertex_shader, sizeof info, NULL, info);
        Platform_DebugMessageBox("Vertex Shader Error:\n\n%s", info);
        GL.glDeleteShader(vertex_shader);
        return 0;
    }
    
    // Compile Fragment Shader
    uint32 fragment_shader = GL.glCreateShader(GL_FRAGMENT_SHADER);
    GL.glShaderSource(fragment_shader, 1, &fragment_source, NULL);
    GL.glCompileShader(fragment_shader);
    
    // Check for errors
    GL.glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        GL.glGetShaderInfoLog(fragment_shader, sizeof info, NULL, info);
        Platform_DebugMessageBox("Fragment Shader Error:\n\n%s", info);
        GL.glDeleteShader(vertex_shader);
        GL.glDeleteShader(fragment_shader);
        return 0;
    }
    
    // Link
    uint32 program = GL.glCreateProgram();
    GL.glAttachShader(program, vertex_shader);
    GL.glAttachShader(program, fragment_shader);
    GL.glLinkProgram(program);
    
    // Check for errors
    GL.glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        GL.glGetProgramInfoLog(program, sizeof info, NULL, info);
        Platform_DebugMessageBox("Shader Linking Error:\n\n%s", info);
        GL.glDeleteProgram(program);
        program = 0;
    }
    
    // Clean-up
    GL.glDeleteShader(vertex_shader);
    GL.glDeleteShader(fragment_shader);
    
    return program;
}

internal void
ColorToVec4(ColorARGB color, vec4 out)
{
    out[0] = (float32)(color>>16 & 0xFF) / 255.0f;
    out[1] = (float32)(color>>8 & 0xFF) / 255.0f;
    out[2] = (float32)(color & 0xFF) / 255.0f;
    out[3] = (float32)(color>>24 & 0xFF) / 255.0f;
}

internal void
CalcBitmapSizeForText(const Render_Font* font, float32 scale, String text, int32* out_width, int32* out_height)
{
    int32 width = 0, line_width = 0, height = 0;
	int32 codepoint, it = 0;
	
	height += (int32)roundf((float32)(font->ascent - font->descent + font->line_gap) * scale);
	while (codepoint = String_Decode(text, &it), codepoint)
	{
		if (codepoint == '\n')
		{
			if (line_width > width)
				width = line_width;
			
			line_width = 0;
			height += (int32)roundf((float32)(font->ascent - font->descent + font->line_gap) * scale);
			continue;
		}
		
		int32 advance, bearing;
		stbtt_GetCodepointHMetrics(&font->info, codepoint, &advance, &bearing);
		
		line_width += (int32)roundf((float32)advance * scale);
	}
    
    if (line_width > width)
		width = line_width;
	
    *out_width = width + 1;
    *out_height = height + 1;
}

//~ Temporary Internal API
internal void
Render_Init(void)
{
    Trace("Render_Init");
    
    global_width = (float32)Platform_WindowWidth();
    global_height = (float32)Platform_WindowHeight();
    
    glm_ortho(0.0f, global_width, global_height, 0.0f, -1.0f, 1.0f, global_proj);
    
#ifdef DEBUG
    GL.glEnable(GL_DEBUG_OUTPUT);
    GL.glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    GL.glDebugMessageCallback(OpenGLDebugMessageCallback, NULL);
#endif
    
    GL.glEnable(GL_BLEND);
    GL.glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    GL.glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    GL.glViewport(0, 0, (int32)global_width, (int32)global_height);
    
    uint32 vbo, vao;
    GL.glGenBuffers(1, &vbo);
    GL.glBindBuffer(GL_ARRAY_BUFFER, vbo);
    GL.glBufferData(GL_ARRAY_BUFFER, sizeof global_quad_vertices, global_quad_vertices, GL_STATIC_DRAW);
    
    GL.glGenVertexArrays(1, &vao);
    GL.glBindVertexArray(vao);
    
    GL.glEnableVertexAttribArray(0);
    GL.glVertexAttribPointer(0, 3, GL_FLOAT, false, sizeof(float32) * 8, 0);
    GL.glEnableVertexAttribArray(1);
    GL.glVertexAttribPointer(1, 3, GL_FLOAT, false, sizeof(float32) * 8, (void*)(sizeof(float32) * 3));
    GL.glEnableVertexAttribArray(2);
    GL.glVertexAttribPointer(2, 2, GL_FLOAT, false, sizeof(float32) * 8, (void*)(sizeof(float32) * 6));
    
    uint32 shader = CompileShader(global_vertex_shader, global_fragment_shader);
    global_uniform_color = GL.glGetUniformLocation(shader, "uColor");
    global_uniform_matrix = GL.glGetUniformLocation(shader, "uMatrix");
    global_uniform_texture = GL.glGetUniformLocation(shader, "uTexture");
    
    GL.glBindVertexArray(vao);
    GL.glUseProgram(shader);
    
    uint32 white_texture[] = {
        0xFFFFFFFF, 0xFFFFFFFF,
        0xFFFFFFFF, 0xFFFFFFFF,
    };
    
    GL.glGenTextures(1, &global_white_texture);
    GL.glBindTexture(GL_TEXTURE_2D, global_white_texture);
	GL.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	GL.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    GL.glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 2, 2, 0, GL_RGBA, GL_UNSIGNED_BYTE, white_texture);
    GL.glBindTexture(GL_TEXTURE_2D, 0);
}

internal void
Render_Deinit(void)
{
    Trace("Render_Init");
}

internal void
Render_ClearBackground(ColorARGB color)
{
    vec4 color_vec;
    ColorToVec4(color, color_vec);
    
    GL.glClearColor(color_vec[0], color_vec[1], color_vec[2], color_vec[3]);
    GL.glClear(GL_COLOR_BUFFER_BIT);
}

internal void
Render_DrawRectangle(vec4 color, vec3 pos, vec3 size)
{
    mat4 matrix = GLM_MAT4_IDENTITY;
    glm_translate(matrix, pos);
    glm_scale(matrix, size);
    glm_translate(matrix, (vec3) { -0.5f, -0.5f }); // NOTE(ljre): Center
    glm_mat4_mul(global_proj, matrix, matrix);
    
    GL.glUniform4fv(global_uniform_color, 1, color);
    GL.glUniformMatrix4fv(global_uniform_matrix, 1, false, (float32*)matrix);
    GL.glUniform1i(global_uniform_texture, 0);
    
    GL.glActiveTexture(GL_TEXTURE0);
    GL.glBindTexture(GL_TEXTURE_2D, global_white_texture);
    
    GL.glDrawArrays(GL_TRIANGLES, 0, 6);
    
    GL.glBindTexture(GL_TEXTURE_2D, 0);
}

API bool32
Render_LoadFontFromFile(String path, Render_Font* out_font)
{
    Render_Font result;
    result.data = Platform_ReadEntireFile(path, &result.data_size);
    
    if (result.data)
    {
        if (stbtt_InitFont(&result.info, result.data, 0))
        {
            stbtt_GetFontVMetrics(&result.info, &result.ascent, &result.descent, &result.line_gap);
            *out_font = result;
            return true;
        }
        else
        {
            Platform_FreeFileMemory(result.data, result.data_size);
            return false;
        }
    }
    else
    {
        return false;
    }
}

API void
Render_DrawText(const Render_Font* font, String text, vec3 pos, float32 char_height, ColorARGB color)
{
    float32 scale = stbtt_ScaleForPixelHeight(&font->info, char_height);
    vec4 color_vec;
    ColorToVec4(color, color_vec);
    
    int32 bitmap_width, bitmap_height;
    CalcBitmapSizeForText(font, scale, text, &bitmap_width, &bitmap_height);
    
    uint8* bitmap = Engine_PushMemory((uintsize)(bitmap_width * bitmap_height));
    
    float32 xx = 0, yy = 0;
    int32 codepoint, it = 0;
    while (codepoint = String_Decode(text, &it), codepoint)
    {
        if (codepoint == '\n')
		{
			xx = 0;
			yy += (float32)(font->ascent - font->descent + font->line_gap) * scale;
			continue;
		}
		
		int32 advance, bearing;
		stbtt_GetCodepointHMetrics(&font->info, codepoint, &advance, &bearing);
		
		int32 char_x1, char_y1;
		int32 char_x2, char_y2;
		stbtt_GetCodepointBitmapBox(&font->info, codepoint, scale, scale,
									&char_x1, &char_y1, &char_x2, &char_y2);
		
		int32 char_width = char_x2 - char_x1;
		int32 char_height = char_y2 - char_y1;
		
		int32 end_x = (int32)roundf(xx + (float32)bearing * scale) + char_x1;
		int32 end_y = (int32)roundf(yy + (float32)font->ascent * scale) + char_y1;
		
		int32 offset = end_x + (end_y * bitmap_width);
		stbtt_MakeCodepointBitmap(&font->info, bitmap + offset, char_width, char_height, bitmap_width, scale, scale, codepoint);
		
		xx += (float32)advance * scale;
    }
    
    //Platform_DebugDumpBitmap("test.ppm", bitmap, bitmap_width, bitmap_height, 1);
    
    mat4 matrix = GLM_MAT4_IDENTITY;
    glm_translate(matrix, (vec3) { pos[0], pos[1], pos[2] });
    glm_scale(matrix, (vec3) { (float32)bitmap_width, (float32)bitmap_height });
    glm_mat4_mul(global_proj, matrix, matrix);
    
    GL.glActiveTexture(GL_TEXTURE0);
    
    uint32 texture;
    GL.glGenTextures(1, &texture);
    GL.glBindTexture(GL_TEXTURE_2D, texture);
	GL.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	GL.glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, (int32[]) { GL_ONE, GL_ONE, GL_ONE, GL_RED });
    GL.glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, bitmap_width, bitmap_height, 0, GL_RED, GL_UNSIGNED_BYTE, bitmap);
    
    GL.glUniform4fv(global_uniform_color, 1, color_vec);
    GL.glUniformMatrix4fv(global_uniform_matrix, 1, false, (float32*)matrix);
    GL.glUniform1i(global_uniform_texture, 0);
    
    GL.glDrawArrays(GL_TRIANGLES, 0, 6);
    
    GL.glBindTexture(GL_TEXTURE_2D, 0);
    GL.glDeleteTextures(1, &texture);
    
    Engine_PopMemory(bitmap);
}
