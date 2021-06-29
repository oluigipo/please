#include <cglm/cglm.h>

#define GL (*opengl)
internal const OpenGL_VTable* opengl;
internal int32 global_uniform_color;
internal int32 global_uniform_matrix;
internal mat4 global_proj;

internal const char* global_vertex_shader =
"#version 330 core\n"
"layout (location = 0) in vec2 aPosition;\n"

"uniform mat4 uMatrix;\n"

"void main() {"
"    gl_Position = uMatrix * vec4(aPosition, 0.0, 1.0);"
"}"
;

internal const char* global_fragment_shader =
"#version 330 core\n"

"out vec4 oFragColor;"

"uniform vec4 uColor;"

"void main() {"
"    oFragColor = uColor;"
"}"
;

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
DrawRectangle(mat4 where, vec4 color, vec3 size)
{
	mat4 matrix;
	glm_mat4_copy(where, matrix);
	glm_scale(matrix, size);
	glm_mat4_mul(global_proj, matrix, matrix);
	
	opengl->glUniform4fv(global_uniform_color, 1, color);
	opengl->glUniformMatrix4fv(global_uniform_matrix, 1, false, (float32*)matrix);
	
	opengl->glDrawArrays(GL_TRIANGLES, 0, 6);
}

API int32
Engine_Main(int32 argc, char** argv)
{
	float32 width = 600.0f;
	float32 height = 600.0f;
	
	if (!Platform_CreateWindow((int32)width, (int32)height, Str("Title"), GraphicsAPI_OpenGL))
		Platform_ExitWithErrorMessage(Str("Could not create window with OpenGL"));
	
	opengl = Platform_GetOpenGLVTable();
	
#ifdef DEBUG
	opengl->glEnable(GL_DEBUG_OUTPUT);
	opengl->glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
	opengl->glDebugMessageCallback(OpenGLDebugMessageCallback, NULL);
#endif
	
	//opengl->glEnable(GL_BLEND);
	//opengl->glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	//opengl->glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	opengl->glViewport(0, 0, 600, 600);
	glm_ortho(0.0f, width, height, 0.0f, -1.0f, 1.0f, global_proj);
	
	float32 vertices[] = {
		-0.5f, -0.5f,
		-0.5f, 0.5f,
		0.5f, -0.5f,
		0.5f, -0.5f,
		-0.5f, 0.5f,
		0.5f, 0.5f,
	};
	
	uint32 vbo, vao;
	opengl->glGenBuffers(1, &vbo);
	opengl->glBindBuffer(GL_ARRAY_BUFFER, vbo);
	opengl->glBufferData(GL_ARRAY_BUFFER, sizeof vertices, vertices, GL_STATIC_DRAW);
	
	opengl->glGenVertexArrays(1, &vao);
	opengl->glBindVertexArray(vao);
	opengl->glEnableVertexAttribArray(0);
	opengl->glVertexAttribPointer(0, 2, GL_FLOAT, false, sizeof(float32) * 2, 0);
	
	uint32 shader = CompileShader(global_vertex_shader, global_fragment_shader);
	global_uniform_color = opengl->glGetUniformLocation(shader, "uColor");
	global_uniform_matrix = opengl->glGetUniformLocation(shader, "uMatrix");
	
	
	while (!Platform_WindowShouldClose())
	{
		const Input_Gamepad* gamepad = Input_GetGamepad(0);
		
		if (gamepad)
		{
			opengl->glClearColor(0.5f, 0.0f, 1.0f, 1.0f);
			opengl->glClear(GL_COLOR_BUFFER_BIT);
			
			mat4 pos;
			vec4 black = { 0.1f, 0.1f, 0.1f, 1.0f };
			vec4 colors[2] = {
				{ 0.3f, 0.3f, 0.3f, 1.0f }, // Enabled
				{ 0.9f, 0.9f, 0.9f, 1.0f }, // Disabled
			};
			
			opengl->glBindVertexArray(vao);
			opengl->glUseProgram(shader);
			
			// NOTE(ljre): Draw Axes
			{
				glm_mat4_identity(pos);
				glm_translate(pos, (vec3) { width / 2.0f - 100.0f, height / 2.0f + 100.0f });
				DrawRectangle(pos, black, (vec3) { 50.0f, 50.0f });
				
				glm_translate(pos, (vec3) { gamepad->left[0] * 20.0f, gamepad->left[1] * 20.0f });
				DrawRectangle(pos,
							  colors[Input_GamepadIsDown(gamepad, Input_GamepadButton_LS)],
							  (vec3) { 20.0f, 20.0f });
				
				glm_mat4_identity(pos);
				glm_translate(pos, (vec3) { width / 2.0f + 100.0f, height / 2.0f + 100.0f });
				DrawRectangle(pos, black, (vec3) { 50.0f, 50.0f });
				
				glm_translate(pos, (vec3) { gamepad->right[0] * 20.0f, gamepad->right[1] * 20.0f });
				DrawRectangle(pos,
							  colors[Input_GamepadIsDown(gamepad, Input_GamepadButton_RS)],
							  (vec3) { 20.0f, 20.0f });
			}
			
			// NOTE(ljre): Draw Buttons
			{
				struct Pair
				{
					vec3 pos;
					vec3 size;
				} buttons[] = {
					[Input_GamepadButton_Y] = { { width * 0.84f, height * 0.44f }, { 30.0f, 30.0f } },
					[Input_GamepadButton_B] = { { width * 0.90f, height * 0.50f }, { 30.0f, 30.0f } },
					[Input_GamepadButton_A] = { { width * 0.84f, height * 0.56f }, { 30.0f, 30.0f } },
					[Input_GamepadButton_X] = { { width * 0.78f, height * 0.50f }, { 30.0f, 30.0f } },
					
					[Input_GamepadButton_LB] = { { width * 0.20f, height * 0.30f }, { 60.0f, 20.0f } },
					[Input_GamepadButton_RB] = { { width * 0.80f, height * 0.30f }, { 60.0f, 20.0f } },
					[Input_GamepadButton_LT] = { { width * 0.20f, height * 0.23f }, { 60.0f, 30.0f } },
					[Input_GamepadButton_RT] = { { width * 0.80f, height * 0.23f }, { 60.0f, 30.0f } },
					
					[Input_GamepadButton_Up]    = { { width * 0.16f, height * 0.44f }, { 30.0f, 30.0f } },
					[Input_GamepadButton_Right] = { { width * 0.22f, height * 0.50f }, { 30.0f, 30.0f } },
					[Input_GamepadButton_Down]  = { { width * 0.16f, height * 0.56f }, { 30.0f, 30.0f } },
					[Input_GamepadButton_Left]  = { { width * 0.10f, height * 0.50f }, { 30.0f, 30.0f } },
					
					[Input_GamepadButton_Back]  = { { width * 0.45f, height * 0.48f }, { 25.0f, 15.0f } },
					[Input_GamepadButton_Start] = { { width * 0.55f, height * 0.48f }, { 25.0f, 15.0f } },
				};
				
				for (int32 i = 0; i < ArrayLength(buttons); ++i)
				{
					glm_mat4_identity(pos);
					glm_translate(pos, buttons[i].pos);
					DrawRectangle(pos, colors[Input_GamepadIsDown(gamepad, i)], buttons[i].size);
				}
			}
		}
		else
		{
			opengl->glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
			opengl->glClear(GL_COLOR_BUFFER_BIT);
		}
		opengl->glSwapBuffers();
		Platform_PollEvents();
	}
	
	return 0;
}
