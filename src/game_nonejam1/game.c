#include "api_engine.h"
#include "util_random.h"

static E_GlobalData* engine;
static G_GlobalData* game;

struct FallingThing
{
	vec2 pos;
	int32 sprite_id;
}
typedef FallingThing;

struct G_GlobalData
{
	E_Font font;
	E_Tex2d tex;
	E_RectBatch batch;
	URng_State rng;
	
	bool paused : 1;
	bool clicking : 1;
	
	vec2 tongue_dir;
	float32 tongue_len;
	
	int32 falling_thing_count;
	FallingThing falling_things[32];
};

static G_GlobalData*
InitGame(void)
{
	Trace();
	game = ArenaPushStruct(engine->persistent_arena, G_GlobalData);
	URng_Init(&game->rng, OS_CurrentPosixTime());
	
	E_FontDesc fontdesc = {
		.arena = engine->persistent_arena,
		.char_height = 32.0f,
		.prebake_ranges = {
			{ 0x21, 0x7E },
			{ 0xA1, 0xFF },
		},
	};
	
	SafeAssert(OS_MapFile(Str("C:/Windows/Fonts/Arial.ttf"), NULL, &fontdesc.ttf));
	SafeAssert(E_MakeFont(&fontdesc, &game->font));
	
	E_Tex2dDesc texdesc = {
		.flag_linear_filtering = true,
	};
	
	SafeAssert(OS_MapFile(Str("assets/nonejam1.png"), NULL, &texdesc.encoded_image));
	SafeAssert(E_MakeTex2d(&texdesc, &game->tex));
	
	return game;
}

static void
UpdateAndRender(void)
{
	Trace();
	
	const int32 tex_cols = game->tex.width / 128;
	const float32 width = 128.0f;
	const float32 height = 128.0f;
	const float32 frog_x = engine->os->window.width / 2;
	const float32 frog_y = engine->os->window.height - height * 1.5f;
	
	//~ NOTE(ljre): Update
	if (engine->os->window.should_close)
		engine->running = false;
	
	if (OS_IsPressed(engine->os->keyboard, OS_KeyboardKey_Escape))
		game->paused ^= 1;
	
	if (!game->paused)
	{
		game->clicking = OS_IsDown(engine->os->mouse, OS_MouseButton_Left);
		
		float32 delta_x = engine->os->mouse.pos[0] - frog_x;
		float32 delta_y = engine->os->mouse.pos[1] - frog_y;
		float32 len = sqrtf(delta_x*delta_x + delta_y*delta_y);
		float32 invlen = 1.0f / len;
		
		delta_x *= invlen;
		delta_y *= invlen;
		
		game->tongue_dir[0] = delta_x;
		game->tongue_dir[1] = delta_y;
		game->tongue_len = len;
	}
	
	//~ NOTE(ljre): Draw
	RB_BeginCmd(engine->renderbackend, &(RB_BeginDesc) {
		.viewport_width = engine->os->window.width,
		.viewport_height = engine->os->window.height,
	});
	
	RB_CmdClear(engine->renderbackend, &(RB_ClearDesc) {
		.flag_color = true,
		.color = { 0.4f, 0.5f, 1.0f, 1.0f },
	});
	
	game->batch = (E_RectBatch) {
		.arena = engine->frame_arena,
		.textures = {
			E_WhiteTexture(),
			game->font.texture,
			game->tex,
		},
		.count = 0,
		.elements = ArenaEndAligned(engine->frame_arena, alignof(E_RectBatchElem)),
	};
	
	E_PushRect(&game->batch, &(E_RectBatchElem) {
		.pos = { frog_x - width/2, frog_y - height/2 },
		.scaling[0][0] = width,
		.scaling[1][1] = height,
		.tex_index = 2,
		.color = { 1.0f, 1.0f, 1.0f, 1.0f },
		.texcoords = { INT16_MAX / tex_cols * game->clicking, 0, INT16_MAX / tex_cols, INT16_MAX },
	});
	
	if (game->clicking)
	{
		const float32 tongue_size = 20.0f;
		const float32 delta_x = game->tongue_dir[0];
		const float32 delta_y = game->tongue_dir[1];
		const float32 len = game->tongue_len;
		
		E_RectBatchElem tonge = {
			.pos = { frog_x + delta_y*tongue_size/2, frog_y - delta_x*tongue_size/2 + 4.0f },
			.scaling = {
				delta_x * len, delta_y * len,
				-delta_y * tongue_size, delta_x * tongue_size,
			},
			.tex_index = 0,
			.color = { 1.0f, 0.1f, 0.1f, 1.0f },
			.texcoords = { 0, 0, INT16_MAX, INT16_MAX },
		};
		
		E_PushRect(&game->batch, &tonge);
	}
	
	E_PushText(&game->batch, &game->font, Str("Hello, World!"), vec2(50.0f, 50.0f), vec2(2.0f, 2.0f), GLM_VEC4_ONE);
	
	if (game->paused)
	{
		E_PushRect(&game->batch, &(E_RectBatchElem) {
			.pos = { 0.0f, 0.0f },
			.scaling[0][0] = engine->os->window.width,
			.scaling[1][1] = engine->os->window.height,
			.color = { 0.0f, 0.0f, 0.0f, 0.4f },
			.texcoords = { 0, 0, INT16_MAX, INT16_MAX },
		});
		
		vec2 size;
		{
			String str = StrInit("Paused");
			
			E_CalcTextSize(&game->font, str, vec2(2.0f, 2.0f), &size);
			vec2 pos = {
				(engine->os->window.width - size[0]) / 2.0f,
				(engine->os->window.height - size[1]) / 2.0f,
			};
			
			E_PushText(&game->batch, &game->font, str, pos, vec2(2.0f, 2.0f), GLM_VEC4_ONE);
		}
		{
			String str = Str("Press Esc to unpause");
			vec2 size2;
			
			E_CalcTextSize(&game->font, str, vec2(1.0f, 1.0f), &size2);
			vec2 pos = {
				(engine->os->window.width - size2[0]) / 2.0f,
				(engine->os->window.height - size2[1])/2.0f + size[1]*0.75f,
			};
			
			E_PushText(&game->batch, &game->font, str, pos, vec2(1.0f, 1.0f), GLM_VEC4_ONE);
		}
	}
	
	E_DrawRectBatch(&game->batch, NULL);
	RB_EndCmd(engine->renderbackend);
	E_FinishFrame();
}

API void
G_Main(E_GlobalData* data)
{
	engine = data;
	
	if (!engine->game)
		engine->game = InitGame();
	
	UpdateAndRender();
}
