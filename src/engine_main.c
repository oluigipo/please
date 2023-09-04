//~ API
API void
E_FinishFrame(void)
{
	Trace();
	
	ArenaClear(global_engine.frame_arena);
	TraceFrameEnd();
	RB_Present(global_engine.renderbackend);
	TraceFrameBegin();
	
	uint64 frequency;
	uint64 current_tick = OS_CurrentTick(&frequency);
	
	for (intsize i = ArrayLength(global_engine.frame_snap_history)-1; i > 0; --i)
		global_engine.frame_snap_history[i] = global_engine.frame_snap_history[i-1];
	
	int32 snap_values[] = { 30, 60, 75, 120, 240 };
	uint64 delta = current_tick - global_engine.last_frame_tick;
	uint64 record = UINT64_MAX;
	intsize record_index = 0;
	
	for (intsize i = 0; i < ArrayLength(snap_values); ++i)
	{
		uint64 snapped = delta * snap_values[i];
		uint64 check = snapped < frequency ? frequency - snapped : snapped - frequency;
		
		if (check < record)
		{
			record = check;
			record_index = i;
		}
	}
	
	global_engine.frame_snap_history[0] = snap_values[record_index];
	global_engine.last_frame_tick = current_tick;
	global_engine.raw_frame_tick_delta = delta;
	
	intsize highest_snap = snap_values[record_index];
	for (intsize i = 1; i < ArrayLength(global_engine.frame_snap_history); ++i)
		highest_snap = Max(highest_snap, global_engine.frame_snap_history[i]);
	
	global_engine.delta_time = 1.0f / (float32)highest_snap;
}

//~ Entry Point
#if 0
API int32
OS_UserMain(const OS_UserMainArgs* args)
{
	Trace();
	
	if (args->thread_id > 0)
		E_WorkerThreadProc_(&global_engine.worker_threads[args->thread_id - 1]);
	else
	{
		// NOTE(ljre): Init basic stuff
		const int32 worker_thread_count = args->thread_count-1;
		const uintsize pagesize = 16ull << 20;
		const uintsize sz_scratch     = 32ull << 20;
		const uintsize sz_frame       = 32ull << 20;
		const uintsize sz_persistent  = 64ull << 20;
		const uintsize sz_audiothread = 256ull << 10;
		
		uintsize game_memory_size = sz_frame + sz_persistent + sz_audiothread + sz_scratch*args->thread_count;
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
			
			global_engine.scratch_arena = ArenaFromUncommitedMemory(memory_head, sz_scratch, pagesize);
			memory_head += sz_scratch;
			
			global_engine.frame_arena = ArenaFromUncommitedMemory(memory_head, sz_frame, pagesize);
			memory_head += sz_frame;
			
			global_engine.persistent_arena = ArenaFromUncommitedMemory(memory_head, sz_persistent, pagesize);
			memory_head += sz_persistent;
			
			for (int32 i = 0; i < worker_thread_count; ++i)
			{
				E_ThreadCtx* ctx = &global_engine.worker_threads[i];
				
				ctx->id = i+1;
				ctx->scratch_arena = ArenaFromUncommitedMemory(memory_head, sz_scratch, pagesize);
				memory_head += sz_scratch;
			}
			
			global_engine.audio_thread_arena = ArenaFromUncommitedMemory(memory_head, sz_audiothread, sz_audiothread);
			memory_head += sz_audiothread;
			
			Assert(memory_head == memory_end);
		}
		
		// NOTE(ljre): Allocate structs
		global_engine.thread_work_queue = ArenaPushStruct(global_engine.persistent_arena, E_ThreadWorkQueue);
		global_engine.audio = ArenaPushStruct(global_engine.audio_thread_arena, E_AudioState);
		
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
		global_engine.last_frame_tick = OS_CurrentTick(NULL);
		
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
		STM_Deinit();
#endif
		E_DeinitRender_();
		E_DeinitAudio_();
	}
	
	return 0;
}
#endif

static void
E_AppSetup_(const OS_SetupArgs* args, OS_InitDesc* out_init)
{
	// NOTE(ljre): Init basic stuff
	const int32 worker_thread_count = args->worker_thread_count;
	const uintsize pagesize = 16ull << 20;
	const uintsize sz_scratch     = 32ull << 20;
	const uintsize sz_frame       = 32ull << 20;
	const uintsize sz_persistent  = 64ull << 20;
	const uintsize sz_audiothread = 256ull << 10;
	
	uintsize game_memory_size = sz_frame + sz_persistent + sz_audiothread + sz_scratch*(1+worker_thread_count);
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
		
		global_engine.scratch_arena = ArenaFromUncommitedMemory(memory_head, sz_scratch, pagesize);
		memory_head += sz_scratch;
		
		global_engine.frame_arena = ArenaFromUncommitedMemory(memory_head, sz_frame, pagesize);
		memory_head += sz_frame;
		
		global_engine.persistent_arena = ArenaFromUncommitedMemory(memory_head, sz_persistent, pagesize);
		memory_head += sz_persistent;
		
		for (int32 i = 0; i < worker_thread_count; ++i)
		{
			E_ThreadCtx* ctx = &global_engine.worker_threads[i];
			
			ctx->id = i+1;
			ctx->scratch_arena = ArenaFromUncommitedMemory(memory_head, sz_scratch, pagesize);
			memory_head += sz_scratch;
		}
		
		global_engine.audio_thread_arena = ArenaFromUncommitedMemory(memory_head, sz_audiothread, sz_audiothread);
		memory_head += sz_audiothread;
		
		Assert(memory_head == memory_end);
	}
	
	// NOTE(ljre): Allocate structs
	global_engine.thread_work_queue = ArenaPushStruct(global_engine.persistent_arena, E_ThreadWorkQueue);
	global_engine.audio = ArenaPushStruct(global_engine.audio_thread_arena, E_AudioState);
	
	if (worker_thread_count > 0)
	{
		OS_InitSemaphore(&global_engine.thread_work_queue->semaphore, worker_thread_count);
		OS_InitEventSignal(&global_engine.thread_work_queue->reached_zero_doing_work_sig);
	}
	
	OS_InitRWLock(&global_engine.mt_lock);
	
	*out_init = (OS_InitDesc) {
		.workerthread_user_data = NULL,
		.audiothread_user_data = global_engine.audio,
	};
}

static void
E_AppUpdate_(OS_State* os)
{
	global_engine.os = os;
	
	if (!global_engine.renderbackend)
	{
		E_InitAudio_();
		E_InitRender_();
	}
	
#ifdef CONFIG_ENABLE_HOT
	void(*func)(E_GlobalData*) = OS_LoadGameLibrary();
	
	func(&global_engine);
#else
	G_Main(&global_engine);
#endif
	
	if (!global_engine.running)
	{
		os->is_terminating = true;
	}
}

static void
E_AppShutdown_(OS_State* os)
{
#ifdef CONFIG_ENABLE_STEAM
	STM_Deinit();
#endif
	E_DeinitRender_();
	E_DeinitAudio_();
}

static void
E_AppWorkerThreadProc_(void* user_data, int32 index)
{
	E_WorkerThreadProc_(&global_engine.worker_threads[index - 1]);
}

static void
E_AppPullAudioSamples_(void* user_data, int16* restrict out_buffer, int32 channels, int32 sample_rate, int32 sample_count)
{
	E_AudioThreadProc_(user_data, out_buffer, channels, sample_rate, sample_count);
}

API const OS_AppApi*
GetAppApi(void)
{
	static const OS_AppApi api = {
		.setup = E_AppSetup_,
		.update = E_AppUpdate_,
		.shutdown = E_AppShutdown_,
		.pull_audio_samples = E_AppPullAudioSamples_,
		.worker_thread_proc = E_AppWorkerThreadProc_,
	};
	
	return &api;
}
