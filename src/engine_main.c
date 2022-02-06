//~ API
API void
Engine_FinishFrame(void)
{
	Trace("Engine_FinishFrame");
	
	if (!global_engine.outputed_sound_this_frame)
		Engine_PlayAudios(NULL, NULL, 1.0f);
	
	global_engine.outputed_sound_this_frame = false;
	
	Platform_FinishFrame();
	TraceFrameMark();
	Platform_PollEvents();
	
	float64 current_time = Platform_GetTime();
	global_engine.delta_time = (float32)( (current_time - global_engine.last_frame_time) * REFERENCE_FPS );
	global_engine.last_frame_time = current_time;
}

//~ Entry Point
API int32
Engine_Main(int32 argc, char** argv)
{
	Trace("Engine_Main");
	
	global_engine.persistent_arena = Arena_Create(Gigabytes(2), Megabytes(32));
	global_engine.temp_arena = Arena_Create(Megabytes(128), Megabytes(8));
	global_engine.delta_time = 1.0f;
	global_engine.running = true;
	
	Platform_Config config = {
		.window_width = 1280,
		.window_height = 720,
		.window_title = StrI("Title"),
		.center_window = true,
		
		.graphics_api = GraphicsAPI_OpenGL,
	};
	
	// NOTE(ljre): Window width & height
	if (!Platform_CreateWindow(&config, &global_graphics))
		Platform_ExitWithErrorMessage(Str("Your computer doesn't seem to support OpenGL 3.3.\nFailed to open."));
	
	Engine_InitRender();
	global_engine.last_frame_time = Platform_GetTime();
	
	// NOTE(ljre): Run
#ifdef INTERNAL_ENABLE_HOT
	do
	{
		void(*func)(Engine_Data*) = Platform_LoadGameLibrary();
		
		func(&global_engine);
	}
	while (global_engine.running);
#else
	do
		Game_Main(&global_engine);
	while (global_engine.running);
#endif
	
	// NOTE(ljre): Deinit
	Engine_DeinitRender();
	
	Arena_Destroy(global_engine.persistent_arena);
	Arena_Destroy(global_engine.temp_arena);
	return 0;
}
