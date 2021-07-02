// NOTE(ljre): this is a xorshift128+
// https://prng.di.unimi.it/

internal uint64 global_random_seed[2]; // uint128

//~ Functions
internal inline uint64
__rotl(const uint64 x, int32 k) {
	return (x << k) | (x >> (64 - k));
}

//~ Internal API
internal void Random_Init(void) {
	static const uint64 jump[] = { 0xdf900294d8f554a5, 0x170865df4b3201fc };
	
	global_random_seed[0] = Platform_CurrentPosixTime();
	global_random_seed[1] = Platform_CurrentPosixTime();
	
	uint64 s0 = 0;
	uint64 s1 = 0;
	
	for (int32  i = 0; i < ArrayLength(jump); ++i) {
		for (int32  b = 0; b < 64; ++b) {
			if (jump[i] & (uint64)1 << b) {
				s0 ^= global_random_seed[0];
				s1 ^= global_random_seed[1];
			}
			Random_U64();
		}
	}
	
	global_random_seed[0] = s0;
	global_random_seed[1] = s1;
}

// Generates a random number between 0 and UINT64_MAX
API uint64
Random_U64(void) {
	const uint64 s0 = global_random_seed[0];
	uint64 s1 = global_random_seed[1];
	const uint64 result = __rotl(s0 + s1, 17) + s0;
	
	s1 ^= s0;
	global_random_seed[0] = __rotl(s0, 49) ^ s1 ^ (s1 << 21); // a, b
	global_random_seed[1] = __rotl(s1, 28); // c
	
	return result;
}

// NOTE(ljre): good enough
API float64
Random_F64(void) {
	return (float64)Random_U64() / (float64)UINT64_MAX;
}

API uint32
Random_U32(void) {
	uint64 result = Random_U64();
	return (uint32)(result >> 32); // the higher bits are more random.
}
