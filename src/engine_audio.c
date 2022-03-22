//~ Functions
// TODO(ljre): Better interpolation method!
internal inline float32
InterpolateSample(float32 frame_index, int32 channels, int32 channel_index, const int16* samples, int32 sample_count)
{
	float32 result = samples[(int32)frame_index * channels + channel_index];
	
	if (frame_index+1 < sample_count)
	{
		result = glm_lerp(result,
						  samples[((int32)frame_index + 1) * channels + channel_index],
						  frame_index - floorf(frame_index));
	}
	
	return result;
}

//~ API
API bool32
Engine_LoadSoundBuffer(String path, Asset_SoundBuffer* out_sound)
{
	Trace("Audio_LoadFile");
	
	uintsize size;
	void* memory = Platform_ReadEntireFile(path, &size, global_engine.temp_arena);
	
	if (!memory)
		return false;
	
	// not inside 'if'
	{
		Trace("stb_vorbis_decode_memory");
		out_sound->sample_count = stb_vorbis_decode_memory(memory, (int32)size, &out_sound->channels,
														   &out_sound->sample_rate, &out_sound->samples);
	}
	
	Arena_Pop(global_engine.temp_arena, memory);
	
	if (out_sound->sample_count == -1)
		return false;
	
	// NOTE(ljre): I don't know why.
	out_sound->sample_count *= out_sound->channels;
	
	return true;
}

API void
Engine_FreeSoundBuffer(Asset_SoundBuffer* sound)
{
	Platform_HeapFree(sound->samples);
}

API void
Engine_PlayAudios(Engine_PlayingAudio* audios, int32* audio_count, float32 volume)
{
	Trace("Engine_PlayAudios");
	
	Assert(!global_engine.outputed_sound_this_frame);
	global_engine.outputed_sound_this_frame = true;
	
	// TODO(ljre): Better audio mixing!!!!
	int32 channels;
	int32 sample_count = 0;
	int32 sample_rate;
	int32 elapsed_frames;
	
	// NOTE(ljre): returned buffer is guaranteed to be zero'd
	int16* samples = Platform_RequestSoundBuffer(&sample_count, &channels, &sample_rate, &elapsed_frames);
	int16* end_samples = samples + sample_count;
	
	if (!samples)
		return;
	
	if (!audio_count)
	{
		Platform_CloseSoundBuffer(samples);
		return;
	}
	
	for (int32 i = 0; i < *audio_count; )
	{
		Engine_PlayingAudio* playing = &audios[i];
		int16* it = samples;
		int16* end_it = end_samples;
		bool32 should_remove = false;
		
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
		
		float32 index = playing->frame_index / scale;
		while (it < end_it)
		{
			float32 index_to_use = index * scale;
			float32 left, right;
			
			left = InterpolateSample(index_to_use, playing->sound->channels, 0,
									 playing->sound->samples, playing->sound->sample_count);
			
			if (playing->sound->channels > 1)
				right = InterpolateSample(index_to_use, playing->sound->channels, 1,
										  playing->sound->samples, playing->sound->sample_count);
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
				MemMove(audios + i, audios + i + 1, sizeof(*audios) * (uintsize)remaining);
			
			--*audio_count;
		}
	}
}
