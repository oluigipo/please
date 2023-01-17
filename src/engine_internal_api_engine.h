#ifndef INTERNAL_API_ENGINE_H
#define INTERNAL_API_ENGINE_H

//~ Main Data
struct Game_Data typedef Game_Data;

struct Engine_RenderApi typedef Engine_RenderApi;

struct Engine_Data
{
	Arena* persistent_arena;
	Arena* scratch_arena;
	Arena* audio_thread_arena;
	Game_Data* game;
	
	const OS_WindowGraphicsContext* graphics_context;
	const Engine_RenderApi* render;
	OS_WindowState* window_state;
	OS_InputState* input;
	
	void* game_memory;
	uintsize game_memory_size;
	
	float32 delta_time;
	float64 last_frame_time;
	
	bool outputed_sound_this_frame;
	bool running;
}
typedef Engine_Data;

// Engine entry point. It shall be called by the platform layer.
API int32 Engine_Main(int32 argc, char** argv);
API void Engine_FinishFrame(void);
API void Game_Main(Engine_Data* data);

//~ Audio
struct Engine_PlayingAudio
{
	const Asset_SoundBuffer* sound;
	int32 frame_index; // if < 0, then it will start playing at '-frame_index - 1'
	bool32 loop;
	float32 volume;
	float32 speed;
}
typedef Engine_PlayingAudio;

API bool32 Engine_LoadSoundBuffer(String path, Asset_SoundBuffer* out_sound);
API void Engine_FreeSoundBuffer(Asset_SoundBuffer* sound);
// NOTE(ljre): Should be called once per frame.
API void Engine_PlayAudios(Engine_PlayingAudio* audios, int32* audio_count, float32 volume);

//~ Basic renderer funcs
struct Render_Camera2D typedef Render_Camera2D;
struct Render_Camera3D typedef Render_Camera3D;

API void Engine_CalcViewMatrix2D(const Render_Camera2D* camera, mat4 out_view);
API void Engine_CalcViewMatrix3D(const Render_Camera3D* camera, mat4 out_view, float32 fov, float32 aspect);
API void Engine_CalcModelMatrix2D(const vec2 pos, const vec2 scale, float32 angle, mat4 out_model);
API void Engine_CalcModelMatrix3D(const vec3 pos, const vec3 scale, const vec3 rot, mat4 out_model);
API void Engine_CalcPointInCamera2DSpace(const Render_Camera2D* camera, const vec2 pos, vec2 out_pos);

#endif //INTERNAL_API_ENGINE_H
