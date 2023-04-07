#ifndef COMMON_MEMORY_X86_H
#define COMMON_MEMORY_X86_H

#include <immintrin.h>

//- Mem_BitCtz
static inline int32
Mem_BitCtz64(uint64 i)
{
	int32 result;
	
	if (i == 0)
		result = sizeof(i)*8;
	else
	{
#if defined(__GNUC__) || defined(__clang__)
		result = __builtin_ctzll(i);
#elif defined(_MSC_VER)
		_BitScanForward64((unsigned long*)&result, i);
#else
		result = Mem_Generic_BitCtz64(i);
#endif
	}
	
	return result;
}

static inline int32
Mem_BitCtz32(uint32 i)
{
	int32 result;
	
	if (i == 0)
		result = sizeof(i)*8;
	else
	{
#if defined(__GNUC__) || defined(__clang__)
		result = __builtin_ctz(i);
#elif defined(_MSC_VER)
		_BitScanForward((unsigned long*)&result, i);
#else
		result = Mem_Generic_BitCtz32(i);
#endif
	}
	
	return result;
}

static inline int32
Mem_BitCtz16(uint16 i)
{ return i == 0 ? sizeof(i)*8 : Mem_BitCtz32(i); }

static inline int32
Mem_BitCtz8(uint8 i)
{ return i == 0 ? sizeof(i)*8 : Mem_BitCtz32(i); }

//- Mem_BitClz
static inline int32
Mem_BitClz64(uint64 i)
{
	int32 result;
	
	if (i == 0)
		result = sizeof(i)*8;
	else
	{
#if defined(__GNUC__) || defined(__clang__)
		result = __builtin_clzll(i);
#elif defined(_MSC_VER)
		_BitScanReverse((unsigned long*)&result, i);
		result = 63 - result;
#else
		result = Mem_Generic_BitClz64(i);
#endif
	}
	
	return result;
}

static inline int32
Mem_BitClz32(uint32 i)
{
	int32 result;
	
	if (i == 0)
		result = sizeof(i)*8;
	else
	{
#if defined(__GNUC__) || defined(__clang__)
		result = __builtin_clz(i);
#elif defined(_MSC_VER)
		_BitScanReverse((unsigned long*)&result, i);
		result = 31 - result;
#else
		result = Mem_Generic_BitClz32(i);
#endif
	}
	
	return result;
}

static inline int32
Mem_BitClz16(uint16 i)
{ return i == 0 ? sizeof(i)*8 : Mem_BitClz32(i) - 16; }

static inline int32
Mem_BitClz8(uint8 i)
{ return i == 0 ? sizeof(i)*8 : Mem_BitClz32(i) - 24; }

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

static inline int32
Mem_PopCnt64(uint64 x)
{ return Mem_Generic_PopCnt64(x); }

static inline int32
Mem_PopCnt32(uint32 x)
{ return Mem_Generic_PopCnt32(x); }

static inline int32
Mem_PopCnt16(uint16 x)
{ return Mem_Generic_PopCnt16(x); }

static inline int32
Mem_PopCnt8(uint8 x)
{ return Mem_Generic_PopCnt8(x); }

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
	result = Mem_Generic_ByteSwap32(x);
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
	result = Mem_Generic_ByteSwap64(x);
#endif
	
	return result;
}

//-
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
#endif
	return dst;
}
#if defined(_MSC_VER) && !defined(__clang__)
#   pragma optimize("", on)
#endif

static inline void
Mem_CopyX16(void* restrict dst, const void* restrict src)
{ _mm_store_ps((float32*)dst, _mm_load_ps((const float32*)src)); }

//- CRT memcpy, memmove, memset & memcmp functions
#ifdef COMMON_DONT_USE_CRT

// NOTE(ljre): Loop vectorization when using clang is disabled.
//             this thing is already vectorized, though it likes to vectorize the 1-by-1 bits still.
//
//             GCC only does this at -O3, which we don't care about. MSVC is ok.

static inline void*
Mem_Copy(void* restrict dst, const void* restrict src, uintsize size)
{
	Trace();
	
	uint8* restrict d = (uint8*)dst;
	const uint8* restrict s = (const uint8*)src;
	
	if (size >= 32)
	{
		// NOTE(ljre): Simply use 'rep movsb'.
#if defined(__clang__) || defined(__GNUC__) || defined(_MSC_VER)
		if (Unlikely(size >= 4096))
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
		
#ifdef __clang__
#   pragma clang loop unroll(disable)
#endif
		while (size >= 128)
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
		
#ifdef __clang__
#   pragma clang loop unroll(disable)
#endif
		while (size >= 32)
		{
			size -= 32;
			_mm_storeu_si128((__m128i*)(d+size+ 0), _mm_loadu_si128((__m128i*)(s+size+ 0)));
			_mm_storeu_si128((__m128i*)(d+size+16), _mm_loadu_si128((__m128i*)(s+size+16)));
		}
	}
	
	switch (size)
	{
		case 0: break;
		
		case 31:        _mm_storeu_si128((__m128i*)d, _mm_loadu_si128((__m128i*)s)); d += 16; s += 16;
		case 15:        *(uint64*)d = *(uint64*)s; d += 8; s += 8;
		case  7: lbl_7: *(uint32*)d = *(uint32*)s; d += 4; s += 4;
		case  3: lbl_3: *(uint16*)d = *(uint16*)s; d += 2; s += 2;
		case  1: lbl_1: *d = *s; break;
		
		case 30:        _mm_storeu_si128((__m128i*)d, _mm_loadu_si128((__m128i*)s)); d += 16; s += 16;
		case 14:        *(uint64*)d = *(uint64*)s; d += 8; s += 8;
		case  6: lbl_6: *(uint32*)d = *(uint32*)s; d += 4; s += 4;
		case  2: lbl_2: *(uint16*)d = *(uint16*)s; d += 2; s += 2; break;
		
		case 29:        _mm_storeu_si128((__m128i*)d, _mm_loadu_si128((__m128i*)s)); d += 16; s += 16;
		case 13:        *(uint64*)d = *(uint64*)s; d += 8; s += 8;
		case  5: lbl_5: *(uint32*)d = *(uint32*)s; d += 4; s += 4; goto lbl_1;
		
		case 28:        _mm_storeu_si128((__m128i*)d, _mm_loadu_si128((__m128i*)s)); d += 16; s += 16;
		case 12:        *(uint64*)d = *(uint64*)s; d += 8; s += 8;
		case  4: lbl_4: *(uint32*)d = *(uint32*)s; break;
		
		case 27: _mm_storeu_si128((__m128i*)d, _mm_loadu_si128((__m128i*)s)); d += 16; s += 16;
		case 11: *(uint64*)d = *(uint64*)s; d += 8; s += 8; goto lbl_3;
		
		case 26: _mm_storeu_si128((__m128i*)d, _mm_loadu_si128((__m128i*)s)); d += 16; s += 16;
		case 10: *(uint64*)d = *(uint64*)s; d += 8; s += 8; goto lbl_2;
		
		case 25: _mm_storeu_si128((__m128i*)d, _mm_loadu_si128((__m128i*)s)); d += 16; s += 16;
		case  9: *(uint64*)d = *(uint64*)s; d += 8; s += 8; goto lbl_1;
		
		case 24: _mm_storeu_si128((__m128i*)d, _mm_loadu_si128((__m128i*)s)); d += 16; s += 16;
		case  8: *(uint64*)d = *(uint64*)s; break;
		
		case 23: _mm_storeu_si128((__m128i*)d, _mm_loadu_si128((__m128i*)s)); d += 16; s += 16; goto lbl_7;
		case 22: _mm_storeu_si128((__m128i*)d, _mm_loadu_si128((__m128i*)s)); d += 16; s += 16; goto lbl_6;
		case 21: _mm_storeu_si128((__m128i*)d, _mm_loadu_si128((__m128i*)s)); d += 16; s += 16; goto lbl_5;
		case 20: _mm_storeu_si128((__m128i*)d, _mm_loadu_si128((__m128i*)s)); d += 16; s += 16; goto lbl_4;
		case 19: _mm_storeu_si128((__m128i*)d, _mm_loadu_si128((__m128i*)s)); d += 16; s += 16; goto lbl_3;
		case 18: _mm_storeu_si128((__m128i*)d, _mm_loadu_si128((__m128i*)s)); d += 16; s += 16; goto lbl_2;
		case 17: _mm_storeu_si128((__m128i*)d, _mm_loadu_si128((__m128i*)s)); d += 16; s += 16; goto lbl_1;
		case 16: _mm_storeu_si128((__m128i*)d, _mm_loadu_si128((__m128i*)s)); break;
	}
	
	return dst;
}

static inline void*
Mem_Move(void* dst, const void* src, uintsize size)
{
	Trace();
	
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
		if (Unlikely(size >= 4096))
		{
#   if defined(__clang__) || defined(__GNUC__)
			__asm__ __volatile__("rep movsb"
				:"+D"(d), "+S"(s), "+c"(size)
				:: "memory");
#   else
			__movsb(d, s, size);
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
#   pragma clang loop unroll(disable)
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
#   pragma clang loop unroll(disable)
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
#   pragma clang loop unroll(disable)
#endif
		while (size >= 8) inc_by_qword:
		{
			*(uint64*)d = *(uint64*)s;
			
			d += 8;
			s += 8;
			size -= 8;
		}
		
#ifdef __clang__
#   pragma clang loop unroll(disable)
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
		if (Unlikely(size >= 4096))
		{
			d += size-1;
			s += size-1;
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
			__movsb(d, s, size);
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
#   pragma clang loop unroll(disable)
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
#   pragma clang loop unroll(disable)
#endif
		while (size >= 32) dec_by_xmm2:
		{
			size -= 32;
			_mm_storeu_si128((__m128i*)(d+size+ 0), _mm_loadu_si128((__m128i*)(s+size+ 0)));
			_mm_storeu_si128((__m128i*)(d+size+16), _mm_loadu_si128((__m128i*)(s+size+16)));
		}
		
#ifdef __clang__
#   pragma clang loop unroll(disable)
#endif
		while (size >= 8) dec_by_qword:
		{
			size -= 8;
			*(uint64*)(d+size) = *(uint64*)(s+size);
		}
		
#ifdef __clang__
#   pragma clang loop unroll(disable)
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
	Trace();
	
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
#   pragma clang loop unroll(disable)
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
#   pragma clang loop unroll(disable)
#endif
	while (size >= 32) xmm2_by_xmm2:
	{
		size -= 32;
		_mm_storeu_si128((__m128i*)(d+size+0 ), xmm);
		_mm_storeu_si128((__m128i*)(d+size+16), xmm);
	}
	
#ifdef __clang__
#   pragma clang loop unroll(disable)
#endif
	while (size >= 8) qword_by_qword:
	{
		size -= 8;
		*(uint64*)(d+size) = qword;
	}
	
#ifdef __clang__
#   pragma clang loop unroll(disable)
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
	Trace();
	
	const uint8* left = (const uint8*)left_;
	const uint8* right = (const uint8*)right_;
	
#ifdef __clang__
#   pragma clang loop unroll(disable)
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
#   pragma clang loop unroll(disable)
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
	Trace();
	
	return Mem_Generic_Strlen(cstr);
}

static inline uintsize
Mem_Strnlen(const char* restrict cstr, uintsize limit)
{
	Trace();
	
	return Mem_Generic_Strnlen(cstr, limit);
}

static inline int32
Mem_Strcmp(const char* left, const char* right)
{
	Trace();
	
	return Mem_Generic_Strcmp(left, right);
}

static inline char*
Mem_Strstr(const char* left, const char* right)
{
	Trace();
	
	return Mem_Generic_Strstr(left, right);
}

static inline const void*
Mem_FindByte(const void* buffer, uint8 byte, uintsize size)
{
	Trace();
	
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
#   pragma clang loop unroll(disable)
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
#   pragma clang loop unroll(disable)
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
	Trace();
	
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
#   pragma clang loop unroll(disable)
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
#   pragma clang loop unroll(disable)
#endif
	while (size >= 32) by_xmm2:
	{
		_mm_storeu_ps((float*)(d+ 0), mzero);
		_mm_storeu_ps((float*)(d+16), mzero);
		size -= 32;
		d += 32;
	}
	
#ifdef __clang__
#   pragma clang loop unroll(disable)
#endif
	while (size >= 8) by_qword:
	{
		*(uint64*)d = 0;
		size -= 8;
		d += 8;
	}
	
#ifdef __clang__
#   pragma clang loop unroll(disable)
#endif
	while (size >= 1) by_byte:
	{
		*d = 0;
		size -= 1;
		d += 1;
	}
	
	return dst;
}

#endif

#endif //COMMON_MEMORY_X86_H