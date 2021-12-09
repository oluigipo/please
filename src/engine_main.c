//~ API
API float32
Engine_DeltaTime(void)
{
	return global_engine.delta_time;
}

API void
Engine_FinishFrame(void)
{
	Trace("Engine_FinishFrame");
	
	if (!global_engine.outputed_sound_this_frame)
		Engine_PlayAudios(NULL, NULL, 1.0f);
	
	global_engine.outputed_sound_this_frame = false;
	
	Discord_Update();
	
	Platform_FinishFrame();
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
	
	global_engine.persistent_arena = Arena_Create(Gigabytes(2), Megabytes(16));
	global_engine.temp_arena = Arena_Create(Megabytes(128), Megabytes(8));
	global_engine.delta_time = 1.0f;
	
	// NOTE(ljre): Window width & height
	if (!Platform_CreateWindow(1280, 720, Str("Title"), GraphicsAPI_OpenGL, &global_graphics))
		Platform_ExitWithErrorMessage(Str("Your computer doesn't seem to support OpenGL 3.3.\nFailed to open."));
	
	Engine_InitRandom();
	Engine_InitRender();
	Discord_Init(DISCORD_APP_ID);
	
	global_engine.last_frame_time = Platform_GetTime();
	
	// NOTE(ljre): Run
	global_engine.current_scene = Game_MainScene;
	
	do
		global_engine.current_scene(&global_engine);
	while (global_engine.current_scene);
	
	// NOTE(ljre): Deinit
	Discord_Deinit();
	Engine_DeinitRender();
	
	Arena_Destroy(global_engine.persistent_arena);
	Arena_Destroy(global_engine.temp_arena);
	return 0;
}
