// TODO(ljre): There are locks and unlocks everywhere.
//
//             The point is to just mix and output audio from another thread that wakes up ~2 times per frame,
//             but if they actually try to access the same stuff at the same time, it'll be *expensive*!

struct E_LoadedSoundRef_
{
	int32 generation;
	int32 next_free;  // NOTE(ljre): 1-indexed
	int32 index; // NOTE(ljre): 1-indexed
}
typedef E_LoadedSoundRef_;

struct E_PlayingSoundRef_
{
	int32 generation;
	int32 next_free;  // NOTE(ljre): 1-indexed
	int32 index; // NOTE(ljre): 1-indexed
}
typedef E_PlayingSoundRef_;

struct E_LoadedSound_
{
	stb_vorbis* vorbis;
	int32 channels;
	int32 sample_rate;
	int32 sample_count;
}
typedef E_LoadedSound_;

struct E_PlayingSound_
{
	int32 ref_index;
	
	E_SoundHandle sound;
	int32 current_scaled_frame;
	
	float32 volume;
	float32 speed;
}
typedef E_PlayingSound_;

struct E_AudioState
{
	OS_RWLock lock;
	bool volatile ready;
	int32 system_sample_rate;
	int32 system_channels;
	
	int32 loaded_sounds_table_size;
	int32 volatile loaded_sounds_table_first_free; // NOTE(ljre): 1-indexed
	E_LoadedSoundRef_* loaded_sounds_table;
	
	int32 playing_sounds_table_size;
	int32 volatile playing_sounds_table_first_free;  // NOTE(ljre): 1-indexed
	E_PlayingSoundRef_* playing_sounds_table;
	
	// Audio thread data
	Arena* arena;
	int32 samples_cache_size_each;
	
	int32 playing_sounds_size;
	int32 playing_sounds_cap;
	E_PlayingSound_* playing_sounds;
	
	int32 loaded_sounds_size;
	int32 loaded_sounds_cap;
	E_LoadedSound_* loaded_sounds;
}
typedef E_AudioState;

//~ Functions
static void
E_AllocSoundHandle_(E_AudioState* audio, E_SoundHandle* out_handle)
{
	int32 ref_index = audio->loaded_sounds_table_first_free;
	E_LoadedSoundRef_* ref = &audio->loaded_sounds_table[ref_index-1];
	
	SafeAssert(ref_index > 0);
	
	*out_handle = (E_SoundHandle) {
		.generation = (uint16)ref->generation,
		.index = (uint16)ref_index,
	};
	
	audio->loaded_sounds_table_first_free = ref->next_free;
	ref->next_free = 0;
}

static void
E_DeallocSoundHandle_(E_AudioState* audio, E_SoundHandle handle)
{
	int32 ref_index = handle.index;
	E_LoadedSoundRef_* ref = &audio->loaded_sounds_table[ref_index-1];
	
	SafeAssert(ref_index > 0);
	
	++ref->generation;
	ref->next_free = audio->loaded_sounds_table_first_free;
	audio->loaded_sounds_table_first_free = ref_index;
}

static void
E_AllocPlayingSoundHandle_(E_AudioState* audio, E_PlayingSoundHandle* out_handle)
{
	int32 ref_index = audio->playing_sounds_table_first_free;
	E_PlayingSoundRef_* ref = &audio->playing_sounds_table[ref_index-1];
	
	SafeAssert(ref_index > 0);
	
	*out_handle = (E_PlayingSoundHandle) {
		.generation = (uint16)ref->generation,
		.index = (uint16)ref_index,
	};
	
	audio->playing_sounds_table_first_free = ref->next_free;
	ref->next_free = 0;
}

static void
E_DeallocPlayingSoundHandle_(E_AudioState* audio, E_PlayingSoundHandle handle)
{
	int32 ref_index = handle.index;
	E_PlayingSoundRef_* ref = &audio->playing_sounds_table[ref_index-1];
	
	SafeAssert(ref_index > 0);
	
	++ref->generation;
	ref->next_free = audio->playing_sounds_table_first_free;
	audio->playing_sounds_table_first_free = ref_index;
}

static bool
E_IsSameSoundHandle_(E_SoundHandle left, E_SoundHandle right)
{ return left.generation == right.generation && left.index == right.index; }

//~ Internal API
static void
E_InitAudio_(void)
{
	Trace();
	E_AudioState* audio = global_engine.audio;
	Arena* arena = global_engine.audio_thread_arena;
	
	const int32 max_loaded_sounds = E_Limits_MaxLoadedSounds;
	const int32 max_playing_sounds = E_Limits_MaxPlayingSounds;
	
	audio->arena = arena;
	audio->system_sample_rate = global_engine.os->audio.mix_sample_rate;
	audio->system_channels = global_engine.os->audio.mix_channels;
	
	audio->loaded_sounds_table_size = max_loaded_sounds;
	audio->loaded_sounds_table_first_free = 1;
	audio->loaded_sounds_table = Arena_PushArray(arena, E_LoadedSoundRef_, max_loaded_sounds);
	for (int32 i = 0; i < max_loaded_sounds-1; ++i)
		audio->loaded_sounds_table[i].next_free = i+2;
	
	audio->playing_sounds_table_size = max_playing_sounds;
	audio->playing_sounds_table_first_free = 1;
	audio->playing_sounds_table = Arena_PushArray(arena, E_PlayingSoundRef_, max_playing_sounds);
	for (int32 i = 0; i < max_playing_sounds-1; ++i)
		audio->playing_sounds_table[i].next_free = i+2;
	
	audio->playing_sounds_size = 0;
	audio->playing_sounds_cap = max_playing_sounds;
	audio->playing_sounds = Arena_PushArray(arena, E_PlayingSound_, max_playing_sounds);
	
	audio->loaded_sounds_size = 0;
	audio->loaded_sounds_cap = max_loaded_sounds;
	audio->loaded_sounds = Arena_PushArray(arena, E_LoadedSound_, max_loaded_sounds);
	
	OS_InitRWLock(&audio->lock);
	
	audio->ready = true;
}

static void
E_DeinitAudio_(void)
{
	Trace();
	E_AudioState* audio = global_engine.audio;
	
	// NOTE(ljre): maybe not :P
	audio->ready = false;
}

static void
E_AudioThreadProc_(void* user_data, int16* restrict out_buffer, int32 channels, int32 sample_rate, int32 sample_count)
{
	Trace();
	E_AudioState* audio = user_data;
	if (!audio->ready)
		return;
	
	Arena_Savepoint scratch_save = Arena_Save(audio->arena);
	const float32 flt_sample_rate = (float32)sample_rate;
	const float32 default_master_volume = 0.25f;
	
	//- Working buffer
	float32* working_samples = Arena_PushAligned(audio->arena, sizeof(float32)*sample_count, 16);
	int32 working_channels = channels;
	int32 working_sample_count = sample_count;
	
	struct ThingToMix
	{
		float32* samples;
		int32 channels;
		int32 frame_count;
		float32 scale;
		float32 volume;
	};
	
	struct ThingToMix things_to_mix[E_Limits_MaxPlayingSounds] = { 0 };
	int32 things_to_mix_count = 0;
	
	//- Figure out what we need to mix
	OS_LockExclusive(&audio->lock);
	for (int32 i = 0; i < audio->playing_sounds_size;)
	{
		E_PlayingSound_* playing = &audio->playing_sounds[i];
		if (Unlikely(!playing->sound.index
			|| audio->loaded_sounds_table[playing->sound.index-1].generation != playing->sound.generation
			|| audio->loaded_sounds_table[playing->sound.index-1].next_free != 0))
		{
			goto lbl_remove;
		}
		
		E_LoadedSound_* sound = &audio->loaded_sounds[audio->loaded_sounds_table[playing->sound.index-1].index];
		float32 scale = (float32)sound->sample_rate / flt_sample_rate;
		int32 temp_channels = Min(working_channels, sound->channels);
		
		int32 current_unscaled_frame = (int32)(playing->current_scaled_frame * scale);
		playing->current_scaled_frame += sample_count / working_channels;
		
		{
			Trace(); TraceName(Str("stb_vorbis_seek"));
			stb_vorbis_seek(sound->vorbis, current_unscaled_frame);
		}
		
		int32 sample_count = (int32)ceilf((float32)working_sample_count * scale);
		float32* samples = Arena_PushAligned(audio->arena, sizeof(float32)*sample_count, 16);
		
		int32 frame_count;
		
		{
			Trace(); TraceName(Str("stb_vorbis_get_samples_float_interleaved"));
			frame_count = stb_vorbis_get_samples_float_interleaved(sound->vorbis, temp_channels, samples, sample_count);
		}
		
		frame_count = (int32)ceilf((float32)frame_count / scale);
		
		struct ThingToMix* thing = &things_to_mix[things_to_mix_count++];
		thing->samples = samples;
		thing->channels = temp_channels;
		thing->frame_count = frame_count;
		thing->scale = scale;
		thing->volume = playing->volume;
		
		if (playing->current_scaled_frame * scale >= sound->sample_count * sound->channels)
		{
			lbl_remove:;
			E_DeallocPlayingSoundHandle_(audio, (E_PlayingSoundHandle) {
				.index = (uint16)(audio->playing_sounds[i].ref_index + 1),
			});
			
			int32 last = --audio->playing_sounds_size;
			audio->playing_sounds[i] = audio->playing_sounds[last];
			audio->playing_sounds_table[audio->playing_sounds[i].ref_index].index = i+1;
		}
		else
			++i;
	}
	OS_UnlockExclusive(&audio->lock);
	
	//- Actual mixing
	for (int32 i = 0; i < things_to_mix_count; ++i)
	{
		struct ThingToMix* thing = &things_to_mix[i];
		Trace(); TraceName(Str("Actual Mixing"));
		
		// TODO(ljre): Vectorize this loop just ~~to learn~~for the meme? :kekw
		for (int32 frame_index = 0; frame_index < thing->frame_count; ++frame_index)
		{
			float32 index_to_use = (float32)frame_index * thing->scale;
			
			for (int32 channel_index = 0; channel_index < working_channels; ++channel_index)
			{
				int32 ch = Min(channel_index, thing->channels-1);
				int32 ind = (int32)index_to_use;
				
				float32 sample_a = thing->samples[(ind+0)*thing->channels + ch];
				float32 sample_b = thing->samples[(ind+1)*thing->channels + ch];
				float32 t = index_to_use - (int32)(index_to_use);
				float32 sample = glm_lerp(sample_a, sample_b, t) * thing->volume;
				
				working_samples[frame_index*working_channels + channel_index] += sample;
			}
		}
	}
	
	//- Cast samples to int16 with saturation.
	int32 head = 0;
	__m128 mul = _mm_set1_ps(INT16_MAX);
	mul = _mm_mul_ps(mul, _mm_set1_ps(default_master_volume));
	
#ifdef __clang__
#   pragma clang loop unroll(disable)
#endif
	for (; head+8 <= sample_count; head += 8)
	{
		__m128i low_half  = _mm_cvtps_epi32(_mm_mul_ps(_mm_load_ps(&working_samples[head+0]), mul));
		__m128i high_half = _mm_cvtps_epi32(_mm_mul_ps(_mm_load_ps(&working_samples[head+4]), mul));
		
		__m128i packed = _mm_packs_epi32(low_half, high_half);
		_mm_storeu_si128((__m128i*)&out_buffer[head], packed);
	}
	
#ifdef __clang__
#   pragma clang loop unroll(disable)
#endif
	for (; head+1 <= sample_count; head += 1)
	{
		float32 sample = working_samples[head] * _mm_cvtss_f32(mul);
		
		if (sample < INT16_MIN)
			sample = INT16_MIN;
		else if (sample > INT16_MAX)
			sample = INT16_MAX;
		
		out_buffer[head] = (int16)sample;
	}
	
	//- Done
	Arena_Restore(scratch_save);
}

//~ API
API bool
E_LoadSound(Buffer ogg, E_SoundHandle* out_sound, E_SoundInfo* out_info)
{
	Trace();
	SafeAssert(ogg.size <= INT32_MAX);
	
	E_AudioState* audio = global_engine.audio;
	
	int32 err;
	stb_vorbis* vorbis = stb_vorbis_open_memory(ogg.data, (int32)ogg.size, &err, NULL);
	if (!vorbis)
		return false;
	
	stb_vorbis_info vorbis_info = stb_vorbis_get_info(vorbis);
	int32 sample_count = stb_vorbis_stream_length_in_samples(vorbis);
	float32 length = stb_vorbis_stream_length_in_seconds(vorbis);
	
	E_SoundHandle handle = { 0 };
	E_SoundInfo info = {
		.channels = vorbis_info.channels,
		.sample_rate = vorbis_info.sample_rate,
		.sample_count = sample_count,
		.length = length,
	};
	E_LoadedSound_ loaded_sound = {
		.vorbis = vorbis,
		.channels = vorbis_info.channels,
		.sample_rate = vorbis_info.sample_rate,
		.sample_count = sample_count,
	};
	
	bool ok = true;
	OS_LockExclusive(&audio->lock);
	{
		if (!audio->loaded_sounds_table_first_free)
			ok = false;
		else
		{
			E_AllocSoundHandle_(audio, &handle);
			int32 index = audio->loaded_sounds_size++;
			audio->loaded_sounds[index] = loaded_sound;
			audio->loaded_sounds_table[handle.index-1].index = index;
		}
	}
	OS_UnlockExclusive(&audio->lock);
	
	if (ok)
	{
		*out_sound = handle;
		
		if (out_info)
			*out_info = info;
	}
	else
		stb_vorbis_close(vorbis);
	
	return ok;
}

API void
E_UnloadSound(E_SoundHandle sound)
{
	Trace();
	E_AudioState* audio = global_engine.audio;
	
	if (E_IsValidSoundHandle(sound))
	{
		OS_LockExclusive(&audio->lock);
		
		int32 index = audio->loaded_sounds_table[sound.index-1].index;
		stb_vorbis_close(audio->loaded_sounds[index].vorbis);
		Mem_Zero(&audio->loaded_sounds[index], sizeof(E_LoadedSound_));
		E_DeallocSoundHandle_(audio, sound);
		
		OS_UnlockExclusive(&audio->lock);
	}
}

API bool
E_IsValidSoundHandle(E_SoundHandle sound)
{
	Trace();
	E_AudioState* audio = global_engine.audio;
	
	if (!sound.index || sound.index > E_Limits_MaxLoadedSounds)
		return false;
	if (audio->loaded_sounds_table[sound.index-1].generation != sound.generation)
		return false;
	if (audio->loaded_sounds_table[sound.index-1].next_free)
		return false;
	
	return true;
}

API bool
E_QuerySoundInfo(E_SoundHandle sound, E_SoundInfo* out_info)
{
	Trace();
	E_AudioState* audio = global_engine.audio;
	bool result = false;
	
	if (E_IsValidSoundHandle(sound))
	{
		result = true;
		
		E_LoadedSound_* loaded_sound = &audio->loaded_sounds[audio->loaded_sounds_table[sound.index-1].index];
		*out_info = (E_SoundInfo) {
			.channels = loaded_sound->channels,
			.sample_rate = loaded_sound->sample_rate,
			.sample_count = loaded_sound->sample_count,
			.length = (float32)loaded_sound->sample_count / (float32)loaded_sound->sample_rate,
		};
	}
	
	return result;
}

API bool
E_PlaySound(E_SoundHandle sound, const E_PlaySoundOptions* options, E_PlayingSoundHandle* out_playing)
{
	Trace();
	E_AudioState* audio = global_engine.audio;
	
	bool result = false;
	E_PlayingSoundHandle handle = { 0 };
	E_PlayingSound_ playing = {
		.sound = sound,
		.current_scaled_frame = 0,
		.volume = options->volume != 0.0f ? options->volume : 1.0f,
		.speed = options->speed != 0.0f ? options->speed : 1.0f,
	};
	
	OS_LockExclusive(&audio->lock);
	
	if (audio->playing_sounds_table_first_free)
	{
		E_AllocPlayingSoundHandle_(audio, &handle);
		result = true;
		
		playing.ref_index = handle.index-1;
		
		int32 index = audio->playing_sounds_size++;
		audio->playing_sounds[index] = playing;
		audio->playing_sounds_table[handle.index-1].index = index;
	}
	
	OS_UnlockExclusive(&audio->lock);
	
	if (out_playing)
		*out_playing = handle;
	
	return result;
}

API bool
E_StopSound(E_PlayingSoundHandle playing_sound)
{
	Trace();
	E_AudioState* audio = global_engine.audio;
	bool result = false;
	
	if (playing_sound.index && playing_sound.index <= E_Limits_MaxPlayingSounds)
	{
		OS_LockExclusive(&audio->lock);
		
		if (audio->playing_sounds_table[playing_sound.index-1].generation == playing_sound.generation
			&& !audio->playing_sounds_table[playing_sound.index-1].next_free)
		{
			int32 index = audio->playing_sounds_table[playing_sound.index-1].index;
			int32 last = --audio->playing_sounds_size;
			audio->playing_sounds[index] = audio->playing_sounds[last];
			audio->playing_sounds_table[audio->playing_sounds[index].ref_index].index = index+1;
			
			E_DeallocPlayingSoundHandle_(audio, playing_sound);
		}
		
		OS_UnlockExclusive(&audio->lock);
	}
	
	return result;
}

API bool
E_QueryPlayingSoundInfo(E_PlayingSoundHandle playing_sound, E_PlayingSoundInfo* out_info)
{
	Trace();
	E_AudioState* audio = global_engine.audio;
	bool result = false;
	
	if (playing_sound.index && playing_sound.index <= E_Limits_MaxPlayingSounds)
	{
		OS_LockExclusive(&audio->lock);
		
		if (audio->playing_sounds_table[playing_sound.index-1].generation == playing_sound.generation
			&& !audio->playing_sounds_table[playing_sound.index-1].next_free)
		{
			result = true;
			
			E_PlayingSound_* playing = &audio->playing_sounds[audio->playing_sounds_table[playing_sound.index-1].index];
			
			*out_info = (E_PlayingSoundInfo) {
				.sound = playing->sound,
				.at = playing->current_scaled_frame / (float32)audio->system_sample_rate,
				.speed = playing->speed,
				.volume = playing->volume,
			};
		}
		
		OS_UnlockExclusive(&audio->lock);
	}
	
	return result;
}

API void
E_StopAllSounds(E_SoundHandle* specific)
{
	Trace();
	E_AudioState* audio = global_engine.audio;
	
	OS_LockExclusive(&audio->lock);
	
	for (int32 i = 0; i < audio->playing_sounds_size; ++i)
	{
		if (!specific || E_IsSameSoundHandle_(*specific, audio->playing_sounds[i].sound))
		{
			E_DeallocPlayingSoundHandle_(audio, (E_PlayingSoundHandle) {
				.index = (uint16)(audio->playing_sounds[i].ref_index + 1),
			});
		}
	}
	
	audio->playing_sounds_size = 0;
	
	OS_UnlockExclusive(&audio->lock);
}

API bool
E_IsValidPlayingSoundHandle(E_PlayingSoundHandle playing_sound)
{
	Trace();
	E_AudioState* audio = global_engine.audio;
	bool result = true;
	
	if (!playing_sound.index || playing_sound.index > E_Limits_MaxPlayingSounds)
		result = false;
	else
	{
		OS_LockExclusive(&audio->lock);
		
		if (audio->playing_sounds_table[playing_sound.index-1].generation != playing_sound.generation)
			result = false;
		else if (audio->playing_sounds_table[playing_sound.index-1].next_free)
			result = false;
		
		OS_UnlockExclusive(&audio->lock);
	}
	
	return result;
}
