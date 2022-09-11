// NOTE(ljre): this is a xorshift128+
// https://prng.di.unimi.it/

struct PlsRandom_State
{
	uint64 seed[2];
}
typedef PlsRandom_State;

//~ Functions
internal inline uint64
BitRotateLeft(const uint64 x, int32 k)
{
	return (x << k) | (x >> (64 - k));
}

//~ Internal API
// Generates a random number between 0 and UINT64_MAX
internal uint64
PlsRandom_U64(PlsRandom_State* state)
{
	const uint64 s0 = state->seed[0];
	uint64 s1 = state->seed[1];
	const uint64 result = BitRotateLeft(s0 + s1, 17) + s0;
	
	s1 ^= s0;
	state->seed[0] = BitRotateLeft(s0, 49) ^ s1 ^ (s1 << 21); // a, b
	state->seed[1] = BitRotateLeft(s1, 28); // c
	
	return result;
}

internal void
PlsRandom_Init(PlsRandom_State* state)
{
	static const uint64 jump[] = { 0xdf900294d8f554a5, 0x170865df4b3201fc };
	
	state->seed[0] = state->seed[1] = Platform_CurrentPosixTime();
	
	uint64 s0 = 0;
	uint64 s1 = 0;
	
	for (int32  i = 0; i < ArrayLength(jump); ++i)
	{
		for (int32  b = 0; b < 64; ++b)
		{
			if (jump[i] & (uint64)1 << b)
			{
				s0 ^= state->seed[0];
				s1 ^= state->seed[1];
			}
			PlsRandom_U64(state);
		}
	}
	
	state->seed[0] = s0;
	state->seed[1] = s1;
}

// NOTE(ljre): good enough
internal float64
PlsRandom_F64(PlsRandom_State* state)
{
	uint64 frac = PlsRandom_U64(state) >> 12;
	uint64 div = UINT64_MAX >> 12;
	
	return (float64)frac / (float64)div;
}

internal float32
PlsRandom_F32Range(PlsRandom_State* state, float32 start, float32 end)
{
	uint64 frac = PlsRandom_U64(state) >> 41;
	uint64 div = UINT64_MAX >> 41;
	
	float32 result = (float32)frac / (float32)div;
	
	return (float32)(result * (end - start) + start);
}

internal uint32
PlsRandom_U32(PlsRandom_State* state)
{
	uint64 result = PlsRandom_U64(state);
	return (uint32)(result >> 32); // the higher bits are more random.
}
