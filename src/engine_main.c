//~ API
API void
E_FinishFrame(void)
{
	Trace();
	
	if (!global_engine.outputed_sound_this_frame)
		E_PlayAudios(NULL, NULL, 1.0f);
	global_engine.outputed_sound_this_frame = false;
	
	if (global_engine.resource_command_list)
	{
		RB_ExecuteResourceCommands(global_engine.scratch_arena, global_engine.resource_command_list);
		
		global_engine.resource_command_list = NULL;
		global_engine.last_resource_command = NULL;
	}
	
	if (global_engine.draw_command_list)
	{
		RB_ExecuteDrawCommands(global_engine.scratch_arena, global_engine.draw_command_list, global_engine.window_state->width, global_engine.window_state->height);
		
		global_engine.draw_command_list = NULL;
		global_engine.last_draw_command = NULL;
	}
	
	RB_Present(global_engine.scratch_arena, 1);
	TraceFrameMark();
	
	Arena_Clear(global_engine.frame_arena);
	OS_PollEvents(global_engine.window_state, global_engine.input);
	
	float64 current_time = OS_GetTimeInSeconds();
	global_engine.delta_time = (float32)(current_time - global_engine.last_frame_time);
	global_engine.last_frame_time = current_time;
}

static bool
E_Streq_(const char* left, const char* right)
{
	for (;;)
	{
		if (*left != *right)
			return false;
		if (!*left)
			return true;
		++left;
		++right;
	}
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
			
			// NOTE(ljre): 16MiB of scratch_arena
			global_engine.scratch_arena = Arena_FromUncommitedMemory(memory_head, 16 << 20, 8 << 20);
			memory_head += 16 << 20;
			
			// NOTE(ljre): 64MiB of frame_arena
			global_engine.frame_arena = Arena_FromUncommitedMemory(memory_head, 64 << 20, 8 << 20);
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
		//.window_desired_api = OS_WindowGraphicsApi_OpenGL,
		
		.simpleaudio_desired_sample_rate = 48000,
	};
	
	for (int32 i = 1; i < argc; ++i)
	{
		if (!Mem_Strcmp(argv[i], "-opengl") || !Mem_Strcmp(argv[i], "-gl") || !Mem_Strcmp(argv[i], "-gl3"))
			init_desc.window_desired_api = OS_WindowGraphicsApi_OpenGL;
		else if (!Mem_Strcmp(argv[i], "-d3d11") || !Mem_Strcmp(argv[i], "-dx11"))
			init_desc.window_desired_api = OS_WindowGraphicsApi_Direct3D11;
		else if (!Mem_Strcmp(argv[i], "-audio-44100"))
			init_desc.simpleaudio_desired_sample_rate = 44100;
		else if (!Mem_Strcmp(argv[i], "-audio-48000"))
			init_desc.simpleaudio_desired_sample_rate = 48000;
	}
	
	OS_InitOutput output;
	
	if (!OS_Init(&init_desc, &output))
		OS_ExitWithErrorMessage("Failed to initialize platform layer.");
	
	// NOTE(ljre): Init global_engine structure
	*global_engine.window_state = output.window_state;
	*global_engine.input = output.input_state;
	global_engine.graphics_context = output.graphics_context;
	
	OS_PollEvents(global_engine.window_state, global_engine.input);
	
	// NOTE(ljre): Init everything else
	E_InitRender_();
	
	// NOTE(ljre): Run
	global_engine.last_frame_time = OS_GetTimeInSeconds();
	
#ifdef CONFIG_ENABLE_HOT
	do
	{
		void(*func)(E_GlobalData*) = OS_LoadGameLibrary();
		
		func(&global_engine);
	}
	while (global_engine.running);
#else
	do
		G_Main(&global_engine);
	while (global_engine.running);
#endif
	
	// NOTE(ljre): Deinit
	E_DeinitRender_();
	
	return 0;
}
