#ifndef API_ENGINE_H
#define API_ENGINE_H

#include "config.h"
#include "api_os.h"
#include "api_renderbackend.h"

DisableWarnings();
#include <ext/cglm/cglm.h>
ReenableWarnings();

#define vec2(...) (vec2) { __VA_ARGS__ }
#define vec3(...) (vec3) { __VA_ARGS__ }
#define vec4(...) (vec4) { __VA_ARGS__ }

//~ Actual Api
enum
{
	E_Limits_MaxWorkerThreadCount = 4,
	E_Limits_MaxThreadWorkCount = 256,
};

struct G_GlobalData typedef G_GlobalData;
struct E_GlobalData typedef E_GlobalData;

struct E_ThreadWorkQueue typedef E_ThreadWorkQueue;

struct E_ThreadCtx
{
	alignas(64) Arena* scratch_arena;
	intsize id;
}
typedef E_ThreadCtx;

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
	bool multithreaded : 1;
	
	E_ThreadWorkQueue* thread_work_queue;
	E_ThreadCtx worker_threads[E_Limits_MaxWorkerThreadCount];
};

API void G_Main(E_GlobalData* data);
API void E_FinishFrame(void);

//- Audio
// TODO(ljre): LEGACY THING DELETE SOON
struct Asset_SoundBuffer
{
	int32 channels;
	int32 sample_rate;
	int32 sample_count;
	int16* samples;
}
typedef Asset_SoundBuffer;

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

//- Rendering API
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

//- Worker Thread API
void typedef E_ThreadWorkProc(E_ThreadCtx* ctx, void* data);

struct E_ThreadWork
{
	E_ThreadWorkProc* callback;
	void* data;
}
typedef E_ThreadWork;

struct E_ThreadWorkQueue
{
	OS_Semaphore semaphore;
	OS_EventSignal reached_zero_doing_work_sig;
	
	volatile int32 remaining_head;
	volatile int32 doing_head;
	volatile int32 doing_count;
	E_ThreadWork works[E_Limits_MaxThreadWorkCount];
};

// Returns true if any work has been done.
// ctx and queue should be NULL if it's being called by the main thread
API bool E_RunThreadWork(E_ThreadCtx* ctx, E_ThreadWorkQueue* queue);

// NOTE(ljre): these should only be called by main thread!
API void E_WaitRemainingThreadWork(void);
API void E_QueueThreadWork(const E_ThreadWork* work);

#endif //API_ENGINE_H
