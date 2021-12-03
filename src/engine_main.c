#define REFERENCE_FPS 60
#define DISCORD_APP_ID 0 // NOTE(ljre): Optional. Only matters if you are going to use Discord Game SDK

struct alignas(16) StackAllocatorHeader typedef StackAllocatorHeader;
struct StackAllocatorHeader
{
	StackAllocatorHeader* previous;
	uintsize size; // NOTE(ljre): This includes the size of the header itself
};

//~ Globals
internal uint8* global_stack_memory; // NOTE(ljre): not 'void*' so we can do arithmetic
internal uintsize global_stack_commited;
internal uintsize global_stack_reserved;
internal StackAllocatorHeader* global_stack_header;
internal const uintsize global_stack_page_size = Megabytes(16);
internal float32 global_delta_time = 1.0f;
internal float64 global_last_time;

//~ API
API void*
Engine_PushMemory(uintsize size)
{
	Trace("Engine_PushMemory");
	
	uint8* result = NULL;
	size = AlignUp(size, 15u) + sizeof(StackAllocatorHeader);
	
	if (global_stack_header)
	{
		result = (uint8*)global_stack_header + global_stack_header->size;
	}
	else
	{
		result = global_stack_memory;
	}
	
	uintsize total_desired_size = (uintsize)(result - (uintsize)global_stack_memory + size);
	
	if (total_desired_size > global_stack_reserved)
	{
		// TODO(ljre): Fallback?
		Assert(false);
	}
	else
	{
		while (total_desired_size > global_stack_commited)
		{
			Platform_VirtualCommit(global_stack_memory + global_stack_commited, global_stack_page_size);
			global_stack_commited += global_stack_page_size;
		}
		
		memset(result, 0, size);
		
		StackAllocatorHeader* header = (void*)result;
		header->size = size;
		header->previous = global_stack_header;
		global_stack_header = header;
		
		result += sizeof(*header);
	}
	
	return result;
}

API void*
Engine_RePushMemory(void* ptr, uintsize size)
{
	Trace("Engine_RePushMemory");
	void* result;
	
	if (!ptr)
	{
		result = Engine_PushMemory(size);
	}
	else
	{
		StackAllocatorHeader* header = ptr;
		header -= 1;
		
		uintsize this_size = header->size - sizeof(StackAllocatorHeader);
		if (this_size <= size)
		{
			result = ptr;
		}
		else
		{
			result = Engine_PushMemory(size);
			memcpy(result, ptr, this_size);
			Engine_PopMemory(ptr);
		}
	}
	
	return result;
}

API void
Engine_PopMemory(void* ptr)
{
	Trace("Engine_PopMemory");
	
	if (!ptr) // NOTE(ljre): similar behaviour from CRT 'free()'
		return;
	
	if ((uint8*)ptr < global_stack_memory || (uint8*)ptr >= global_stack_memory + global_stack_reserved)
	{
		// TODO(ljre): Fallback
		Assert(false);
	}
	else
	{
		StackAllocatorHeader* header = ptr;
		header -= 1;
		
		if (global_stack_header == header)
		{
			global_stack_header = header->previous;
		}
		else
		{
			StackAllocatorHeader* next_header = (void*)((uint8*)header + header->size);
			next_header->previous = header->previous;
			
			if (header->previous)
			{
				header->previous->size += header->size;
			}
		}
	}
}

API void*
Engine_PushMemoryState(void)
{
	return global_stack_header;
}

API void
Engine_PopMemoryState(void* state)
{
	global_stack_header = state;
}

API float32
Engine_DeltaTime(void)
{
	return global_delta_time;
}

API void
Engine_FinishFrame(void)
{
	Trace("Engine_FinishFrame");
	
	if (!global_outputed_sound_this_frame)
		Engine_PlayAudios(NULL, NULL, 1.0f);
	
	global_outputed_sound_this_frame = false;
	
	Discord_Update();
	
	Platform_FinishFrame();
	Platform_PollEvents();
	
	float64 current_time = Platform_GetTime();
	global_delta_time = (float32)( (current_time - global_last_time) * REFERENCE_FPS );
	global_last_time = current_time;
}

//~ Entry Point
API int32
Engine_Main(int32 argc, char** argv)
{
	Trace("Engine_Main");
	
	// NOTE(ljre): Init Global Stack Allocator
	{
		global_stack_reserved = Gigabytes(2);
		global_stack_memory = Platform_VirtualReserve(global_stack_reserved);
		if (!global_stack_memory)
			Platform_ExitWithErrorMessage(Str("Could not reserve enough memory. Please report this error."));
		
		global_stack_commited = global_stack_page_size;
		Platform_VirtualCommit(global_stack_memory, global_stack_commited);
		global_stack_header = NULL;
	}
	
	// NOTE(ljre): Window width & height
	if (!Platform_CreateWindow(1280, 720, Str("Title"), GraphicsAPI_OpenGL, &global_graphics))
		Platform_ExitWithErrorMessage(Str("Your computer doesn't seem to support OpenGL 3.3.\nFailed to open."));
	
	Engine_InitRandom();
	Engine_InitRender();
	Discord_Init(DISCORD_APP_ID);
	
	global_last_time = Platform_GetTime();
	
	// NOTE(ljre): Run
	Scene current_scene = Game_MainScene;
	void* shared_data = NULL;
	
	while (current_scene)
	{
		current_scene = current_scene(&shared_data);
	}
	
	// NOTE(ljre): Deinit
	Discord_Deinit();
	Engine_DeinitRender();
	return 0;
}
