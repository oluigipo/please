#ifndef COMMON_MEMORY_H
#define COMMON_MEMORY_H

#include <immintrin.h>

internal inline int32
BitCtz(int32 i)
{
	Assume(i != 0);
	int32 result;
	
#if defined(__GNUC__) || defined(__clang__)
	result = __builtin_ctz(i);
#elif defined(_MSC_VER)
	_BitScanForward(&result, i);
#else
	result = 0;
	
	while ((i & 1<<result) == 0)
		++result;
#endif
	
	return result;
}

internal inline int32
BitClz(int32 i)
{
	Assume(i != 0);
	int32 result;
	
#if defined(__GNUC__) || defined(__clang__)
	result = __builtin_clz(i);
#elif defined(_MSC_VER)
	_BitScanReverse(&result, i);
	result = 31 - result;
#else
	result = 0;
	
	while ((i & 1<<(31-result)) == 0)
		++result;
#endif
	
	return result;
}

// NOTE(ljre): https://en.wikipedia.org/wiki/Hamming_weight#Efficient_implementation
internal inline int32
PopCnt64(uint64 x)
{
	x -= (x >> 1) & 0x5555555555555555ull;
    x = (x & 0x3333333333333333ull) + ((x >> 2) & 0x3333333333333333ull);
    x = (x + (x >> 4)) & 0x0f0f0f0f0f0f0f0full;
    return (x * 0x0101010101010101ull) >> 56;
}

internal inline int32
PopCnt32(uint32 x)
{
	x -= (x >> 1) & 0x55555555u;
    x = (x & 0x33333333u) + ((x >> 2) & 0x33333333u);
    x = (x + (x >> 4)) & 0x0f0f0f0fu;
    return (x * 0x01010101u) >> 24;
}

internal inline int32
PopCnt16(uint16 x)
{
	return PopCnt32(x);
}

internal inline uint16
ByteSwap16(uint16 x)
{
	return (uint16)((x >> 8) | (x << 8));
}

internal inline uint32
ByteSwap32(uint32 x)
{
	uint32 result;
	
#if defined(__GNUC__) || defined(__clang__)
	result = __builtin_bswap32(x);
#elif defined (_MSC_VER)
	extern unsigned long _byteswap_ulong(unsigned long);
	
	result = _byteswap_ulong(x);
#else
	result = (x << 24) | (x >> 24) | (x >> 8 & 0xff00) | (x << 8 & 0xff0000);
#endif
	
	return result;
}

internal inline uint64
ByteSwap64(uint64 x)
{
	uint64 result;
	
#if defined(__GNUC__) || defined(__clang__)
	result = __builtin_bswap64(x);
#elif defined (_MSC_VER)
	extern unsigned __int64 _byteswap_uint64(unsigned __int64);
	
	result = _byteswap_uint64(x);
#else
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
#endif
	
	return result;
}

#ifdef COMMON_DONT_USE_CRT
internal inline void*
MemCopy(void* restrict dst, const void* restrict src, uintsize size)
{
#if defined(__clang__) || defined(__GNUC__)
	void* d = dst;
	__asm__ __volatile__("rep movsb"
						 :"+D"(d), "+S"(src), "+c"(size)
						 :: "memory");
#elif defined(_MSC_VER)
	__movsb(dst, src, size);
#else
	uint8* restrict d = (uint8*)dst;
	const uint8* restrict s = (const uint8*)src;
	while (size--)
		*d++ = *s++;
#endif
	
	return dst;
}

internal inline void*
MemSet(void* restrict dst, uint8 byte, uintsize size)
{
#if defined(__clang__) || defined(__GNUC__)
	void* d = dst;
	__asm__ __volatile__("rep stosb"
						 :"+D"(d), "+a"(byte), "+c"(size)
						 :: "memory");
#elif defined(_MSC_VER)
	__stosb(dst, byte, size);
#else
	uint8* restrict d = (uint8*)dst;
	while (size--)
		*d++ = byte;
#endif
	
	return dst;
}

internal inline void*
MemMove(void* dst, const void* src, uintsize size)
{
	uint8* d = (uint8*)dst;
	const uint8* s = (const uint8*)src;
	
	if (d <= s)
	{
		// NOTE(ljre): Good news: 'rep movsb' allows overlapping memory :)
#if defined(__clang__) || defined(__GNUC__)
		__asm__ __volatile__("rep movsb"
							 :"+D"(d), "+S"(src), "+c"(size)
							 :: "memory");
#elif defined(_MSC_VER)
		__movsb(dst, src, size);
#else
		while (size--)
			*d++ = *s++;
#endif
	}
	else
	{
		// NOTE(ljre): Reversed copy.
		d += size;
		s += size;
		
		while (size--)
			*--d = *--s;
	}
	
	return dst;
}

internal inline int32
MemCmp(const void* left_, const void* right_, uintsize size)
{
    const uint8* left = (const uint8*)left_;
    const uint8* right = (const uint8*)right_;
	
    while (size >= 16)
    {
        __m128i ldata, rdata;
		
        ldata = _mm_loadu_si128((const __m128i_u*)left);
        rdata = _mm_loadu_si128((const __m128i_u*)right);
		
        int32 cmp = _mm_movemask_epi8(_mm_cmpeq_epi8(ldata, rdata));
        cmp = ~cmp & 0xffff;
		
        if (Unlikely(cmp != 0))
        {
            union
            {
                __m128i reg;
                int8 bytes[16];
            } diff = { _mm_sub_epi8(ldata, rdata) };
			
            return diff.bytes[BitCtz(cmp)] < 0 ? -1 : 1;
        }
		
        size -= 16;
        left += 16;
        right += 16;
    }
	
    while (size --> 0)
    {
        if (Unlikely(*left != *right))
            return (*left - *right < 0) ? -1 : 1;
		
        ++left;
        ++right;
    }
	
    return 0;
}

#else // COMMON_DONT_USE_CRT

#include <string.h>

internal inline void*
MemCopy(void* restrict dst, const void* restrict src, uintsize size)
{ return memcpy(dst, src, size); }

internal inline void*
MemMove(void* dst, const void* src, uintsize size)
{ return memmove(dst, src, size); }

internal inline void*
MemSet(void* restrict dst, uint8 byte, uintsize size)
{ return memset(dst, byte, size); }

internal inline int32
MemCmp(const void* left_, const void* right_, uintsize size)
{ return memcmp(left_, right_, size); }

#endif

#endif // COMMON_MEMORY_H
