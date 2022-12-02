#ifndef INTERNAL_API_AUDIO_H
#define INTERNAL_API_AUDIO_H

struct Audio_PlayingHandle
{ uint32 id; }
typedef Audio_PlayingHandle;

struct Audio_Handle
{ uint32 id; }
typedef Audio_Handle;

struct Audio_Info
{
	int32 channels;
	int32 sample_rate, sample_count;
}
typedef Audio_Info;

struct Audio_PlayingState
{
	Audio_Handle audio;
	float32 speed;
	float32 volume;
	int32 current_frame_index;
	bool loop;
}
typedef Audio_PlayingState;

API Audio_Handle Audio_LoadVorbis(const void* buffer, uintsize size, Audio_Info* out_info);
API bool Audio_FreeHandle(Audio_Handle audio);
API Audio_PlayingHandle Audio_Play(Audio_Handle audio, float32 speed, float32 volume, bool loop, int32 start_at_frame);
API bool Audio_Stop(Audio_PlayingHandle playing);
API bool Audio_QueryPlayingState(Audio_PlayingHandle playing, Audio_PlayingState* out_state);
API bool Audio_QueryInfo(Audio_Handle audio, Audio_Info* out_info);

#endif //INTERNAL_API_AUDIO_H
