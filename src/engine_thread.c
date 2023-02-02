static void
E_WorkerThreadProc_(void* arg)
{
	E_ThreadCtx* ctx = arg;
	E_ThreadWorkQueue* queue = global_engine.thread_work_queue;
	
	for (;;)
	{
		if (!E_RunThreadWork(ctx, queue))
			OS_WaitForSemaphore(&queue->semaphore);
	}
}

//~ API
API bool
E_RunThreadWork(E_ThreadCtx* ctx, E_ThreadWorkQueue* queue)
{
	Trace();
	
	E_ThreadCtx default_ctx;
	if (!ctx)
	{
		default_ctx = (E_ThreadCtx) {
			.scratch_arena = global_engine.scratch_arena,
			.id = 0,
		};
		
		ctx = &default_ctx;
	}
	
	if (!queue)
		queue = global_engine.thread_work_queue;
	
	for (;;)
	{
		int32 doing_head = queue->doing_head;
		if (doing_head == queue->remaining_head)
			break;
		
		int32 next_index = (doing_head+1) % ArrayLength(queue->works);
		int32 index = OS_InterlockedCompareExchange32(&queue->doing_head, next_index, doing_head);
		
		if (index == doing_head)
		{
			int32 doing_count;
			
			OS_InterlockedIncrement32(&queue->doing_count);
			
			E_ThreadWork work = queue->works[index];
			work.callback(ctx, work.data);
			
			doing_count = OS_InterlockedDecrement32(&queue->doing_count);
			if (doing_count == 0 && queue->remaining_head == queue->doing_head)
				OS_SetEventSignal(&queue->reached_zero_doing_work_sig);
			
			return true;
		}
	}
	
	return false;
}

API void
E_QueueThreadWork(const E_ThreadWork* work)
{
	Trace();
	
	E_ThreadWorkQueue* queue = global_engine.thread_work_queue;
	int32 index = queue->remaining_head;
	int32 next_index = (index+1) % ArrayLength(queue->works);
	
	queue->works[index] = *work;
	queue->remaining_head = next_index;
	
	OS_SignalSemaphore(&queue->semaphore, 1);
}

API void
E_WaitRemainingThreadWork(void)
{
	Trace();
	
	OS_WaitEventSignal(&global_engine.thread_work_queue->reached_zero_doing_work_sig);
}
