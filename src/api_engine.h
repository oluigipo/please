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
ReenableWarnings();

#define vec2(...) (vec2) { __VA_ARGS__ }
#define vec3(...) (vec3) { __VA_ARGS__ }
#define vec4(...) (vec4) { __VA_ARGS__ }

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
struct G_GlobalData typedef G_GlobalData;

struct E_GlobalData
{
	Arena* persistent_arena;
	Arena* scratch_arena;
	Arena* frame_arena;
	G_GlobalData* game;
	
	const OS_WindowGraphicsContext* graphics_context;
	OS_WindowState* window_state;
	OS_InputState* input;
	
	RB_ResourceCommand* resource_command_list;
	RB_ResourceCommand* last_resource_command;
	
	RB_DrawCommand* draw_command_list;
	RB_DrawCommand* last_draw_command;
	
	void* game_memory;
	uintsize game_memory_size;
	
	float32 delta_time;
	float64 last_frame_time;
	
	bool outputed_sound_this_frame : 1;
	bool running : 1;
}
typedef E_GlobalData;

API void G_Main(E_GlobalData* data);
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
struct E_Camera2D
{
	vec2 pos;
	vec2 size;
	float32 zoom;
	float32 angle;
}
typedef E_Camera2D;

struct E_Camera3D
{
	vec3 pos;
	vec3 dir;
	vec3 up;
}
typedef E_Camera3D;

API void E_CalcViewMatrix2D(const E_Camera2D* camera, mat4 out_view);
API void E_CalcViewMatrix3D(const E_Camera3D* camera, mat4 out_view, float32 fov, float32 aspect);
API void E_CalcModelMatrix2D(const vec2 pos, const vec2 scale, float32 angle, mat4 out_model);
API void E_CalcModelMatrix3D(const vec3 pos, const vec3 scale, const vec3 rot, mat4 out_model);
API void E_CalcPointInCamera2DSpace(const E_Camera2D* camera, const vec2 pos, vec2 out_pos);

//- YET ANOTHER NEW RENDERER API!!!
struct E_Tex2d
{
	RB_Handle handle;
	int32 width;
	int32 height;
}
typedef E_Tex2d;

struct E_FontGlyphEntry
{
	uint32 codepoint;
	
	uint16 x, y;
	uint16 width, height;
	
	int16 xoff, yoff;
	int16 advance, bearing;
}
typedef E_FontGlyphEntry;

struct E_Font
{
	RB_Handle texture;
	
	Buffer ttf;
	void* stb_fontinfo;
	
	uint8* bitmap;
	int32 tex_size;
	int32 tex_currline;
	int32 tex_currcol;
	int32 tex_linesize;
	
	uint32 glyphmap_count, glyphmap_log2cap;
	E_FontGlyphEntry* glyphmap;
	E_FontGlyphEntry invalid_glyph;
	
	int32 ascent, descent, line_gap, space_advance;
	float32 char_scale;
}
typedef E_Font;

struct E_CachedBatch
{
	RB_Handle vbuffer;
	RB_Handle ubuffer;
	RB_Handle* samplers[RB_Limits_SamplersPerDrawCall];
	uint32 instance_count;
}
typedef E_CachedBatch;

struct E_Tex2dDesc
{
	Buffer encoded_image;
	Buffer raw_image;
	
	int32 width, height;
	uint32 raw_image_channel_count;
	
	bool flag_linear_filtering : 1;
}
typedef E_Tex2dDesc;

struct E_FontDesc
{
	Arena* arena;
	Buffer ttf;
	
	int32 bitmap_size; // default: 1024
	float32 char_height; // default: 32
	int32 hashmap_log2cap; // default: 16
	
	struct
	{
		uint32 begin;
		uint32 end;
	}
	prebake_ranges[8];
}
typedef E_FontDesc;

API bool E_MakeTex2d(const E_Tex2dDesc* desc, E_Tex2d* out_tex);
API bool E_MakeFont(const E_FontDesc* desc, E_Font* out_font);

struct E_RectBatchElem
{
	vec2 pos;
	mat2 scaling;
	float32 tex_index;
	float32 tex_kind;
	
	vec4 texcoords;
	vec4 color;
}
typedef E_RectBatchElem;

struct E_RectBatch
{
	RB_Handle* textures[RB_Limits_SamplersPerDrawCall];
	
	uint32 count;
	E_RectBatchElem* elements;
}
typedef E_RectBatch;

API bool E_PushTextToRectBatch(E_RectBatch* batch, Arena* arena, E_Font* font, String text, vec2 pos, vec2 scale, vec4 color);

API void E_CacheRectBatch(const E_RectBatch* batch, E_CachedBatch* out_cached_batch, Arena* to_clone_batch_at);

API void E_DrawClear(float32 r, float32 g, float32 b, float32 a);
API void E_DrawCachedBatch(const E_CachedBatch* batch);
API void E_DrawRectBatch(const E_RectBatch* batch, const E_Camera2D* cam);

API void E_RawDrawCommand(RB_DrawCommand* commands);

#if 0
//- NEW AUDIO MIXER
struct E_AudioHandle { uint32 id; } typedef E_AudioHandle;
struct E_PlayingHandle { uint32 id; } typedef E_PlayingHandle;

API bool E_LoadBunchOfAudios(const Buffer* oggs, E_AudioHandle* out_handles, uint32 audio_count);
API void E_FreeBunchOfAudios(E_AudioHandle* handles, uint32 audio_count);

struct E_PlayAudioDesc
{
	float32 volume; // if 0, defaults to 1. if you really want 0, then use -0
	float32 balance; // -1 = left; 0 = middle; 1 = right. clamped.
	
	// both clamped, and won't play if duration <= 0
	float32 start_at_second;
	float32 duration_in_seconds;
	
	float32 fadein_duration;
	
	int32 priority; // audios with lower priority might be removed automatically.
	
	bool loop : 1;
	bool loop_at_start : 1; // imply loop.
}
typedef E_PlayAudioDesc;

API E_PlayingHandle E_PlayAudio(E_AudioHandle audio, E_PlayAudioDesc* desc);
API bool E_StopPlayingAudio(E_PlayingHandle playing);
API bool E_FadeOutPlayingAudio(E_PlayingHandle playing, float32 fade_duration_in_seconds);
API bool E_VolumePlayingAudio(E_PlayingHandle playing, float32 target_volume, float32 transition_duration_in_seconds);

struct E_AudioInfo
{
	bool is_ok : 1;
	E_AudioHandle handle;
	
	int32 frame_rate;
	int32 channel_count;
	int32 duration_in_frames;
	float32 duration_in_seconds;
	
	uint32 playing_count;
}
typedef E_AudioInfo;

struct E_PlayingInfo
{
	bool is_ok : 1;
	E_AudioInfo audio_info;
	
	int32 playing_frame_index;
	float32 playing_second;
}
typedef E_PlayingInfo;

API void E_FetchAudioInfo(E_AudioHandle audio, E_AudioInfo* out_info);
API void E_FetchPlayingInfo(E_PlayingHandle playing, E_PlayingInfo* out_info);
#endif

#endif //API_ENGINE_H
