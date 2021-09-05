struct Audio_PlayingBuffer
{
	Asset_SoundBuffer sound;
	int32 frame_index;
	bool32 loop;
	float64 volume;
	float64 speed;
}
typedef Audio_PlayingBuffer;

//~ Globals
internal Audio_PlayingBuffer global_playing_buffers[32]; // NOTE(ljre): maybe update to dynamic array
internal int32 global_playing_buffer_count = 0;
internal float64 global_sound_volume = 0.25;

//~ Functions
internal inline void
FreePlayingBufferIndex(int32 index)
{
	if (global_playing_buffer_count > 1)
		global_playing_buffers[index] = global_playing_buffers[global_playing_buffer_count-1];
	
	--global_playing_buffer_count;
}

internal inline Audio_PlayingBuffer*
PushPlayingBufferIndex(void)
{
	Audio_PlayingBuffer* result = NULL;
	
	if (global_playing_buffer_count < ArrayLength(global_playing_buffers))
	{
		result = &global_playing_buffers[global_playing_buffer_count++];
	}
	
	return result;
}

internal inline float64
Lerp(float64 a, float64 b, float64 t)
{
	return a * (1.0-t) + b * t;
}

internal inline float64
Clamp(float64 v, float64 min, float64 max)
{
	if (v >= max)
		v = max;
	
	return fmax(v, min);
}

// TODO(ljre): Better interpolation method!
internal inline float64
InterpolateSample(float64 frame_index, int32 channels, int32 channel_index, const int16* samples, int32 sample_count)
{
	float64 result = (float64)samples[(int32)frame_index * channels + channel_index];
	
	if (frame_index+1 < (float64)sample_count)
	{
		result = Lerp(result,
					  (float64)samples[(int32)(frame_index + 1.0) * channels + channel_index],
					  frame_index - floor(frame_index));
	}
	
	return result;
}

//~ Internal API
internal void
Engine_UpdateAudio(void)
{
	Trace("Engine_UpdateAudio");
	
	// TODO(ljre): Better audio mixing!!!!
	
	int32 out_channels;
	int32 out_sample_count = 0;
	int32 out_sample_rate;
	
	// NOTE(ljre): returned buffer is guaranteed to be zero'd
	int16* out_samples = Platform_RequestSoundBuffer(&out_sample_count, &out_channels, &out_sample_rate);
	int16* out_end_samples = out_samples + out_sample_count;
	float64 volume_scale = 1.0;// / sqrt((float64)global_playing_buffer_count);
	
	if (!out_samples)
		return;
	
	for (int32 i = 0; i < global_playing_buffer_count;)
	{
		Audio_PlayingBuffer* playing = &global_playing_buffers[i];
		int16* out_it = out_samples;
		
		float64 scale = (float64)playing->sound.sample_rate / (float64)out_sample_rate * playing->speed;
		
		if (playing->frame_index < 0)
		{
			float64 diff = (float64)playing->frame_index + (float64)out_sample_count / scale;
			if (diff > 0.0)
			{
				playing->frame_index = 0;
				out_it = out_samples + (int32)diff;
			}
			else
			{
				playing->frame_index += out_sample_count;
				continue;
			}
		}
		
		while (out_it < out_end_samples)
		{
			float64 index_to_use = (float64)playing->frame_index * scale;
			float64 left, right;
			
			left = InterpolateSample(index_to_use, playing->sound.channels, 0,
									 playing->sound.samples, playing->sound.sample_count);
			
			if (playing->sound.channels > 1)
			{
				right = InterpolateSample(index_to_use, playing->sound.channels, 1,
										  playing->sound.samples, playing->sound.sample_count);
			}
			else
			{
				right = left;
			}
			
			left  = left  * global_sound_volume * playing->volume * volume_scale;
			right = right * global_sound_volume * playing->volume * volume_scale;
			
			left  = Clamp(left,  INT16_MIN, INT16_MAX);
			right = Clamp(right, INT16_MIN, INT16_MAX);
			
			if (out_channels == 2)
			{
				*out_it++ += (int16)left;
				*out_it++ += (int16)right;
			}
			else
			{
				*out_it++ += (int16)left;
			}
			
			playing->frame_index += 1;
			
			if ((float64)playing->frame_index * scale >= (float64)playing->sound.sample_count)
			{
				if (playing->loop)
					playing->frame_index = 0;
				else
				{
					FreePlayingBufferIndex(i);
					goto _freed_playing_buffer;
				}
			}
		}
		
		++i;
		
		_freed_playing_buffer:;
	}
	
}

//~ API
API bool32
Audio_LoadFile(String path, Asset_SoundBuffer* out_sound)
{
	Trace("Audio_LoadFile");
	
	uintsize size;
	void* memory = Platform_ReadEntireFile(path, &size);
	
	if (!memory)
		return false;
	
	{
		Trace("stb_vorbis_decode_memory");
		out_sound->sample_count = stb_vorbis_decode_memory(memory, (int32)size, &out_sound->channels,
														   &out_sound->sample_rate, &out_sound->samples);
	}
	
	Platform_FreeFileMemory(memory, size);
	
	if (out_sound->sample_count == -1)
	{
		return false;
	}
	
	return true;
}

API void
Audio_FreeSoundBuffer(Asset_SoundBuffer* sound)
{
	Platform_HeapFree(sound->samples);
}

API void
Audio_Play(const Asset_SoundBuffer* sound, bool32 loop, float64 volume, float64 speed)
{
	Audio_PlayingBuffer* playing = PushPlayingBufferIndex();
	
	if (playing)
	{
		playing->sound = *sound;
		playing->frame_index = 0;
		playing->loop = loop;
		playing->volume = volume;
		playing->speed = speed;
	}
}

API void
Audio_StopByBuffer(const Asset_SoundBuffer* sound, int32 max)
{
	for (int32 i = 0; i < global_playing_buffer_count && max != 0;)
	{
		Audio_PlayingBuffer* playing = &global_playing_buffers[i];
		
		if (playing->sound.samples == sound->samples)
		{
			FreePlayingBufferIndex(i);
		}
		else
		{
			++i;
		}
	}
}
