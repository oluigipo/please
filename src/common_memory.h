#ifndef COMMON_MEMORY_H
#define COMMON_MEMORY_H

//~ NOTE(ljre): Functions
static inline int32 Mem_BitCtz64(uint64 i);
static inline int32 Mem_BitCtz32(uint32 i);
static inline int32 Mem_BitCtz16(uint16 i);
static inline int32 Mem_BitCtz8(uint8 i);

static inline int32 Mem_BitClz64(uint64 i);
static inline int32 Mem_BitClz32(uint32 i);
static inline int32 Mem_BitClz16(uint16 i);
static inline int32 Mem_BitClz8(uint8 i);

static inline int32 Mem_PopCnt64(uint64 x);
static inline int32 Mem_PopCnt32(uint32 x);
static inline int32 Mem_PopCnt16(uint16 x);
static inline int32 Mem_PopCnt8(uint8 x);

static inline uint64 Mem_ByteSwap64(uint64 x);
static inline uint32 Mem_ByteSwap32(uint32 x);
static inline uint16 Mem_ByteSwap16(uint16 x);

static inline void Mem_CopyX16(void* restrict dst, const void* restrict src);

static inline uintsize Mem_Strlen(const char* restrict cstr);
static inline uintsize Mem_Strnlen(const char* restrict cstr, uintsize limit);
static inline int32 Mem_Strcmp(const char* left, const char* right);
static inline char* Mem_Strstr(const char* left, const char* right);
static inline const void* Mem_FindByte(const void* buffer, uint8 byte, uintsize size);
static inline void* Mem_Zero(void* restrict dst, uintsize size);
static inline void* Mem_ZeroSafe(void* restrict dst, uintsize size);

static inline void* Mem_Copy(void* restrict dst, const void* restrict src, uintsize size);
static inline void* Mem_Move(void* dst, const void* src, uintsize size);
static inline void* Mem_Set(void* restrict dst, uint8 byte, uintsize size);
static inline int32 Mem_Compare(const void* left_, const void* right_, uintsize size);

//~ NOTE(ljre): Generic Implementation
static inline int32 Mem_Generic_BitCtz64(uint64 i);
static inline int32 Mem_Generic_BitCtz32(uint32 i);
static inline int32 Mem_Generic_BitCtz16(uint16 i);
static inline int32 Mem_Generic_BitCtz8(uint8 i);

static inline int32 Mem_Generic_BitClz64(uint64 i);
static inline int32 Mem_Generic_BitClz32(uint32 i);
static inline int32 Mem_Generic_BitClz16(uint16 i);
static inline int32 Mem_Generic_BitClz8(uint8 i);

static inline int32 Mem_Generic_PopCnt64(uint64 x);
static inline int32 Mem_Generic_PopCnt32(uint32 x);
static inline int32 Mem_Generic_PopCnt16(uint16 x);
static inline int32 Mem_Generic_PopCnt8(uint8 x);

static inline uint64 Mem_Generic_ByteSwap64(uint64 x);
static inline uint32 Mem_Generic_ByteSwap32(uint32 x);
static inline uint16 Mem_Generic_ByteSwap16(uint16 x);

static inline void Mem_Generic_CopyX16(void* restrict dst, const void* restrict src);

static inline uintsize Mem_Generic_Strlen(const char* restrict cstr);
static inline uintsize Mem_Generic_Strnlen(const char* restrict cstr, uintsize limit);
static inline int32 Mem_Generic_Strcmp(const char* left, const char* right);
static inline char* Mem_Generic_Strstr(const char* left, const char* right);
static inline const void* Mem_Generic_FindByte(const void* buffer, uint8 byte, uintsize size);
static inline void* Mem_Generic_Zero(void* restrict dst, uintsize size);
static inline void* Mem_Generic_ZeroSafe(void* restrict dst, uintsize size);

static inline void* Mem_Generic_Copy(void* restrict dst, const void* restrict src, uintsize size);
static inline void* Mem_Generic_Move(void* dst, const void* src, uintsize size);
static inline void* Mem_Generic_Set(void* restrict dst, uint8 byte, uintsize size);
static inline int32 Mem_Generic_Compare(const void* left_, const void* right_, uintsize size);

//~ NOTE(ljre): Implementation
#include "common_memory_generic.h"

#if defined(CONFIG_ARCH_X86FAMILY)
#   include "common_memory_x86.h"
//#elif defined(CONFIG_ARCH_ARMFAMILY)
//#   include "common_memory_arm.h"
#else
static inline int32 Mem_BitCtz64(uint64 i) { return Mem_Generic_BitCtz64(i); }
static inline int32 Mem_BitCtz32(uint32 i) { return Mem_Generic_BitCtz32(i); }
static inline int32 Mem_BitCtz16(uint16 i) { return Mem_Generic_BitCtz16(i); }
static inline int32 Mem_BitCtz8(uint8 i) { return Mem_Generic_BitCtz8(i); }

static inline int32 Mem_BitClz64(uint64 i) { return Mem_Generic_BitClz64(i); }
static inline int32 Mem_BitClz32(uint32 i) { return Mem_Generic_BitClz32(i); }
static inline int32 Mem_BitClz16(uint16 i) { return Mem_Generic_BitClz16(i); }
static inline int32 Mem_BitClz8(uint8 i) { return Mem_Generic_BitClz8(i); }

static inline int32 Mem_PopCnt64(uint64 x) { return Mem_Generic_PopCnt64(x); }
static inline int32 Mem_PopCnt32(uint32 x) { return Mem_Generic_PopCnt32(x); }
static inline int32 Mem_PopCnt16(uint16 x) { return Mem_Generic_PopCnt16(x); }
static inline int32 Mem_PopCnt8(uint8 x) { return Mem_Generic_PopCnt8(x); }

static inline uint64 Mem_ByteSwap64(uint64 x) { return Mem_Generic_ByteSwap64(x); }
static inline uint32 Mem_ByteSwap32(uint32 x) { return Mem_Generic_ByteSwap32(x); }
static inline uint16 Mem_ByteSwap16(uint16 x) { return Mem_Generic_ByteSwap16(x); }

static inline void Mem_CopyX16(void* restrict dst, const void* restrict src) { return Mem_Generic_CopyX16(dst, src); }

#ifdef COMMON_DONT_USE_CRT
static inline uintsize Mem_Strlen(const char* restrict cstr) { return Mem_Generic_Strlen(cstr); }
static inline uintsize Mem_Strnlen(const char* restrict cstr, uintsize limit) { return Mem_Generic_Strnlen(cstr, limit); }
static inline int32 Mem_Strcmp(const char* left, const char* right) { return Mem_Generic_Strcmp(left, right); }
static inline char* Mem_Strstr(const char* left, const char* right) { return Mem_Generic_Strstr(left, right); }
static inline const void* Mem_FindByte(const void* buffer, uint8 byte, uintsize size) { return Mem_Generic_FindByte(buffer, byte, size); }
static inline void* Mem_Zero(void* restrict dst, uintsize size) { return Mem_Generic_Zero(dst, size); }
static inline void* Mem_ZeroSafe(void* restrict dst, uintsize size) { return Mem_Generic_ZeroSafe(dst, size); }

static inline void* Mem_Copy(void* restrict dst, const void* restrict src, uintsize size) { return Mem_Generic_Copy(dst, src, size); }
static inline void* Mem_Move(void* dst, const void* src, uintsize size) { return Mem_Generic_Move(dst, src, size); }
static inline void* Mem_Set(void* restrict dst, uint8 byte, uintsize size) { return Mem_Generic_Set(dst, byte, size); }
static inline int32 Mem_Compare(const void* left_, const void* right_, uintsize size) { return Mem_Generic_Compare(left_, right_, size); }
#endif //COMMON_DONT_USE_CRT

#endif //defined(CONFIG_ARCH_*)

#ifndef COMMON_DONT_USE_CRT
#include <string.h>

static inline void*
Mem_Copy(void* restrict dst, const void* restrict src, uintsize size)
{ Trace(); return memcpy(dst, src, size); }

static inline void*
Mem_Move(void* dst, const void* src, uintsize size)
{ Trace(); return memmove(dst, src, size); }

static inline void*
Mem_Set(void* restrict dst, uint8 byte, uintsize size)
{ Trace(); return memset(dst, byte, size); }

static inline int32
Mem_Compare(const void* left_, const void* right_, uintsize size)
{ Trace(); return memcmp(left_, right_, size); }

static inline uintsize
Mem_Strlen(const char* restrict cstr)
{ Trace(); return strlen(cstr); }

static inline uintsize
Mem_Strnlen(const char* restrict cstr, uintsize limit)
{ Trace(); return strnlen(cstr, limit); }

static inline int32
Mem_Strcmp(const char* left, const char* right)
{ Trace(); return strcmp(left, right); }

static inline char*
Mem_Strstr(const char* left, const char* right)
{ Trace(); return strstr(left, right); }

static inline const void*
Mem_FindByte(const void* buffer, uint8 byte, uintsize size)
{ Trace(); return memchr(buffer, byte, size); }

static inline void*
Mem_Zero(void* restrict dst, uintsize size)
{ Trace(); return memset(dst, 0, size); }

#endif //COMMON_DONT_USE_CRT

#endif // COMMON_MEMORY_H
