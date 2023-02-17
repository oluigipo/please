//~ API
API void
E_FinishFrame(void)
{
	Trace();
	
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
	
	TraceFrameEnd();
	RB_Present(global_engine.scratch_arena, 1);
	TraceFrameBegin();
	
	Arena_Clear(global_engine.frame_arena);
	OS_PollEvents(global_engine.window_state, global_engine.input);
	
	float64 current_time = OS_GetTimeInSeconds();
	global_engine.delta_time = (float32)(current_time - global_engine.last_frame_time);
	global_engine.last_frame_time = current_time;
}

//~ Entry Point
API int32
OS_UserMain(const OS_UserMainArgs* args)
{
	Trace();
	
	// NOTE(ljre): Desired initial state
	OS_WindowState window_state = args->default_window_state;
	
	window_state.width = 1280;
	window_state.height = 720;
	Mem_Copy(window_state.title, "Title", 6);
	window_state.center_window = true;
	
	OS_InitDesc init_desc = {
		.window_initial_state = window_state,
		//.window_desired_api = OS_WindowGraphicsApi_Direct3D9c,
		
		.workerthreads_count = Min(E_Limits_MaxWorkerThreadCount, args->cpu_core_count/2),
		.workerthreads_args = (void*[E_Limits_MaxWorkerThreadCount]) { 0 },
		.workerthreads_proc = E_WorkerThreadProc_,
		
		.audiothread_proc = E_AudioThreadProc_,
		.audiothread_user_data = NULL,
	};
	
	int32 argc = args->argc;
	const char* const* argv = args->argv;
	
	for (int32 i = 1; i < argc; ++i)
	{
		if (!Mem_Strcmp(argv[i], "-opengl"))
			init_desc.window_desired_api = OS_WindowGraphicsApi_OpenGL;
		else if (!Mem_Strcmp(argv[i], "-d3d11"))
			init_desc.window_desired_api = OS_WindowGraphicsApi_Direct3D11;
		else if (!Mem_Strcmp(argv[i], "-d3d9c"))
			init_desc.window_desired_api = OS_WindowGraphicsApi_Direct3D9c;
		else if (!Mem_Strcmp(argv[i], "-no-worker-threads"))
			init_desc.workerthreads_count = 0;
	}
	
	// NOTE(ljre): Init basic stuff
	{
		const uintsize pagesize = 16ull << 20;
		const uintsize sz_scratch     = 16ull << 20;
		const uintsize sz_frame       = 64ull << 20;
		const uintsize sz_persistent  = 128ull << 20;
		const uintsize sz_audiothread = 256ull << 10;
		
		uintsize game_memory_size = sz_scratch + sz_frame + sz_persistent + sz_audiothread;
		for (int32 i = 0; i < init_desc.workerthreads_count; ++i)
			game_memory_size += sz_scratch;
		
		void* game_memory = OS_VirtualReserve(game_memory_size);
		
		global_engine.game_memory = game_memory;
		global_engine.game_memory_size = game_memory_size;
		global_engine.delta_time = 1.0f;
		global_engine.running = true;
		
		// NOTE(ljre): Reserving pieces of the game memory to different arenas
		{
			uint8* memory_head = (uint8*)game_memory;
			uint8* memory_end = (uint8*)game_memory + game_memory_size;
			
			global_engine.scratch_arena = Arena_FromUncommitedMemory(memory_head, sz_scratch, pagesize);
			memory_head += sz_scratch;
			
			global_engine.frame_arena = Arena_FromUncommitedMemory(memory_head, sz_frame, pagesize);
			memory_head += sz_frame;
			
			global_engine.persistent_arena = Arena_FromUncommitedMemory(memory_head, sz_persistent, pagesize);
			memory_head += sz_persistent;
			
			for (int32 i = 0; i < init_desc.workerthreads_count; ++i)
			{
				E_ThreadCtx* ctx = &global_engine.worker_threads[i];
				
				ctx->id = i+1;
				ctx->scratch_arena = Arena_FromUncommitedMemory(memory_head, sz_scratch, pagesize);
				memory_head += sz_scratch;
				
				init_desc.workerthreads_args[i] = ctx;
			}
			
			global_engine.audio_thread_arena = Arena_FromUncommitedMemory(memory_head, sz_audiothread, sz_audiothread);
			memory_head += sz_audiothread;
			
			Assert(memory_head == memory_end);
		}
		
		// NOTE(ljre): Allocate structs
		global_engine.input = Arena_PushStruct(global_engine.persistent_arena, OS_InputState);
		global_engine.window_state = Arena_PushStructData(global_engine.persistent_arena, OS_WindowState, &window_state);
		global_engine.thread_work_queue = Arena_PushStruct(global_engine.persistent_arena, E_ThreadWorkQueue);
		global_engine.audio = Arena_PushStruct(global_engine.audio_thread_arena, E_AudioState);
		
		OS_InitSemaphore(&global_engine.thread_work_queue->semaphore, init_desc.workerthreads_count);
		OS_InitEventSignal(&global_engine.thread_work_queue->reached_zero_doing_work_sig);
		OS_InitRWLock(&global_engine.mt_lock);
	}
	
	init_desc.audiothread_user_data = global_engine.audio;
	
	OS_InitOutput output;
	
	if (!OS_Init(&init_desc, &output))
		OS_ExitWithErrorMessage("Failed to initialize platform layer.");
	
	// NOTE(ljre): Init global_engine structure
	*global_engine.window_state = output.window_state;
	*global_engine.input = output.input_state;
	global_engine.graphics_context = output.graphics_context;
	global_engine.worker_thread_count = init_desc.workerthreads_count;
	
	OS_PollEvents(global_engine.window_state, global_engine.input);
	
	// NOTE(ljre): Init everything else
	E_InitAudio_(&output);
	E_InitRender_(&output);
	
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
#ifdef CONFIG_ENABLE_STEAM
	S_Deinit();
#endif
	E_DeinitRender_();
	E_DeinitAudio_();
	
	return 0;
}
