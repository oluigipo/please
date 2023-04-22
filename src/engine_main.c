//~ API
API void
E_FinishFrame(void)
{
	Trace();
	
	TraceFrameEnd();
	RB_Present(global_engine.renderbackend);
	TraceFrameBegin();
	
	Arena_Clear(global_engine.frame_arena);
	OS_PollEvents();
	
	float64 current_time = OS_GetTimeInSeconds();
	global_engine.delta_time = (float32)(current_time - global_engine.last_frame_time);
	global_engine.last_frame_time = current_time;
}

//~ Entry Point
API int32
OS_UserMain(const OS_UserMainArgs* args)
{
	Trace();
	
	if (args->thread_id > 0)
	{
		E_WorkerThreadProc_(&global_engine.worker_threads[args->thread_id - 1]);
	}
	else
	{
		// NOTE(ljre): Init basic stuff
		const int32 worker_thread_count = args->thread_count-1;
		const uintsize pagesize = 16ull << 20;
		const uintsize sz_scratch     = 16ull << 20;
		const uintsize sz_frame       = 32ull << 20;
		const uintsize sz_persistent  = 64ull << 20;
		const uintsize sz_audiothread = 256ull << 10;
		
		uintsize game_memory_size = sz_scratch + sz_frame + sz_persistent + sz_audiothread;
		for (int32 i = 0; i < worker_thread_count; ++i)
			game_memory_size += sz_scratch;
		
		void* game_memory = OS_VirtualReserve(NULL, game_memory_size);
		
		global_engine.game_memory = game_memory;
		global_engine.game_memory_size = game_memory_size;
		global_engine.delta_time = 1.0f;
		global_engine.running = true;
		global_engine.enable_vsync = true;
		global_engine.worker_thread_count = worker_thread_count;
		
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
			
			for (int32 i = 0; i < worker_thread_count; ++i)
			{
				E_ThreadCtx* ctx = &global_engine.worker_threads[i];
				
				ctx->id = i+1;
				ctx->scratch_arena = Arena_FromUncommitedMemory(memory_head, sz_scratch, pagesize);
				memory_head += sz_scratch;
			}
			
			global_engine.audio_thread_arena = Arena_FromUncommitedMemory(memory_head, sz_audiothread, sz_audiothread);
			memory_head += sz_audiothread;
			
			Assert(memory_head == memory_end);
		}
		
		// NOTE(ljre): Allocate structs
		global_engine.thread_work_queue = Arena_PushStruct(global_engine.persistent_arena, E_ThreadWorkQueue);
		global_engine.audio = Arena_PushStruct(global_engine.audio_thread_arena, E_AudioState);
		
		if (worker_thread_count > 0)
		{
			OS_InitSemaphore(&global_engine.thread_work_queue->semaphore, worker_thread_count);
			OS_InitEventSignal(&global_engine.thread_work_queue->reached_zero_doing_work_sig);
		}
		
		OS_InitRWLock(&global_engine.mt_lock);
		
		OS_InitDesc init_desc = {
			.audiothread_proc = E_AudioThreadProc_,
			.audiothread_user_data = global_engine.audio,
		};
		
		if (!OS_Init(&init_desc, &global_engine.os))
			OS_ExitWithErrorMessage("Failed to initialize platform layer.");
		
		OS_PollEvents();
		
		// NOTE(ljre): Init everything else
		E_InitAudio_();
		E_InitRender_();
		
		// NOTE(ljre): Run
		global_engine.last_frame_time = OS_GetTimeInSeconds();
		
#ifdef CONFIG_ENABLE_HOT
		do
		{
			void(*func)(E_GlobalData*) = OS_LoadGameLibrary();
			
			func(&global_engine);
		}
		while (global_engine.running && !global_engine.os->is_terminating);
#else
		do
			G_Main(&global_engine);
		while (global_engine.running && !global_engine.os->is_terminating);
#endif
		
		// NOTE(ljre): Deinit
#ifdef CONFIG_ENABLE_STEAM
		S_Deinit();
#endif
		E_DeinitRender_();
		E_DeinitAudio_();
	}
	
	return 0;
}
