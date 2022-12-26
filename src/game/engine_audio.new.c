struct Engine_LoadedAudio_
{
	uint16 generation;
	bool active;
	
	int32 channels;
	uint32 sample_rate, sample_count;
	stb_vorbis* vorbis;
}
typedef Engine_LoadedAudio_;

struct Engine_PlayingAudio_
{
	uint16 generation;
	bool active;
	
	Audio_Handle audio;
	float32 speed;
	float32 volume;
	uint32 frame_index;
}
typedef Engine_PlayingAudio_;

struct Engine_AudioData
{
	Arena* arena;
	RWLock lock;
	
	uint32 loaded_count, loaded_cap;
	Engine_LoadedAudio_* loaded;
	uint32 playing_count, playing_cap;
	Engine_PlayingAudio_* playing;
}
typedef Engine_AudioData;

static Engine_LoadedAudio_*
Engine_PushLoadedAudio_(Audio_Handle* out_handle)
{
	return NULL;
}

static void
Engine_AudioThreadProc_(void* user_data, int16* out_buffer, uint32 sample_count, int32 channels, uint32 sample_rate, uint32 elapsed_frames)
{
	Engine_AudioData* audio = (Engine_AudioData*)user_data;
	void* arena_save = Arena_End(audio->arena);
	
	// ...
	
	Arena_Pop(audio->arena, arena_save);
}

API Audio_Handle
Audio_LoadVorbis(const void* buffer, uintsize size, Audio_Info* out_info)
{
	Trace();
	Assert(size <= INT32_MAX);
	Audio_Handle result = { 0 };
	
	stb_vorbis* vorbis = stb_vorbis_open_memory(buffer, (int32)size, NULL, NULL);
	if (vorbis)
	{
		stb_vorbis_info info = stb_vorbis_get_info(vorbis);
		int32 channels = info.channels;
		int32 sample_rate = (int32)info.sample_rate;
		int32 sample_count = stb_vorbis_stream_length_in_samples(vorbis);
		
		Engine_LoadedAudio_* loaded_audio = Engine_PushLoadedAudio_(&result);
		loaded_audio->channels = channels;
		loaded_audio->sample_rate = sample_rate;
		loaded_audio->sample_count = sample_count;
	}
	
	return result;
}
