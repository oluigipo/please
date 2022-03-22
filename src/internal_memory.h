#ifndef INTERNAL_MEMORY_H
#define INTERNAL_MEMORY_H

#include <immintrin.h>

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
	uint8* restrict d = dst;
	const uint8* restrict s = src;
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
	uint8* restrict d = dst;
	while (size--)
		*d++ = byte;
#endif
	
	return dst;
}

internal inline void*
MemMove(void* dst, const void* src, uintsize size)
{
	uint8* d = dst;
	const uint8* s = src;
	
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
		uint8* d = dst;
		const uint8* s = src;
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
MemCmp(const void* left_, const void* right_, uintsize size)
{
    const uint8* left = left_;
    const uint8* right = right_;
	
    while (size >= 16)
    {
        __m128i ldata, rdata;
		
        ldata = _mm_loadu_si128((void*)left);
        rdata = _mm_loadu_si128((void*)right);
		
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
        if (*left != *right)
            return (*left - *right < 0) ? -1 : 1;
		
        ++left;
        ++right;
    }
	
    return 0;
}

#endif //INTERNAL_MEMORY_H
