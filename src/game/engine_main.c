//~ API
API void
Engine_FinishFrame(void)
{
	Trace();
	
	if (!global_engine.outputed_sound_this_frame)
		Engine_PlayAudios(NULL, NULL, 1.0f);
	
	global_engine.outputed_sound_this_frame = false;
	
	Platform_FinishFrame();
	TraceFrameMark();
	Platform_PollEvents(global_engine.platform, global_engine.input);
	
	float64 current_time = Platform_GetTime();
	global_engine.delta_time = (float32)( (current_time - global_engine.last_frame_time) * REFERENCE_FPS );
	global_engine.last_frame_time = current_time;
}

API bool
Engine_IsGamepadConnected(uint32 index)
{
	return global_engine.input->connected_gamepads & (1 << index);
}

API int32
Engine_ConnectedGamepadCount(void)
{
	return Mem_PopCnt64(global_engine.input->connected_gamepads);
}

API int32
Engine_ConnectedGamepadsIndices(int32 out_indices[Engine_MAX_GAMEPAD_COUNT])
{
	uint64 c = global_engine.input->connected_gamepads;
	int32 len = 0;
	
	while (c)
	{
		out_indices[len++] = Mem_BitCtz32((uint32)c);
		
		c &= c-1; // NOTE(ljre): Clear last bit
	}
	
	return len;
}

//~ Entry Point
API int32
Engine_Main(int32 argc, char** argv)
{
	Trace();
	
	Platform_Data config = { 0 };
	Platform_DefaultData(&config);
	
	config.window_width = 1280;
	config.window_height = 720;
	config.window_title = Str("Title");
	config.center_window = true;
	config.graphics_api = Platform_GraphicsApi_OpenGL;
	
	global_engine.platform = &config;
	global_engine.persistent_arena = Arena_Create(512 << 20, 32 << 20);
	global_engine.temp_arena = Arena_Create(128 << 20, 8 << 20);
	global_engine.delta_time = 1.0f;
	global_engine.running = true;
	global_engine.input = Arena_Push(global_engine.persistent_arena, sizeof(*global_engine.input));
	
	// NOTE(ljre): Window width & height
	if (!Platform_CreateWindow(&config, &global_engine.graphics_context, global_engine.input))
		Platform_ExitWithErrorMessage(Str("Your computer doesn't seem to support OpenGL 3.3.\nFailed to open."));
	
	Engine_InitRender(&global_engine.render);
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
