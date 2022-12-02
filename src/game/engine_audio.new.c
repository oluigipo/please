struct Engine_LoadedAudio_
{
	uint16 generation;
	bool active;
	
	int32 channels;
	int32 sample_rate, sample_count;
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
	int32 frame_index;
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
	
	uint32 audios_to_free_count, playing_audios_to_stop_count;
	Audio_Handle* audios_to_free;
	Audio_PlayingHandle* playing_audios_to_stop;
}
typedef Engine_AudioData;

static void
Engine_AudioThreadProc_(void* user_data, int16* out_buffer, int32 frame_count)
{
	Engine_AudioData* audio = (Engine_AudioData*)user_data;
	
	// TODO(ljre): Mix all playing buffers together.
}
