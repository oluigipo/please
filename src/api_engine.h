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
	E_Limits_MaxThreadWorkCount = 1024,
	E_Limits_MaxLoadedSounds = 128,
	E_Limits_MaxPlayingSounds = 16,
};

struct G_GlobalData typedef G_GlobalData;
struct E_GlobalData typedef E_GlobalData;

struct E_AudioState typedef E_AudioState;
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
	Arena* audio_thread_arena;
	G_GlobalData* game;
	
	const OS_WindowGraphicsContext* graphics_context;
	OS_WindowState* window_state;
	OS_InputState* input;
	E_AudioState* audio;
	
	RB_ResourceCommand* resource_command_list;
	RB_ResourceCommand* last_resource_command;
	RB_DrawCommand* draw_command_list;
	RB_DrawCommand* last_draw_command;
	
	void* game_memory;
	uintsize game_memory_size;
	
	float32 delta_time;
	float64 last_frame_time;
	
	bool running : 1;
	bool multithreaded : 1;
	
	// NOTE(ljre): Convenience lock for mutating global engine data. Should only be used if the main thread is
	//             currently waiting and individual worker threads needs to access global resources.
	OS_RWLock mt_lock;
	
	E_ThreadWorkQueue* thread_work_queue;
	E_ThreadCtx worker_threads[E_Limits_MaxWorkerThreadCount];
};

API void G_Main(E_GlobalData* data);
API void E_FinishFrame(void);

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
	Arena* arena;
	RB_Handle* textures[RB_Limits_SamplersPerDrawCall];
	
	uint32 count;
	E_RectBatchElem* elements;
}
typedef E_RectBatch;

API bool E_PushText(E_RectBatch* batch, E_Font* font, String text, vec2 pos, vec2 scale, vec4 color);
API void E_PushRect(E_RectBatch* batch, const E_RectBatchElem* rect);

API void E_DrawClear(float32 r, float32 g, float32 b, float32 a);
API void E_DrawRectBatch(const E_RectBatch* batch, const E_Camera2D* cam);

API void E_RawResourceCommands(RB_ResourceCommand* first, RB_ResourceCommand* last);
API void E_RawDrawCommands(RB_DrawCommand* first, RB_DrawCommand* last);

API bool E_DecodeImage(Arena* output_arena, Buffer image, void** out_pixels, int32* out_width, int32* out_height);

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

//- Audio API
struct E_SoundHandle
{
	uint16 generation;
	uint16 index;
}
typedef E_SoundHandle;

struct E_SoundInfo
{
	int32 channels;
	int32 sample_rate;
	int32 sample_count;
	
	float32 length; // seconds
}
typedef E_SoundInfo;

API bool E_LoadSound(Buffer ogg, E_SoundHandle* out_sound, E_SoundInfo* out_info);
API void E_UnloadSound(E_SoundHandle sound);
API bool E_IsValidSoundHandle(E_SoundHandle sound);
API bool E_QuerySoundInfo(E_SoundHandle sound, E_SoundInfo* out_info);

struct E_PlayingSoundHandle
{
	uint16 generation;
	uint16 index;
}
typedef E_PlayingSoundHandle;

struct E_PlayingSoundInfo
{
	E_SoundHandle sound;
	
	float32 at; // seconds
	float32 speed;
	float32 volume;
}
typedef E_PlayingSoundInfo;

struct E_PlaySoundOptions
{
	float32 volume;
	float32 speed;
}
typedef E_PlaySoundOptions;

API bool E_PlaySound(E_SoundHandle sound, const E_PlaySoundOptions* options, E_PlayingSoundHandle* out_playing);
API bool E_StopSound(E_PlayingSoundHandle playing_sound);
API bool E_QueryPlayingSoundInfo(E_PlayingSoundHandle playing_sound, E_PlayingSoundInfo* out_info);
API void E_StopAllSounds(E_SoundHandle* specific);
API bool E_IsValidPlayingSoundHandle(E_PlayingSoundHandle playing_sound);

#endif //API_ENGINE_H
