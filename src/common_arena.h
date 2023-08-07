#ifndef COMMON_ARENA_H
#define COMMON_ARENA_H

#define ArenaPushStruct(arena, Type) \
	((Type*)ArenaPushAligned(arena, sizeof(Type), alignof(Type)))
#define ArenaPushStructData(arena, Type, ...) \
	((Type*)MemoryCopy(ArenaPushDirtyAligned(arena, sizeof(Type), alignof(Type)), __VA_ARGS__, sizeof(Type)))
#define ArenaPushStructInit(arena, Type, ...) \
	((Type*)ArenaPushMemoryAligned(arena, &(Type) __VA_ARGS__, sizeof(Type), alignof(Type)))
#define ArenaPushArray(arena, Type, count) \
	((Type*)ArenaPushAligned(arena, sizeof(Type)*(count), alignof(Type)))
#define ArenaPushArrayData(arena, Type, data, count) \
	((Type*)MemoryCopy(ArenaPushDirtyAligned(arena, sizeof(Type)*(count), alignof(Type)), data, sizeof(Type)*(count)))
#define ArenaPushData(arena, data) \
	MemoryCopy(ArenaPushDirtyAligned(arena, sizeof*(data), 1), data, sizeof*(data))
#define ArenaPushDataArray(arena, data, count) \
	MemoryCopy(ArenaPushDirtyAligned(arena, sizeof*(data)*(count), 1), data, sizeof*(data)*(count))
#define ArenaTempScope(arena_) \
	(ArenaSavepoint _temp__ = { arena_, (arena_)->offset }; _temp__.arena; _temp__.arena->offset = _temp__.offset, _temp__.arena = NULL)

struct Arena
{
	// NOTE(ljre): If this arena owns it's memory, then 'page_size != 0'.
	// This is needed to check if we can actually commit more memory.
	//
	// An arena created using 'ArenaFromMemory' does not own it's memory.
	
	uintsize reserved;
	uintsize commited;
	uintsize offset;
	uintsize page_size;
	uintsize peak;
	
	alignas(16) uint8 memory[];
}
typedef Arena;

struct ArenaSavepoint
{
	Arena* arena;
	uintsize offset;
}
typedef ArenaSavepoint;

static Arena* ArenaCreate(uintsize reserved, uintsize page_size);
static Arena* ArenaFromMemory(void* memory, uintsize size);
static Arena* ArenaFromUncommitedMemory(void* memory, uintsize reserved, uintsize page_size);
static void   ArenaDestroy(Arena* arena);

static inline void* ArenaPush(Arena* arena, uintsize size);
static inline void* ArenaPushDirty(Arena* arena, uintsize size);
static inline void* ArenaPushAligned(Arena* arena, uintsize size, uintsize alignment);
static        void* ArenaPushDirtyAligned(Arena* arena, uintsize size, uintsize alignment);
static inline void* ArenaPushMemory(Arena* arena, const void* buf, uintsize size);
static inline void* ArenaPushMemoryAligned(Arena* arena, const void* buf, uintsize size, uintsize alignment);
static inline String ArenaPushString(Arena* arena, String str);
static inline String ArenaPushStringAligned(Arena* arena, String str, uintsize alignment);
static inline const char* ArenaPushCString(Arena* arena, String str);

static String ArenaVPrintf(Arena* arena, const char* fmt, va_list args);
static String ArenaPrintf(Arena* arena, const char* fmt, ...);

static void         ArenaPop(Arena* arena, void* ptr);
static void*        ArenaEndAligned(Arena* arena, uintsize alignment);
static inline void  ArenaClear(Arena* arena);
static inline void* ArenaEnd(Arena* arena);

static inline ArenaSavepoint ArenaSave(Arena* arena);
static inline void            ArenaRestore(ArenaSavepoint savepoint);

#ifndef ArenaOsReserve_
#   if defined(_WIN32)
externC_ void* __stdcall VirtualAlloc(void* base, uintsize size, unsigned long type, unsigned long protect);
externC_ int32 __stdcall VirtualFree(void* base, uintsize size, unsigned long type);
#       define ArenaOsReserve_(size) VirtualAlloc(NULL,size,0x00002000/*MEM_RESERVE*/,0x04/*PAGE_READWRITE*/)
#       define ArenaOsCommit_(ptr, size) VirtualAlloc(ptr,size,0x00001000/*MEM_COMMIT*/,0x04/*PAGE_READWRITE*/)
#       define ArenaOsFree_(ptr, size) ((void)(size), VirtualFree(ptr,0,0x00008000/*MEM_RELEASE*/))
#   elif defined(__linux__)
#       include <sys/mman.h>
#       define ArenaOsReserve_(size) mmap(NULL,size,PROT_NONE,MAP_ANONYMOUS|MAP_PRIVATE,-1,0)
#       define ArenaOsCommit_(ptr, size) mprotect(ptr,size,PROT_READ|PROT_WRITE)
#       define ArenaOsFree_(ptr, size) munmap(ptr,size)
#   endif
#endif

#ifndef ArenaDEFAULT_ALIGNMENT
#   define ArenaDEFAULT_ALIGNMENT 16
#endif

static_assert(ArenaDEFAULT_ALIGNMENT != 0 && IsPowerOf2(ArenaDEFAULT_ALIGNMENT), "Default Arena alignment should always be non-zero and a power of two");

static Arena*
ArenaCreate(uintsize reserved, uintsize page_size)
{
	Assert(page_size && IsPowerOf2(page_size));
	Assert(reserved > 0);
	
	reserved = AlignUp(reserved, page_size-1);
	Arena* result = (Arena*)ArenaOsReserve_(reserved);
	SafeAssert(result);
	
	if (result)
	{
		SafeAssert(ArenaOsCommit_(result, page_size));
		
		result->reserved = reserved;
		result->commited = page_size;
		result->offset = 0;
		result->page_size = page_size;
		result->peak = 0;
	}
	
	return result;
}

static Arena*
ArenaFromMemory(void* memory, uintsize size)
{
	Assert(size > sizeof(Arena));
	Assert(((uintptr)memory & ~15) == (uintptr)memory);
	
	Arena* result = (Arena*)memory;
	
	result->reserved = size;
	result->commited = size;
	result->offset = 0;
	result->page_size = 0;
	result->peak = 0;
	
	return result;
}

static Arena*
ArenaFromUncommitedMemory(void* memory, uintsize reserved, uintsize page_size)
{
	Assert(page_size && IsPowerOf2(page_size));
	Assert(reserved >= page_size);
	
	reserved = AlignDown(reserved, page_size-1);
	SafeAssert(ArenaOsCommit_(memory, page_size));
	
	Arena* result = (Arena*)memory;
	result->reserved = reserved;
	result->commited = page_size;
	result->offset = 0;
	result->page_size = page_size;
	result->peak = 0;
	
	return result;
}

static void
ArenaDestroy(Arena* arena)
{ ArenaOsFree_(arena, arena->reserved); }

static void*
ArenaEndAligned(Arena* arena, uintsize alignment)
{
	Assert(alignment != 0 && IsPowerOf2(alignment));
	
	arena->offset = AlignUp(arena->offset + sizeof(Arena), alignment-1) - sizeof(Arena);
	return arena->memory + arena->offset;
}

static void*
ArenaPushDirtyAligned(Arena* arena, uintsize size, uintsize alignment)
{
	Assert(alignment != 0 && IsPowerOf2(alignment));
	
	ArenaEndAligned(arena, alignment);
	uintsize needed = arena->offset + size + sizeof(Arena);
	
	if (Unlikely(needed > arena->commited))
	{
		// NOTE(ljre): If we don't own this memory, just return NULL as we can't commit
		//             more memory.
		if (!arena->page_size)
			return NULL;
		
		uintsize size_to_commit = AlignUp(needed - arena->commited, arena->page_size-1);
		SafeAssert(size_to_commit + arena->commited <= arena->reserved);
		
		SafeAssert(ArenaOsCommit_((uint8*)arena + arena->commited, size_to_commit));
		arena->commited += size_to_commit;
	}
	
	void* result = arena->memory + arena->offset;
	arena->offset += size;
	arena->peak = Max(arena->peak, arena->offset);
	
	return result;
}

static void
ArenaPop(Arena* arena, void* ptr)
{
	uint8* p = (uint8*)ptr;
	Assert(p >= arena->memory && p <= arena->memory + arena->offset);
	
	uintsize new_offset = p - arena->memory;
	arena->offset = new_offset;
}

static String
ArenaVPrintf(Arena* arena, const char* fmt, va_list args)
{
	va_list args2;
	va_copy(args2, args);
	
	uintsize size = StringVPrintfSize(fmt, args2);
	uint8* data = (uint8*)ArenaPushDirtyAligned(arena, size, 1);
	
	size = StringVPrintfBuffer((char*)data, size, fmt, args);
	
	String result = { size, data, };
	
	va_end(args2);
	
	return result;
}

static String
ArenaPrintf(Arena* arena, const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	String result = ArenaVPrintf(arena, fmt, args);
	va_end(args);
	
	return result;
}

static inline ArenaSavepoint
ArenaSave(Arena* arena)
{
	ArenaSavepoint ret = {
		arena,
		arena->offset,
	};
	
	return ret;
}

static inline const char*
ArenaPushCString(Arena* arena, String str)
{
	char* buffer = (char*)ArenaPushDirtyAligned(arena, str.size + 1, 1);
	MemoryCopy(buffer, str.data, str.size);
	buffer[str.size] = 0;
	return buffer;
}

static inline void
ArenaRestore(ArenaSavepoint savepoint)
{ savepoint.arena->offset = savepoint.offset; }

static inline void*
ArenaPushDirty(Arena* arena, uintsize size)
{ return ArenaPushDirtyAligned(arena, size, ArenaDEFAULT_ALIGNMENT); }

static inline void*
ArenaPushAligned(Arena* arena, uintsize size, uintsize alignment)
{ return MemoryZero(ArenaPushDirtyAligned(arena, size, alignment), size); }

static inline void*
ArenaPush(Arena* arena, uintsize size)
{ return MemoryZero(ArenaPushDirtyAligned(arena, size, ArenaDEFAULT_ALIGNMENT), size); }

static inline void*
ArenaPushMemory(Arena* arena, const void* buf, uintsize size)
{ return MemoryCopy(ArenaPushDirtyAligned(arena, size, 1), buf, size); }

static inline void*
ArenaPushMemoryAligned(Arena* arena, const void* buf, uintsize size, uintsize alignment)
{ return MemoryCopy(ArenaPushDirtyAligned(arena, size, alignment), buf, size); }

static inline String
ArenaPushString(Arena* arena, String str)
{ return StrMake(str.size, MemoryCopy(ArenaPushDirtyAligned(arena, str.size, 1), str.data, str.size)); }

static inline String
ArenaPushStringAligned(Arena* arena, String str, uintsize alignment)
{ return StrMake(str.size, MemoryCopy(ArenaPushDirtyAligned(arena, str.size, alignment), str.data, str.size)); }

static inline void
ArenaClear(Arena* arena)
{ arena->offset = 0; }

static inline void*
ArenaEnd(Arena* arena)
{ return arena->memory + arena->offset; }

#endif // COMMON_ARENA_H
