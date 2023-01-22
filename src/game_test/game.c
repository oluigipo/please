#include "api_engine.h"

static G_GlobalData* game;
static E_GlobalData* engine;

struct G_GlobalData
{
	int32 dummy;
};

static void
G_Init(void)
{
	game = engine->game = Arena_PushStruct(engine->persistent_arena, G_GlobalData);
	
	
}

static void
G_UpdateAndRender(void)
{
	if (engine->window_state->should_close || OS_IsPressed(engine->input->keyboard, OS_KeyboardKey_Escape))
		engine->running = false;
	
	E_DrawClear(0.2f, 0.0f, 0.4f, 1.0f);
	
	E_RectBatchElem* elements = Arena_EndAligned(engine->frame_arena, alignof(E_RectBatchElem));
	uint32 count = 0;
	
	for (int32 i = 0; i < 100; ++i)
	{
		uint64 random = Hash_IntHash64(i);
		
		float32 x = (random & 511) - 256.0f;
		float32 y = (random>>9 & 511) - 256.0f;
		float32 angle = (random>>18 & 511) / 512.0f * (float32)Math_PI*2;
		float32 size = 32.0f;
		vec3 color = {
			(random>>27 & 255) / 255.0f,
			(random>>35 & 255) / 255.0f,
			(random>>43 & 255) / 255.0f,
		};
		
		E_RectBatchElem elem = {
			.pos = { x, y },
			.scaling = {
				{ size*cosf(angle), size*-sinf(angle) },
				{ size*sinf(angle), size* cosf(angle) },
			},
			.tex_index = 0.0f,
			.texcoords = { 0.0f, 0.0f, 1.0f, 1.0f },
			.color = { color[0], color[1], color[2], 1.0f },
		};
		
		Arena_PushData(engine->frame_arena, &elem);
		++count;
	}
	
	E_RectBatch batch = {
		.count = count,
		.elements = elements,
	};
	
	E_DrawRectBatch(&batch);
	E_FinishFrame();
}

API void
G_Main(E_GlobalData* data)
{
	engine = data;
	game = data->game;
	
	if (!game)
		G_Init();
	
	for Arena_TempScope(data->scratch_arena)
		G_UpdateAndRender();
}