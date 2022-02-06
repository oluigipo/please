#ifndef INTERNAL_API_ENGINE_H
#define INTERNAL_API_ENGINE_H

struct Engine_Data typedef Engine_Data;
struct Game_Data typedef Game_Data;

struct Engine_Data
{
	Arena* persistent_arena;
	Arena* temp_arena;
	Game_Data* game;
	
	bool8 outputed_sound_this_frame;
	bool8 running;
	
	float32 delta_time;
	float64 last_frame_time;
};

// Engine entry point. It shall be called by the platform layer.
API int32 Engine_Main(int32 argc, char** argv);
API void Engine_FinishFrame(void);

API void Game_Main(Engine_Data* data);

//- Discord Game SDK (OPTIONAL)
#ifdef INTERNAL_ENABLE_DISCORD_SDK
API bool32 Discord_Init(int64 appid);
API void Discord_UpdateActivityAssets(String large_image, String large_text, String small_image, String small_text);
API void Discord_UpdateActivity(int32 type, String name, String state, String details);
API void Discord_Update(void);
API void Discord_Deinit(void);
#else
#   define Discord_Init(...) ((void)(__VA_ARGS__), 0)
#   define Discord_UpdateActivityAssets(...) ((void)(__VA_ARGS__))
#   define Discord_UpdateActivity(...) ((void)(__VA_ARGS__))
#   define Discord_Update() ((void)0)
#   define Discord_Deinit() ((void)0)
#endif

//- Audio
struct Engine_PlayingAudio
{
	const Asset_SoundBuffer* sound;
	int32 frame_index; // if < 0, then it was added this frame
	bool32 loop;
	float32 volume;
	float32 speed;
}
typedef Engine_PlayingAudio;

API bool32 Engine_LoadSoundBuffer(String path, Asset_SoundBuffer* out_sound);
API void Engine_FreeSoundBuffer(Asset_SoundBuffer* sound);
API void Engine_PlayAudios(Engine_PlayingAudio* audios, int32* audio_count, float32 volume);

#endif //INTERNAL_API_ENGINE_H
