#ifndef COMMON_ARENA_H
#define COMMON_ARENA_H

#define Arena_PushStruct(arena, Type) \
((Type*)Arena_PushAligned(arena, sizeof(Type), alignof(Type)))
#define Arena_PushStructData(arena, Type, data) \
((Type*)Mem_Copy(Arena_PushStruct(arena, Type), data, sizeof(Type)))
#define Arena_PushStructInit(arena, Type, ...) \
((Type*)Arena_PushMemoryAligned(arena, &(Type) __VA_ARGS__, sizeof(Type), alignof(Type)))
#define Arena_PushArray(arena, Type, count) \
((Type*)Arena_PushAligned(arena, sizeof(Type)*(count), alignof(Type)))
#define Arena_PushArrayData(arena, Type, data, count) \
((Type*)Mem_Copy(Arena_PushArray(arena, Type, count), data, sizeof(Type)*(count)))
#define Arena_PushData(arena, data) \
Mem_Copy(Arena_PushDirtyAligned(arena, sizeof*(data), 1), data, sizeof*(data))
#define Arena_PushDataArray(arena, data, count) \
Mem_Copy(Arena_PushDirtyAligned(arena, sizeof*(data)*(count), 1), data, sizeof*(data)*(count))
#define Arena_TempScope(arena_) \
(Arena_Savepoint _temp__ = { arena_, (arena_)->offset }; _temp__.arena; _temp__.arena->offset = _temp__.offset, _temp__.arena = NULL)

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

struct Arena_Savepoint
{
	Arena* arena;
	uintsize offset;
}
typedef Arena_Savepoint;

static Arena* Arena_Create(uintsize reserved, uintsize page_size);
static Arena* Arena_FromMemory(void* memory, uintsize size);
static Arena* Arena_FromUncommitedMemory(void* memory, uintsize reserved, uintsize page_size);
static void   Arena_Destroy(Arena* arena);

static inline void* Arena_Push(Arena* arena, uintsize size);
static inline void* Arena_PushDirty(Arena* arena, uintsize size);
static inline void* Arena_PushAligned(Arena* arena, uintsize size, uintsize alignment);
static        void* Arena_PushDirtyAligned(Arena* arena, uintsize size, uintsize alignment);
static inline void* Arena_PushMemory(Arena* arena, const void* buf, uintsize size);
static inline void* Arena_PushMemoryAligned(Arena* arena, const void* buf, uintsize size, uintsize alignment);
static inline String Arena_PushString(Arena* arena, String str);
static inline String Arena_PushStringAligned(Arena* arena, String str, uintsize alignment);
static inline const char* Arena_PushCString(Arena* arena, String str);

static String Arena_VPrintf(Arena* arena, const char* fmt, va_list args);
static String Arena_Printf(Arena* arena, const char* fmt, ...);

static void         Arena_Pop(Arena* arena, void* ptr);
static void*        Arena_EndAligned(Arena* arena, uintsize alignment);
static inline void  Arena_Clear(Arena* arena);
static inline void* Arena_End(Arena* arena);

static inline Arena_Savepoint Arena_Save(Arena* arena);
static inline void            Arena_Restore(Arena_Savepoint savepoint);

#ifndef Arena_OsReserve_
#   if defined(_WIN32)
externC_ void* __stdcall VirtualAlloc(void* base, uintsize size, unsigned long type, unsigned long protect);
externC_ int32 __stdcall VirtualFree(void* base, uintsize size, unsigned long type);
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

static_assert(Arena_DEFAULT_ALIGNMENT != 0 && IsPowerOf2(Arena_DEFAULT_ALIGNMENT));

static Arena*
Arena_Create(uintsize reserved, uintsize page_size)
{
	Assert(page_size && IsPowerOf2(page_size));
	Assert(reserved > 0);
	
	reserved = AlignUp(reserved, page_size-1);
	Arena* result = (Arena*)Arena_OsReserve_(reserved);
	SafeAssert(result);
	
	if (result)
	{
		SafeAssert(Arena_OsCommit_(result, page_size));
		
		result->reserved = reserved;
		result->commited = page_size;
		result->offset = 0;
		result->page_size = page_size;
	}
	
	return result;
}

static Arena*
Arena_FromMemory(void* memory, uintsize size)
{
	Assert(size > sizeof(Arena));
	Assert(((uintptr)memory & ~15) == (uintptr)memory);
	
	Arena* result = (Arena*)memory;
	
	result->reserved = size;
	result->commited = size;
	result->offset = 0;
	result->page_size = 0;
	
	return result;
}

static Arena*
Arena_FromUncommitedMemory(void* memory, uintsize reserved, uintsize page_size)
{
	Assert(page_size && IsPowerOf2(page_size));
	Assert(reserved >= page_size);
	
	reserved = AlignDown(reserved, page_size-1);
	SafeAssert(Arena_OsCommit_(memory, page_size));
	
	Arena* result = (Arena*)memory;
	result->reserved = reserved;
	result->commited = page_size;
	result->offset = 0;
	result->page_size = page_size;
	
	return result;
}

static void
Arena_Destroy(Arena* arena)
{ Arena_OsFree_(arena, arena->reserved); }

static void*
Arena_EndAligned(Arena* arena, uintsize alignment)
{
	Assert(alignment != 0 && IsPowerOf2(alignment));
	
	arena->offset = AlignUp(arena->offset + sizeof(Arena), alignment-1) - sizeof(Arena);
	return arena->memory + arena->offset;
}

static void*
Arena_PushDirtyAligned(Arena* arena, uintsize size, uintsize alignment)
{
	Assert(alignment != 0 && IsPowerOf2(alignment));
	
	Arena_EndAligned(arena, alignment);
	uintsize needed = arena->offset + size + sizeof(Arena);
	
	if (Unlikely(needed > arena->commited))
	{
		// NOTE(ljre): If we don't own this memory, just return NULL as we can't commit
		//             more memory.
		if (!arena->page_size)
			return NULL;
		
		uintsize size_to_commit = AlignUp(needed - arena->commited, arena->page_size-1);
		SafeAssert(size_to_commit + arena->commited <= arena->reserved);
		
		SafeAssert(Arena_OsCommit_((uint8*)arena + arena->commited, size_to_commit));
		arena->commited += size_to_commit;
	}
	
	void* result = arena->memory + arena->offset;
	arena->offset += size;
	
	return result;
}

static void
Arena_Pop(Arena* arena, void* ptr)
{
	uint8* p = (uint8*)ptr;
	Assert(p >= arena->memory && p <= arena->memory + arena->offset);
	
	uintsize new_offset = p - arena->memory;
	arena->offset = new_offset;
}

static String
Arena_VPrintf(Arena* arena, const char* fmt, va_list args)
{
	va_list args2;
	va_copy(args2, args);
	
	uintsize size = String_VPrintfSize(fmt, args2);
	uint8* data = (uint8*)Arena_PushDirtyAligned(arena, size, 1);
	
	size = String_VPrintfBuffer((char*)data, size, fmt, args);
	
	String result = { size, data, };
	
	va_end(args2);
	
	return result;
}

static String
Arena_Printf(Arena* arena, const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	String result = Arena_VPrintf(arena, fmt, args);
	va_end(args);
	
	return result;
}

static inline Arena_Savepoint
Arena_Save(Arena* arena)
{
	Arena_Savepoint ret = {
		arena,
		arena->offset,
	};
	
	return ret;
}

static inline const char*
Arena_PushCString(Arena* arena, String str)
{
	char* buffer = (char*)Arena_PushDirtyAligned(arena, str.size + 1, 1);
	Mem_Copy(buffer, str.data, str.size);
	buffer[str.size] = 0;
	return buffer;
}

static inline void
Arena_Restore(Arena_Savepoint savepoint)
{ savepoint.arena->offset = savepoint.offset; }

static inline void*
Arena_PushDirty(Arena* arena, uintsize size)
{ return Arena_PushDirtyAligned(arena, size, Arena_DEFAULT_ALIGNMENT); }

static inline void*
Arena_PushAligned(Arena* arena, uintsize size, uintsize alignment)
{ return Mem_Zero(Arena_PushDirtyAligned(arena, size, alignment), size); }

static inline void*
Arena_Push(Arena* arena, uintsize size)
{ return Mem_Zero(Arena_PushDirtyAligned(arena, size, Arena_DEFAULT_ALIGNMENT), size); }

static inline void*
Arena_PushMemory(Arena* arena, const void* buf, uintsize size)
{ return Mem_Copy(Arena_PushDirtyAligned(arena, size, 1), buf, size); }

static inline void*
Arena_PushMemoryAligned(Arena* arena, const void* buf, uintsize size, uintsize alignment)
{ return Mem_Copy(Arena_PushDirtyAligned(arena, size, alignment), buf, size); }

static inline String
Arena_PushString(Arena* arena, String str)
{ return StrMake(str.size, Mem_Copy(Arena_PushDirtyAligned(arena, str.size, 1), str.data, str.size)); }

static inline String
Arena_PushStringAligned(Arena* arena, String str, uintsize alignment)
{ return StrMake(str.size, Mem_Copy(Arena_PushDirtyAligned(arena, str.size, alignment), str.data, str.size)); }

static inline void
Arena_Clear(Arena* arena)
{ arena->offset = 0; }

static inline void*
Arena_End(Arena* arena)
{ return arena->memory + arena->offset; }

#endif // COMMON_ARENA_H
