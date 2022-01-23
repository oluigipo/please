#ifndef INTERNAL_ARENA_H
#define INTERNAL_ARENA_H

struct Arena
{
	uintsize reserved;
	uintsize commited;
	uintsize offset;
	uintsize page_size;
	
	alignas(16) uint8 memory[];
}
typedef Arena;

API void* Platform_VirtualReserve(uintsize size);
API void Platform_VirtualCommit(void* ptr, uintsize size);
API void Platform_VirtualFree(void* ptr, uintsize size);

internal Arena*
Arena_Create(uintsize reserved, uintsize page_size)
{
	Assert(IsPowerOf2(page_size));
	Assert(reserved > 0);
	
	reserved = AlignUp(reserved, page_size-1);
	Arena* result = Platform_VirtualReserve(reserved + sizeof(Arena));
	if (result)
	{
		Platform_VirtualCommit(result, sizeof(Arena) + page_size);
		
		result->reserved = reserved;
		result->commited = page_size;
		result->offset = 0;
		result->page_size = page_size;
	}
	
	return result;
}

internal void*
Arena_PushDirtyAligned(Arena* arena, uintsize size, uintsize alignment_mask)
{
	Assert(IsPowerOf2(alignment_mask+1));
	
	arena->offset = AlignUp(arena->offset, alignment_mask);
	uintsize needed = arena->offset + size;
	
	if (needed > arena->commited)
	{
		uintsize size_to_commit = AlignUp(arena->commited - needed, arena->page_size);
		Assert(size_to_commit + arena->commited <= arena->reserved);
		
		Platform_VirtualCommit(arena->memory + arena->commited, size_to_commit);
		arena->commited += size_to_commit;
	}
	
	void* result = arena->memory + arena->offset;
	arena->offset += size;
	
	return result;
}

internal inline void*
Arena_PushDirty(Arena* arena, uintsize size)
{
	return Arena_PushDirtyAligned(arena, size, 15);
}

internal inline void*
Arena_PushAligned(Arena* arena, uintsize size, uintsize alignment_mask)
{
	return memset(Arena_PushDirtyAligned(arena, size, alignment_mask), 0, size);
}

internal inline void*
Arena_Push(Arena* arena, uintsize size)
{
	return memset(Arena_PushDirtyAligned(arena, size, 15), 0, size);
}

internal void
Arena_Pop(Arena* arena, void* ptr)
{
	uint8* p = ptr;
	Assert(p >= arena->memory && p <= arena->memory + arena->offset);
	
	uintsize new_offset = p - arena->memory;
	arena->offset = new_offset;
}

internal void*
Arena_End(Arena* arena)
{
	return arena->memory + arena->offset;
}

internal void
Arena_Destroy(Arena* arena)
{
	Platform_VirtualFree(arena, arena->reserved + sizeof(Arena));
}

#endif //INTERNAL_ARENA_H
