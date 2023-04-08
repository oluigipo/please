#ifndef COMMON_MEMORY_ARM_H
#define COMMON_MEMORY_ARM_H

#ifdef CONFIG_ARCH_AARCH64
#   include <arm_neon.h>
#endif

static inline int32 Mem_BitCtz64(uint64 i) { return __builtin_ctzll(i); }
static inline int32 Mem_BitCtz32(uint32 i) { return __builtin_ctz(i); }
static inline int32 Mem_BitCtz16(uint16 i) { return __builtin_ctz(i); }
static inline int32 Mem_BitCtz8(uint8 i) { return __builtin_ctz(i); }

static inline int32 Mem_BitClz64(uint64 i) { return __builtin_clzll(i); }
static inline int32 Mem_BitClz32(uint32 i) { return __builtin_clz(i); }
static inline int32 Mem_BitClz16(uint16 i) { return __builtin_clz(i)-16; }
static inline int32 Mem_BitClz8(uint8 i) { return __builtin_clz(i)-24; }

#ifdef CONFIG_ARCH_AARCH64
static inline int32 Mem_PopCnt64(uint64 x) { return __builtin_popcountll(x); }
static inline int32 Mem_PopCnt32(uint32 x) { return __builtin_popcount(x); }
static inline int32 Mem_PopCnt16(uint16 x) { return __builtin_popcount(x); }
static inline int32 Mem_PopCnt8(uint8 x) { return __builtin_popcount(x); }
#else //CONFIG_ARCH_AARCH64
static inline int32 Mem_PopCnt64(uint64 x) { return Mem_Generic_PopCnt64(x); }
static inline int32 Mem_PopCnt32(uint32 x) { return Mem_Generic_PopCnt32(x); }
static inline int32 Mem_PopCnt16(uint16 x) { return Mem_Generic_PopCnt16(x); }
static inline int32 Mem_PopCnt8(uint8 x) { return Mem_Generic_PopCnt8(x); }
#endif //CONFIG_ARCH_AARCH64

static inline uint64 Mem_ByteSwap64(uint64 x) { return __builtin_bswap64(x); }
static inline uint32 Mem_ByteSwap32(uint32 x) { return __builtin_bswap32(x); }
static inline uint16 Mem_ByteSwap16(uint16 x) { return __builtin_bswap16(x); }

#ifdef CONFIG_ARCH_AARCH64
static inline void
Mem_CopyX16(void* restrict dst, const void* restrict src)
{ vst1q_u64((uint64*)dst, vld1q_u64((const uint64*)src)); }
#else //CONFIG_ARCH_AARCH64
static inline void
Mem_CopyX16(void* restrict dst, const void* restrict src)
{
	((uint32*)dst)[0] = ((const uint32*)src)[0];
	((uint32*)dst)[1] = ((const uint32*)src)[1];
	((uint32*)dst)[2] = ((const uint32*)src)[2];
	((uint32*)dst)[3] = ((const uint32*)src)[3];
}
#endif //CONFIG_ARCH_AARCH64

#endif //COMMON_MEMORY_ARM_H
