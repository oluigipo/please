#include "api_engine.h"

static G_GlobalData* game;
static E_GlobalData* engine;

struct G_GlobalData
{
	E_Font font;
	E_Tex2d tex_pexe;
};

static void
G_Init(void)
{
	game = engine->game = Arena_PushStruct(engine->persistent_arena, G_GlobalData);
	
	{
		E_FontDesc desc = {
			.arena = engine->persistent_arena,
			.ttf = { 0 },
			
			.char_height = 32.0f,
			.prebake_ranges = {
				{ 0x21, 0x7E+1 },
				{ 0xA1, 0xFF+1 },
			},
		};
		
		SafeAssert(OS_ReadEntireFile(Str("C:/Windows/Fonts/Arial.ttf"), engine->persistent_arena, (void**)&desc.ttf.data, &desc.ttf.size));
		SafeAssert(E_MakeFont(&desc, &game->font));
	}
	
	for Arena_TempScope(engine->scratch_arena)
	{
		E_Tex2dDesc desc = {
			0
		};
		
		SafeAssert(OS_ReadEntireFile(Str("./assets/pexe.png"), engine->scratch_arena, (void**)&desc.encoded_image.data, &desc.encoded_image.size));
		SafeAssert(E_MakeTex2d(&desc, &game->tex_pexe));
	}
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
		
		float32 time = (float32)OS_GetTimeInSeconds() * 0.25f + (float32)i;
		float32 xoffset = cosf(time * 5.0f) * 100.0f;
		float32 yoffset = sinf(time * 1.4f) * 100.0f;
		
		float32 x = (random & 511) - 256.0f + xoffset;
		float32 y = (random>>9 & 511) - 256.0f + yoffset;
		float32 angle = (random>>18 & 511) / 512.0f * (float32)Math_PI*2 + time;
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
		.textures[0] = &game->tex_pexe.handle,
		.count = count,
		.elements = elements,
	};
	
	E_PushTextToRectBatch(&batch, engine->frame_arena, &game->font, 1, Str("Olá, mundo!\n(menos pro cien)"), GLM_VEC2_ZERO, GLM_VEC2_ONE, GLM_VEC4_ONE);
	
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
