#ifndef API_ENGINE_H
#define API_ENGINE_H

#include "config.h"
#include "api_os.h"
#include "api_renderbackend.h"

// NOTE(ljre): The reference FPS 'E_GlobalData.delta_time' is going to be based of.
//             60 FPS = 1.0 DT
#define REFERENCE_FPS 60

DisableWarnings();
#include <ext/cglm/cglm.h>
#define STBTT_STATIC
#include <ext/stb_truetype.h>
ReenableWarnings();

//~ Actual Api
// TODO(ljre): LEGACY THING DELETE SOON
struct Asset_SoundBuffer
{
	int32 channels;
	int32 sample_rate;
	int32 sample_count;
	int16* samples;
}
typedef Asset_SoundBuffer;

//- Main Data
struct E_RenderApi typedef E_RenderApi;
struct G_GlobalData typedef G_GlobalData;

struct E_GlobalData
{
	Arena* persistent_arena;
	Arena* scratch_arena;
	Arena* audio_thread_arena;
	G_GlobalData* game;
	
	const OS_WindowGraphicsContext* graphics_context;
	const E_RenderApi* render;
	OS_WindowState* window_state;
	OS_InputState* input;
	
	void* game_memory;
	uintsize game_memory_size;
	
	float32 delta_time;
	float64 last_frame_time;
	
	bool outputed_sound_this_frame;
	bool running;
}
typedef E_GlobalData;

API void G_Main(struct E_GlobalData* data);
API void E_FinishFrame(void);

//- Audio
struct E_PlayingAudio
{
	const Asset_SoundBuffer* sound;
	int32 frame_index; // if < 0, then it will start playing at '-frame_index - 1'
	bool32 loop;
	float32 volume;
	float32 speed;
}
typedef E_PlayingAudio;

API bool32 E_LoadSoundBuffer(String path, Asset_SoundBuffer* out_sound);
API void E_FreeSoundBuffer(Asset_SoundBuffer* sound);
// NOTE(ljre): Should be called once per frame.
API void E_PlayAudios(E_PlayingAudio* audios, int32* audio_count, float32 volume);

//- Basic renderer funcs
struct Render_Camera2D typedef Render_Camera2D;
struct Render_Camera3D typedef Render_Camera3D;

API void E_CalcViewMatrix2D(const Render_Camera2D* camera, mat4 out_view);
API void E_CalcViewMatrix3D(const Render_Camera3D* camera, mat4 out_view, float32 fov, float32 aspect);
API void E_CalcModelMatrix2D(const vec2 pos, const vec2 scale, float32 angle, mat4 out_model);
API void E_CalcModelMatrix3D(const vec3 pos, const vec3 scale, const vec3 rot, mat4 out_model);
API void E_CalcPointInCamera2DSpace(const Render_Camera2D* camera, const vec2 pos, vec2 out_pos);

//- YET ANOTHER NEW RENDERER API!!!
struct E_Tex2d typedef E_Tex2d;
struct E_CachedBatch typedef E_CachedBatch;

struct E_Tex2dDesc
{
	Buffer encoded_image;
	Buffer raw_image;
	
	int32 width, height;
	uint32 raw_image_channel_count;
	
	bool f_linear_filtering : 1;
}
typedef E_Tex2dDesc;

API E_Tex2d* E_MakeTex2d(const E_Tex2dDesc* desc, Arena* arena);

struct E_RectBatchElem
{
	vec2 top_left;
	vec2 top_right;
	vec2 bottom_left;
	float32 tex_index;
	
	vec4 texcoords;
	vec4 color;
}
typedef E_RectBatchElem;

struct E_RectBatch
{
	uint32 count;
	E_Tex2d* textures[8];
	
	E_RectBatchElem elements[];
}
typedef E_RectBatch;

API E_CachedBatch E_CacheRectBatch(const E_RectBatch* batch, Arena* to_clone_at);

API void E_DrawClear(float32 r, float32 g, float32 b, float32 a);
API void E_DrawCachedBatch(const E_CachedBatch* batch);
API void E_DrawRectBatch(const E_RectBatch* batch);
API void E_RawDrawCommand(RB_DrawCommand* commands);

// TODO(ljre): MORE LEGACY STUFF TO DELETE
#include "engine_internal_api_render.h"

#endif //API_ENGINE_H
