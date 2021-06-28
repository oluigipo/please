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

API int32
Engine_Main(int32 argc, char** argv)
{
	if (!Platform_CreateWindow(600, 600, Str("Title"), GraphicsAPI_OpenGL))
		Platform_ExitWithErrorMessage(Str("Could not create window with OpenGL"));
	
	const OpenGL_VTable* opengl = Platform_GetOpenGLVTable();
	
#ifdef DEBUG
	opengl->glEnable(GL_DEBUG_OUTPUT);
	opengl->glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
	opengl->glDebugMessageCallback(OpenGLDebugMessageCallback, NULL);
#endif
	
	opengl->glEnable(GL_BLEND);
	opengl->glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	opengl->glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	opengl->glViewport(0, 0, 600, 600);
	
	while (!Platform_WindowShouldClose())
	{
		if (Input_GetGamepad(0))
			opengl->glClearColor(0.5f, 0.0f, 1.0f, 1.0f);
		else
			opengl->glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		opengl->glClear(GL_COLOR_BUFFER_BIT);
		
		opengl->glSwapBuffers();
		Platform_PollEvents();
	}
	
	return 0;
}
