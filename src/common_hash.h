#ifndef COMMON_HASH_H
#define COMMON_HASH_H

// NOTE(ljre): Just a normal FNV-1a implementation.
static inline uint64
Hash_StringHash(String memory)
{
	uint64 result = 14695981039346656037u;
	
	for (uintsize i = 0; i < memory.size; ++i)
	{
		uint64 value = (uint64)memory.data[i];
		
		result ^= value;
		result *= 1099511628211u;
	}
	
	return result;
}

// NOTE(ljre): Perfect hash of 32bit integer permutation
//             Name: lowbias32.
//             https://github.com/skeeto/hash-prospector
static inline uint32
Hash_IntHash32(uint32 x)
{
    x ^= x >> 16;
    x *= 0x7feb352d;
    x ^= x >> 15;
    x *= 0x846ca68b;
    x ^= x >> 16;
    return x;
}

// NOTE(ljre): Perfect hash of 64bit integer permitation.
//             Name: SplittableRandom / SplitMix64
//             https://xoshiro.di.unimi.it/splitmix64.c
static inline uint64
Hash_IntHash64(uint64 x)
{
    x ^= x >> 30;
    x *= 0xbf58476d1ce4e5b9;
    x ^= x >> 27;
    x *= 0x94d049bb133111eb;
    x ^= x >> 31;
    return x;
}

// NOTE(ljre): This is a super simple implementation of "MSI hash table".
//             https://nullprogram.com/blog/2022/08/08/
static inline int32
Hash_Msi(uint32 log2_of_cap, uint64 hash, int32 index)
{
	uint32 exp = log2_of_cap;
	uint32 mask = (1u << exp) - 1;
	uint32 step = (uint32)(hash >> (64 - exp)) | 1;
	return (index + step) & mask;
}

#endif //COMMON_HASH_H
