#ifndef COMMON_MEMORY_GENERIC_H
#define COMMON_MEMORY_GENERIC_H

static inline int32
Mem_Generic_BitCtz64(uint64 i)
{
	int32 result;
	
	if (i == 0)
		result = sizeof(i)*8;
	else
	{
		result = 0;
		
		while ((i & 1<<result) == 0)
			++result;
	}
	
	return result;
}

static inline int32
Mem_Generic_BitCtz32(uint32 i)
{
	int32 result;
	
	if (i == 0)
		result = sizeof(i)*8;
	else
	{
		result = 0;
		
		while ((i & 1<<result) == 0)
			++result;
	}
	
	return result;
}

static inline int32
Mem_Generic_BitCtz16(uint16 i)
{ return i == 0 ? sizeof(i)*8 : Mem_Generic_BitCtz32(i); }

static inline int32
Mem_Generic_BitCtz8(uint8 i)
{ return i == 0 ? sizeof(i)*8 : Mem_Generic_BitCtz32(i); }

static inline int32
Mem_Generic_BitClz64(uint64 i)
{
	int32 result;
	
	if (i == 0)
		result = sizeof(i)*8;
	else
	{
		result = 0;
		
		while ((i & 1<<(63-result)) == 0)
			++result;
	}
	
	return result;
}

static inline int32
Mem_Generic_BitClz32(uint32 i)
{
	int32 result;
	
	if (i == 0)
		result = sizeof(i)*8;
	else
	{
		result = 0;
		
		while ((i & 1<<(31-result)) == 0)
			++result;
	}
	
	return result;
}

static inline int32
Mem_Generic_BitClz16(uint16 i)
{ return i == 0 ? sizeof(i)*8 : Mem_Generic_BitClz32(i) - 16; }

static inline int32
Mem_Generic_BitClz8(uint8 i)
{ return i == 0 ? sizeof(i)*8 : Mem_Generic_BitClz32(i) - 24; }

static inline int32
Mem_Generic_PopCnt64(uint64 x)
{
	x -= (x >> 1) & 0x5555555555555555u;
	x = (x & 0x3333333333333333u) + ((x >> 2) & 0x3333333333333333u);
	x = (x + (x >> 4)) & 0x0f0f0f0f0f0f0f0fu;
	return (x * 0x0101010101010101u) >> 56;
}

static inline int32
Mem_Generic_PopCnt32(uint32 x)
{
	x -= (x >> 1) & 0x55555555u;
	x = (x & 0x33333333u) + ((x >> 2) & 0x33333333u);
	x = (x + (x >> 4)) & 0x0f0f0f0fu;
	return (x * 0x01010101u) >> 24;
}

static inline int32
Mem_Generic_PopCnt16(uint16 x)
{ return Mem_Generic_PopCnt32(x); }

static inline int32
Mem_Generic_PopCnt8(uint8 x)
{ return Mem_Generic_PopCnt32(x); }

static inline uint16
Mem_Generic_ByteSwap16(uint16 x)
{ return (uint16)((x >> 8) | (x << 8)); }

static inline uint32
Mem_Generic_ByteSwap32(uint32 x)
{
	uint32 result;
	
	result = (x << 24) | (x >> 24) | (x >> 8 & 0xff00) | (x << 8 & 0xff0000);
	
	return result;
}

static inline uint64
Mem_Generic_ByteSwap64(uint64 x)
{
	uint64 result;
	
	union
	{
		uint64 v;
		uint8 arr[8];
	}
	r = { .v = x };
	uint8 tmp;
	
	tmp = r.arr[0]; r.arr[0] = r.arr[7]; r.arr[7] = tmp;
	tmp = r.arr[1]; r.arr[1] = r.arr[6]; r.arr[6] = tmp;
	tmp = r.arr[2]; r.arr[2] = r.arr[5]; r.arr[5] = tmp;
	tmp = r.arr[3]; r.arr[3] = r.arr[4]; r.arr[4] = tmp;
	
	result = r.v;
	
	return result;
}

static inline void*
Mem_Generic_ZeroSafe(void* restrict dst, uintsize size)
{
	Mem_Generic_Zero(dst, size);
	return dst;
}

static inline void
Mem_Generic_CopyX16(void* restrict dst, const void* restrict src)
{
	((uint64*)dst)[0] = ((uint64*)src)[0];
	((uint64*)dst)[1] = ((uint64*)src)[1];
}

static inline void*
Mem_Generic_Copy(void* restrict dst, const void* restrict src, uintsize size)
{
	uint8* restrict d = (uint8*)dst;
	const uint8* restrict s = (const uint8*)src;
	
	while (size --> 0)
		*d++ = *s++;
	
	return dst;
}

static inline void*
Mem_Generic_Move(void* dst, const void* src, uintsize size)
{
	uint8* restrict d = (uint8*)dst;
	const uint8* restrict s = (const uint8*)src;
	
	if (d < s)
	{
		while (size--)
			*d++ = *s++;
	}
	else
	{
		d += size;
		s += size;
		
		while (size--)
			*--d = *--s;
	}
	
	return dst;
}

static inline void*
Mem_Generic_Set(void* restrict dst, uint8 byte, uintsize size)
{
	uint8* restrict d = (uint8*)dst;
	
	while (size--)
		*d++ = byte;
	
	return dst;
}

static inline int32
Mem_Generic_Compare(const void* left_, const void* right_, uintsize size)
{
	const uint8* left = (const uint8*)left_;
	const uint8* right = (const uint8*)right_;
	
	while (size --> 0)
	{
		if (Unlikely(*left != *right))
			return (*left - *right < 0) ? -1 : 1;
		
		++left;
		++right;
	}
	
	return 0;
}

static inline uintsize
Mem_Generic_Strlen(const char* restrict cstr)
{
	const char* begin = cstr;
	
	while (*cstr)
		++cstr;
	
	return cstr - begin;
}

static inline uintsize
Mem_Generic_Strnlen(const char* restrict cstr, uintsize limit)
{
	const char* begin = cstr;
	
	while (limit-- && *cstr)
		++cstr;
	
	return cstr - begin;
}

static inline int32
Mem_Generic_Strcmp(const char* left, const char* right)
{
	for (;;)
	{
		if (*left != *right)
			return *left - *right;
		if (!*left)
			return 0;
		++left;
		++right;
	}
}

static inline char*
Mem_Generic_Strstr(const char* left, const char* right)
{
	for (; *left; ++left)
	{
		const char* it_left = left;
		const char* it_right = right;
		
		while (*it_left == *it_right)
		{
			if (!*it_left)
				return (char*)it_left;
			
			++it_left;
			++it_right;
		}
	}
	
	return NULL;
}

static inline const void*
Mem_Generic_FindByte(const void* buffer, uint8 byte, uintsize size)
{
	const uint8* buf = (const uint8*)buffer;
	
	for (; size--; ++buf)
	{
		if (*buf == byte)
			return buf;
	}
	
	return NULL;
}

static inline void*
Mem_Generic_Zero(void* restrict dst, uintsize size)
{
	uint8* restrict d = (uint8*)dst;
	
	while (size--)
		*d++ = 0;
	
	return dst;
}

#endif //COMMON_MEMORY_GENERIC_H
