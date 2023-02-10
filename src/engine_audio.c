//~ Functions
// TODO(ljre): Better interpolation method!
static inline float32
InterpolateSample(float32 frame_index, int32 channels, int32 channel_index, const int16* samples, int32 sample_count)
{
	float32 result = samples[(int32)frame_index * channels + channel_index];
	
	if ((frame_index+1) * channels < sample_count)
	{
		result = glm_lerp(result, samples[((int32)frame_index + 1) * channels + channel_index], frame_index - floorf(frame_index));
	}
	
	return result;
}

//~ API
API bool32
E_LoadSoundBuffer(String path, Asset_SoundBuffer* out_sound)
{
	Trace(); TraceText(path);
	
	uintsize size;
	void* memory;
	
	for Arena_TempScope(global_engine.scratch_arena)
	{
		if (!OS_ReadEntireFile(path, global_engine.scratch_arena, &memory, &size))
			return false;
		
		void* newmem = OS_HeapAlloc(size);
		Mem_Copy(newmem, memory, size);
		memory = newmem;
	}
	
	stb_vorbis* vorbis = NULL;
	int32 channels, sample_rate, sample_count;
	
	{
		Trace(); TraceName(Str("stb_vorbis_open_memory"));
		int32 err;
		vorbis = stb_vorbis_open_memory(memory, (int32)size, &err, NULL);
	}
	
	if (!vorbis)
	{
		OS_HeapFree(memory);
		return false;
	}
	
	stb_vorbis_info info = stb_vorbis_get_info(vorbis);
	channels = info.channels;
	sample_rate = info.sample_rate;
	sample_count = stb_vorbis_stream_length_in_samples(vorbis);
	
	out_sound->samples = memory;
	out_sound->channels = channels;
	out_sound->sample_rate = sample_rate;
	out_sound->sample_count = sample_count;
	out_sound->vorbis = vorbis;
	
	// NOTE(ljre): I don't know why.
	out_sound->sample_count *= out_sound->channels;
	
	return true;
}

API void
E_FreeSoundBuffer(Asset_SoundBuffer* sound)
{
	stb_vorbis_close(sound->vorbis);
	OS_HeapFree(sound->samples);
}

API void
E_PlayAudios(E_PlayingAudio* audios, int32* audio_count, float32 volume)
{
	Trace();
	
	Assert(!global_engine.outputed_sound_this_frame);
	global_engine.outputed_sound_this_frame = true;
	
	// TODO(ljre): Better audio mixing!!!!
	int32 channels;
	int32 sample_count = 0;
	int32 sample_rate;
	int32 elapsed_frames;
	
	// NOTE(ljre): returned buffer is guaranteed to be zero'd
	int16* samples = OS_RequestSoundBuffer(&sample_count, &channels, &sample_rate, &elapsed_frames);
	int16* end_samples = samples + sample_count;
	
	if (!samples)
		return;
	
	if (!audio_count)
	{
		OS_CloseSoundBuffer(samples);
		return;
	}
	
	for Arena_TempScope(global_engine.scratch_arena) for (int32 i = 0; i < *audio_count; )
	{
		E_PlayingAudio* playing = &audios[i];
		int16* it = samples;
		int16* end_it = end_samples;
		bool32 should_remove = false;
		stb_vorbis* vorbis = playing->sound->vorbis;
		
		float32 scale = (float32)playing->sound->sample_rate / sample_rate;
		int32 scaled_elapsed_frames = (int32)(elapsed_frames * scale);
		
		if (playing->frame_index < 0)
			playing->frame_index = -playing->frame_index - 1;
		else
			playing->frame_index += scaled_elapsed_frames;
		
		int32 sam = playing->frame_index * playing->sound->channels;
		if (!playing->loop)
		{
			if (sam >= playing->sound->sample_count)
			{
				end_it = it;
				should_remove = true;
			}
			else if (sam + (int32)(sample_count*scale) > playing->sound->sample_count)
				end_it = it + (playing->sound->sample_count - sam);
		}
		else
			playing->frame_index %= playing->sound->sample_count / playing->sound->channels;
		
		int32 predecoded_samples_count = (int32)ceilf((end_it - it) / scale + 2);
		int16* predecoded_samples = Arena_PushArray(global_engine.scratch_arena, int16, predecoded_samples_count);
		
		{
			Trace(); TraceName(Str("stb_vorbis_seek"));
			stb_vorbis_seek(vorbis, (int32)(playing->frame_index));
		}
		
		{
			Trace(); TraceName(Str("stb_vorbis_get_samples_short_interleaved"));
			stb_vorbis_get_samples_short_interleaved(vorbis, playing->sound->channels, predecoded_samples, predecoded_samples_count);
		}
		
		float32 index = playing->frame_index / scale;
		while (it < end_it)
		{
			float32 index_to_use = index * scale;
			float32 left, right;
			
			left = InterpolateSample(index_to_use - playing->frame_index, playing->sound->channels, 0, predecoded_samples, predecoded_samples_count);
			
			if (playing->sound->channels > 1)
				right = InterpolateSample(index_to_use - playing->frame_index, playing->sound->channels, 1, predecoded_samples, predecoded_samples_count);
			else
				right = left;
			
			left  = left  * playing->volume * volume;
			right = right * playing->volume * volume;
			
			left  = glm_clamp(left,  INT16_MIN, INT16_MAX);
			right = glm_clamp(right, INT16_MIN, INT16_MAX);
			
			if (channels == 2)
			{
				*it++ += (int16)left;
				*it++ += (int16)right;
			}
			else
			{
				int16 v = (int16)(left + right) / 2;
				for (int32 j = channels; j > 0; --j)
					*it++ += v;
			}
			
			index += 1;
			
			if (index * scale > playing->sound->sample_count / playing->sound->channels)
				index = 0;
		}
		
		if (!should_remove)
			++i;
		else
		{
			int32 remaining = *audio_count - i;
			if (remaining > 0)
				Mem_Move(audios + i, audios + i + 1, sizeof(*audios) * (uintsize)remaining);
			
			--*audio_count;
		}
	}
	
	OS_CloseSoundBuffer(samples);
}
