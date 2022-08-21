#ifndef COMMON_ARENA_H
#define COMMON_ARENA_H

#define Arena_Scope(arena) for (void* end__ = Arena_End(arena); end__; Arena_Pop(arena, end__), end__ = 0)
#define Arena_PushStruct(arena, Type) Arena_PushAligned(arena, sizeof(Type), alignof(Type))
#define Arena_PushStructArray(arena, Type, count) Arena_PushAligned(arena, sizeof(Type)*(count), alignof(Type))
#define Arena_PushStructData(arena, Type, data) MemCopy(Arena_PushStruct(arena, Type), data, sizeof(Type))

struct Arena
{
	// NOTE(ljre): If this arena owns it's memory, then 'page_size != 0'.
	// This is needed to check if we can actually commit more memory.
	//
	// An arena created using 'Arena_FromMemory' does not own it's memory.
	
	uintsize reserved;
	uintsize commited;
	uintsize offset;
	uintsize page_size;
	
	alignas(16) uint8 memory[];
}
typedef Arena;

#ifndef Arena_OsReserve_
#   if defined(_WIN32)
extern void* __stdcall VirtualAlloc(void* base, uintsize size, unsigned long type, unsigned long protect);
extern int32 __stdcall VirtualFree(void* base, uintsize size, unsigned long type);
#       define Arena_OsReserve_(size) VirtualAlloc(NULL,size,0x00002000/*MEM_RESERVE*/,0x04/*PAGE_READWRITE*/)
#       define Arena_OsCommit_(ptr, size) VirtualAlloc(ptr,size,0x00001000/*MEM_COMMIT*/,0x04/*PAGE_READWRITE*/)
#       define Arena_OsFree_(ptr, size) ((void)(size), VirtualFree(ptr,0,0x00008000/*MEM_RELEASE*/))
#   elif defined(__linux__)
#       include <sys/mman.h>
#       define Arena_OsReserve_(size) mmap(NULL,size,PROT_NONE,MAP_ANONYMOUS|MAP_PRIVATE,-1,0)
#       define Arena_OsCommit_(ptr, size) mprotect(ptr,size,PROT_READ|PROT_WRITE)
#       define Arena_OsFree_(ptr, size) munmap(ptr,size)
#   endif
#endif

#ifndef Arena_DEFAULT_ALIGNMENT
#   define Arena_DEFAULT_ALIGNMENT 16
#endif

internal Arena*
Arena_Create(uintsize reserved, uintsize page_size)
{
	Assert(IsPowerOf2(page_size));
	Assert(reserved > 0);
	
	reserved = AlignUp(reserved, page_size-1);
	Arena* result = (Arena*)Arena_OsReserve_(reserved + sizeof(Arena));
	if (result)
	{
		Arena_OsCommit_(result, sizeof(Arena) + page_size);
		
		result->reserved = reserved;
		result->commited = page_size;
		result->offset = 0;
		result->page_size = page_size;
	}
	
	return result;
}

internal Arena*
Arena_FromMemory(void* memory, uintsize size)
{
	Assert(size > sizeof(Arena));
	Assert(((uintptr)memory & ~15) == (uintptr)memory);
	
	Arena* result = (Arena*)memory;
	size -= sizeof(Arena);
	
	result->reserved = size;
	result->commited = size;
	result->offset = 0;
	result->page_size = 0;
	
	return result;
}

internal void
Arena_Destroy(Arena* arena)
{ Arena_OsFree_(arena, arena->reserved + sizeof(Arena)); }

internal void*
Arena_EndAligned(Arena* arena, uintsize alignment)
{
	arena->offset = AlignUp(arena->offset, alignment-1);
	return arena->memory + arena->offset;
}

internal void*
Arena_PushDirtyAligned(Arena* arena, uintsize size, uintsize alignment)
{
	Assert(IsPowerOf2(alignment));
	
	arena->offset = AlignUp(arena->offset, alignment-1);
	uintsize needed = arena->offset + size;
	
	if (Unlikely(needed > arena->commited))
	{
		// NOTE(ljre): If we don't own this memory, just return NULL as we can't commit
		//             more memory.
		if (!arena->page_size)
			return NULL;
		
		uintsize size_to_commit = AlignUp(needed - arena->commited, arena->page_size-1);
		Assert(size_to_commit + arena->commited <= arena->reserved);
		
		Arena_OsCommit_(arena->memory + arena->commited, size_to_commit);
		arena->commited += size_to_commit;
	}
	
	void* result = arena->memory + arena->offset;
	arena->offset += size;
	
	return result;
}

internal void
Arena_Pop(Arena* arena, void* ptr)
{
	uint8* p = (uint8*)ptr;
	Assert(p >= arena->memory && p <= arena->memory + arena->offset);
	
	uintsize new_offset = p - arena->memory;
	arena->offset = new_offset;
}

internal inline void*
Arena_PushDirty(Arena* arena, uintsize size)
{ return Arena_PushDirtyAligned(arena, size, Arena_DEFAULT_ALIGNMENT); }

internal inline void*
Arena_PushAligned(Arena* arena, uintsize size, uintsize alignment)
{ return MemSet(Arena_PushDirtyAligned(arena, size, alignment), 0, size); }

internal inline void*
Arena_Push(Arena* arena, uintsize size)
{ return MemSet(Arena_PushDirtyAligned(arena, size, Arena_DEFAULT_ALIGNMENT), 0, size); }

internal inline void*
Arena_PushMemory(Arena* arena, const void* buf, uintsize size)
{ return MemCopy(Arena_PushDirtyAligned(arena, size, 1), buf, size); }

internal inline void*
Arena_PushMemoryAligned(Arena* arena, const void* buf, uintsize size, uintsize alignment)
{ return MemCopy(Arena_PushDirtyAligned(arena, size, alignment), buf, size); }

internal inline void
Arena_Clear(Arena* arena)
{ arena->offset = 0; }

internal inline void*
Arena_End(Arena* arena)
{ return arena->memory + arena->offset; }

#undef Arena_OsReserve_
#undef Arena_OsCommit_
#undef Arena_OsFree_

#endif // COMMON_ARENA_H
