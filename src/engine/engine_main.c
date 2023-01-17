//~ API
API void
Engine_FinishFrame(void)
{
	Trace();
	
	if (!global_engine.outputed_sound_this_frame)
		Engine_PlayAudios(NULL, NULL, 1.0f);
	
	global_engine.outputed_sound_this_frame = false;
	
	global_engine.graphics_context->present_and_vsync(1);
	TraceFrameMark();
	OS_PollEvents(global_engine.window_state, global_engine.input);
	
	float64 current_time = OS_GetTimeInSeconds();
	global_engine.delta_time = (float32)( (current_time - global_engine.last_frame_time) * REFERENCE_FPS );
	global_engine.last_frame_time = current_time;
}

//~ Entry Point
API int32
OS_UserMain(int32 argc, char* argv[])
{
	Trace();
	
	// NOTE(ljre): Desired initial state
	OS_WindowState window_state = { 0 };
	OS_DefaultWindowState(&window_state);
	
	window_state.width = 1280;
	window_state.height = 720;
	Mem_Copy(window_state.title, "Title", 6);
	window_state.center_window = true;
	
	// NOTE(ljre): Init basic stuff
	{
		uintsize game_memory_size = 512ull << 20;
		void* game_memory = OS_VirtualReserve(game_memory_size); // 512 MiB
		
		global_engine.game_memory = game_memory;
		global_engine.game_memory_size = game_memory_size;
		global_engine.delta_time = 1.0f;
		global_engine.running = true;
		
		// NOTE(ljre): Reserving pieces of the game memory to different arenas
		{
			uint8* memory_head = (uint8*)game_memory;
			uint8* memory_end = (uint8*)game_memory + game_memory_size;
			
			// NOTE(ljre): 64MiB of scratch_arena
			global_engine.scratch_arena = Arena_FromUncommitedMemory(memory_head, 64 << 20, 8 << 20);
			memory_head += 64 << 20;
			
			// NOTE(ljre): All the rest of persistent_arena
			global_engine.persistent_arena = Arena_FromUncommitedMemory(memory_head, memory_end - memory_head, 32 << 20);
		}
		
		// NOTE(ljre): Allocate structs
		global_engine.input = Arena_PushStruct(global_engine.persistent_arena, OS_InputState);
		global_engine.window_state = Arena_PushStructData(global_engine.persistent_arena, OS_WindowState, &window_state);
	}
	
	OS_InitDesc init_desc = {
		.flags = OS_InitFlags_WindowAndGraphics | OS_InitFlags_SimpleAudio,
		
		.window_initial_state = window_state,
		.window_desired_api = 0,
		
		.simpleaudio_desired_sample_rate = 48000,
	};
	
	OS_InitOutput output;
	
	if (!OS_Init(&init_desc, &output))
		OS_ExitWithErrorMessage("Failed to initialize platform layer.");
	
	*global_engine.window_state = output.window_state;
	*global_engine.input = output.input_state;
	global_engine.graphics_context = output.graphics_context;
	
	// NOTE(ljre): Init global_engine structure
	OS_PollEvents(global_engine.window_state, global_engine.input);
	
	// NOTE(ljre): Init everything else
	Engine_InitRender(&global_engine.render);
	
	// NOTE(ljre): Run
	global_engine.last_frame_time = OS_GetTimeInSeconds();
	
#ifdef CONFIG_ENABLE_HOT
	do
	{
		void(*func)(Engine_Data*) = OS_LoadGameLibrary();
		
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
	
	return 0;
}
