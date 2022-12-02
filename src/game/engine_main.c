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
{ return global_engine.input->connected_gamepads & (1 << index); }

API int32
Engine_ConnectedGamepadCount(void)
{ return Mem_PopCnt64(global_engine.input->connected_gamepads); }

API int32
Engine_ConnectedGamepadsIndices(int32 out_indices[Engine_MAX_GAMEPAD_COUNT])
{
	uint64 c = global_engine.input->connected_gamepads;
	int32 len = 0;
	
	while (c)
	{
		out_indices[len++] = Mem_BitCtz64(c);
		
		c &= c-1; // NOTE(ljre): Clear last bit
	}
	
	return len;
}

//~ Entry Point
API int32
Engine_Main(int32 argc, char** argv)
{
	Trace();
	
	// NOTE(ljre): Desired initial state
	Engine_PlatformData config = { 0 };
	Platform_DefaultState(&config);
	
	config.window_width = 1280;
	config.window_height = 720;
	config.window_title = Str("Title");
	config.center_window = true;
	config.graphics_api = Engine_GraphicsApi_OpenGL;
	
	// NOTE(ljre): Init basic stuff
	{
		uintsize game_memory_size = 512ull << 20;
		void* game_memory = Platform_VirtualReserve(game_memory_size); // 512 MiB
		
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
			
			// NOTE(ljre): 16MiB of audio memory
			global_engine.audio_arena = Arena_FromUncommitedMemory(memory_head, 16 << 20, 16 << 20);
			memory_head += 16 << 20;
			
			// NOTE(ljre): All the rest of persistent_arena
			global_engine.persistent_arena = Arena_FromUncommitedMemory(memory_head, memory_end - memory_head, 32 << 20);
		}
		
		// NOTE(ljre): Allocate structs
		global_engine.input = Arena_PushStruct(global_engine.persistent_arena, Engine_InputData);
		global_engine.platform = Arena_PushStructData(global_engine.persistent_arena, Engine_PlatformData, &config);
		global_engine.audio = Arena_PushStruct(global_engine.audio_arena, Engine_AudioData);
		
		global_engine.audio->arena = global_engine.audio_arena;
	}
	
	Platform_InitDesc init_desc = {
		.engine = &global_engine,
		
		.audio_thread_proc = Engine_AudioThreadProc_,
		.audio_thread_data = global_engine.audio,
		
		.inout_state = global_engine.platform,
		.out_graphics = &global_engine.graphics_context,
	};
	
	if (!Platform_Init(&init_desc))
		Platform_ExitWithErrorMessage(Str("Failed to initialize platform layer."));
	
	// NOTE(ljre): Init global_engine structure
	Platform_PollEvents(global_engine.platform, global_engine.input);
	
	// NOTE(ljre): Init everything else
	Engine_InitRender(&global_engine.render);
	
	// NOTE(ljre): Run
	global_engine.last_frame_time = Platform_GetTime();
	
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
	
	return 0;
}
