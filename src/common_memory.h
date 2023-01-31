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

static inline void Mem_Copy8(void* dst, const void* src, uintsize count);
static inline void Mem_Copy64(void* dst, const void* src, uintsize count);
static inline void Mem_Copy128U(void* dst, const void* src, uintsize count);
static inline void Mem_Copy128A(void* dst, const void* src, uintsize count);
static inline void Mem_Copy128Ux4(void* dst, const void* src, uintsize count);
static inline void Mem_Copy128Ax4(void* dst, const void* src, uintsize count);

static inline uintsize Mem_Strlen(const char* restrict cstr);
static inline int32 Mem_Strcmp(const char* left, const char* right);
static inline const void* Mem_FindByte(const void* buffer, uint8 byte, uintsize size);
static inline void* Mem_Zero(void* restrict dst, uintsize size);
static inline void* Mem_ZeroSafe(void* restrict dst, uintsize size);

static inline void* Mem_Copy(void* restrict dst, const void* restrict src, uintsize size);
static inline void* Mem_Move(void* dst, const void* src, uintsize size);
static inline void* Mem_Set(void* restrict dst, uint8 byte, uintsize size);
static inline int32 Mem_Compare(const void* left_, const void* right_, uintsize size);

//~ NOTE(ljre): amd64 implementation (currently the single one we care about)
#include <immintrin.h>

//- Mem_BitCtz
static inline int32
Mem_BitCtz64(uint64 i)
{
	Assume(i != 0);
	int32 result;
	
#if defined(__GNUC__) || defined(__clang__)
	result = __builtin_ctzll(i);
#elif defined(_MSC_VER)
	_BitScanForward64(&result, i);
#else
	result = 0;
	
	while ((i & 1<<result) == 0)
		++result;
#endif
	
	return result;
}

static inline int32
Mem_BitCtz32(uint32 i)
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

static inline int32
Mem_BitCtz16(uint16 i)
{ return Mem_BitCtz32(i); }

static inline int32
Mem_BitCtz8(uint8 i)
{ return Mem_BitCtz32(i); }

//- Mem_BitClz
static inline int32
Mem_BitClz64(uint64 i)
{
	Assume(i != 0);
	int32 result;
	
#if defined(__GNUC__) || defined(__clang__)
	result = __builtin_clzll(i);
#elif defined(_MSC_VER)
	_BitScanReverse(&result, i);
	result = 63 - result;
#else
	result = 0;
	
	while ((i & 1<<(63-result)) == 0)
		++result;
#endif
	
	return result;
}

static inline int32
Mem_BitClz32(uint32 i)
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

static inline int32
Mem_BitClz16(uint16 i)
{ return Mem_BitClz32(i) - 16; }

static inline int32
Mem_BitClz8(uint8 i)
{ return Mem_BitClz32(i) - 24; }

//- Mem_PopCnt
#ifdef __POPCNT__

static inline int32
Mem_PopCnt64(uint64 x)
{ return (int32)_mm_popcnt_u64(x); }

static inline int32
Mem_PopCnt32(uint32 x)
{ return (int32)_mm_popcnt_u32(x); }

static inline int32
Mem_PopCnt16(uint16 x)
{ return (int32)_mm_popcnt_u32(x); }

static inline int32
Mem_PopCnt8(uint8 x)
{ return (int32)_mm_popcnt_u32(x); }

#else

// NOTE(ljre): The 'popcnt' instruction is not actually guaranteed to be supported on all amd64 CPUs, so
//             we'll use this implementation found on wikipedia.
//
//             https://en.wikipedia.org/wiki/Hamming_weight#Efficient_implementation
static inline int32
Mem_PopCnt64(uint64 x)
{
	x -= (x >> 1) & 0x5555555555555555u;
	x = (x & 0x3333333333333333u) + ((x >> 2) & 0x3333333333333333u);
	x = (x + (x >> 4)) & 0x0f0f0f0f0f0f0f0fu;
	return (x * 0x0101010101010101u) >> 56;
}

static inline int32
Mem_PopCnt32(uint32 x)
{
	x -= (x >> 1) & 0x55555555u;
	x = (x & 0x33333333u) + ((x >> 2) & 0x33333333u);
	x = (x + (x >> 4)) & 0x0f0f0f0fu;
	return (x * 0x01010101u) >> 24;
}

static inline int32
Mem_PopCnt16(uint16 x)
{ return Mem_PopCnt32(x); }

static inline int32
Mem_PopCnt8(uint8 x)
{ return Mem_PopCnt32(x); }

#endif

//- Mem_ByteSwap
static inline uint16
Mem_ByteSwap16(uint16 x)
{ return (uint16)((x >> 8) | (x << 8)); }

static inline uint32
Mem_ByteSwap32(uint32 x)
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

static inline uint64
Mem_ByteSwap64(uint64 x)
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

//- NOTE(ljre): Mem_CopyN
static inline void
Mem_Copy8(void* dst, const void* src, uintsize count)
{
	uint8* d = (uint8*)dst;
	const uint8* s = (const uint8*)src;
	
#ifdef __clang__
#   pragma clang loop vectorize(disable)
#endif
	while (count--)
		*d++ = *s++;
}

static inline void
Mem_Copy64(void* dst, const void* src, uintsize count)
{
	uint64* d = (uint64*)dst;
	const uint64* s = (const uint64*)src;
	
#ifdef __clang__
#   pragma clang loop vectorize(disable)
#endif
	while (count--)
		*d++ = *s++;
}

static inline void
Mem_Copy128U(void* dst, const void* src, uintsize count)
{
	__m128* d = (__m128*)dst;
	const __m128* s = (const __m128*)src;
	
#ifdef __clang__
#   pragma clang loop vectorize(disable)
#endif
	while (count--)
		_mm_storeu_ps((float32*)d++, _mm_loadu_ps((const float32*)s++));
}

static inline void
Mem_Copy128A(void* dst, const void* src, uintsize count)
{
	__m128* d = (__m128*)dst;
	const __m128* s = (const __m128*)src;
	
#ifdef __clang__
#   pragma clang loop vectorize(disable)
#endif
	while (count--)
		_mm_store_ps((float32*)d++, _mm_load_ps((const float32*)s++));
}

static inline void
Mem_Copy128Ux4(void* dst, const void* src, uintsize count)
{
	__m128* d = (__m128*)dst;
	const __m128* s = (const __m128*)src;
	
#ifdef __clang__
#   pragma clang loop vectorize(disable)
#endif
	while (count--)
	{
		_mm_storeu_ps((float32*)(d+0), _mm_loadu_ps((const float32*)(s+0)));
		_mm_storeu_ps((float32*)(d+1), _mm_loadu_ps((const float32*)(s+1)));
		_mm_storeu_ps((float32*)(d+2), _mm_loadu_ps((const float32*)(s+2)));
		_mm_storeu_ps((float32*)(d+3), _mm_loadu_ps((const float32*)(s+3)));
		
		d += 4;
		s += 4;
	}
}

static inline void
Mem_Copy128Ax4(void* dst, const void* src, uintsize count)
{
	__m128* d = (__m128*)dst;
	const __m128* s = (const __m128*)src;
	
#ifdef __clang__
#   pragma clang loop vectorize(disable)
#endif
	while (count--)
	{
		_mm_store_ps((float32*)(d+0), _mm_load_ps((const float32*)(s+0)));
		_mm_store_ps((float32*)(d+1), _mm_load_ps((const float32*)(s+1)));
		_mm_store_ps((float32*)(d+2), _mm_load_ps((const float32*)(s+2)));
		_mm_store_ps((float32*)(d+3), _mm_load_ps((const float32*)(s+3)));
		
		d += 4;
		s += 4;
	}
}

#if defined(_MSC_VER) && !defined(__clang__)
#   pragma optimize("", off)
#endif
static inline void*
Mem_ZeroSafe(void* restrict dst, uintsize size)
{
	Mem_Zero(dst, size);
#if defined(__GNUC__) || defined(__clang__)
	__asm__ __volatile__ ("" : "+d"(dst) :: "memory");
#elif defined(_MSC_VER)
	_ReadWriteBarrier();
#else
	// TODO
#endif
	return dst;
}
#if defined(_MSC_VER) && !defined(__clang__)
#   pragma optimize("", on)
#endif

//- CRT memcpy, memmove, memset & memcmp functions
#ifdef COMMON_DONT_USE_CRT

// NOTE(ljre): Loop vectorization when using clang is disabled.
//             this thing is already vectorized, though it likes to vectorize the 1-by-1 bits still.
//
//             GCC only does this at -O3, which we don't care about. MSVC is ok.

// NOTE(ljre): the *_by_* labels lead directly inside the loop since the (size >= N) condition should
//             already be met.

static inline void*
Mem_Copy(void* restrict dst, const void* restrict src, uintsize size)
{
	uint8* restrict d = (uint8*)dst;
	const uint8* restrict s = (const uint8*)src;
	
	if (Unlikely(size == 0))
		return dst;
	if (size < 8)
		goto one_by_one;
	if (size < 32)
		goto qword_by_qword;
	if (size < 128)
		goto xmm2_by_xmm2;
	
	// NOTE(ljre): Simply use 'rep movsb'.
#if defined(__clang__) || defined(__GNUC__) || defined(_MSC_VER)
	if (Unlikely(size >= 2048))
	{
#   if defined(__clang__) || defined(__GNUC__)
		__asm__ __volatile__ (
			"rep movsb"
			: "+D"(d), "+S"(s), "+c"(size)
			:: "memory");
#   else
		__movsb(d, s, size);
#   endif
		return dst;
	}
#endif
	
	// fallthrough
#ifdef __clang__
#   pragma clang loop vectorize(disable)
#endif
	do
	{
		size -= 128;
		_mm_storeu_si128((__m128i*)(d+size+  0), _mm_loadu_si128((__m128i*)(s+size+  0)));
		_mm_storeu_si128((__m128i*)(d+size+ 16), _mm_loadu_si128((__m128i*)(s+size+ 16)));
		_mm_storeu_si128((__m128i*)(d+size+ 32), _mm_loadu_si128((__m128i*)(s+size+ 32)));
		_mm_storeu_si128((__m128i*)(d+size+ 48), _mm_loadu_si128((__m128i*)(s+size+ 48)));
		_mm_storeu_si128((__m128i*)(d+size+ 64), _mm_loadu_si128((__m128i*)(s+size+ 64)));
		_mm_storeu_si128((__m128i*)(d+size+ 80), _mm_loadu_si128((__m128i*)(s+size+ 80)));
		_mm_storeu_si128((__m128i*)(d+size+ 96), _mm_loadu_si128((__m128i*)(s+size+ 96)));
		_mm_storeu_si128((__m128i*)(d+size+112), _mm_loadu_si128((__m128i*)(s+size+112)));
	}
	while (size >= 128);
	
#ifdef __clang__
#   pragma clang loop vectorize(disable)
#endif
	while (size >= 32) xmm2_by_xmm2:
	{
		size -= 32;
		_mm_storeu_si128((__m128i*)(d+size+ 0), _mm_loadu_si128((__m128i*)(s+size+ 0)));
		_mm_storeu_si128((__m128i*)(d+size+16), _mm_loadu_si128((__m128i*)(s+size+16)));
	}
	
#ifdef __clang__
#   pragma clang loop vectorize(disable)
#endif
	while (size >= 8) qword_by_qword:
	{
		size -= 8;
		*(uint64*)(d+size) = *(uint64*)(s+size);
	}
	
#ifdef __clang__
#   pragma clang loop vectorize(disable)
#endif
	while (size) one_by_one:
	{
		size -= 1;
		d[size] = s[size];
	}
	
	return dst;
}

static inline void*
Mem_Move(void* dst, const void* src, uintsize size)
{
	uint8* d = (uint8*)dst;
	const uint8* s = (const uint8*)src;
	uintsize diff;
	{
		const intsize diff_signed = d - s;
		diff = diff_signed < 0 ? -diff_signed : diff_signed;
	}
	
	if (size == 0)
		return dst;
	
	if (diff > size)
		return Mem_Copy(dst, src, size);
	
	if (d <= s)
	{
		// NOTE(ljre): Forward copy.
		
#if defined(__clang__) || defined(__GNUC__) || defined(_MSC_VER)
		if (Unlikely(size >= 2048))
		{
#   if defined(__clang__) || defined(__GNUC__)
			__asm__ __volatile__("rep movsb"
				:"+D"(d), "+S"(s), "+c"(size)
				:: "memory");
#   else
			__movsb(dst, src, size);
#   endif
			return dst;
		}
#endif
		
		if (diff < 8)
			goto inc_by_one;
		if (diff < 32)
			goto inc_by_qword;
		if (diff < 128)
			goto inc_by_xmm2;
		
#ifdef __clang__
#   pragma clang loop vectorize(disable)
#endif
		do
		{
			_mm_storeu_si128((__m128i*)(d+  0), _mm_loadu_si128((__m128i*)(s+  0)));
			_mm_storeu_si128((__m128i*)(d+ 16), _mm_loadu_si128((__m128i*)(s+ 16)));
			_mm_storeu_si128((__m128i*)(d+ 32), _mm_loadu_si128((__m128i*)(s+ 32)));
			_mm_storeu_si128((__m128i*)(d+ 48), _mm_loadu_si128((__m128i*)(s+ 48)));
			_mm_storeu_si128((__m128i*)(d+ 64), _mm_loadu_si128((__m128i*)(s+ 64)));
			_mm_storeu_si128((__m128i*)(d+ 80), _mm_loadu_si128((__m128i*)(s+ 80)));
			_mm_storeu_si128((__m128i*)(d+ 96), _mm_loadu_si128((__m128i*)(s+ 96)));
			_mm_storeu_si128((__m128i*)(d+112), _mm_loadu_si128((__m128i*)(s+112)));
			
			d += 128;
			s += 128;
			size -= 128;
		}
		while (size >= 128);
		
#ifdef __clang__
#   pragma clang loop vectorize(disable)
#endif
		while (size >= 32) inc_by_xmm2:
		{
			_mm_storeu_si128((__m128i*)(d+ 0), _mm_loadu_si128((__m128i*)(s+ 0)));
			_mm_storeu_si128((__m128i*)(d+16), _mm_loadu_si128((__m128i*)(s+16)));
			
			d += 32;
			s += 32;
			size -= 32;
		}
		
#ifdef __clang__
#   pragma clang loop vectorize(disable)
#endif
		while (size >= 8) inc_by_qword:
		{
			*(uint64*)d = *(uint64*)s;
			
			d += 8;
			s += 8;
			size -= 8;
		}
		
#ifdef __clang__
#   pragma clang loop vectorize(disable)
#endif
		while (size) inc_by_one:
		{
			size -= 1;
			*d++ = *s++;
		}
	}
	else
	{
		// NOTE(ljre): Backwards copy.
		
#if defined(__clang__) || defined(__GNUC__) || defined(_MSC_VER)
		if (Unlikely(size >= 2048))
		{
#   if defined(__clang__) || defined(__GNUC__)
			__asm__ __volatile__("std\n"
				"rep movsb\n"
				"cld"
				:"+D"(d), "+S"(s), "+c"(size)
				:: "memory");
#   else
			// TODO(ljre): maybe reconsider this? I couldn't find a way for MSVC to directly
			//     generate 'std' & 'cld' instructions. (SeT Direction flag & CLear Direction flag)
			__writeeflags(__readeflags() | 0x0400);
			__movsb(dst, src, size);
			__writeeflags(__readeflags() & ~(uint64)0x0400);
#   endif
			return dst;
		}
#endif
		
		if (diff < 8)
			goto dec_by_one;
		if (diff < 32)
			goto dec_by_qword;
		if (diff < 128)
			goto dec_by_xmm2;
		
#ifdef __clang__
#   pragma clang loop vectorize(disable)
#endif
		do
		{
			size -= 128;
			_mm_storeu_si128((__m128i*)(d+size+  0), _mm_loadu_si128((__m128i*)(s+size+  0)));
			_mm_storeu_si128((__m128i*)(d+size+ 16), _mm_loadu_si128((__m128i*)(s+size+ 16)));
			_mm_storeu_si128((__m128i*)(d+size+ 32), _mm_loadu_si128((__m128i*)(s+size+ 32)));
			_mm_storeu_si128((__m128i*)(d+size+ 48), _mm_loadu_si128((__m128i*)(s+size+ 48)));
			_mm_storeu_si128((__m128i*)(d+size+ 64), _mm_loadu_si128((__m128i*)(s+size+ 64)));
			_mm_storeu_si128((__m128i*)(d+size+ 80), _mm_loadu_si128((__m128i*)(s+size+ 80)));
			_mm_storeu_si128((__m128i*)(d+size+ 96), _mm_loadu_si128((__m128i*)(s+size+ 96)));
			_mm_storeu_si128((__m128i*)(d+size+112), _mm_loadu_si128((__m128i*)(s+size+112)));
		}
		while (size >= 128);
		
#ifdef __clang__
#   pragma clang loop vectorize(disable)
#endif
		while (size >= 32) dec_by_xmm2:
		{
			size -= 32;
			_mm_storeu_si128((__m128i*)(d+size+ 0), _mm_loadu_si128((__m128i*)(s+size+ 0)));
			_mm_storeu_si128((__m128i*)(d+size+16), _mm_loadu_si128((__m128i*)(s+size+16)));
		}
		
#ifdef __clang__
#   pragma clang loop vectorize(disable)
#endif
		while (size >= 8) dec_by_qword:
		{
			size -= 8;
			*(uint64*)(d+size) = *(uint64*)(s+size);
		}
		
#ifdef __clang__
#   pragma clang loop vectorize(disable)
#endif
		while (size) dec_by_one:
		{
			size -= 1;
			d[size] = s[size];
		}
	}
	
	return dst;
}

static inline void*
Mem_Set(void* restrict dst, uint8 byte, uintsize size)
{
	uint8* restrict d = (uint8*)dst;
	uint64 qword;
	__m128i xmm;
	
	if (Unlikely(size == 0))
		return dst;
	if (size < 8)
		goto one_by_one;
	
	qword = byte * 0x0101010101010101;
	if (size < 32)
		goto qword_by_qword;
	
	xmm = _mm_set1_epi64x(qword);
	if (size < 128)
		goto xmm2_by_xmm2;
	
#if defined(__clang__) || defined(__GNUC__) || defined(_MSC_VER)
	if (Unlikely(size >= 2048))
	{
#   if defined(__clang__) || defined(__GNUC__)
		__asm__ __volatile__("rep stosb"
			:"+D"(d), "+a"(byte), "+c"(size)
			:: "memory");
#   elif defined(_MSC_VER)
		__stosb(d, byte, size);
#   endif
		
		return dst;
	}
#endif
	
#ifdef __clang__
#   pragma clang loop vectorize(disable)
#endif
	do
	{
		size -= 128;
		_mm_storeu_si128((__m128i*)(d+size+0 ), xmm);
		_mm_storeu_si128((__m128i*)(d+size+16), xmm);
		_mm_storeu_si128((__m128i*)(d+size+32), xmm);
		_mm_storeu_si128((__m128i*)(d+size+48), xmm);
		_mm_storeu_si128((__m128i*)(d+size+64), xmm);
		_mm_storeu_si128((__m128i*)(d+size+80), xmm);
		_mm_storeu_si128((__m128i*)(d+size+96), xmm);
		_mm_storeu_si128((__m128i*)(d+size+112), xmm);
	}
	while (size >= 128);
	
#ifdef __clang__
#   pragma clang loop vectorize(disable)
#endif
	while (size >= 32) xmm2_by_xmm2:
	{
		size -= 32;
		_mm_storeu_si128((__m128i*)(d+size+0 ), xmm);
		_mm_storeu_si128((__m128i*)(d+size+16), xmm);
	}
	
#ifdef __clang__
#   pragma clang loop vectorize(disable)
#endif
	while (size >= 8) qword_by_qword:
	{
		size -= 8;
		*(uint64*)(d+size) = qword;
	}
	
#ifdef __clang__
#   pragma clang loop vectorize(disable)
#endif
	while (size >= 1) one_by_one:
	{
		size -= 1;
		d[size] = byte;
	}
	
	return dst;
}

static inline int32
Mem_Compare(const void* left_, const void* right_, uintsize size)
{
	const uint8* left = (const uint8*)left_;
	const uint8* right = (const uint8*)right_;
	
#ifdef __clang__
#   pragma clang loop vectorize(disable)
#endif
	while (size >= 16)
	{
		__m128i ldata, rdata;
		
		ldata = _mm_loadu_si128((const __m128i*)left);
		rdata = _mm_loadu_si128((const __m128i*)right);
		
		int32 cmp = _mm_movemask_epi8(_mm_cmpeq_epi8(ldata, rdata));
		cmp = ~cmp & 0xffff;
		
		if (Unlikely(cmp != 0))
		{
			union
			{
				__m128i reg;
				int8 bytes[16];
			} diff = { _mm_sub_epi8(ldata, rdata) };
			
			return diff.bytes[Mem_BitCtz32(cmp)] < 0 ? -1 : 1;
		}
		
		size -= 16;
		left += 16;
		right += 16;
	}
	
#ifdef __clang__
#   pragma clang loop vectorize(disable)
#endif
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
Mem_Strlen(const char* restrict cstr)
{
	const char* begin = cstr;
	
	while (*cstr)
		++cstr;
	
	return cstr - begin;
}

static inline int32
Mem_Strcmp(const char* left, const char* right)
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

static inline const void*
Mem_FindByte(const void* buffer, uint8 byte, uintsize size)
{
	const uint8* buf = (const uint8*)buffer;
	const uint8* const end = buf + size;
	
	if (Unlikely(size == 0))
		return NULL;
	if (size < 16)
		goto by_byte;
	
	// NOTE(ljre): XMM by XMM
	{
		__m128i mask = _mm_set1_epi8(byte);
		
#ifdef __clang__
#   pragma clang loop vectorize(disable)
#endif
		while (buf + 16 < end)
		{
			__m128i data = _mm_loadu_si128((const __m128i*)buf);
			int32 cmp = _mm_movemask_epi8(_mm_cmpeq_epi8(data, mask));
			
			if (Unlikely(cmp != 0))
				return buf + Mem_BitCtz16((uint16)cmp);
			
			buf += 16;
		}
	}
	
	// NOTE(ljre): Byte by byte
#ifdef __clang__
#   pragma clang loop vectorize(disable)
#endif
	while (buf < end) by_byte:
	{
		if (Unlikely(*buf == byte))
			return buf;
		
		++buf;
	}
	
	// NOTE(ljre): byte wasn't found.
	return NULL;
}

static inline void*
Mem_Zero(void* restrict dst, uintsize size)
{
	uint8* restrict d = (uint8*)dst;
	__m128 mzero = _mm_setzero_ps();
	
	if (Unlikely(size == 0))
		return dst;
	if (size < 8)
		goto by_byte;
	if (size < 32)
		goto by_qword;
	if (size < 128)
		goto by_xmm2;
	
#if defined(__clang__) || defined(__GNUC__) || defined(_MSC_VER)
	if (Unlikely(size >= 2048))
	{
#   if defined(__clang__) || defined(__GNUC__)
		uint8 byte = 0;
		__asm__ __volatile__("rep stosb"
			:"+D"(d), "+a"(byte), "+c"(size)
			:: "memory");
#   elif defined(_MSC_VER)
		__stosb(d, 0, size);
#   endif
		
		return dst;
	}
#endif
	
#ifdef __clang__
#   pragma clang loop vectorize(disable)
#endif
	do
	{
		_mm_storeu_ps((float*)(d+  0), mzero);
		_mm_storeu_ps((float*)(d+ 16), mzero);
		_mm_storeu_ps((float*)(d+ 32), mzero);
		_mm_storeu_ps((float*)(d+ 48), mzero);
		_mm_storeu_ps((float*)(d+ 64), mzero);
		_mm_storeu_ps((float*)(d+ 80), mzero);
		_mm_storeu_ps((float*)(d+ 96), mzero);
		_mm_storeu_ps((float*)(d+112), mzero);
		size -= 128;
		d += 128;
	}
	while (size >= 128);
	
#ifdef __clang__
#   pragma clang loop vectorize(disable)
#endif
	while (size >= 32) by_xmm2:
	{
		_mm_storeu_ps((float*)(d+ 0), mzero);
		_mm_storeu_ps((float*)(d+16), mzero);
		size -= 32;
		d += 32;
	}
	
#ifdef __clang__
#   pragma clang loop vectorize(disable)
#endif
	while (size >= 8) by_qword:
	{
		*(uint64*)d = 0;
		size -= 8;
		d += 8;
	}
	
#ifdef __clang__
#   pragma clang loop vectorize(disable)
#endif
	while (size >= 1) by_byte:
	{
		*d = 0;
		size -= 1;
		d += 1;
	}
	
	return dst;
}

#else // COMMON_DONT_USE_CRT

#include <string.h>

static inline void*
Mem_Copy(void* restrict dst, const void* restrict src, uintsize size)
{ return memcpy(dst, src, size); }

static inline void*
Mem_Move(void* dst, const void* src, uintsize size)
{ return memmove(dst, src, size); }

static inline void*
Mem_Set(void* restrict dst, uint8 byte, uintsize size)
{ return memset(dst, byte, size); }

static inline int32
Mem_Compare(const void* left_, const void* right_, uintsize size)
{ return memcmp(left_, right_, size); }

static inline uintsize
Mem_Strlen(const char* restrict cstr)
{ return strlen(cstr); }

static inline int32
Mem_Strcmp(const char* left, const char* right)
{ return strcmp(left, right); }

static inline const void*
Mem_FindByte(const void* buffer, uint8 byte, uintsize size)
{ return memchr(buffer, byte, size); }

static inline void*
Mem_Zero(void* restrict dst, uintsize size)
{ return memset(dst, 0, size); }

#endif

#endif // COMMON_MEMORY_H
